#pragma once

#include <coroutine>

#include "concepts.h"
#include "noncopyable.h"
#include "task_promise_storage.h"

namespace coro::detail {

template <typename Sync, typename T>
  requires requires(Sync s) { s.notify_awaitable_completed(); }
class [[nodiscard]] SynchronizedTask {
 public:
  using value_type = T;

  struct promise_type : private detail::Nocopyable, TaskPromiseStorage<T> {
    Sync* sync = nullptr;

    static auto initial_suspend() noexcept {
      TRACE();
      return std::suspend_always{};
    }

    static AwaiterOf<void> auto final_suspend() noexcept {
      struct FinalAwaiter : std::suspend_always {
        void await_suspend(std::coroutine_handle<promise_type> this_coro) {
          TRACE();
          // 当前协程结束通知外部的同步对象，当前协程的 awaitable 已经完成
          this_coro.promise().sync->notify_awaitable_completed();
        }
      };
      TRACE();
      return FinalAwaiter{};
    }

    auto get_return_object() nonexcept {
      TRACE();
      return SynchronizedTask{
          std::coroutine_handle<promise_type>::from_promise(*this)};
    }
  };

  void Start(Sync& sync) {
    handle_.promise().sync = &sync;
    handle_.resume();
  }

  [[nodiscard]] decltype(auto) Get() const& {
    TRACE();
    return handle_.promise().get();
  }

  [[nodiscard]] decltype(auto) Get() const&& {
    TRACE();
    return tsd::move(handle_.promise()).get();
  }

 private:
  std::coroutine_handle<promise_type> handle_;

  explicit SynchronizedTask(std::coroutine_handle<promise_type> handle)
      : handle_{handle} {}
};

template <typename Sync, Awaitable A>
  requires requires(Sync s) { s.notify_awaitable_completed(); }
SynchronizedTask<Sync, remove_rvalue_reference<AwaitResultT<A>>>
MakeSynchronizedTask(A&& awaitable) {
  co_return co_await std::forward<A>(awaitable);

}  // namespace coro::detail