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

int n;
int *A, *T, *idx;
int sem_mtx, sem_ntfy, *sem_sync;

#define P(s) semop(s, &pop, 1)
#define V(s) semop(s, &vop, 1)

int isTopologicalSortValid(const int *A, const int *T, const int n) {
    int inDegree[MAX_NODES] = {0};
    int i, j;

    // Calculate in-degree for each vertex
    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            if (*(A + i * n + j) == 1) {
                inDegree[j]++;
            }
        }
    }

    // Check if the given order is a valid topological sort
    for (i = 0; i < n; i++) {
        if (inDegree[T[i]] != 0) {
            return 0; // Order is invalid
        }
        for (j = 0; j < n; j++) {
            if (*(A + T[i] * n + j) == 1) {
                inDegree[j]--;
            }
        }
    }

    return 1; // Order is valid
}

int main() {
    // Read adjacency matrix from graph.txt
    FILE *file = fopen("graph.txt", "r");
    if (!file) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    struct sembuf pop, vop ;
	pop.sem_num = vop.sem_num = 0;
	pop.sem_flg = vop.sem_flg = 0;
	pop.sem_op = -1 ; vop.sem_op = 1 ;

    printf("+++ Boss: Setup done...\n");

    fscanf(file, "%d", &n);

    int size = n * n * sizeof(int);

    // Create shared memory segments
    key_t shm_key_A = ftok(FILE_PATH, PROJECT_ID);
    int shm_id_A = shmget(shm_key_A, size, 0666 | IPC_CREAT);
    A = (int *)shmat(shm_id_A, NULL, 0);

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            fscanf(file, "%d", &A[i * n + j]);
        }
    }
    fclose(file);

    key_t shm_key_T = ftok(FILE_PATH, PROJECT_ID + 1);
    int shm_id_T = shmget(shm_key_T, n * sizeof(int), 0666 | IPC_CREAT);
    T = (int *)shmat(shm_id_T, NULL, 0);

    key_t shm_key_idx = ftok(FILE_PATH, PROJECT_ID + 2);
    int shm_id_idx = shmget(shm_key_idx, sizeof(int), 0666 | IPC_CREAT);
    idx = (int *)shmat(shm_id_idx, NULL, 0);

    // Initialize index
    *idx = 0;

    // Create and initialize semaphores
    key_t sem_key_mtx = ftok(FILE_PATH, PROJECT_ID + 3);
    sem_mtx = semget(sem_key_mtx, 1, 0666 | IPC_CREAT);
    semctl(sem_mtx, 0, SETVAL, 1);

    key_t sem_key_ntfy = ftok(FILE_PATH, PROJECT_ID + 4);
    sem_ntfy = semget(sem_key_ntfy, 1, 0666 | IPC_CREAT);
    semctl(sem_ntfy, 0, SETVAL, 0);

    sem_sync = (int *)malloc(n * sizeof(int));
    for (int i = 0; i < n; i++) {
        key_t sem_key_sync = ftok(FILE_PATH, PROJECT_ID + 5 + i);
        sem_sync[i] = semget(sem_key_sync, 1, 0666 | IPC_CREAT);
        semctl(sem_sync[i], 0, SETVAL, 0);
    }

    // Wait for notifications from workers
    for (int i = 0; i < n; i++) {
        P(sem_ntfy);
    }

    // Print and check topological sorting
    printf("\n+++ Topological sorting of the vertices\n==> ");
    for (int i = 0; i < n; i++) {
        printf("%d ", T[i]);
    }
    printf("\n\n");

    // Check if the topological sort is valid
    if (isTopologicalSortValid(A, T, n)) {
        printf("+++ Topological sorting is valid.\n");
    } else {
        printf("+++ Topological sorting is invalid.\n");
    }

    // Clean up semaphores and shared memory
    semctl(sem_mtx, 0, IPC_RMID, 0);
    semctl(sem_ntfy, 0, IPC_RMID, 0);
    for (int i = 0; i < n; i++) {
        semctl(sem_sync[i], 0, IPC_RMID, 0);
    }

    shmdt(A);
    shmdt(T);
    shmdt(idx);
    shmctl(shm_id_A, IPC_RMID, NULL);
    shmctl(shm_id_T, IPC_RMID, NULL);
    shmctl(shm_id_idx, IPC_RMID, NULL);

    printf("\n+++ Boss: Well done, my team...\n");

    return 0;
}