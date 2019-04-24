
CC = gcc -std=gnu99

client:client.o util.o epoll.o
	$(CC) -w -o client *.o

client.o:client.c 
	$(CC) -w -c client.c

util.o:util.c util.h head.h
	$(CC) -w -c util.c

epoll.o:epoll.c head.h
	$(CC) -w -c epoll.c

clean:
	rm *.o