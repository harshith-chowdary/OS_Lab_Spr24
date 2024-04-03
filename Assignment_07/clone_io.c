#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <sys/wait.h>

#define STACK_SIZE (1024 * 1024) // 1 MB stack size

// Function to be executed by the cloned process
int child_function(void *arg) {
    printf("Child process: PID=%d\n", getpid());
    
    // Child shares I/O with parent
    printf("Hello from child process!\n");
    
    return 0;
}

int main() {
    printf("Parent process: PID=%d\n", getpid());
    
    // Allocate memory for child's stack
    char *stack = malloc(STACK_SIZE);
    if (stack == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    
    // Clone the child process, sharing I/O with the parent
    int flags = CLONE_VM | CLONE_FS | CLONE_FILES | SIGCHLD;
    pid_t pid = clone(child_function, stack + STACK_SIZE, flags, NULL);
    if (pid == -1) {
        perror("clone");
        exit(EXIT_FAILURE);
    }
    
    // Wait for the child to terminate
    waitpid(pid, NULL, 0);
    
    printf("Parent process exiting.\n");
    
    // Free the allocated stack memory
    free(stack);
    
    return 0;
}
