HEADERS = 

default: main

main: main.c
	gcc -pthread main.c -o main

clean:
	-rm -f main
