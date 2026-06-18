#pragma once
#include <cinttypes>

#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>

class ThreadPool {
public:
  ThreadPool(size_t n = 4);
  ~ThreadPool();

  ThreadPool(const ThreadPool &other) = delete;
  ThreadPool &operator=(const ThreadPool &other) = delete;

  ThreadPool(ThreadPool &&other) = delete;
  ThreadPool &operator=(ThreadPool &&other) = delete;

  void submit(std::function<void()> task);

  template <class F, class... Args>
  auto enqueue(F &&f, Args &&...args)
      -> std::future<std::invoke_result_t<F, Args...>> {
    using Ret = std::invoke_result_t<F, Args...>;

    auto task = std::make_shared<std::packaged_task<Ret()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));

    std::future<Ret> result = task->get_future();
    {
      std::unique_lock<std::mutex> lock(mtx_);
      if (shutdown)
        throw std::runtime_error("enqueue on stopped ThreadPool");
      tasks.emplace([task] { (*task)(); });
    }

    cv_.notify_one();
    return result;
  }

private:
  std::mutex mtx_;
  std::condition_variable cv_;
  std::vector<std::thread> workers;
  std::queue<std::function<void()>> tasks;

  bool shutdown = false;

  void worker_task();
};
