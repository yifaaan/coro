#include "async.h"
#include "task.h"
#include "log.h"

#include <chrono>
#include <exception>
#include <iostream>
#include <thread>

using coro::ScopedLogger;

coro::task<int> test_async_value() {
    LOGF();
    auto value = co_await coro::async([] {
        std::this_thread::sleep_for(std::chrono::milliseconds{100});
        return 42;
    });

    std::cout << "async value = " << value << '\n';
    co_return value;
}

coro::task<void> run_async_examples() {
    auto value = co_await test_async_value();
    std::cout << "received value = " << value << '\n';
    co_return;
}

int main() {
    try {
        LOGF();
        auto f = run_async_examples();

        f.resume();
        // coro::sync_wait(run_async_examples());
        // std::cout << "finish\n";
        // f.resume();
    } catch (const std::exception& ex) {
        std::cout << "unhandled exception: " << ex.what() << '\n';
    }
}
