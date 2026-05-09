#pragma once

#include <coroutine>

#include "concepts.h"
#include "coro_handle.h"
#include "noncopyable.h"
#include "task_promise_storage.h"
#include "trace.h"
#include "type_traits.h"

// mp_coro::task<int> child() {
//  co_return 42;
// }
// mp_coro::task<void> parent() {
//   int x = co_await child();
// }

// --- co_await child() 在语义上拆成两段 ---

// [阶段 A] 求值 child()（子协程启动，全是编译器为 child 生成的逻辑）：
//   1) 分配 child 帧，构造 promise
//   2) promise.get_return_object() → 得到 task（持有 child_handle）
//   3) co_await promise.initial_suspend()；suspend_always → child
//   在入口挂起，co_return 尚未执行 4) child() 表达式结束，把 task 交给 parent

// [阶段 B] parent 里 co_await 该 task：
//   5) task::operator co_await() → awaiter
//   6) awaiter.await_ready()
//        true  → awaiter.await_resume()（从子 promise 存储取结果），parent
//        不挂起 false → 挂起 parent，调用 awaiter.await_suspend(parent_handle)
//                  - child.promise().continuation = parent_handle
//                  - return child_handle → 运行时 resume(child)
//   7) child 从挂点继续：越 initial_suspend → co_return → return_value → 写存储
//   8) co_await final_suspend：final_awaiter::await_suspend 返回
//   continuation（parent） 9) resume(parent)；回到 parent 的 co_await 之后执行
//   await_resume() → x
namespace coro {

template <typename T = void, typename Allocator = void>
class [[nodiscard]] Task {
 public:
  using value_type = T;

  struct promise_type : private Noncopyable,
                        detail::TaskPromiseStorage<value_type> {
    static auto initial_suspend() noexcept {
      TRACE();
      return std::suspend_always{};
    }

    // 当前coroutine结束时执行
    static AwaiterOf<void> auto final_suspend() noexcept {
      struct final_awaiter : std::suspend_always {
        // 参数h表示可能被暂停的调用者，调用者是当前coroutine
        std::coroutine_handle<> await_suspend(
            std::coroutine_handle<promise_type> h) {
          TRACE();
          return h.promise().continuation;  // 恢复parent的执行
        }
      };
      TRACE();
      return final_awaiter{};
    }

    Task get_return_object() noexcept {
      TRACE();
      return Task{*this};
    }

    std::coroutine_handle<> continuation = std::noop_coroutine();
  };

  // await task;做了什么
  // compiler does:
  // 创建task: promise.get_return_object()
  // co_await promise.initial_suspend() -> 挂起
  // 从task获取awaiter对象(operator co_await ...)
  // awaiter.await_ready：
  //            1. true awaiter.await_resume()取结果
  //            2. false 挂起， 执行awaiter.await_suspend(parent_handle)
  //                  1. 保存 continuation=parent_handle
  //                  2. 返回值：接下来要resume的handle，这里是child自己
  //   -> child_handle.resume(); ()
  //        从挂起地方恢复执行
  //              -> co_return 42; 写入 promise
  //              -> auto parent = final_suspend()
  //              -> parent.resume();
  struct Awaiter {
    [[nodiscard]] bool await_ready() const noexcept {
      TRACE();
      return handle.done();
    }

    // await_suspend 的句柄参数，永远是「当前正在执行这次
    // co_await、并且即将因这次等待而挂起」的那条协程——谁在做
    // co_await，参数就是谁
    // Awaiter::atait_suspend()是被praent调用的，parent会挂起
    [[nodiscard]] std::coroutine_handle<> await_suspend(
        std::coroutine_handle<> h) const {
      TRACE();
      // 保存 parent coroutine handle
      handle.promise().continuation = h;
      return handle;  // 返回当前 coroutine
                      // handle，表示切换到当前coroutine继续执行
    }

    [[nodiscard]] decltype(auto) await_resume() const {
      TRACE();
      return handle.promise().get();
    }
    std::coroutine_handle<promise_type> handle;
  };

  auto operator co_await() const& noexcept {
    TRACE();
    return Awaiter{*handle_};
  }

  auto operator co_await() && noexcept {
    TRACE();

    struct RValueAwaiter : Awaiter {
      [[nodiscard]] decltype(auto) await_resume() const {
        TRACE();
        return std::move(this->handle.promise()).get();
      }
    };
    return RValueAwaiter{*handle_};
  }

 private:
  explicit Task(promise_type& promise) : handle_{promise} {}

  CoroHandle<promise_type> handle_;
};

template <Awaitable A>
Task<std::remove_reference_t<AwaitResultT<A>>> MakeTask(A&& a) {
  co_return co_await std::forward<A>(a);
}

}  // namespace coro