CC=gcc

CFLAGS=-c

DEPS_PROGS=dnsclient.o

all: program
program:$(DEPS_PROGS)
	$(CC) $(DEPS_PROGS) -o a.out
dnsclient.o:dnsclient.c
	$(CC) $(CFLAGS) dnsclient.c
clean:
	rm -rf *.out *.o
