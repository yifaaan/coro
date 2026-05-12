// The MIT License (MIT)
//
// Copyright (c) 2021 Mateusz Pusz
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <concepts>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <syncstream>
#include <type_traits>

#include "async.h"
#include "synchronized_task.h"
#include "task.h"

coro::task<int> foo() {
    const int res = co_await coro::async([] { return 42; });
    co_await coro::async([&] { std::cout << "Result: " << res << std::endl; });
    co_return res + 23;
}

coro::task<> bar() {
    const auto res = co_await foo();
    std::cout << "Result of foo: " << res << std::endl;
}

coro::task<> example() {
    co_await bar();
    const auto res = co_await foo();
    std::cout << "Result of foo: " << res << std::endl;
}

int main() { coro::sync_wait(example()); }
