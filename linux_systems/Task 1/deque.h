
#include <pthread.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
/**
 * Double‑ended queue (deque) with:
 *   - bounded capacity (circular buffer)
 *   - blocking & non‑blocking push/pop
 *   - timed push (with absolute deadline)
 *   - eventfd for epoll‑based wakeup (used by watcher)
 *
 * A single mutex protects all internal state.
 * Two condition variables allow blocking push/pop.
 * The eventfd counter tracks the number of items; it is incremented on
 * every successful push and decremented on every successful pop
 */
typedef struct {
  int *buf;
  size_t cap;
  size_t head;
  size_t count;
  pthread_mutex_t lock;
  pthread_cond_t not_full;
  pthread_cond_t not_empty;

  int ev_fd;

} deque_t;

static void deadline(struct timespec *ts, long timeout_ms) {
  clock_gettime(CLOCK_REALTIME, ts);
  ts->tv_sec += timeout_ms / 1000;
  ts->tv_nsec += (timeout_ms % 1000) * 1000000L;
  if (ts->tv_nsec >= 1000000000L) {
    ts->tv_sec += 1;
    ts->tv_nsec -= 1000000000L;
  }
}
/**
 * Initialise deque with given capacity.
 * Returns 0 on success, -1 on allocation/initialisation failure.
 * Creates an eventfd with EFD_NONBLOCK | EFD_SEMAPHORE.
 */
int deque_init(deque_t *q, size_t capacity) {
  q->buf = (int *)malloc(capacity * sizeof(int));
  if (!q->buf)
    return -1;

  q->cap = capacity;
  q->count = 0;
  q->head = 0;

  q->ev_fd = eventfd(0, EFD_NONBLOCK | EFD_SEMAPHORE);
  if (q->ev_fd < 0) {
    free(q->buf);
    return -1;
  }

  if (pthread_mutex_init(&q->lock, NULL) != 0) {
    free(q->buf);
    return -1;
  }

  pthread_cond_init(&q->not_empty, NULL);
  pthread_cond_init(&q->not_full, NULL);

  return 0;
}

void deque_destroy(deque_t *q) {

  close(q->ev_fd);

  pthread_mutex_destroy(&q->lock);
  pthread_cond_destroy(&q->not_full);
  pthread_cond_destroy(&q->not_empty);

  free(q->buf);
  q->buf = NULL;
}

int deque_push_back_try(deque_t *q, int value) {
  if (pthread_mutex_trylock(&q->lock) != 0)
    return -EAGAIN;

  if (q->count == q->cap) {
    pthread_mutex_unlock(&q->lock);
    return -EAGAIN;
  }

  size_t slot = (q->head + q->count) % q->cap;
  q->buf[slot] = value;
  q->count++;

  uint64_t one = 1;
  write(q->ev_fd, &one, sizeof(one));

  pthread_cond_signal(&q->not_empty);
  pthread_mutex_unlock(&q->lock);
  return 0;
}
/**
 * Non‑blocking pop from front.
 * Returns 0 on success, -EAGAIN if mutex is locked or queue is empty.
 * On success, *out receives the value and the eventfd count is decremented.
 */
int deque_pop_front_try(deque_t *q, int *out) {

  if (pthread_mutex_trylock(&q->lock) != 0)
    return -EAGAIN;

  if (q->count == 0) {
    pthread_mutex_unlock(&q->lock);
    return -EAGAIN;
  }

  *out = q->buf[q->head];
  q->head = (q->head + 1) % q->cap;
  q->count--;
  uint64_t one = 1;
  read(q->ev_fd, &one, sizeof(one));

  pthread_cond_signal(&q->not_full);
  pthread_mutex_unlock(&q->lock);

  return 0;
}
/**
 * Timed push to back. Blocks until space is available or timeout (ms) elapses.
 * Returns 0 on success, -ETIMEDOUT if timed out.
 * Uses CLOCK_REALTIME for deadline
 */
int deque_push_back_timed(deque_t *q, int value, long timeout_ms) {

  struct timespec deadline_time;
  deadline(&deadline_time, timeout_ms);

  pthread_mutex_lock(&q->lock);

  while (q->count == q->cap) {
    int rc = pthread_cond_timedwait(&q->not_full, &q->lock, &deadline_time);
    if (rc == ETIMEDOUT) {
      pthread_mutex_unlock(&q->lock);
      return -ETIMEDOUT;
    }
  }

  size_t slot = (q->head + q->count) % q->cap;

  q->buf[slot] = value;
  q->count++;
  uint64_t one = 1;
  write(q->ev_fd, &one, sizeof(one));

  pthread_cond_signal(&q->not_empty);
  pthread_mutex_unlock(&q->lock);

  return 0;
}

void deque_push_back(deque_t *q, int value) {

  pthread_mutex_lock(&q->lock);

  while (q->count == q->cap)
    pthread_cond_wait(&q->not_full, &q->lock);

  size_t slot = (q->head + q->count) % q->cap;

  q->buf[slot] = value;
  q->count++;
  uint64_t one = 1;
  write(q->ev_fd, &one, sizeof(one));

  pthread_cond_signal(&q->not_empty);
  pthread_mutex_unlock(&q->lock);
}

bool deque_push_front(deque_t *q, int value) {

  pthread_mutex_lock(&q->lock);

  if (q->count == q->cap) {
    pthread_mutex_unlock(&q->lock);
    return false;
  }

  q->head = (q->head + q->cap - 1) % q->cap;
  q->buf[q->head] = value;
  q->count++;
  uint64_t one = 1;
  write(q->ev_fd, &one, sizeof(one));

  pthread_mutex_unlock(&q->lock);
  return true;
}

int deque_pop_front(deque_t *q, int *x) {

  pthread_mutex_lock(&q->lock);
  while (q->count == 0)
    pthread_cond_wait(&q->not_empty, &q->lock);

  int value = q->buf[q->head];
  q->head = (q->head + 1) % q->cap;
  q->count--;

  uint64_t one;
  read(q->ev_fd, &one, sizeof(one));

  pthread_cond_signal(&q->not_full);
  pthread_mutex_unlock(&q->lock);
  *x = value;

  return 0;
}

bool deque_pop_back(deque_t *q, int *out) {

  pthread_mutex_lock(&q->lock);

  if (q->count == 0) {
    pthread_mutex_unlock(&q->lock);
    return false;
  }

  size_t slot = (q->head + q->count - 1) % q->cap;

  *out = q->buf[slot];
  q->count--;
  uint64_t one = 1;
  read(q->ev_fd, &one, sizeof(one));
  pthread_mutex_unlock(&q->lock);
  return true;
}
/**
 * Watcher thread: uses epoll to wait for the eventfd.
 * When an event arrives, it tries to pop from the front.
 *
 */
void *watcher(void *arg) {
  deque_t *q = arg;

  int epfd = epoll_create1(0);
  if (!epfd)
    return NULL;

  struct epoll_event ev = {
      .events = EPOLLIN,
      .data.fd = q->ev_fd,

  };

  epoll_ctl(epfd, EPOLL_CTL_ADD, q->ev_fd, &ev);

  for (int i = 0; i < 5; i++) {
    struct epoll_event out[1];

    int n = epoll_wait(epfd, out, 1, -1);

    if (n > 0 && (out[0].events & EPOLLIN)) {
      int v;
      if (deque_pop_front_try(q, &v) == 0)
        printf("watcher: eppol up, popped %d\n", v);
    }
  }

  close(epfd);
  return NULL;
}
