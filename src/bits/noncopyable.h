#pragma once

namespace coro {

struct Noncopyable {
  Noncopyable() = default;
  ~Noncopyable() = default;
  Noncopyable(const Noncopyable&) = delete;
  Noncopyable& operator=(const Noncopyable&) = delete;
  Noncopyable(Noncopyable&&) = delete;
  Noncopyable& operator=(Noncopyable&&) = delete;
};

}  // namespace coro