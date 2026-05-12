#pragma once

#include "concepts.h"
#include "task_promise_storage.h"
#include "type_traits.h"

#include <coroutine>
#include <latch>
#include <utility>

namespace coro {

template <typename Sync, typename T>
    requires requires(Sync& sync) { sync.notify_awaitable_completed(); }
class synchronized_task {
public:
    synchronized_task(const synchronized_task&) = delete;
    synchronized_task& operator=(const synchronized_task&) = delete;

    synchronized_task(synchronized_task&& other) noexcept
        : handle_{std::exchange(other.handle_, {})} {}

    synchronized_task& operator=(synchronized_task&& other) noexcept {
        if (this != &other) {
            if (handle_) {
                handle_.destroy();
            }
            handle_ = std::exchange(other.handle_, {});
        }
        return *this;
    }

    ~synchronized_task() {
        if (handle_) {
            handle_.destroy();
        }
    }

    struct promise_type : detail::task_promise_storage<T> {
        static auto initial_suspend() noexcept { return std::suspend_always{}; }

        static auto final_suspend() noexcept {
            struct final_awaiter {
                bool await_ready() const noexcept { return false; }

                void await_suspend(std::coroutine_handle<promise_type> handle) const noexcept {
                    handle.promise().sync->notify_awaitable_completed();
                }

                void await_resume() const noexcept {}
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
    [[nodiscard]] decltype(auto) get() && { return std::move(handle_.promise()).get(); }

private:
    std::coroutine_handle<promise_type> handle_{};

    explicit synchronized_task(std::coroutine_handle<promise_type> handle) noexcept
        : handle_{handle} {}
};

template <typename Sync, awaitable A>
    requires requires(Sync& sync) { sync.notify_awaitable_completed(); }
synchronized_task<Sync, remove_rvalue_reference_t<awaiter_result_t<A>>> make_synchronized_task(
    A&& awaitable) {
    co_return co_await std::forward<A>(awaitable);
}

template <awaitable A>
decltype(auto) sync_wait(A&& awaitable) {
    struct sync {
        void notify_awaitable_completed() { latch.count_down(); }

        std::latch latch{1};
    };

    auto done = sync{};
    auto sync_task = make_synchronized_task<sync, A>(std::forward<A>(awaitable));
    sync_task.start(done);

    done.latch.wait();
    return std::move(sync_task).get();
}

}  // namespace coro
