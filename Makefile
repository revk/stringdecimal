all: stringdecimal.o stringdecimal

stringdecimal.o: stringdecimal.c stringdecimal.h
	cc -O -c -o $@ $< -DLIB 

stringdecimal: stringdecimal.c stringdecimal.h
	cc -O -o $@ $< -g

