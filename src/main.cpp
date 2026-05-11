#include <iostream>

#include "synchronized_task.h"
#include "task.h"
#include "log.h"

using coro::ScopedLogger;

coro::task<int> g() {
    LOGF();
    co_return 32;
}

coro::task<int> f() {
    LOG("f");
    auto x = co_await g();
    std::cout << "x = " << x << '\n';
    co_return 32;
}

int main() {
    // LOGF();
    // auto x = f();

    // x.resume();
    auto x = coro::sync_wait(f());
    std::cout << x;
}