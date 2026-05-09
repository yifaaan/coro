#include <iostream>

#include "generator.h"

coro::Generator<int> Example(int n) {
  for (int i = 0; i < n; ++i) {
    co_yield i;
  }
}

int main(int argc, char** argv) {
  auto generator = Example(3);
  for (auto value : generator) {
    std::cout << value << std::endl;
  }
  return 0;
}
