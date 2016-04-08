CC=gcc
CFLAGS=-g
SOURCES=node.c
EXECUTABLE=driver

node.o: node.c
	$(CC) $(CFLAGS) node.c -o $(EXECUTABLE)

clean:
	rm -f *.o *.txt
