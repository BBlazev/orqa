#include <poll.h>
#include <pthread.h>
#include <unistd.h>

#include <stdio.h>

#include "deque.h"

static int drain_evfd_count(int fd) {
  int n = 0;
  uint64_t v;
  while (read(fd, &v, sizeof(v)) == (ssize_t)sizeof(v))
    n++;
  return n;
}

static int evfd_is_ready(int fd) {
  struct pollfd pfd = {.fd = fd, .events = POLLIN};
  int rc = poll(&pfd, 1, 0);
  return rc > 0 && (pfd.revents & POLLIN);
}

int main(void) {
  deque_t q;
  deque_init(&q, 8);

  printf("empty: ready=%d (expect 0)\n", evfd_is_ready(q.ev_fd));

  deque_push_back(&q, 10);
  deque_push_back(&q, 20);
  deque_push_back(&q, 30);
  printf("after 3 pushes: count=%zu ready=%d\n", q.count,
         evfd_is_ready(q.ev_fd));

  int x;
  deque_pop_front(&q, &x); // count 3 -> 2
  printf("after 1 pop: count=%zu ready=%d\n", q.count, evfd_is_ready(q.ev_fd));

  int fd_units = drain_evfd_count(q.ev_fd);
  printf("eventfd units drained = %d, count = %zu\n", fd_units, q.count);

  deque_destroy(&q);
  return 0;
}
