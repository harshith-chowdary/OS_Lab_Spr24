#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main ( int argc, char *argv[] )
{
   int n, i, j, t, **A, *PERM;
   double p;

   srand((unsigned int)time(NULL));
   n = (argc > 1) ? atoi(argv[1]) : 16;
   p = (argc > 2) ? atof(argv[2]) : 0.2;

   A = (int **)malloc(n * sizeof(int *));
   for (i=0; i<n; ++i) A[i] = (int *)malloc(n * sizeof(int));
   PERM = (int *)malloc(n * sizeof(int));
   for (i=0; i<n; ++i) PERM[i] = i;
   for (i=0; i<n; ++i) {
      j = i + rand() % (n - i);
      t = PERM[i]; PERM[i] = PERM[j]; PERM[j] = t;
   }
   for (i=0; i<n; ++i) {
      A[i][i] = 0;
      for (j=i+1; j<n; ++j) {
         A[PERM[i]][PERM[j]] = (((double)rand() / (double)RAND_MAX) <= p) ? 1 : 0;
         A[PERM[j]][PERM[i]] = 0;
      }
   }
   printf("%d\n", n);
   for (i=0; i<n; ++i) {
      for (j=0; j<n; ++j) printf("%d ", A[i][j]);
      printf("\n");
   }
   exit(0);
}
