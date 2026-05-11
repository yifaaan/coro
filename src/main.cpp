#include <iostream>

#include "task.h"

coro::task<int> f() {
    std::cout << "1\n";
    std::cout << "123\n";
    co_return 32;
}

int main() {
    auto x = f();
    std::cout  << "def\n";
    // x.resume();
    std::cout << "abc";
}