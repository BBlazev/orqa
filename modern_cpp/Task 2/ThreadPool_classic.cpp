#include <cstddef>
#include <mutex>

#include "ThreadPool_classic.hpp"

ThreadPool::ThreadPool(size_t n) {
  for (size_t i = 0; i < n; ++i) {
    workers.emplace_back([this] { worker_task(); });
  }
}
ThreadPool::~ThreadPool() {
  {
    std::unique_lock<std::mutex> lokc(mtx_);
    shutdown = true;
  }

  cv_.notify_all();

  for (auto &thread : workers)
    if (thread.joinable())
      thread.join();
}

void ThreadPool::submit(std::function<void()> task) {
  {
    std::unique_lock<std::mutex> lock(mtx_);
    tasks.emplace(std::move(task));
  }
  cv_.notify_one();
}

void ThreadPool::worker_task() {
  while (true) {

    std::function<void()> task;
    {
      std::unique_lock<std::mutex> lock(mtx_);

      cv_.wait(lock, [this] { return !tasks.empty() || shutdown; });

      if (shutdown && tasks.empty())
        return;

      task = std::move(tasks.front());
      tasks.pop();
    }
    task();
  }
}
