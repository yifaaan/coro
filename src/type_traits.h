#pragma once

#include <utility>

namespace coro::detail {

template <typename T, template <typename...> typename Type>
constexpr auto is_specialization_of = false;

template <typename... Prams, template <typename...> typename Type>
constexpr auto is_specialization_of<Type<Prams...>, Type> = true;

}  // namespace coro::detail

namespace coro {

template <typename T, template <typename...> typename Template>
concept specialization_of = detail::is_specialization_of<T, Template>;

}  // namespace coro