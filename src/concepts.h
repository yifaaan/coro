#pragma once

#include "get_awaiter.h"

#include <concepts>
#include <coroutine>
#include <type_traits>

namespace coro {
namespace detail {

// 判断T是否是Template的实例
template <typename T, template <typename...> class Template>
constexpr auto is_specialization_of = false;

template <typename... Ts, template <typename...> class Template>
constexpr auto is_specialization_of<Template<Ts...>, Template> = true;

template <typename Ret, typename Handle>
Handle await_suspend_arg(Ret (*)(Handle));
template <typename Ret, typename Handle>
Handle await_suspend_arg(Ret (*)(Handle) noexcept);

template <typename Ret, typename T, typename Handle>
Handle await_suspend_arg(Ret (T::*)(Handle));
template <typename Ret, typename T, typename Handle>
Handle await_suspend_arg(Ret (T::*)(Handle) noexcept);
template <typename Ret, typename T, typename Handle>
Handle await_suspend_arg(Ret (T::*)(Handle) &);
template <typename Ret, typename T, typename Handle>
Handle await_suspend_arg(Ret (T::*)(Handle) & noexcept);
template <typename Ret, typename T, typename Handle>
Handle await_suspend_arg(Ret (T::*)(Handle) &&);
template <typename Ret, typename T, typename Handle>
Handle await_suspend_arg(Ret (T::*)(Handle) && noexcept);
template <typename Ret, typename T, typename Handle>
Handle await_suspend_arg(Ret (T::*)(Handle) const);
template <typename Ret, typename T, typename Handle>
Handle await_suspend_arg(Ret (T::*)(Handle) const noexcept);
template <typename Ret, typename T, typename Handle>
Handle await_suspend_arg(Ret (T::*)(Handle) const&);
template <typename Ret, typename T, typename Handle>
Handle await_suspend_arg(Ret (T::*)(Handle) const& noexcept);
template <typename Ret, typename T, typename Handle>
Handle await_suspend_arg(Ret (T::*)(Handle) const&&);
template <typename Ret, typename T, typename Handle>
Handle await_suspend_arg(Ret (T::*)(Handle) const&& noexcept);

// await_suspend的返回值只有三种类型
template <typename T>
concept await_suspend_result =
    std::is_void_v<T> or std::same_as<std::remove_cvref_t<T>, bool> or
    is_specialization_of<std::remove_cvref_t<T>, std::coroutine_handle>;

}  // namespace detail

// awaiter满足:
// ```
//  await_ready() -> bool
//  await_suspend() -> void/bool/coroutine_handle
//  await_resume()
// ```
template <typename T>
concept awaiter = requires(
    T&& t,
    decltype(detail::await_suspend_arg(&std::remove_reference_t<T>::await_suspend)) await_suspend_arg) {
    { std::forward<T>(t).await_ready() } -> std::convertible_to<bool>;
    { await_suspend_arg } -> std::convertible_to<std::coroutine_handle<>>;
    { std::forward<T>(t).await_suspend(await_suspend_arg) } -> detail::await_suspend_result;
    std::forward<T>(t).await_resume();
};

// T是产生类型V值的awaiter
template <typename T, typename V>
concept awaiter_of = awaiter<T> and requires(T&& t) {
    { std::forward<T>(t).await_resume() } -> std::same_as<V>;
};

// awaitable
template <typename T>
concept awaitable = requires(T&& t) {
    { detail::get_awaiter(std::forward<T>(t)) } -> awaiter;
};

template <typename T, typename V>
concept awaitable_of = awaitable<T> and requires(T&& t) {
    { detail::get_awaiter(std::forward<T>(t)) } -> awaiter_of<V>;
};

}  // namespace coro