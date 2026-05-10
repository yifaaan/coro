#pragma once

#include <utility>

namespace coro::detail {

// 根据awaitable对象获取awaiter
template <typename T>
decltype(auto) get_awaiter(T&& awaitable) {
    if constexpr (requires { std::forward<T>(awaitable).operator co_await(); }) {
        return std::forward<T>(awaitable).operator co_await();
    } else if constexpr (requires { operator co_await(std::forward<T>(awaitable)); }) {
        return operator co_await(std::forward<T>(awaitable));
    } else {
        return std::forward<T>(awaitable);
    }
}

}  // namespace coro::detail