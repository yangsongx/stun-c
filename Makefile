PREFIX=/usr
BINDIR=$(PREFIX)/bin

CC=gcc
INSTALL=ginstall

all:	stun-c
distclean:	clean

clean:
	rm stun-c


install: all
	$(INSTALL) -D stun-c $(DESTDIR)$(BINDIR)/stun-c

stun-c:
	$(CC) stun.c -o stun-c -lpthread
