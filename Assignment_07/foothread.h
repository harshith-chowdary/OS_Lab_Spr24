#define _GNU_SOURCE

#ifndef FOOTHREAD_H
#define FOOTHREAD_H

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <stdbool.h>

#define FOOTHREAD_DEFAULT_STACK_SIZE (2 * 1024 * 1024) // 2MB

#define FOOTHREAD_JOINABLE 1
#define FOOTHREAD_DETACHED 0

typedef struct {
    int join_type;
    int stack_size;
} foothread_attr_t;

#define FOOTHREAD_ATTR_INITIALIZER { FOOTHREAD_DETACHED, FOOTHREAD_DEFAULT_STACK_SIZE }

typedef struct {
    int tid;
} foothread_t;

void foothread_create(foothread_t *, foothread_attr_t *, int (*)(void *), void *);
void foothread_attr_setjointype(foothread_attr_t *, int);
void foothread_attr_setstacksize(foothread_attr_t *, int);
void foothread_exit();

typedef struct {
    int semaphore;
    int tid; // Thread ID of the thread holding the lock
    int status; // 0 for unlocked, 1 for locked
} foothread_mutex_t;

void foothread_mutex_init(foothread_mutex_t *);
void foothread_mutex_lock(foothread_mutex_t *);
void foothread_mutex_unlock(foothread_mutex_t *);
void foothread_mutex_destroy(foothread_mutex_t *);

typedef struct {
    int count;
    int reached;
    int semaphore;
    int mutex_semaphore;
} foothread_barrier_t;

void foothread_barrier_init(foothread_barrier_t *, int);
void foothread_barrier_wait(foothread_barrier_t *);
void foothread_barrier_destroy(foothread_barrier_t *);

// Shared thread table structure
typedef struct {
    pid_t tid;
    pid_t parent_tid;
    int join_type;
    int stack_size;
    int valid; // Flag to indicate if thread table entry is valid
    int semaphore; // Semaphore for thread synchronization
} ThreadEntry;

void print_thread_table();

#endif /* FOOTHREAD_H */
