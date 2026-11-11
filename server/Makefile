CC = $(CROSS_COMPILE)gcc
CFLAGS ?= -Wall -Wextra -Werror -std=c11 -O2
LDFLAGS ?=
.PHONY: all default clean
default: all
all: aesdsocket
aesdsocket: aesdsocket.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)
clean:
	rm -f aesdsocket *.o
