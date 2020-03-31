all: stringdecimal.o stringdecimal

stringdecimal.o: stringdecimal.c stringdecimal.h xparse.c xparse.h Makefile
	cc -g -O -c -o $@ $< -DLIB --std=gnu99 -Wall

stringdecimal: stringdecimal.c stringdecimal.h xparse.c xparse.h Makefile
	cc -g -O -o $@ $< -g --std=gnu99 -Wall

