#pragma once

#include <format>
#include <iostream>
#include <source_location>
#include <syncstream>
namespace coro::detail {

struct Location {
  std::source_location loc;
  friend std::ostream& operator<<(std::ostream& os, const Location& loc) {
    os << std::format(
        "[TRACE]: Function: {}\n[TRACE]: File: {}, Line: {}, Column: {}\n",
        loc.loc.function_name(), loc.loc.file_name(), loc.loc.line(),
        loc.loc.column());
    return os;
  }
};

struct TraceOnFinish {
  Location loc;
  ~TraceOnFinish() {
    std::osyncstream(std::cout) << "[TRACE]: FINISH\n" << loc;
  }
};

}  // namespace coro::detail

namespace coro {

// inline void TraceFunc(
//     std::source_location loc = std::source_location::current()) {
//   std::osyncstream(std::cout) << detail::Location{loc} << std::endl;
// }

inline detail::TraceOnFinish TraceFunc(
    std::source_location loc = std::source_location::current()) {
  std::osyncstream(std::cout) << "[TRACE]: START\n";
  std::osyncstream(std::cout) << detail::Location{loc};
  return detail::TraceOnFinish{loc};
}

#define TRACE() auto _ = ::coro::TraceFunc()

}  // namespace coro