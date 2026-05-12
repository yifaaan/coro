#pragma once

#include "storage.h"

#include <concepts>
#include <coroutine>
#include <exception>
#include <thread>
#include <type_traits>

namespace coro {

template <std::invocable F>
struct async {
public:
    using result_type = std::invoke_result_t<F>;

    template <class Arg>
        requires std::constructible_from<F, Arg>
    explicit async(Arg&& func)
        : func_{std::forward<Arg>(func)} {}

    // only support `co_await async{...}`
    decltype(auto) operator co_await() & = delete;

    decltype(auto) operator co_await() && {
        struct awaiter {
            bool await_ready() const noexcept { return false; }
            void await_suspend(std::coroutine_handle<> handle) noexcept {
                auto work = [this, handle] {
                    try {
                        if constexpr (std::is_void_v<result_type>) {
                            this->asc.func_();
                        } else {
                            this->asc.result_.set_value(this->asc.func_());
                        }
                    } catch (...) {
                        this->asc.result_.set_exception(std::current_exception());
                    }
                    // 在新线程里，执行handle的后续逻辑
                    handle.resume();
                };
                std::jthread{work}.detach();
            }
            decltype(auto) await_resume() { return std::move(this->asc.result_).get(); }

            async asc;
        };
        return awaiter{std::move(*this)};
    }

    F func_;
    detail::storage<result_type> result_;
};

template <std::invocable F>
async(F) -> async<F>;

}  // namespace coro
