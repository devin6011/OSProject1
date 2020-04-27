CC = gcc -O2 -Wall -Wextra -Wshadow -pedantic -DDEBUG

all: scheduler

scheduler: scheduler.c
	$(CC) scheduler.c -o scheduler

clean:
	rm scheduler
