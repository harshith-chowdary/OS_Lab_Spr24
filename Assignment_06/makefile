visitors:
	gcc -o arrival -Wall -I. arrival.c
	./arrival

event.o: event.h
	gcc -c -Wall -I. event.c

run: session.c event.o
	gcc -Wall -o session -pthread -I. session.c event.o
	./session

debug: session.c event.o
	gcc -Wall -o session -pthread -I. -g session.c event.o
	gdb session

all:
	make visitors
	make run

clean:
	-rm -f arrival arrival.txt event.o session
