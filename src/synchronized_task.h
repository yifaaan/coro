#pragma once

#include "concepts.h"
#include "task_promise_storage.h"
#include "type_traits.h"

#include <coroutine>
#include <latch>
#include <utility>

namespace coro {

template <typename Sync, typename T>
    requires requires(Sync&& sync) { sync.notify_awaitable_completed(); }
class synchronized_task {
public:
    ~synchronized_task() {
        if (handle_) {
            handle_.destroy();
        }
    }
    
    struct promise_type : detail::task_promise_storage<T> {
        static auto initial_suspend() noexcept { return std::suspend_always{}; }

        // 当前协程执行完后，调用sync.notify_awaitable_completed通知等待者
        static awaiter_of<void> decltype(auto) final_suspend() noexcept {
            struct final_awaiter : std::suspend_always {
                void await_suspend(std::coroutine_handle<promise_type> this_coro) noexcept {
                    this_coro.promise().sync->notify_awaitable_completed();
                }
            };
            return final_awaiter{};
        }

        auto get_return_object() noexcept {
            return synchronized_task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        Sync* sync = nullptr;
    };

    void start(Sync& sync) {
        handle_.promise().sync = &sync;
        handle_.resume();
    }

    [[nodiscard]] decltype(auto) get() const& { return handle_.promise().get(); }
    [[nodiscard]] decltype(auto) get() const&& { return std::move(handle_.promise()).get(); }

private:
    std::coroutine_handle<promise_type> handle_;
    explicit synchronized_task(std::coroutine_handle<promise_type> h)
        : handle_{h} {}
};

template <typename Sync, awaitable A>
    requires requires(Sync&& sync) { sync.notify_awaitable_completed(); }
synchronized_task<Sync, remove_rvalue_reference_t<awaiter_result_t<A>>> make_synchronized_task(
    A&& awaitable) {
    co_return co_await std::forward<A>(awaitable);
}

// 阻塞等待直到awaitable执行完毕
template <awaitable A>
decltype(auto) sync_wait(A&& awaitable) {
    struct sync {
        void notify_awaitable_completed() { latch.count_down(); }
        std::latch latch; // 类似 go:: sync.WaitGroup
    };
    auto done = sync{std::latch{1}};
    auto sync_task = make_synchronized_task<sync, A>(std::forward<A>(awaitable));
    sync_task.start(done);

    done.latch.wait();
    return std::move(sync_task).get();
}

}  // namespace coro
