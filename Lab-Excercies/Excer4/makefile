CC=gcc

CFLAGS=-c

DEPS_PROGS=selectServer.o

all: program
program:$(DEPS_PROGS)
	$(CC) $(DEPS_PROGS) -o a.out
selectServer.o:selectServer.c
	$(CC) $(CFLAGS) selectServer.c
clean:
	rm -rf *.out *.o
