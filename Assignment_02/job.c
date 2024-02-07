#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int main ( int argc, char *argv[] )
{
   int i;

   srand((unsigned int)time(NULL));

   printf("Running job %s\n", (argc == 1) ? "" : argv[1]);
   for (i=0; i<10; ++i) {
      if (argc == 1) printf("%c ", 'A' + rand() % 26);
      else printf("%c ", argv[1][0]);
      fflush(stdout);
      sleep(1);
   }
   printf("\n");
   exit(0);
}
