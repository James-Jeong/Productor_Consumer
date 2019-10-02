all: hw1

hw1: hw1.o
	gcc -o hw1 hw1.o -lpthread -lrt

hw1.o: hw1.c
	gcc -c -o hw1.o hw1.c

clean:
	rm -f *.o
	rm -f hw1

.PHONY: clean
