# The source is main.c and attacker.c
CC = gcc
SOURCE = main.c 
ATTACKER = attacker.c
CFLAGS = -O0 -std=gnu99 -g
# Path: makefile
# The object files are main.o and attacker.o

compile: 
	$(CC) $(CFLAGS) $(SOURCE) -o main 
	$(CC) $(CFLAGS) $(ATTACKER) -o attacker

run: compile
	./main

clean:
	rm -f main attacker
