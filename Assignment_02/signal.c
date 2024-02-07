/***********************************************************/
/*** Sample program demonstrating the sending of signals ***/
/*** Last updated by Abhijit Das, 11-Jan-2024            ***/
/***********************************************************/

/***********************************************************/
/*** When this program runs, look at the status of the   ***/
/*** parent and child in another shell. Keep on running  ***/
/*** ps af | grep a.out                                  ***/
/***********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

/* The signal handler for the child process */
void childSigHandler ( int sig )
{
   if (sig == SIGUSR1) {
      printf("\n+++ Child : Received signal SIGUSR1 from parent...\n");
      sleep(1);
   } else if (sig == SIGUSR2) {
      printf("\n+++ Child : Received signal SIGUSR2 from parent...\n");
      sleep(5);
   }
}

int main ()
{
   int pid;

   srand((unsigned int)time(NULL));

   pid = fork();                                   /* Spawn the child process */
   if (pid) {
                                                            /* Parent process */
      int t;

      t = 5 + rand() % 5;
      printf("\n+++ Parent: Going to sleep for %d seconds\n", t);
      sleep(t);       /* Sleep for some time before sending a signal to child */

      t = 5 + rand() % 5;
      printf("+++ Parent: Going to send signal SIGUSR%d to child\n", t);
      kill(pid, (t == 1) ? SIGUSR1 : SIGUSR2);        /* Send signal to child */

      t = 5 + rand() % 5;
      printf("\n+++ Parent: Going to sleep for %d seconds\n", t);
      sleep(t);            /* Sleep for some time before suspending the child */
      printf("+++ Parent: Going to suspend child\n");
      kill(pid, SIGTSTP);
      
      t = 5 + rand() % 5;
      printf("\n+++ Parent: Going to sleep for %d seconds\n", t);
      sleep(t);             /* Sleep for some time before waking up the child */
      printf("+++ Parent: Going to wake up child\n");
      kill(pid, SIGCONT);
      
      t = 5 + rand() % 5;
      printf("\n+++ Parent: Going to sleep for %d seconds\n", t);
      sleep(t);             /* Sleep for some time before waking up the child */
      printf("+++ Parent: Going to wake up child\n");
      kill(pid, SIGCONT);
      
      t = 5 + rand() % 5;
      printf("\n+++ Parent: Going to sleep for %d seconds\n", t);
      sleep(t);            /* Sleep for some time before terminating the child */
      printf("+++ Parent: Going to terminate child\n");
      kill(pid, SIGINT);
      
      t = 5 + rand() % 5;
      printf("\n+++ Parent: Going to sleep for %d seconds\n", t);
      sleep(t);               /* Sleep for some time before waiting for child */
      waitpid(pid, NULL, 0);                        /* Wait for child to exit */
      printf("+++ Parent: Child exited\n");

      t = 5 + rand() % 5;                           
      printf("\n+++ Parent: Going to sleep for %d seconds\n", t);
      sleep(t);                         /* Sleep for some time before exiting */

   } else {
                                                             /* Child process */
      signal(SIGUSR1, childSigHandler);           /* Register SIGUSR1 handler */
      signal(SIGUSR2, childSigHandler);           /* Register SIGUSR2 handler */
      while (1) sleep(1);     /* Sleep until a signal is received from parent */

   }

   exit(0);
}
