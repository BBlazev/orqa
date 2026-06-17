#pragma once
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

class ThreadPool {
public:
  ThreadPool(size_t n = 4);
  ~ThreadPool();

  ThreadPool(const ThreadPool &other) = delete;
  ThreadPool &operator=(const ThreadPool &other) = delete;

  ThreadPool(ThreadPool &&other) = delete;
  ThreadPool &operator=(ThreadPool &&other) = delete;

  void submit(std::function<void()> task);

private:
  std::mutex mtx_;
  std::condition_variable cv_;
  std::vector<std::thread> workers;
  std::queue<std::function<void()>> tasks;

  bool shutdown = false;

  void worker_task();
};
