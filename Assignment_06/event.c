#include <stdio.h>
#include <stdlib.h>
#include <event.h>

int eventcmp ( event e1, event e2 )
{
   if (e1.time < e2.time) return -1;
   if (e1.time > e2.time) return 1;
   if (e1.type == 'R') return -1;
   if (e2.type == 'R') return 1;
   if (e1.type == 'P') return -1;
   if (e2.type == 'P') return 1;
   if (e1.type == 'S') return -1;
   if (e2.type == 'S') return 1;
   return 0;
}

int emptyQ ( eventQ E )
{
   return (E.n == 0);
}

eventQ initEQ ( char *fname )
{
   FILE *fp;
   eventQ E;
   char type;
   int i, j, l, r, m;
   event t;

   fp = (FILE *)fopen(fname, "r");
   E.n = 0;
   E.Q = (event *)malloc(128 * sizeof(event));
   while (1) {
      fscanf(fp, "%c", &type);
      if (type == 'E') break;
      E.Q[E.n].type = type;
      fscanf(fp, "%d%d", &(E.Q[E.n].time), &(E.Q[E.n].duration));
      ++E.n;
      while (fgetc(fp) != '\n');
   }
   fclose(fp);

   for (j = E.n / 2 - 1; j >= 0; --j) {
      i = j;
      while (1) {
         l = 2 * i + 1; r = 2* i + 2;
         if (l >= E.n) break;
         m = ( (r == E.n) || (eventcmp(E.Q[l],E.Q[r]) < 0) ) ? l : r;
         if (eventcmp(E.Q[i], E.Q[m]) < 0) break;
         t = E.Q[i]; E.Q[i] = E.Q[m]; E.Q[m] = t;
         i = m;
      }
   }

   return E;
}

eventQ addevent ( eventQ E, event e )
{
   int i, p;
   event t;

   E.Q[E.n] = e;
   i = E.n;
   while (1) {
      if (i == 0) break;
      p = (i - 1) / 2;
      if (eventcmp(E.Q[p], E.Q[i]) < 0) break;
      t = E.Q[i]; E.Q[i] = E.Q[p]; E.Q[p] = t;
      i = p;
   }
   ++E.n;
   return E;
}

eventQ delevent ( eventQ E )
{
   event t;
   int i, l, r, m;

   if (E.n > 0) {
      E.Q[0] = E.Q[E.n - 1];
      --E.n;
      i = 0;
      while (1) {
         l = 2 * i + 1; r = 2* i + 2;
         if (l >= E.n) break;
         m = ( (r == E.n) || (eventcmp(E.Q[l],E.Q[r]) < 0) ) ? l : r;
         if (eventcmp(E.Q[i], E.Q[m]) < 0) break;
         t = E.Q[i]; E.Q[i] = E.Q[m]; E.Q[m] = t;
         i = m;
      }
   }
   return E;
}

event nextevent ( eventQ E )
{
   if (E.n == 0) return (event){'E', 0, 0};
   return E.Q[0];
}
