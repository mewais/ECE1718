part1: part1.o ADXL345.o
	gcc -o $@ $^

part1.o: part1.c
	gcc -Wall -c -o $@ $<

ADXL345.o: ADXL345.c
	gcc -Wall -c -o $@ $<

clean:
	rm -f *.o part1
