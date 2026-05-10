#pragma once

#include <coroutine>

#include "task_promise_storage.h"

namespace coro {

template <typename T = void, typename Allocator = void>
class [[nodiscard]] task {
public:
    using value_type = T;

    struct promise_type : private detail::noncopyable, detail::task_promise_storage {
        static auto initial_suspend() noexcept { return std::suspend_always{}; }

        // 当前协程(task)已执行完毕，编译器最后会调用这个函数
        // auto final_awaiter = promise.final_suspend();
        // co_await final_awaiter;
        //      if (!final_awaiter.await_ready) {
        //          auto to_run = final_awaiter.await_suspend(this_corotine);
        //          弹出/清理掉当前this_corotine的活跃调用栈
        //          在平级的调用栈上，直接执行 to_run.resume()
        //      }
        static auto final_suspend() noexcept {
            struct final_awaiter : std::suspend_always {
                auto await_suspend(std::coroutine_handle<promise_type> this_corotine) {
                    return this_corotine.promise().continuation;
                }
            };

            return final_awaiter{};
        }

        task get_return_object() noexcept { return task{*this}; }
        // 当前协程执行完后，将控制流转移到该协程(对称转移)
        std::coroutine_handle<> continuation = std::noop_coroutine{};
    };

private:
    std::coroutine_handle<promise_type> handle_;
    explicit task(promise_type& promise)
        : handle_{std::coroutine_handle<promise_type>::from_promise(promise)} {}
};
}  // namespace coro