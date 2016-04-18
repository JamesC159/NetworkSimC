CC=gcc
CFLAGS=-g
SOURCES=node.c
EXECUTABLE=driver

node.o: node.c
	make clean
	$(CC) $(CFLAGS) node.c -o $(EXECUTABLE)
	./$(EXECUTABLE) 0 10 9 "hello this message is to be broken into pieces" 1 2 9
	make clean

debug:
	gdb ./$(EXECUTABLE) 0 10 9 "hello this message is to be broken into pieces" 1 2 5

clean:
	rm -f *.o *.txt
