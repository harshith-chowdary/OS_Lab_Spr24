SHELL := /bin/bash

lib: foothread.o
	gcc -shared -Wall -o libfoothread.so foothread.o

foothread.o: foothread.h foothread.c
	gcc -fPIC -Wall -c -I. foothread.c

app: lib computesum.c
	gcc -Wall -g -Wl,-rpath=. -I. -L. -o computesum computesum.c -lfoothread

tree: gentree.c
	gcc -Wall -o gentree gentree.c

run: app
	./computesum

debug: app
	gdb ./computesum

newrun: app tree
	./gentree
	./computesum

clean:
	-rm -f foothread.o libfoothread.so computesum gentree
