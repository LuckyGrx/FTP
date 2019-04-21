CC = gcc -std=gnu99

server:server.o util.o epoll.o ftp_request.o threadpool.o
	$(CC) -w -o server *.o -lpthread

server.o:server.c
	$(CC) -w -c server.c

epoll.o:epoll.c
	$(CC) -w -c epoll.c

util.o:util.c
	$(CC) -w -c util.c

ftp_request.o:ftp_request.c
	$(CC) -w -c ftp_request.c

threadpool.o:threadpool.c
	$(CC) -w -c threadpool.c

clean:
	rm *.o