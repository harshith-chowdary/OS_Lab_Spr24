#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main ( int argc, char *argv[] )
{
   int n, *PERM, *P, i, j, t;
   int nextchild, nextparent, remaining;
   FILE *fp;

   n = (argc == 1) ? 25 : atoi(argv[1]);
   srand((unsigned int)time(NULL));

   PERM = (int *)malloc(n * sizeof(int));
   for (i=0; i<n; ++i) PERM[i] = i;
   for (i=0; i<n; ++i) {
      j = i + rand() % (n - i);
      t = PERM[i]; PERM[i] = PERM[j]; PERM[j] = t;
   }

   P = (int *)malloc(n * sizeof(int));
   P[PERM[0]] = PERM[0];
   nextchild = nextparent = 0; remaining = n - 1;
   while (remaining > 0) {
      if (remaining > 5)
         t = (nextchild == nextparent) ? 1 + rand() % 5 : rand() % 5;
      else
         t = (nextchild == nextparent) ? 1 + rand() % remaining : rand() % remaining;
      remaining -= t;
      for (i=0; i<t; ++i) {
         ++nextchild;
         P[PERM[nextchild]] = PERM[nextparent];
      }
      ++nextparent;
   }

   fp = (FILE *)fopen("tree.txt", "w");
   fprintf(fp, "%d\n", n);
   for (i=0; i<n; ++i) fprintf(fp, "%d %d\n", i, P[i]);
   fclose(fp);

   free(P); free(PERM);

   exit(0);
}
