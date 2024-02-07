SHELL = /bin/bash

all:
	gcc -Wall -o gengraph gengraph.c
	gcc -Wall -o boss boss.c
	gcc -Wall -o worker worker.c

clean:
	-rm -f gengraph boss worker
