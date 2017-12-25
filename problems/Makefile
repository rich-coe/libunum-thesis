PROBLEMS = euler devil bank spike

CFLAGS= -g 

all: $(PROBLEMS)

%: %.c
	cc -g -I../libunum -o $@ $^ ../libunum/libunum.a

clean:
	rm -rf $(PROBLEMS)

