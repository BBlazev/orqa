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

void *producer(void *arg) {
  deque_t *q = arg;
  for (int i = 1; i <= 5; i++) {
    usleep(400000);
    deque_push_back(q, i * 10);
    printf("producer: pushing %d\n", i * 10);
  }

  return NULL;
}

int main(void) {

  deque_t q;
  deque_init(&q, 8);

  pthread_t w, p;
  pthread_create(&w, NULL, watcher, &q);
  pthread_create(&p, NULL, producer, &q);

  pthread_join(p, NULL);
  pthread_join(w, NULL);

  deque_destroy(&q);
  return 0;
}
