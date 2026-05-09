#pragma once

#include <coroutine>
#include <type_traits>
#include <utility>

#include "get_awaiter.h"
#include "type_traits.h"

namespace coro::detail {

// 提取await_suspend参数的类型
template <typename Ret, typename T, typename Handle>
Handle FuncArg(Ret (*)(Handle));

template <typename Ret, typename T, typename Handle>
Handle FuncArg(Ret (T::*)(Handle));

template <typename Ret, typename T, typename Handle>
Handle FuncArg(Ret (T::*)(Handle) &);

template <typename Ret, typename T, typename Handle>
Handle FuncArg(Ret (T::*)(Handle) &&);

template <typename Ret, typename T, typename Handle>
Handle FuncArg(Ret (T::*)(Handle) const);

template <typename Ret, typename T, typename Handle>
Handle FuncArg(Ret (T::*)(Handle) const&);

template <typename Ret, typename T, typename Handle>
Handle FuncArg(Ret (T::*)(Handle) const&&);

// 约束 await_suspend 的返回类型只能是协程标准允许的几种
template <typename T>
concept AwaitSuspendReturnType = std::is_void_v<T> || std::is_same_v<T, bool> ||
                                 is_specialization_of<T, std::coroutine_handle>;
                                 /*
void await_suspend(std::coroutine_handle<>);
std::coroutine_handle<> await_suspend(std::coroutine_handle<Promise>);
*/
template <typename T>
concept Awaiter = requires(
    T&& t, decltype(detail::FuncArg(
               &std::remove_reference_t<T>::await_suspend)) await_suspend_arg) {
  // await_ready() -> bool
  { std::forward<T>(t).await_ready() } -> std::convertible_to<bool>;
  // the await_suspend's arg
  { await_suspend_arg } -> std::convertible_to<std::coroutine_handle<>>;
  // the await_suspend's return type
  { std::forward<T>(t).await_suspend() } -> detail::AwaitSuspendReturnType;
  std::forward<T>(t).await_resume();
};

}  // namespace coro::detail

namespace coro {



// Awaiter T的 await_resume() 的返回类型必须是Value
template <typename T, typename Value>
concept AwaiterOf = detail::Awaiter<T> && requires(T&& t) {
  { std::forward<T>(t).await_resume() } -> std::same_as<Value>;
};

// 可以被 co_await
template <typename T>
concept Awaitable = requires(T&& t) {
  { detail::GetAwaiter(std::forward<T>(t)) } -> detail::Awaiter;
};

template <typename A>
  requires Awaitable<A>
using AwaitResultT =
    decltype(detail::GetAwaiter(std::declval<A>()).await_resume());

// T 可以被 co_await，并且 co_await T 的结果类型是 Value
template <typename T, typename Value>
concept AwaitableOf = Awaitable<T> && requires(T&& t) {
  { detail::GetAwaiter(std::forward<T>(t)) } -> AwaiterOf<Value>;
};

}  // namespace coro