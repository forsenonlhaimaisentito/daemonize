CFLAGS=-Wall -Wextra -O2

.PHONY: all clean

all: daemonize

daemonize: daemonize.c
	$(CC) $(CFLAGS) daemonize.c -o daemonize

clean:
	rm -f daemonize
