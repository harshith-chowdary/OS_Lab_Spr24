#define _GNU_SOURCE

#include "foothread.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <signal.h>
#include <sys/syscall.h>
#include <sys/sem.h>
#include <sys/wait.h>

#define MAX_THREADS 50 // Maximum number of threads
#define SEM_KEY 67890 // Semaphore key for mutex

#define wait(s) semop(s, &pop, 1)  /* pop is the structure we pass for doing
                the P(s) operation */
#define signal(s) semop(s, &vop, 1)  /* vop is the structure we pass for doing
                the V(s) operation */

struct sembuf pop, vop;

int init = 0; // Semaphore ID for mutex

int mutex_semaphore; // Semaphore for mutex
ThreadEntry thread_table[MAX_THREADS]; // Shared thread table structure

int tid_counter = 1;

// Function to get TID of current thread
pid_t gettid() {
    return syscall(SYS_gettid);
}

// Function to print thread table
void print_thread_table() {
    wait(mutex_semaphore);
    printf("**********************************************************\n");
    for (int i = 0; i < MAX_THREADS; ++i) {
        if (thread_table[i].valid) {
            printf("TID %d, PTID %d, Join type %d\n", thread_table[i].tid, thread_table[i].parent_tid, thread_table[i].join_type);
        }
    }
    printf("**********************************************************\n");
    signal(mutex_semaphore);
}

// Function to create a new thread
void foothread_create(foothread_t *thread, foothread_attr_t *attr, int (*start_routine)(void *), void *arg) {
    int stack_size = (attr != NULL) ? attr->stack_size : FOOTHREAD_DEFAULT_STACK_SIZE;
    char *stack = (char *)malloc(stack_size);
    if (stack == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1 ; vop.sem_op = 1 ;

    if(init == 0){
        mutex_semaphore = semget(SEM_KEY, 1, IPC_CREAT | 0666);
        if (mutex_semaphore == -1) {
            perror("semget");
            exit(EXIT_FAILURE);
        }
        if (semctl(mutex_semaphore, 0, SETVAL, 1) == -1) {
            perror("semctl");
            exit(EXIT_FAILURE);
        }

        init = 1;

        for (int i = 0; i < MAX_THREADS; ++i) {
            thread_table[i].valid = 0;
        }
    }
    else{
        mutex_semaphore = semget(SEM_KEY, 1, 0666);
        if (mutex_semaphore == -1) {
            perror("semget");
            exit(EXIT_FAILURE);
        }
    }

    wait(mutex_semaphore);

    int i;
    for (i = 0; i < MAX_THREADS; ++i) {
        if (thread_table[i].valid == 0) {
            break;
        }
    }

    if (i == MAX_THREADS) {
        fprintf(stderr, "Error: Maximum number of threads reached\n");
        signal(mutex_semaphore);
        exit(EXIT_FAILURE);
    }

    if (i == 0) {
        thread_table[i].tid = gettid();
        thread_table[i].parent_tid = gettid(); // Store parent thread ID
        thread_table[i].join_type = FOOTHREAD_JOINABLE;
        thread_table[i].valid = 1;

        // Create semaphore for synchronization
        thread_table[i].semaphore = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
        if (thread_table[i].semaphore == -1) {
            perror("semget");
            signal(mutex_semaphore);
            exit(EXIT_FAILURE);
        }
        if (semctl(thread_table[i].semaphore, 0, SETVAL, 0) == -1) {
            perror("semctl");
            signal(mutex_semaphore);
            exit(EXIT_FAILURE);
        }

        // printf("MAIN THREAD => TID %d\n", thread_table[i].tid);
        i++;
    }
    
    pid_t tid = clone(start_routine, stack + stack_size, CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | CLONE_THREAD, arg);
    if (tid == -1) {
        perror("clone");
        signal(mutex_semaphore);
        exit(EXIT_FAILURE);
    }
    
    if (thread != NULL)
        thread->tid = tid; // Assign the TID to the foothread_t variable
    
    thread_table[i].tid = tid;
    thread_table[i].parent_tid = gettid(); // Store parent thread ID
    thread_table[i].join_type = (attr != NULL) ? attr->join_type : FOOTHREAD_DETACHED;
    thread_table[i].stack_size = stack_size;
    thread_table[i].valid = 1;
    
    // Create semaphore for synchronization
    thread_table[i].semaphore = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
    if (thread_table[i].semaphore == -1) {
        perror("semget");
        signal(mutex_semaphore);
        exit(EXIT_FAILURE);
    }
    if (semctl(thread_table[i].semaphore, 0, SETVAL, 0) == -1) {
        perror("semctl");
        signal(mutex_semaphore);
        exit(EXIT_FAILURE);
    }

    signal(mutex_semaphore);
}

// Function to exit the current thread
void foothread_exit() {
    pid_t tid = gettid();
    int is_joinable = 0;
    int semaphore = -1;
    pid_t parent_tid = -1;

    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1 ; vop.sem_op = 1 ;
    
    wait(mutex_semaphore);

    // printf("TID %d call exit\n", tid);
    // for (int i = 0; i < MAX_THREADS; ++i) {
    //     if (thread_table[i].valid) {
    //         printf("TID %d, PTID %d, Join type %d, Valid ? %d\n", thread_table[i].tid, thread_table[i].parent_tid, thread_table[i].join_type, thread_table[i].valid);
    //     }
    // }

    int index = -1;
    // Find the thread
    for (int i = 0; i < MAX_THREADS; ++i) {
        if (thread_table[i].valid && thread_table[i].tid == tid) {
            // Mark the thread as completed and get its join type, semaphore, and parent thread ID
            is_joinable = (thread_table[i].join_type == FOOTHREAD_JOINABLE);
            semaphore = thread_table[i].semaphore;
            parent_tid = thread_table[i].parent_tid;

            index = i;
            break;
        }
    }

    // If the thread is joinable, signal its parent thread to proceed
    if (is_joinable && parent_tid != tid) {
        for (int i = 0; i < MAX_THREADS; ++i) {
            if (thread_table[i].valid && thread_table[i].tid == parent_tid) {
                signal(thread_table[i].semaphore);

                // printf("SINGALLED MAIN THREAD\n");
                break;
            }
        }
    }

    int count = 0;
    bool mychild[MAX_THREADS];
    for (int i = 0; i < MAX_THREADS; ++i) {
        mychild[i] = false;
    }

    for (int i = 0; i < MAX_THREADS; ++i) {
        if (thread_table[i].valid && thread_table[i].parent_tid == tid && thread_table[i].join_type == FOOTHREAD_JOINABLE && thread_table[i].tid != tid) {
            // Mark the thread as completed and get its join type, semaphore, and parent thread ID
            count++;
            mychild[i] = true;
        }
    }

    if (count > 0) {
        // printf("TID %d => SEM = %d exits\n", tid, -count);
        signal(mutex_semaphore);

        pop.sem_op = -count;
        wait(semaphore);
    }
    else {
        signal(mutex_semaphore);
    }

    pop.sem_op = -1;
    wait(mutex_semaphore);

    if (index != -1) {
        // thread_table[index].valid = 0;

        // if(gettid() == getpid()) printf("TERMINATED\n");
        // printf("TID %d exit !!!\n", tid);

        for(int i = 0; i < MAX_THREADS; i++){
            if(mychild[i]){
                thread_table[i].valid = 0;
            }
        }
    }

    signal(mutex_semaphore);

    // print_thread_table();
}

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
    semctl(mutex->semaphore, 0, IPC_RMID, 0);

    // if (semctl(mutex->semaphore, 0, IPC_RMID, 0) == -1) {
    //     perror("semctl");
    //     exit(EXIT_FAILURE);
    // }
}

