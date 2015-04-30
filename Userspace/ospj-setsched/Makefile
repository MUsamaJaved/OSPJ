CC=gcc -O0 -g -Wall -Werror -std=c99
prefix=/usr
executable=ospj-setsched
%.o: %.c
		$(CC) -DBUILD_VERSION='"0.2-$(shell date +%Y%m%d%H%M)"' -o $@ -c $<

all: ospj-setsched

ospj-setsched: ospj_setsched.o
		$(CC) -o $(executable) ospj_setsched.o

install: ospj-setsched
		install -m 0755 $(executable) $(prefix)/bin

.PHONY: install

clean:
		rm -f *.o $(executable)
