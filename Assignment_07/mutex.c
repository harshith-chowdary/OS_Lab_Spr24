#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sched.h>
#include <sys/sem.h>
#include <pthread.h>

#define MAX_THREADS 500 // Maximum number of threads
#define SEM_KEY 67890 // Semaphore key for mutex

#define wait(s) semop(s, &pop, 1)  /* pop is the structure we pass for doing
                the P(s) operation */
#define signal(s) semop(s, &vop, 1)  /* vop is the structure we pass for doing
                the V(s) operation */

struct sembuf pop, vop;

typedef struct {
    int semaphore;
    int tid; // Thread ID of the thread holding the lock
    int status; // 0 for unlocked, 1 for locked
} foothread_mutex_t;

// Function to initialize mutex
void foothread_mutex_init(foothread_mutex_t *mutex) {
    int key = ftok(".", getpid() ^ SEM_KEY);
    mutex->semaphore = semget(key, 1, IPC_CREAT | 0666);
    if (mutex->semaphore == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
    }
    if (semctl(mutex->semaphore, 0, SETVAL, 1) == -1) {
        perror("semctl");
        exit(EXIT_FAILURE);
    }
    mutex->tid = -1; // Initialize tid to -1, indicating no thread currently holds the lock
    mutex->status = 0; // Initialize status to 0, indicating the mutex is unlocked
}

// Function to lock mutex
void foothread_mutex_lock(foothread_mutex_t *mutex) {
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1 ; vop.sem_op = 1 ;

    wait(mutex->semaphore); // Wait until the semaphore becomes 0 (i.e., mutex is unlocked)
    mutex->tid = gettid(); // Set the tid to the current thread ID
    mutex->status = 1; // Set the status to 1, indicating the mutex is locked
}

// Function to unlock mutex
void foothread_mutex_unlock(foothread_mutex_t *mutex) {
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1 ; vop.sem_op = 1 ;

    if (mutex->tid != gettid()) {
        fprintf(stderr, "Error: Attempting to unlock mutex from a different thread\n");
        exit(EXIT_FAILURE);
    }
    if(mutex->status == 0) {
        fprintf(stderr, "Error: Attempting to unlock an already unlocked mutex\n");
        exit(EXIT_FAILURE);
    }
    
    signal(mutex->semaphore); // Release the lock by incrementing the semaphore
    mutex->tid = -1; // Reset tid to -1, indicating no thread currently holds the lock
    mutex->status = 0; // Reset status to 0, indicating the mutex is unlocked
}

// Function to destroy mutex
void foothread_mutex_destroy(foothread_mutex_t *mutex) {
    if (semctl(mutex->semaphore, 0, IPC_RMID, 0) == -1) {
        perror("semctl");
        exit(EXIT_FAILURE);
    }
}


// Shared resource
int shared_resource = 0;

#define NUM_THREADS 3
foothread_mutex_t mutex;

// Function that threads will execute
void *thread_function(void *arg) {
    for (int i = 0; i < 5; ++i) {
        // Lock the mutex before accessing the shared resource
        foothread_mutex_lock(&mutex);
        printf("Thread %d: Locked mutex\n", gettid());

        // Critical section: Access the shared resource
        shared_resource++;

        printf("Iteration %d => Shared resource value: %d\n", i, shared_resource);
        
        printf("Thread %d: Un-Locked mutex\n", gettid());
        // Unlock the mutex after accessing the shared resource
        foothread_mutex_unlock(&mutex);
    }

    return NULL;
}

int main() {
    // Initialize your custom mutex
    foothread_mutex_init(&mutex);

    // Create an array to hold thread IDs
    pthread_t threads[NUM_THREADS];

    // Create threads
    for (int i = 0; i < NUM_THREADS; ++i) {
        if (pthread_create(&threads[i], NULL, thread_function, NULL) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }

    // Wait for all threads to finish
    for (int i = 0; i < NUM_THREADS; ++i) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join");
            exit(EXIT_FAILURE);
        }
    }

    // Print the value of the shared resource
    printf("Value of shared resource after all threads finish: %d\n", shared_resource);

    // Destroy your custom mutex
    foothread_mutex_destroy(&mutex);

    return 0;
}