#pragma once

#include <coroutine>
#include <utility>

namespace coro {

// 包装 std::coroutine_handle 类，析构时自动调用 destroy 方法
template <typename Promise>
class CoroHandle {
 public:
  using HandleType = std::coroutine_handle<Promise>;

  explicit CoroHandle(HandleType handle) : handle_{handle} {}
  explicit CoroHandle(Promise& promise)
      : handle_{HandleType::from_promise(promise)} {}
  ~CoroHandle() {
    if (handle_) {
      handle_.destroy();
    }
  }

  CoroHandle(const CoroHandle&) = delete;
  CoroHandle& operator=(const CoroHandle&) = delete;

  CoroHandle(CoroHandle&& other) noexcept
      : handle_{std::exchange(other.handle_, nullptr)} {}

  CoroHandle& operator=(CoroHandle&& other) noexcept {
    if (handle_) {
      handle_.destroy();
    }
    handle_ = std::exchange(other.handle_, nullptr);
    return *this;
  }

  constexpr explicit operator bool() const noexcept { return static_cast<bool>(handle_); }

  const HandleType* operator->() const noexcept { return &handle_; }
  const HandleType& operator*() const noexcept { return handle_; }

 private:
  HandleType handle_;
};
}  // namespace coro