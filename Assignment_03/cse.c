#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX 100

int main(int argc, char*argv[]){
    if (argc<2) {
        fprintf(stderr, "Usage: %s <mode>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *mode = argv[1];

    if(strcmp(mode, "S") == 0){
        printf("+++ CSE in supervisor mode: Started\n");

        // printf("STDIN_FILENO : %d\n", STDIN_FILENO);
        // printf("STDOUT_FILENO : %d\n", STDOUT_FILENO);

        // Create pipes
        int pipe_a[2], pipe_b[2];
        if (pipe(pipe_a) == -1 || pipe(pipe_b) == -1) {
            perror("Pipe creation failed");
            exit(EXIT_FAILURE);
        }

        printf("+++ CSE in supervisor mode: Forking first child in command-input mode\n");
        pid_t pid_1 = fork();

        if (pid_1 == -1) {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        }

        if (pid_1 == 0) {

            // Execute xterm in First Child mode
            char pa0[5], pa1[5], pb0[5], pb1[5];
            sprintf(pa0, "%d", pipe_a[0]);
            sprintf(pa1, "%d", pipe_a[1]);
            sprintf(pb0, "%d", pipe_b[0]);
            sprintf(pb1, "%d", pipe_b[1]);
            
            execlp("xterm", "xterm", "-T", "First Child", "-e", argv[0], "C", pa0, pa1, pb0, pb1, NULL);
            perror("execlp failed");
            exit(EXIT_FAILURE);
        }

        printf("+++ CSE in supervisor mode: Forking second child in execute mod\n");
        pid_t pid_2 = fork();
        if (pid_2 == -1) {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        }

        if (pid_2 == 0) {

            // Execute xterm in Second Child mode
            char pa0[5], pa1[5], pb0[5], pb1[5];
            sprintf(pa0, "%d", pipe_a[0]);
            sprintf(pa1, "%d", pipe_a[1]);
            sprintf(pb0, "%d", pipe_b[0]);
            sprintf(pb1, "%d", pipe_b[1]);
            
            execlp("xterm", "xterm", "-T", "Second Child", "-e", argv[0], "E", pa0, pa1, pb0, pb1, NULL);
            perror("execlp failed");
            exit(EXIT_FAILURE);
        }

        // Parent process (Supervisor)
        close(pipe_a[0]); // Close unused read end of a pipe
        close(pipe_a[1]); // Close unused write end of a pipe
        close(pipe_b[0]); // Close unused read end of b pipe
        close(pipe_b[1]); // Close unused write end of b pipe

        // Wait for both child processes to finish
        waitpid(pid_1, NULL, WUNTRACED);
        waitpid(pid_2, NULL, WUNTRACED);

        printf("+++ CSE in supervisor mode: First child terminated\n");
        printf("+++ CSE in supervisor mode: Second child terminated\n");

    }
    else if(strcmp(mode, "C") == 0){
        int a_r = atoi(argv[2]);
        int a_w = atoi(argv[3]);

        int b_r = atoi(argv[4]);
        int b_w = atoi(argv[5]);

        close(a_r);
        close(b_w);

        int IN = dup(STDIN_FILENO);
        int OUT = dup(STDOUT_FILENO);

        char buff[MAX];
        char st;
        int mode = 0;
        while(1){
            if(mode==0){
                dup2(a_w, STDOUT_FILENO);
                dup2(IN, STDIN_FILENO);

                write(OUT, "Enter command> ", 15);
                memset(buff, 0, MAX);
                fgets(buff, MAX-1, stdin);
                
                printf("%s", buff);
                fflush(stdout);

                if(strstr(buff, "exit")){
                    exit(0);
                }

                if(strstr(buff, "swaprole")){
                    mode = 1-mode;
                }

                continue;
            }
            
            dup2(b_r, STDIN_FILENO);
            dup2(OUT, STDOUT_FILENO);

            printf("Waiting for command> ");
            fflush(stdout);

            memset(buff, 0, MAX);
            fgets(buff, MAX-1, stdin);

            int i = 0;
            while(i<MAX && buff[i] != '\0'){
                if(buff[i] == '\n'){
                    buff[i] = '\0';
                    break;
                }
                i++;
            }

            printf("%s\n", buff);

            fflush(stdout);

            if(strstr(buff, "exit")){
                exit(0);
            }

            pid_t pid = fork();

            if (pid == -1) {
                perror("Grandchild Fork failed");
                exit(EXIT_FAILURE);
            }

            if (pid == 0) {
                dup2(IN, STDIN_FILENO);
                dup2(OUT, STDOUT_FILENO);

                char *targs[MAX];
                char *token = strtok(buff, " ");

                int i = 0;
                while (token != NULL) {
                    targs[i] = token;
                    token = strtok(NULL, " ");
                    i++;
                }

                targs[i] = NULL;

                if (execvp(targs[0], targs) == -1) {
                    perror("execvp failed");
                    exit(EXIT_FAILURE);
                }

                exit(EXIT_FAILURE);
            }

            waitpid(pid, NULL, WUNTRACED);

            fflush(stdout);

            if(strstr(buff, "swaprole")){
                mode = 1-mode;
            }
        }
        
    }
    else{
        int a_r = atoi(argv[2]);
        int a_w = atoi(argv[3]);

        int b_r = atoi(argv[4]);
        int b_w = atoi(argv[5]);

        close(a_w);
        close(b_r);

        int IN = dup(STDIN_FILENO);
        int OUT = dup(STDOUT_FILENO);

        char buff[MAX];
        char st;
        int mode = 1;
        while(1){

            if(mode){
                dup2(a_r, STDIN_FILENO);
                dup2(OUT, STDOUT_FILENO);

                printf("Waiting for command> ");
                fflush(stdout);

                memset(buff, 0, MAX);
                fgets(buff, MAX-1, stdin);

                int i = 0;
                while(i<MAX && buff[i] != '\0'){
                    if(buff[i] == '\n'){
                        buff[i] = '\0';
                        break;
                    }
                    i++;
                }

                printf("%s\n", buff);

                fflush(stdout);

                if(strstr(buff, "exit")){
                    exit(0);
                }

                if(strstr(buff, "swaprole")){
                    mode = 1-mode;
                    continue;
                }

                pid_t pid = fork();

                if (pid == -1) {
                    perror("Grandchild Fork failed");
                    exit(EXIT_FAILURE);
                }

                if (pid == 0) {
                    dup2(IN, STDIN_FILENO);
                    dup2(OUT, STDOUT_FILENO);

                    char *targs[MAX];
                    char *token = strtok(buff, " ");

                    int i = 0;
                    while (token != NULL) {
                        targs[i] = token;
                        token = strtok(NULL, " ");
                        i++;
                    }

                    targs[i] = NULL;

                    if (execvp(targs[0], targs) == -1) {
                        perror("execvp failed");
                        exit(EXIT_FAILURE);
                    }

                    exit(EXIT_FAILURE);
                }

                waitpid(pid, NULL, WUNTRACED);

                fflush(stdout);

                continue;
            }

            dup2(b_w, STDOUT_FILENO);
            dup2(IN, STDIN_FILENO);

            write(OUT, "Enter command> ", 15);
            memset(buff, 0, MAX);
            fgets(buff, MAX-1, stdin);
            
            printf("%s", buff);
            fflush(stdout);

            if(strstr(buff, "exit")){
                exit(0);
            }

            if(strstr(buff, "swaprole")){
                mode = 1-mode;
            }
        }
                
    }

    return 0;
}