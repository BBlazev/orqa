#include <chrono>
#include <iostream>
#include <thread>

#include "ThreadPool.hpp"

struct Sensor {
  double read(int channel) { return channel * 2.8; }
};

int main() {

  ThreadPool t(4);
  Sensor sensor;

  auto r = t.enqueue(&Sensor::read, &sensor, 2);
  std::cout << "Example STRUCT FUNCTION: " << r.get() << "\n";

  for (int i = 1; i <= 6; ++i) {
    t.enqueue([i] {
      std::cout << "Example VOID TASK: " << std::to_string(i) << "\n";
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
    });
  }

  std::future<int> sum = t.enqueue([](int a, int b) { return a + b; }, 3, 4);
  std::cout << "Example INT TASK:" << sum.get() << "\n";

  std::vector<std::future<int>> v;
  for (int i = 1; i <= 5; i++)
    v.push_back(t.enqueue([](int x) { return x * x; }, i));

  for (auto &x : v)
    std::cout << "Example VECTOR TASK: " << x.get() << "\n";

  std::future<char> c = t.enqueue([](char c) { return c; }, 'A');
  std::cout << "Example CHAR TASK: " << c.get() << "\n";

  std::future<std::string> s =
      t.enqueue([](std::string str) { return str; }, "std::string funkcija");
  std::cout << "Example STRING TASK: " << s.get() << "\n";
  return 0;
}
