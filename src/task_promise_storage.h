#pragma once

#include <exception>
#include <utility>

#include "storage.h"
#include "trace.h"

namespace coro::detail {

template <typename T>
struct TaskPromiseStorageBase : Storage<T> {
  void unhandled_exception() noexcept {
    TRACE();
    this->set_exception(std::current_exception());
  }
};

template <typename T>
struct TaskPromiseStorage : TaskPromiseStorageBase<T> {
  template <std::convertible_to<T> U>
  void return_value(U&& value) noexcept(
      noexcept(this->set_value(std::forward<U>(value)))) {
    TRACE();
    this->set_value(std::forward<U>(value));
  }
};

template <>
struct TaskPromiseStorage<void> : TaskPromiseStorageBase<void> {
  static void return_void() noexcept { TRACE(); }
}

}  // namespace coro::detail