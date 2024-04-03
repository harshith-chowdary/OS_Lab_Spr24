#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>

#define STACK_SIZE (1024 * 1024) // Stack size for the cloned process

int shared_variable = 0; // Shared variable between the calling process and the cloned process

int cloned_function(void *arg) {
    printf("Cloned process: shared_variable = %d\n", shared_variable);

    shared_variable = 43; // Modify the shared variable in the cloned process
    return 0;
}

int main() {
    char *stack = malloc(STACK_SIZE); // Allocate stack for the cloned process

    if (stack == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    // Clone a new process sharing the same memory space
    int flags = CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | CLONE_THREAD;
    pid_t pid = clone(cloned_function, stack + STACK_SIZE, flags, NULL);

    if (pid == -1) {
        perror("clone");
        exit(EXIT_FAILURE);
    }

    shared_variable = 42; // Modify the shared variable in the calling process

    sleep(10); // Sleep for 100ms to allow the cloned process to execute

    printf("Calling process: shared_variable = %d\n", shared_variable);

    free(stack); // Free the allocated stack memory

    return 0;
}
