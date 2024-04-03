#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define BUFFER_SIZE 5

int buffer[BUFFER_SIZE];
int count = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t full = PTHREAD_COND_INITIALIZER;

void *producer(void *arg) {
  int i;
  for (i = 0; i < 10; i++) {
    pthread_mutex_lock(&mutex);
    while (count == BUFFER_SIZE) {
      pthread_cond_wait(&full, &mutex);
    }
    buffer[count] = i;
    count++;
    printf("<<< Produced %d\n", i);
    pthread_cond_signal(&full);
    pthread_mutex_unlock(&mutex);
  }
  return NULL;
}

void *consumer(void *arg) {
  int i;
  for (i = 0; i < 10; i++) {
    pthread_mutex_lock(&mutex);
    while (count == 0) {
      pthread_cond_wait(&full, &mutex);
    }
    count--;
    printf(">>> Consumed %d\n", buffer[count]);
    pthread_cond_signal(&full);
    pthread_mutex_unlock(&mutex);
  }
  return NULL;
}

int main() {
  pthread_t producer_thread, consumer_thread;
  pthread_create(&producer_thread, NULL, producer, NULL);
  pthread_create(&consumer_thread, NULL, consumer, NULL);
  pthread_join(producer_thread, NULL);
  pthread_join(consumer_thread, NULL);
  return 0;
}