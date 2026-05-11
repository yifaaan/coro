#pragma once

#include "storage.h"

#include <concepts>
#include <coroutine>
#include <exception>
#include <thread>
#include <type_traits>
#include <utility>

namespace coro {

template <std::invocable Func>
class async {
public:
    using return_type = std::invoke_result_t<Func>;

    template <std::convertible_to<Func> F>
    explicit async(F&& func)
        : func_{std::forward<F>(func)} {}

    decltype(auto) operator co_await() & = delete;

    decltype(auto) operator co_await() && {
        struct awaiter {
            async& awaitable;

            [[nodiscard]] bool await_ready() const noexcept { return false; }

            void await_suspend(std::coroutine_handle<> handle) {
                auto work = [this, handle] {
                    try {
                        if constexpr (std::is_void_v<return_type>) {
                            awaitable.func_();
                        } else {
                            awaitable.result_.set_value(awaitable.func_());
                        }
                    } catch (...) {
                        awaitable.result_.set_exception(std::current_exception());
                    }
                    handle.resume();
                };

                std::jthread{work}.detach();
            }

            [[nodiscard]] decltype(auto) await_resume() {
                return std::move(awaitable.result_).get();
            }
        };

        return awaiter{*this};
    }

private:
    Func func_;
    detail::storage<return_type> result_;
};

template <std::invocable F>
async(F) -> async<F>;

}  // namespace coro
