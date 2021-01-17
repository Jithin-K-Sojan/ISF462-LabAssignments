CC=gcc

CFLAGS=-c

DEPS_PROGS=signal.o

all: program
program:$(DEPS_PROGS)
	$(CC) $(DEPS_PROGS) -o a.out
signal.o:signal.c
	$(CC) $(CFLAGS) signal.c
clean:
	rm -rf *.out *.o