#define BAR_SEM_KEY 56789 // Example semaphore key

struct sembuf pop, vop;

// Function to initialize barrier
void foothread_barrier_init(foothread_barrier_t *barrier, int count) {
    int key = ftok(".", gettid() ^ BAR_SEM_KEY);
    barrier->semaphore = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
    if (barrier->semaphore == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
    }

    // Initialize semaphore value to 0
    if (semctl(barrier->semaphore, 0, SETVAL, 0) == -1) {
        perror("semctl");
        exit(EXIT_FAILURE);
    }

    key = ftok(".", (gettid() ^ BAR_SEM_KEY) + 1);
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
    
    // wait(mutex_semaphore);
    // printf("reached: %d, count: %d\n", barrier->reached, barrier->count);
    // signal(mutex_semaphore);

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
    semctl(barrier->semaphore, 0, IPC_RMID, 0);
    // if (semctl(barrier->semaphore, 0, IPC_RMID, 0) == -1) {
    //     perror("semctl");
    //     exit(EXIT_FAILURE);
    // }
    
    semctl(barrier->mutex_semaphore, 0, IPC_RMID, 0);
    // if (semctl(barrier->mutex_semaphore, 0, IPC_RMID, 0) == -1) {
    //     perror("semctl");
    //     exit(EXIT_FAILURE);
    // }

    barrier->count = 0;
    barrier->reached = 0;
}
