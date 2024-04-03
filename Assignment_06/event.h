#ifndef __EVENT_H
#define __EVENT_H

typedef struct {
   char type;
   int time;
   int duration;
} event;

typedef struct {
   int n;
   event *Q;
} eventQ;

int eventcmp ( event , event ) ;
int emptyQ ( eventQ ) ;
eventQ initEQ ( char * ) ;
eventQ addevent ( eventQ , event ) ;
eventQ delevent ( eventQ ) ;
event nextevent ( eventQ ) ;

#endif
