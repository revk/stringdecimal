all: stringdecimal.o stringdecimaleval.o stringdecimal

stringdecimal.o: stringdecimal.c stringdecimal.h Makefile
	cc -g -O -c -o $@ $< -DLIB --std=gnu99 -Wall

stringdecimaleval.o: stringdecimal.c stringdecimal.h xparse.c xparse.h Makefile
	cc -g -O -c -o $@ $< -DLIB --std=gnu99 -Wall -DEVAL

stringdecimal: stringdecimal.c stringdecimal.h xparse.c xparse.h Makefile
	cc -g -O -o $@ $< -g --std=gnu99 -Wall -DEVAL

