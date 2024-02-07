#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <time.h>

const int MAXSLEEP = 5;
const int MAX_CONSUMERS = 100;
#define SHM_SIZE (sizeof(int) * 2)

int main(int argc, char *argv[]) {
    int n, t, shmid;
    srand(time(NULL));

    if (argc != 3) {
        printf("Usage: %s <n> <t>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    n = atoi(argv[1]);
    t = atoi(argv[2]);


    pid_t pid[n+1]; // PIDs of consumers

    printf("n = %d\nt = %d\n", n, t);

    if (n <= 0 || n > MAX_CONSUMERS || t <= 0) {
        printf("Invalid arguments\n");
        exit(EXIT_FAILURE);
    }

    // Create shared memory segment
    if ((shmid = shmget(IPC_PRIVATE, SHM_SIZE, IPC_CREAT | 0666)) < 0) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    int * ptrs[n + 1]; // Pointers to shared memory for each consumer

    // Fork consumers
    for (int i = 1; i <= n; i++) {
        pid[i] = fork();
        if (pid[i] < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid[i] == 0) { // Child process (consumer)
            printf("\t\t\t\t\t\tConsumer %2d is alive\n", i);
            int count = 0, sum = 0;

            ptrs[i] = (int *) shmat(shmid, NULL, 0);
            while (1) {
                if (ptrs[i][0] == i) {
            
            #ifdef VERBOSE
                    printf("\t\t\t\t\t\tConsumer %2d reads %d\n", i, ptrs[i][1]);
            #endif
                    sum += ptrs[i][1];
                    count++;
                    ptrs[i][0] = 0;
                    if (ptrs[i][1] == -1) {
                        printf("\t\t\t\t\t\tConsumer %2d has read %3d items: Checksum = %d\n", i, count, sum);
                        shmdt(ptrs[i]);
                        exit(EXIT_SUCCESS);
                    }
                } else if (ptrs[i][0] == -1) {
                    printf("\t\t\t\t\t\tConsumer %2d has read %3d items: Checksum = %d\n", i, count, sum);
                    shmdt(ptrs[i]);
                    exit(EXIT_SUCCESS);
                }
            }
        }
    }

    printf("Producer is alive\n");

    // Initialize pointers to shared memory
    if ((ptrs[0] = shmat(shmid, NULL, 0)) == (int *) -1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    ptrs[0][0] = 0; // Initialize M[0] to 0

    int num_items[n + 1]; // Number of items produced for each consumer
    int sum_items[n + 1]; // Checksum for each consumer
    memset(num_items, 0, sizeof(num_items));
    memset(sum_items, 0, sizeof(sum_items));

    // Producer loop
    for (int i = 0; i < t; i++) {
        int item = rand() % 899 + 100; // Generate random 3-digit int
        int c = rand() % n + 1; // Generate random consumer
        while (ptrs[0][0] != 0) {} // Busy wait until M[0] becomes 0

#ifdef VERBOSE
            printf("Producer produces %3d for Consumer %2d\n", item, c);
#endif
        ptrs[0][0] = c;

#ifdef SLEEP
        // usleep(rand() % MAXSLEEP + 1);
        usleep(MAXSLEEP);
#endif

        ptrs[0][1] = item;
        num_items[c]++;
        sum_items[c] += item;
    }

    // Wait until the last item is consumed
    while (ptrs[0][0] != 0) {}
    ptrs[0][0] = -1;

    // Wait for all consumers to terminate
    for (int i = 1; i <= n; i++) {
        waitpid(pid[i], NULL, 0);
    }

    printf("Producer has produced %d items\n", t);

    for (int i = 1; i <= n; i++) {
        printf("%3d items for Consumer %2d: Checksum = %d\n", num_items[i], i, sum_items[i]);
    }

    // Detach and remove shared memory
    shmdt(ptrs[0]);
    shmctl(shmid, IPC_RMID, NULL);

    return 0;
}