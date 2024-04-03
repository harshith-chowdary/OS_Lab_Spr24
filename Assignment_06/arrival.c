#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main ()
{
   int n, i;
   FILE *fp;

   srand((unsigned int)time(NULL));

   fp = (FILE *)fopen("ARRIVAL.txt", "w");

   /* Patients */
   n = 25 + rand() % 16;
   for (i=0; i<n; ++i) fprintf(fp, "P %d %d\n", rand() % 345 - 15, 5 + rand() % 11);

   /* Reports */
   n = 5 + rand() % 11;
   for (i=0; i<n; ++i) fprintf(fp, "R %d %d\n", rand() % 330, 1 + rand() % 2);

   /* Sales Reps */
   n = 3 + rand() % 5;
   for (i=0; i<n; ++i) fprintf(fp, "S %d %d\n", rand() % 360 - 30, 5 + rand() % 6);

   fprintf(fp, "E\n");

   fclose(fp);
   exit(0);
}
