
client:client.o util.o epoll.o
	gcc -o client *.o -lcrypt

client.o:client.c 
	gcc -c client.c

util.o:util.c util.h head.h
	gcc -c util.c

epoll.o:epoll.c head.h
	gcc -c epoll.c

clean:
	rm *.o