build: all

all: alocator.o
	gcc alocator.o -lm -o alocator
alocator.o: main2.c
	gcc -c -Wall -lm main2.c -o alocator.o
clean:
	rm -f all
	rm -f alocator.o
	rm -f alocator
