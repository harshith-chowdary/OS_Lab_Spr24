#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

// Constants
#define MAX_JOBS 10

// Job status
#define RUNNING 0
#define FINISHED 1
#define TERMINATED 2
#define SUSPENDED 3
#define KILLED 4

char STATUS[5][15] = {"RUNNING\0", "FINISHED\0", "TERMINATED\0", "SUSPENDED\0", "KILLED\0"};

// Process Table Entry structure
struct ProcessTableEntry {
    pid_t pid;
    pid_t pgid;
    int status;
    char argument[MAX_JOBS];
};

// Global variables
struct ProcessTableEntry PT[MAX_JOBS + 1];  // PT[0] reserved for manager itself
int currentJobIndex = 0;
int totalJobs = 0;

// Function prototypes
void handleSIGINT(int signo);
void handleSIGTSTP(int signo);
void spawnJob();
void printHelp();
void printProcessTable();
void cleanup();

int main() {
    // Set signal handlers for SIGINT and SIGTSTP
    signal(SIGINT, handleSIGINT);
    signal(SIGTSTP, handleSIGTSTP);

    // Initialize process table
    PT[0].pid = getpid();
    PT[0].pgid = getpgid(0);
    PT[0].status = RUNNING;
    sprintf(PT[0].argument, "mgr");

    char userInput;

    while (1) {
        printf("mgr> ");
        fflush(stdin);
        fflush(stdout);
        scanf(" %c", &userInput);

        int i, numberOfSuspendedJobs, chosenIndex, status;

        switch (userInput) {
            case 'p':
                printProcessTable();
                break;

            case 'r':
                if (totalJobs < MAX_JOBS) {
                    totalJobs++;
                    currentJobIndex = totalJobs;
                    spawnJob();
                } else {
                    printf("Cannot initiate more than %d jobs.\n", MAX_JOBS);
                    printf("Process Table is full. Quitting...\n");
                    exit(0);
                }
                break;

            case 'c':
                // Implement job continuation
                printf("Suspened jobs : ");
                numberOfSuspendedJobs = 0;
                
                for(i = 0; i <= totalJobs; i++){
                    if(PT[i].status == SUSPENDED){
                        if(numberOfSuspendedJobs == 0) printf("%d", i);
                        else printf(", %d", i);
                        numberOfSuspendedJobs++;
                    }
                }

                if(numberOfSuspendedJobs == 0) {
                    printf("NONE\n\n"); 
                    break;
                }

                printf(" (Pick one): ");
                
                scanf("%d", &chosenIndex);

                printf("\n");

                if (chosenIndex < 0 || chosenIndex > totalJobs || PT[chosenIndex].status != SUSPENDED) {
                    printf("Invalid choice or job not in suspended state.\n");
                    break;
                }

                // Update process table entry to indicate the job is running
                PT[chosenIndex].status = RUNNING;

                currentJobIndex = chosenIndex;

                // Send SIGCONT to the chosen job
                kill(PT[chosenIndex].pid, SIGCONT);

                // Wait for the child process to finish
                waitpid(PT[chosenIndex].pid, &status, WUNTRACED);

                // Update job status to FINISHED after the child process exits
                if(PT[currentJobIndex].status == RUNNING) {
                    PT[currentJobIndex].status = FINISHED;
                    printf("\n");
                }
                break;

            case 'k':
                // Implement job killing
                printf("Suspened jobs : ");
                numberOfSuspendedJobs = 0;
                
                for(i = 0; i <= totalJobs; i++){
                    if(PT[i].status == SUSPENDED){
                        if(numberOfSuspendedJobs == 0) printf("%d", i);
                        else printf(", %d", i);
                        numberOfSuspendedJobs++;
                    }
                }

                if(numberOfSuspendedJobs == 0) {
                    printf("NONE\n\n"); 
                    break;
                }

                printf(" (Pick one): ");
                
                scanf("%d", &chosenIndex);

                printf("\n");

                if (chosenIndex < 0 || chosenIndex > totalJobs || PT[chosenIndex].status != SUSPENDED) {
                    printf("Invalid choice or job not in suspended state.\n");
                    break;
                }

                // Send SIGKILL to the chosen job
                kill(PT[chosenIndex].pid, SIGKILL);

                // Update process table entry to indicate the job is KILLED
                PT[chosenIndex].status = KILLED;

                // Wait for the child process to finish
                waitpid(PT[chosenIndex].pid, &status, WUNTRACED);
                break;

            case 'h':
                printHelp();
                break;

            case 'q':
                cleanup();
                exit(0);

            default:
                printf("Invalid command. Please try again.\n\n");
        }
    }

    return 0;
}

