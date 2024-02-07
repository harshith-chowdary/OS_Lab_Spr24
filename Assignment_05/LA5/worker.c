#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

#define MAX_NODES 100
#define FILE_PATH "."
#define PROJECT_ID 1000

int n, id;
int *A, *T, *idx;
int sem_mtx, sem_ntfy, *sem_sync;

#define P(s) semop(s, &pop, 1)
#define V(s) semop(s, &vop, 1)

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <n> <id>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    n = atoi(argv[1]);
    id = atoi(argv[2]);

    struct sembuf pop, vop ;
	pop.sem_num = vop.sem_num = 0;
	pop.sem_flg = vop.sem_flg = 0;
	pop.sem_op = -1 ; vop.sem_op = 1 ;

    // Attach to shared memory segments
    key_t shm_key_A = ftok(FILE_PATH, PROJECT_ID);
    int shm_id_A = shmget(shm_key_A, n * n * sizeof(int), 0666);
    A = (int *)shmat(shm_id_A, NULL, 0);

    key_t shm_key_T = ftok(FILE_PATH, PROJECT_ID + 1);
    int shm_id_T = shmget(shm_key_T, n * sizeof(int), 0666);
    T = (int *)shmat(shm_id_T, NULL, 0);

    key_t shm_key_idx = ftok(FILE_PATH, PROJECT_ID + 2);
    int shm_id_idx = shmget(shm_key_idx, sizeof(int), 0666);
    idx = (int *)shmat(shm_id_idx, NULL, 0);

    // Attach to semaphores
    key_t sem_key_mtx = ftok(FILE_PATH, PROJECT_ID + 3);
    sem_mtx = semget(sem_key_mtx, 1, 0666);
    
    key_t sem_key_ntfy = ftok(FILE_PATH, PROJECT_ID + 4);
    sem_ntfy = semget(sem_key_ntfy, 1, 0666);

    sem_sync = (int *)malloc(n * sizeof(int));

    for (int i = 0; i < n; i++) {
        key_t sem_key_sync = ftok(FILE_PATH, PROJECT_ID + 5 + i);
        sem_sync[i] = semget(sem_key_sync, 1, 0666);
    }

    // Wait for sync signals from incoming nodes
    for (int i = 0; i < n; i++) {
        if (A[i * n + id] == 1 && i != id) { // Check if there's an incoming edge from node i
            P(sem_sync[id]);
        }
    }

    // Critical section
    P(sem_mtx);
    T[*idx] = id;
    (*idx)++;
    V(sem_mtx);

    // Send sync signals to outgoing nodes
    for (int i = 0; i < n; i++) {
        if (A[id * n + i] == 1 && i != id) { // Check if there's an outgoing edge to node i
            V(sem_sync[i]);
        }
    }

    // Notify boss
    V(sem_ntfy);

    // Detach from shared memory and semaphores
    shmdt(A);
    shmdt(T);
    shmdt(idx);

    // Terminate
    exit(EXIT_SUCCESS);
}