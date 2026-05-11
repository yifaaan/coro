#pragma once

#include "storage.h"
#include "log.h"

namespace coro::detail {

template <typename T>
struct task_promise_storage_base : storage<T> {
    void unhandled_exception() noexcept { this->set_exception(std::current_exception()); }
};

// for co_return x;
template <typename T>
struct task_promise_storage : task_promise_storage_base<T> {
    template <std::convertible_to<T> U>
    void return_value(U&& value) noexcept(noexcept(this->set_value(std::forward<U>(value)))) {
        this->set_value(std::forward<U>(value));
    }
};


// for co_return;
template <>
struct task_promise_storage<void> : task_promise_storage_base<void> {
    static void return_void() noexcept {}
};

}  // namespace coro::detail