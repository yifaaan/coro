#pragma once

#include <coroutine>
#include <iterator>
#include <ranges>
#include <type_traits>

#include "coro_handle.h"
#include "noncopyable.h"
#include "task_promise_storage.h"

namespace coro {
template <typename T>
class [[nodiscard]] Generator {
 public:
  using value_type = std::remove_reference_t<T>;
  using reference = std::conditional_t<std::is_reference_v<T>, T, T&>;
  using pointer = std::add_pointer_t<value_type>;

  // 协程完成类型为 void（co_yield 走 yield_value）；不要用 TaskPromiseStorage<T>，
  // 否则会带上 return_value，与 return_void 不能同时出现在同一 promise_type。
  struct promise_type : private Noncopyable, detail::TaskPromiseStorageBase<void> {
    static auto initial_suspend() noexcept {
      TRACE();
      return std::suspend_always{};
    }
    static auto final_suspend() noexcept {
      TRACE();
      return std::suspend_always{};
    }

    void return_void() noexcept { TRACE(); }

    auto yield_value(reference v) noexcept {
      TRACE();
      value = std::addressof(v);
      return std::suspend_always{};
    }

    Generator get_return_object() noexcept {
      TRACE();
      return Generator{*this};
    }

    void unhandled_exception() noexcept {
      TRACE();
      this->set_exception(std::current_exception());
    }

    void await_transform() = delete;

    pointer value = nullptr;
  };

  class iterator {
   public:
    using value_type = Generator::value_type;
    using difference_type = std::ptrdiff_t;

    iterator(const iterator&) = delete;
    iterator& operator=(const iterator&) = delete;

    iterator(iterator&& other) noexcept
        : handle_{std::exchange(other.handle_, nullptr)} {}
    iterator& operator=(iterator&& other) noexcept {
      handle_ = std::exchange(other.handle_, nullptr);
      return *this;
    }
    ~iterator() = default;

    iterator& operator++() noexcept {
      TRACE();
      handle_.resume();
      return *this;
    }

    iterator operator++(int) noexcept {
      TRACE();
      auto tmp = *this;
      ++*this;
      return tmp;
    }

    [[nodiscard]] reference operator*() const noexcept {
      TRACE();
      return *handle_.promise().value;
    }

    [[nodiscard]] pointer operator->() const noexcept {
      TRACE();
      return std::addressof(operator*());
    }

    [[nodiscard]] bool operator==(std::default_sentinel_t) const noexcept {
      TRACE();
      return handle_.done();
    }

   private:
    friend class Generator;

    explicit iterator(std::coroutine_handle<promise_type> h) noexcept
        : handle_{h} {}
    std::coroutine_handle<promise_type> handle_;
  };

  [[nodiscard]] iterator begin() {
    TRACE();
    (*handle_).resume();
    return iterator{*handle_};
  }

  [[nodiscard]] std::default_sentinel_t end() const noexcept {
    return std::default_sentinel;
  }

 private:
  explicit Generator(promise_type& promise) : handle_{promise} {}
  CoroHandle<promise_type> handle_;
};
}  // namespace coro

template <typename T>
constexpr auto std::ranges::enable_view<coro::Generator<T>> = true;