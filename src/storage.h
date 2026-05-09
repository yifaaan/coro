#pragma once

#include <concepts>
#include <exception>
#include <memory>
#include <utility>
#include <variant>
namespace coro::detail {

template <typename T, typename V>
decltype(auto) VariantGet(V&& result) {
  // 有异常就直接抛出
  if (std::holds_alternative<std::exception_ptr>(result)) {
    std::rethrow_exception(std::get<std::exception_ptr>(result));
  }
  if constexpr (!std::is_void_v<T>) {
    return std::get<T>(std::forward<V>(result));
  }
}

// 保存 promise 对应的coroutine的返回值，异常
template <typename T>
class StorageBase {
 public:
  template <std::convertible_to<T> U>
  void set_value(U&& value) noexcept(
      std::is_nothrow_convertible_v<T, decltype(std::forward<U>(value))>) {
    this->result_.template emplace<T>(std::forward<U>(value));
  }

  [[nodiscard]] const T& get() const& {
    return detail::VariantGet<T>(this->result_);
  }
  [[nodiscard]] T get() && {
    return detail::VariantGet<T>(std::move(this->result_));
  }

 protected:
  std::variant<std::monostate, std::exception_ptr, T> result_;
};

template <typename T>
class StorageBase<T&> {
 public:
  void set_value(T& value) noexcept { this->result_ = std::addressof(value); }
  [[nodiscard]] T& get() const {
    return *detail::VariantGet<T*>(this->result_);
  }

 protected:
  std::variant<std::monostate, std::exception_ptr, T*> result_;
};

template <>
class StorageBase<void> {
 public:
  void get() const { return detail::VariantGet<void>(this->result_); }

 protected:
  std::variant<std::monostate, std::exception_ptr> result_;
};


template <typename T>
class Storage : public StorageBase<T> {
 public:
  void set_exception(std::exception_ptr ptr) noexcept { this->result_ = ptr; }
};

}  // namespace coro::detail