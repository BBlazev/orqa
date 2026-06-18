
#include <pthread.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  int *buf;
  size_t cap;
  size_t head;
  size_t count;
  pthread_mutex_t lock;
  pthread_cond_t not_full;
  pthread_cond_t not_empty;
} deque_t;

int deque_init(deque_t *q, size_t capacity) {
  q->buf = (int *)malloc(capacity * sizeof(int));
  if (!q->buf)
    return -1;

  q->cap = capacity;
  q->count = 0;
  q->head = 0;

  if (pthread_mutex_init(&q->lock, NULL) != 0) {
    free(q->buf);
    return -1;
  }

  pthread_cond_init(&q->not_empty, NULL);
  pthread_cond_init(&q->not_full, NULL);

  return 0;
}

void deque_destroy(deque_t *q) {

  pthread_mutex_destroy(&q->lock);
  pthread_cond_destroy(&q->not_full);
  pthread_cond_destroy(&q->not_empty);

  free(q->buf);
  q->buf = NULL;
}

void deque_push_back(deque_t *q, int value) {

  pthread_mutex_lock(&q->lock);

  while (q->count == q->cap)
    pthread_cond_wait(&q->not_full, &q->lock);

  size_t slot = (q->head + q->count) % q->cap;

  q->buf[slot] = value;
  q->count++;

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

  pthread_mutex_unlock(&q->lock);
  return true;
}

int deque_pop_front(deque_t *q) {

  pthread_mutex_lock(&q->lock);

  while (q->count == 0)
    pthread_cond_wait(&q->not_empty, &q->lock);

  int value = q->buf[q->head];
  q->head = (q->head + 1) % q->cap;
  q->count--;

  pthread_cond_signal(&q->not_full);
  pthread_mutex_unlock(&q->lock);

  return value;
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

  pthread_mutex_unlock(&q->lock);
  return true;
}
