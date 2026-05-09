#pragma once

#include <utility>
namespace coro::detail {

template <typename T>
decltype(auto) GetAwaiter(T&& awaitable) {
  if constexpr (requires { std::forward<T>(awaitable).operator co_await(); }) {
    return std::forward<T>(awaitable).operator co_await();
  } else if constexpr (requires {
                         operator co_await(std::forward<T>(awaitable));
                       }) {
    return operator co_await(std::forward<T>(awaitable));
  } else {
    return std::forward<T>(awaitable);
  }
}

}  // namespace coro::detail