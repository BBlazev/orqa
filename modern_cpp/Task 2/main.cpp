#include <chrono>
#include <iostream>
#include <thread>

#include "ThreadPool.hpp"

int main() {

  ThreadPool t(2);

  for (int i = 1; i <= 10; i++) {
    t.submit([i] {
      std::cout << "Task: " << i << "\n";
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
    });
  }

  return 0;
}
