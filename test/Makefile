CROSS_COMPILE = arm-linux-gnueabi-
CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld
CFLAGS := -Wall -Werror -g -I../tools/gmp/include
INCLUDE = -I. -I../linux-3.10-artik/usr/include -I../linux-3.10-artik/include -I../linux-3.10-artik/arch/arm/include
LDFLAGS := -L../tools/gmp/lib -lgmp -lm -static $(INCLUDE)


all: test infinite

test: test_prime.c
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

clean:
	rm -f test.o test

.PHONY: clean
