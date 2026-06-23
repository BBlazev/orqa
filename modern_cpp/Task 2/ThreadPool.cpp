#include <cstddef>
#include <mutex>

#include "ThreadPool.hpp"

ThreadPool::ThreadPool(size_t n) {
  for (size_t i = 0; i < n; ++i) {
    workers.emplace_back([this] { worker_task(); });
  }
}

ThreadPool::~ThreadPool() {
  {
    std::unique_lock<std::mutex> lock(mtx_);
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

    if (shutdown)
      throw std::runtime_error("submit on stopped ThreadPool");
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

      // Drain semantics: only exit once there is genuinely no work left.
      // Checking shutdown alone would discard still-queued tasks.
      if (shutdown && tasks.empty())
        return;

      task = std::move(tasks.front());
      tasks.pop();
    }
    // Run OUTSIDE the lock: holding mtx_ during task execution would serialise
    // every worker and defeat the whole point of the pool.
    task();
  }
}