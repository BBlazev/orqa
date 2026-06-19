#include <unistd.h>

#include "deque.h"

void *producer(void *arg) {
  deque_t *q = arg;
  for (int i = 1; i <= 5; i++) {
    usleep(300000);
    deque_push_back(q, i);
    printf("produced %d\n", i);
  }
  return NULL;
}

void *consumer(void *arg) {
  deque_t *q = arg;
  for (int i = 0; i < 5; i++) {
    int v = deque_pop_front(q);
    printf("        consumed %d\n", v);
  }
  return NULL;
}

int main(void) {
  deque_t q;
  deque_init(&q, 2);

  pthread_t p, c;
  // pthread_create(&p, NULL, producer, &q);
  // pthread_create(&c, NULL, consumer, &q);

  // pthread_join(p, NULL);
  // pthread_join(c, NULL);

  deque_push_back(&q, 1);
  deque_push_back(&q, 2);
  // deque_push_back(&q, 3);
  // deque_push_back(&q, 4);
  // deque_push_back(&q, 5);
  printf("count=%zu cap=%zu\n", q.count, q.cap);
  printf("trying timed push...\n");
  int rc = deque_push_back_timed(&q, 5, 500);
  printf("rc = %d  (-ETIMEDOUT is %d)\n", rc, -ETIMEDOUT);

  deque_destroy(&q);
  return 0;
}
