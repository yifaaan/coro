#pragma once

namespace coro {

template <typename T = void, typename Allocator = void>
class [[nodiscard]] task {
public:
    using value_type = T;

    struct promise_type : private detail::noncopyable {

    };
};
}  // namespace coro