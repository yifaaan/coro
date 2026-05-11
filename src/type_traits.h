#pragma once

#include "concepts.h"

namespace coro {

template <typename T>
struct remove_rvalue_reference {
    using type = T;
};

template <typename T>
struct remove_rvalue_reference<T&&> {
    using type = T;
};

template <typename T>
using remove_rvalue_reference_t = remove_rvalue_reference<T>::type;

template <awaitable A>
using awaiter_for_t = decltype(detail::get_awaiter(std::declval<A>()));

template <awaitable A>
using awaiter_result_t = decltype(std::declval<awaiter_for_t<A>>().await_resume());
}  // namespace coro