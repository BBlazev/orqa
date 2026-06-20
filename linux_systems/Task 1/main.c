#include <pthread.h>
#include <unistd.h>

#include <errno.h>
#include <stdio.h>

#include "deque.h"

void *producer(void *arg) {

  deque_t *q = arg;
  for (int i = 1; i <= 5; i++) {
    usleep(400000);
    deque_push_back(q, i * 10);
    printf("producer: pushing %d\n", i * 10);
  }

  return NULL;
}
static void *blocker(void *arg) {

  deque_t *q = arg;
  pthread_mutex_lock(&q->lock);
  usleep(300000);
  pthread_mutex_unlock(&q->lock);

  return NULL;
}

int main(void) {

  deque_t q;

  if (deque_init(&q, 4) != 0) {
    fprintf(stderr, "init failed\n");
    return 1;
  }

  deque_push_back(&q, 10);
  deque_push_back(&q, 20);
  deque_push_front(&q, 5);

  int v;
  deque_pop_front(&q, &v);
  printf("pop_front -> %d\n", v);

  deque_pop_back(&q, &v);
  printf("pop_back  -> %d\n", v);

  deque_pop_front(&q, &v);
  printf("pop_front -> %d\n", v);

  printf("\nfilling to capacity\n");
  for (int i = 1; i <= 4; i++)
    deque_push_back(&q, i * 100);

  printf("try_push on full -> %d (want %d)\n", deque_push_back_try(&q, 999),
         -EAGAIN);

  printf("timed_push on full, 500ms -> %d (want %d)\n",
         deque_push_back_timed(&q, 999, 500), -ETIMEDOUT);

  printf("\ndraining\n");

  while (deque_pop_front_try(&q, &v) == 0)
    printf("drained %d\n", v);

  printf("try_pop on empty -> %d (want %d)\n", deque_pop_front_try(&q, &v),
         -EAGAIN);

  printf("\ncontended trylock\n");

  pthread_t t;
  pthread_create(&t, NULL, blocker, &q);
  usleep(100000);
  printf("try_push while held -> %d (want %d)\n", deque_push_back_try(&q, 1),
         -EAGAIN);
  pthread_join(t, NULL);

  printf("\nblocking handoff across threads\n");

  pthread_t w, p;
  pthread_create(&w, NULL, watcher, &q);
  pthread_create(&p, NULL, producer, &q);
  pthread_join(p, NULL);
  pthread_join(w, NULL);

  deque_destroy(&q);
  return 0;
}
