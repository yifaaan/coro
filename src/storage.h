#pragma once

#include <concepts>
#include <exception>
#include <type_traits>
#include <variant>

namespace coro::detail {

template <typename T, typename V>
decltype(auto) variant_get(V&& result) {
    if (std::holds_alternative<std::exception_ptr>(result)) {
        std::rethrow_exception(std::get<std::exception_ptr>(std::forward<V>(result)));
    }
    if constexpr (!std::is_void_v<T>) {
        return std::get<T>(std::forward<V>(result));
    }
}

template <typename T>
class storage_base {
protected:
    std::variant<std::monostate, std::exception_ptr, T> result;

public:
    template <std::convertible_to<T> U>
    void set_value(U&& value) noexcept(
        std::is_nothrow_constructible_v<T, decltype(std::forward<U>(value))>) {
        result.template emplace<T>(std::forward<U>(value));
    }

    [[nodiscard]] const T& get() const& { return detail::variant_get<T>(result); }

    [[nodiscard]] T get() && { return detail::variant_get<T>(std::move(result)); }
};

template <typename T>
class storage_base<T&> {
protected:
    std::variant<std::monostate, std::exception_ptr, T*> result;

public:
    void set_value(T& value) noexcept { result = std::addressof(value); }
    [[nodiscard]] T& get() const { return *detail::variant_get<T*>(result); }
};

template <>
class storage_base<void> {
protected:
    std::variant<std::monostate, std::exception_ptr> result;

public:
    void get() const { detail::variant_get<void>(this->result); }
};

template <typename T>
class storage : public storage_base<T> {
public:
    void set_exception(std::exception_ptr ptr) noexcept { this->result = ptr; }
};
}  // namespace coro::detail