#pragma once

#include <coroutine>

#include "noncopyable.h"
#include "task_promise_storage.h"
#include "type_traits.h"
#include "concepts.h"
#include "log.h"

namespace coro {

// 假设有一个简单的 task 类型：当另一个协程 co_await 它时，才会惰性执行其函数体。这个 task
// 类型不支持返回值。 当 bar() 执行 co_await foo() 时。 调用 foo() 会执行若干步骤：
// 为协程帧（coroutine frame）分存储（通常在堆上）
// 将参数拷贝进协程帧（这里没有配参数，所以是空操作）
// 在协程帧里构造 promise 对象
// 调用 promise.get_return_object() 获取 foo() 的返回值。它会产生一个将被返回的 task
// 对象，并用一个指向刚创建的协程帧的 std::coroutine_handle 初始化它。 在 initial-suspend
// 点（也就是左花括号 { 处）挂起协程执行 把 task 返回给 bar() 接着，bar() 会对 foo() 返回的 task
// 求值 co_await 表达式： bar() 挂起，然后调用返回的 awaiter 的 await_suspend()，并把指向 bar()
// 协程帧的 std::coroutine_handle 传进去 await_suspend() 会把 bar() 的 std::coroutine_handle 存到
// foo() 的 promise 对象里，然后对 foo() 的 std::coroutine_handle 调用 .resume() 来恢复 foo() 协程

template <typename T = void, typename Allocator = void>
class [[nodiscard]] task {
public:
    ~task() {
        if (handle_) {
            handle_.destroy();
        }
    }
    using value_type = T;

    struct promise_type : private detail::noncopyable, detail::task_promise_storage<T> {
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
            // 实现“完成后唤醒等待者”
            // 需要唤醒父coroutine，所以当前corotine总是在最后阶段挂起，执行await_suspend(否则当前coroutine会直接销毁)
            struct final_awaiter : std::suspend_always {
                auto await_suspend(std::coroutine_handle<promise_type> this_corotine) noexcept {
                    return this_corotine.promise().continuation;
                }
            };
            return final_awaiter{};
        }

        task get_return_object() noexcept {
            return task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        // 当前协程执行完后，将控制流转移到该协程(对称转移)
        std::coroutine_handle<> continuation = std::noop_coroutine();
    };

    void resume() {
        if (!handle_.done()) {
            handle_();
        }
    }

    auto operator co_await() const& noexcept { return awaiter{handle_}; }
    auto operator co_await() && noexcept {
        struct rvalue_awaiter : public awaiter {
            [[nodiscard]] decltype(auto) await_resume() {
                return std::move(this->handle.promise()).get();
            }
        };
        return rvalue_awaiter{{handle_}};
    }

private:
    struct awaiter {
        [[nodiscard]] auto await_ready() const noexcept { return handle.done(); }

        // 当一个协程 await 一个 task 时，我们希望等待方协程总是挂起；挂起后，把等待方的 h
        // 存进即将被恢复的协程的 promise 里 然后恢复task协程
        [[nodiscard]] std::coroutine_handle<> await_suspend(
            std::coroutine_handle<> h) const noexcept {
            handle.promise().continuation = h;
            // 唤醒 task's coroutine, which is currently suspended
            // at the initial-suspend-point (ie. at the open curly brace).
            return handle;
        }

        [[nodiscard]] decltype(auto) await_resume() const { return handle.promise().get(); }
        std::coroutine_handle<promise_type> handle;
    };

private:
    std::coroutine_handle<promise_type> handle_;
    explicit task(std::coroutine_handle<promise_type> h) noexcept
        : handle_{h} {}
};

template <awaitable A>
task<remove_rvalue_reference_t<awaiter_result_t<A>>> make_task(A&& awaitable) {
    co_return co_await std::forward<A>(awaitable);
}

}  // namespace coro
