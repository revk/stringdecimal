all: stringdecimal.o stringdecimal

stringdecimal.o: stringdecimal.c stringdecimal.h Makefile
	cc -O -c -o $@ $< -DLIB --std=gnu99 -Wall

stringdecimal: stringdecimal.c stringdecimal.h Makefile
	cc -O -o $@ $< -g --std=gnu99 -Wall

