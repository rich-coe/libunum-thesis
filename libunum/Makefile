UBITS=12
DIGITS=2

CFLAGS= -g

all: libunum.a

libunum.a: table.o unum.o
	ar rcs libunum.a table.o unum.o

unum.o: unum.c
	cc ${CFLAGS} -c unum.c -lm

table.o: table.c
	cc ${CFLAGS} -c table.c

table.c: gen
	./gen 2> unum.h 1> table.c

gen: gen.c
	cc ${CFLAGS} -o gen -DUBITS=${UBITS} -DDIGITS=${DIGITS} gen.c -lm

%: %.c libunum.a
	cc ${CFLAGS} $^ -o $@

clean:
	rm -f gen table.c unum.h table.o unum.o libunum.a
