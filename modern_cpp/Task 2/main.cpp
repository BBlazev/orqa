#include <chrono>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <thread>

#include "ThreadPool_classic.hpp"

int main() {

  // ThreadPool t(2);

  // for (int i = 1; i <= 10; i++) {
  //   t.submit([i] {
  //     std::cout << "Task: " << i << "\n";
  //     std::this_thread::sleep_for(std::chrono::milliseconds(200));
  //   });
  // }

  /*
   * zapakiram task koji spava i vraca int 21 u packaged_task
   * taj task stavljam u shared ptr zato sto std::function ne prima move only
   * objekte nego copy samo shared ptr se moze kopirati, stoga task-get_future()
   * radi u wrapperu pokrecem dereferencirani shared ptr task
   *
   */
  auto task = std::make_shared<std::packaged_task<int()>>([] {
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    return 21;
  });

  std::future<int> f = task->get_future();

  std::function<void()> wrapper = [task] { (*task)(); };

  std::cout << "waiting...\n";

  wrapper();

  std::cout << f.get() << "\n";

  return 0;
}
