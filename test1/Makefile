CROSS_COMPILE = arm-linux-gnueabi-
CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld
CFLAGS := -Wall -Werror -g -I../tools/gmp/include
INCLUDE = -I. -I../linux-3.10-artik/usr/include -I../linux-3.10-artik/include -I../linux-3.10-artik/arch/arm/include
LDFLAGS := -L../tools/gmp/lib -lgmp -lm -static $(INCLUDE)
ORIENT_OBJ = orient_lock.o

all: trial pollard selector


trial: trial.c orient_lock.o
	$(CC) -o $@ $< $(ORIENT_OBJ) $(CFLAGS) $(LDFLAGS)

pollard: pollard.c orient_lock.o 
	$(CC) -o $@ $(ORIENT_OBJ) $< $(CFLAGS) $(LDFLAGS)

selector: selector.c orient_lock.o
	$(CC) -o $@ $(ORIENT_OBJ) $< $(CFLAGS) $(LDFLAGS)

orient_lock: orient_lock.c
	$(CC) -c $@.c $< $(CFLAGS) $(LDFLAGS)

clean:
	rm -f pollard.o trial.o selector.o orient_lock.o trial pollard selector

.PHONY: clean
