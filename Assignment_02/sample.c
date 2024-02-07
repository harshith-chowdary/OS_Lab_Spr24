/***********************************************************/
/*** Sample program demonstrating the sending of signals ***/
/*** Last updated by Abhijit Das, 11-Jan-2024            ***/
/***********************************************************/

/***********************************************************/
/*** When this program runs, look at the status of the   ***/
/*** parent and the child in another shell.              ***/
/*** To do so, keep on running the following command:    ***/
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

      /* Child is terminated, but the parent has not yet wait()-ed for it.    */
      /* So the child is now a zombie process. ps af will show this           */
      /* STAT Z and <defunct>.                                                */
      
      t = 5 + rand() % 5;
      printf("\n+++ Parent: Going to sleep for %d seconds\n", t);
      sleep(t);               /* Sleep for some time before waiting for child */
      waitpid(pid, NULL, 0);                        /* Wait for child to exit */
      printf("+++ Parent: Child exited\n");

      /* Child is now completely removed from the system as ps af will show.  */

      t = 5 + rand() % 5;                           
      printf("\n+++ Parent: Going to sleep for %d seconds\n", t);
      sleep(t);                         /* Sleep for some time before exiting */

   } else {
                                                             /* Child process */

      /* The child will receive several signals from its parent. The first    */
      /* signal is randomly chosen between SIGUSR1 abd SIGUSR2. The default   */
      /* handlers of these signals woul terminate the child. But the child    */
      /* refuses to die by registering the following custom-made handler      */
      /* for these two signals.                                               */
      signal(SIGUSR1, childSigHandler);           /* Register SIGUSR1 handler */
      signal(SIGUSR2, childSigHandler);           /* Register SIGUSR2 handler */

      /* The child will also receive the following signals. These are not     */
      /* handled by specific handler routines, so the default signal handling */
      /* will take place.                                                     */
      /* SIGTSTP : Suspend a process (same as ^Z sent to the child)           */
      /* SIGCONT : Make a suspended process alive again                       */
      /* SIGINT  : Interrupt and terminate a process (same as ^C from user)   */

      while (1) sleep(1);     /* Sleep until a signal is received from parent */

      /* Eventually the child is terminated by SIGINT from parent.            */

   }

   exit(0);
}
