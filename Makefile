CC = gcc -O2 -Wall -Wextra -Wshadow -pedantic -DDEBUG
CXX = g++ -std=c++17 -O2 -Wall -Wextra -Wshadow

all: scheduler theoretical

scheduler: scheduler.c
	$(CC) scheduler.c -o scheduler

theoretical: theoretical.cpp
	$(CXX) theoretical.cpp -o theoretical

clean:
	rm scheduler theoretical
