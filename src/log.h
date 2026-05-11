#pragma once

#include <iostream>
#include <source_location>

namespace coro {
// Logs to cout, with current indent
#define LOG(...) std::cout << std::string(ScopedLogger::indent, ' ') << __VA_ARGS__ << std::endl

// Logs in and out of a scope
struct ScopedLogger {
    static int indent;
    std::string name;
    ScopedLogger(std::string n)
        : name(std::move(n)) {
        LOG("\\" << name);
        indent++;
    }
    ~ScopedLogger() {
        indent--;
        LOG("/" << name);
    }
};

inline int ScopedLogger::indent = 0;

// Creates a ScopedLogger for the current function
#define LOGF() ScopedLogger logger##__LINE__(std::source_location::current().function_name())
}  // namespace coro