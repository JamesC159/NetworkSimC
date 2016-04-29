CC=gcc
CFLAGS=-g
SOURCES=node.c
EXECUTABLE=driver

node.o: node.c
	make clean
	$(CC) $(CFLAGS) node.c -o $(EXECUTABLE)
	./$(EXECUTABLE) 0 10 2 "hello from node 0" 2 1 2 &
	./$(EXECUTABLE) 1 10 2 "how is it going from node 1" 2 0 2 &
	./$(EXECUTABLE) 2 15 2 1
clean:
	rm -f *.o *.txt
