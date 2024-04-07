all: master scheduler mmu process clear

master: master.c
	gcc -g master.c -o master -lm

scheduler: sched.c
	gcc sched.c -o scheduler -lm

mmu: mmu.c
	gcc mmu.c -o mmu -lm

process: proc.c
	gcc proc.c -o process -lm

clean: clear
	rm -f process master scheduler mmu *.o 

clear: clear.c
	gcc clear.c -o clearer -lm
	./clearer
	rm clearer