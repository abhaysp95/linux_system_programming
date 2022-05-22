TARGET: all

all: error_handling

error_handling.o: error_handling.c
	gcc -I . -ggdb -c $^ -o $@

error_handling: error_handling.o
	gcc -L . $^ -o $@

clean:
	rm -r *.o
	rm error_handling
