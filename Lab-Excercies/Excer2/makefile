CC=gcc

CFLAGS=-c

DEPS_PROGS=diningphilosophers.o

all: program
program:$(DEPS_PROGS)
	$(CC) $(DEPS_PROGS) -o a.out
diningphilosophers.o:diningphilosophers.c
	$(CC) $(CFLAGS) diningphilosophers.c
clean:
	rm -rf *.out *.o
