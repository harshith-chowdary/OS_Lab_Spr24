#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>
#include <pthread.h>

#define wait(s) semop(s, &pop, 1)  /* pop is the structure we pass for doing
                the P(s) operation */
#define signal(s) semop(s, &vop, 1)  /* vop is the structure we pass for doing
                the V(s) operation */

#define SEM_KEY 56789 // Example semaphore key

struct sembuf pop, vop;

typedef struct {
    int count;
    int reached;
    int semaphore;
    int mutex_semaphore;
} foothread_barrier_t;

// Function to initialize barrier
void foothread_barrier_init(foothread_barrier_t *barrier, int count) {
    int key = ftok(".", SEM_KEY);
    barrier->semaphore = semget(key, 1, IPC_CREAT | 0666);
    if (barrier->semaphore == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
    }

    // Initialize semaphore value to 0
    if (semctl(barrier->semaphore, 0, SETVAL, 0) == -1) {
        perror("semctl");
        exit(EXIT_FAILURE);
    }

    key = ftok(".", SEM_KEY + 1);
    barrier->mutex_semaphore = semget(key, 1, IPC_CREAT | 0666);
    if (barrier->mutex_semaphore == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
    }

    // Initialize semaphore value to 1
    if (semctl(barrier->mutex_semaphore, 0, SETVAL, 1) == -1) {
        perror("semctl");
        exit(EXIT_FAILURE);
    }

    barrier->count = count;
    barrier->reached = 0;
}

// Function to wait at barrier
void foothread_barrier_wait(foothread_barrier_t *barrier) {
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1 ; vop.sem_op = 1 ;

    wait(barrier->mutex_semaphore); // Wait for the mutex
    barrier->reached = barrier->reached + 1; // Increment the number of threads reached the barrier
    printf("reached: %d, count: %d\n", barrier->reached, barrier->count);

    if (barrier->reached == barrier->count) {
        signal(barrier->mutex_semaphore); // Release the mutex
        // All threads reached the barrier, release them
        vop.sem_op = barrier->count;
        signal(barrier->semaphore);
        barrier->reached = 0; // Reset reached count

        wait(barrier->semaphore); // Wait for all threads to pass the barrier
    }
    else{
        signal(barrier->mutex_semaphore); // Release the mutex
        wait(barrier->semaphore); // Wait for release from the barrier
    }
}

// Function to destroy barrier
void foothread_barrier_destroy(foothread_barrier_t *barrier) {
    if (semctl(barrier->semaphore, 0, IPC_RMID) == -1) {
        perror("semctl");
        exit(EXIT_FAILURE);
    }

    if (semctl(barrier->mutex_semaphore, 0, IPC_RMID) == -1) {
        perror("semctl");
        exit(EXIT_FAILURE);
    }

    barrier->count = 0;
    barrier->reached = 0;
}

void *thread_function(void *arg) {
    foothread_barrier_t *barrier = (foothread_barrier_t *)arg;

    printf("Thread %ld reached the barrier\n", pthread_self());
    fflush(stdout);
    foothread_barrier_wait(barrier);
    printf("Thread %ld passed the barrier\n", pthread_self());
    fflush(stdout);
    pthread_exit(NULL);
}

int main() {
    int mutex_sem = semget(SEM_KEY, 1, IPC_CREAT | 0666);
    if (mutex_sem == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
    }
    if (semctl(mutex_sem, 0, SETVAL, 1) == -1) {
        perror("semctl");
        exit(EXIT_FAILURE);
    }

    foothread_barrier_t barrier;
    int thread_count = 5;

    foothread_barrier_init(&barrier, thread_count);

    printf("Barrier initialized\n");
    printf("count: %d\n", barrier.count);
    printf("reached: %d\n", barrier.reached);

    pthread_t threads[thread_count];

    for (int i = 0; i < thread_count; i++) {
        if (pthread_create(&threads[i], NULL, thread_function, (void *)&barrier) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < thread_count; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join");
            exit(EXIT_FAILURE);
        }
    }

    foothread_barrier_destroy(&barrier);

    return 0;
}

// int main() {
//     int mutex_sem = semget(SEM_KEY, 1, IPC_CREAT | 0666);
//     if (mutex_sem == -1) {
//         perror("semget");
//         exit(EXIT_FAILURE);
//     }

//     // Initialize semaphore value to 1
//     if (semctl(mutex_sem, 0, SETVAL, 1) == -1) {
//         perror("semctl");
//         exit(EXIT_FAILURE);
//     }

//     foothread_barrier_t barrier;
//     int thread_count = 5; // Example: 5 threads

//     foothread_barrier_init(&barrier, thread_count);

//     printf("Barrier initialized\n");
//     printf("count: %d\n", barrier.count);
//     printf("reached: %d\n", barrier.reached);

//     // Simulate threads reaching the barrier
//     for (int i = 0; i < thread_count; i++) {
//         if (fork() == 0) { // Child process
//             int mutex_semaphore = mutex_sem;
//             // Simulate work
//             printf("Thread %d reached the barrier\n", getpid());
//             fflush(stdout);
//             foothread_barrier_wait(&barrier, mutex_semaphore);
//             printf("Thread %d passed the barrier\n", getpid());
//             fflush(stdout);
//             exit(EXIT_SUCCESS);
//         }
//     }

//     // // Wait for all child processes to finish
//     // for (int i = 0; i < thread_count; i++) {
//     //     wait(NULL);
//     // }

//     foothread_barrier_destroy(&barrier);

//     return 0;
// }
