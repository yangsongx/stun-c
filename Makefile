PREFIX=/usr
BINDIR=$(PREFIX)/bin

CC=gcc
SRC=stun.c another_method.c

OBJS=$(SRC:.c=.o)

CFLAGS = -c -g -O0 -W -Wall -Wunused-parameter
LDFLAGS = -pthread

all:stun

stun:$(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) $< -o $@

.PHONY:clean
clean:
	rm *.o stun