void handleSIGINT(int signo) {
    // Handle SIGINT by sending it to the currently running job
    if (currentJobIndex != -1 && PT[currentJobIndex].status == RUNNING) {
        PT[currentJobIndex].status = TERMINATED;
        kill(PT[currentJobIndex].pid, SIGINT);
        printf("\n\n");
    }
    else printf("No running job to SIGINT.\n");
}

void handleSIGTSTP(int signo) {
    // Handle SIGTSTP by sending it to the currently running job
    if (currentJobIndex != -1 && PT[currentJobIndex].status == RUNNING) {
        PT[currentJobIndex].status = SUSPENDED;
        kill(PT[currentJobIndex].pid, SIGTSTP);
        printf("\n\n");
    }
    else printf("No running job to SIGTSTP\n");
}

void spawnJob() {
    pid_t pid = fork();

    char argument[2];
    argument[0] = 'A' + rand() % 26;
    argument[1] = '\0';

    if (pid == 0) {
        // Child process

        execl("./job", "./job", &argument, NULL);  // Execute the job program
        exit(0);
    } else if (pid > 0) {
        setpgid(pid, pid);  // Change process group id to the child's pid

        // Set signal handlers for SIGINT and SIGTSTP
        signal(SIGINT, handleSIGINT);
        signal(SIGTSTP, handleSIGTSTP);

        // Parent process
        PT[currentJobIndex].pid = pid;
        PT[currentJobIndex].pgid = getpgid(pid);
        PT[currentJobIndex].status = RUNNING;

        memset(PT[currentJobIndex].argument, 0, MAX_JOBS);
        sprintf(PT[currentJobIndex].argument, "Job %s", argument);

        // Wait for the child process to finish
        int status;
        waitpid(pid, &status, WUNTRACED);

        // Update job status to FINISHED after the child process exits
        if(PT[currentJobIndex].status == RUNNING) {
            PT[currentJobIndex].status = FINISHED;
        }
    } else {
        // Error in forking
        perror("Error forking child process");
    }
}

void printHelp() {
    printf("\tCommands:\n");
    printf("\t\tp - Print the Process Table\n");
    printf("\t\tr - Start a new job\n");
    printf("\t\tc - Continue a suspended job\n");
    printf("\t\tk - Kill a suspended job\n");
    printf("\t\th - Print help\n");
    printf("\t\tq - Quit\n");
    printf("\n");
}

void printProcessTable() {
    printf("Process Table:\n");
    printf("NO\tPID\tPGID\tSTATUS\t\tNAME\n");
    int i;
    for (i = 0; i <= totalJobs; i++) {
        printf("%d\t%d\t%d\t%s\t\t%s\n", i, PT[i].pid, PT[i].pgid, STATUS[PT[i].status], PT[i].argument);
    }
    printf("\n");
}

void cleanup() {
    // Kill any running or suspended jobs before exiting
    int i;
    for (i = 1; i <= totalJobs; i++) {
        if (PT[i].status == RUNNING || PT[i].status == SUSPENDED) {
            kill(PT[i].pid, SIGKILL);
        }
    }

    printf("All jobs terminated. Exiting...\n");
}