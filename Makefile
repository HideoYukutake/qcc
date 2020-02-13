CFLAGS=-std=c11 -g -static
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

qcc: $(OBJS)
	$(CC) -o qcc $(OBJS) $(LDFLAGS)

$(OBJS): qcc.h

test: qcc
	./test.sh

clean:
	rm -f qcc *.o *~ tmp*

.PHONY: test clean
