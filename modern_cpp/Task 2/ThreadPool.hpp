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

/**
 * @brief Fixed-size pool of worker threads draining a shared task queue.
 *
 * Construction spins up N workers that block on a condition variable until
 * work arrives. Tasks are FIFO-ordered in the queue.
 * Callers needing a result synchronise via the returned std::future.
 *
 * Thread-safety: every member that touches `tasks` or `shutdown` does so
 * under `mtx_`. Public methods (enqueue/submit) are safe to call from any
 * thread, including from inside a running task. The destructor must be called
 * from a single thread that does not itself hold the pool busy.
 *
 * Lifetime: the pool is non-copyable and non-movable. Holding raw std::thread
 * members and a mutex makes the object's address part of its identity (workers
 * captured `this`), so moving would dangle those captures, thats why all four
 * special members are deleted.
 */
class ThreadPool {
public:
  /**
   * @brief Construct and immediately start `n` worker threads.
   * @param n Number of workers. Defaults to 4. A value of 0 produces a pool
   *          that accepts tasks but never runs them -- futures would block
   *          forever, so callers should pass n >= 1.
   * @throws std::system_error if a thread cannot be created.
   */
  ThreadPool(size_t n = 4);

  /**
   * @brief Signals shutdown, wakes all workers, and joins them.
   * @note Drains the queue: tasks already enqueued still run to completion
   *       before workers exit (workers only return once the queue is empty
   *       AND shutdown is set). Blocks until every worker has joined.
   */
  ~ThreadPool();

  ThreadPool(const ThreadPool &other) = delete;
  ThreadPool &operator=(const ThreadPool &other) = delete;

  ThreadPool(ThreadPool &&other) = delete;
  ThreadPool &operator=(ThreadPool &&other) = delete;

  /**
   * @brief Enqueue a fire-and-forget task with no result handle.
   * @param task Callable taking no args and returning void.
   * @warning Unlike enqueue(), this does NOT check the shutdown flag, so a
   *          task submitted during/after destruction may be silently dropped
   *          (workers exit once the queue empties). Prefer enqueue() unless a
   *          future is genuinely unwanted.
   */
  void submit(std::function<void()> task);

  /**
   * @brief Enqueue any callable plus its arguments and get a future for the result.
   *
   * @tparam F    Callable type (function, lambda, member-fn pointer, functor).
   * @tparam Args Argument types forwarded to `f`.
   * @param f     The callable to run on a worker thread.
   * @param args  Arguments bound to `f` at enqueue time.
   * @return std::future<Ret> that resolves to f(args...) once a worker runs it.
   *         If the task throws, the exception is stored in the future and
   *         re-thrown at the call to .get().
   * @throws std::runtime_error if called after shutdown has begun.
   *
   * @note The packaged_task is heap-allocated via shared_ptr because
   *       std::function (the queue's element type) requires a copyable target,
   *       but packaged_task is move-only. The shared_ptr makes the wrapping
   *       lambda copyable while keeping a single underlying task alive.
   */
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

    cv_.notify_one(); // one task added -> wake exactly one waiter
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