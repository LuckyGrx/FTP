CC = gcc -std=gnu99

server:server.o util.o epoll.o ftp_connection.o threadpool.o rio.o time_wheel.o
	$(CC) -w -g -o server *.o -lpthread

server.o:server.c
	$(CC) -w -g -c server.c

epoll.o:epoll.c
	$(CC) -w -g -c epoll.c

util.o:util.c
	$(CC) -w -g -c util.c

ftp_connection.o:ftp_connection.c
	$(CC) -w -g -c ftp_connection.c

rio.o:rio.c
	$(CC) -w -g -c rio.c

time_wheel.o:time_wheel.c
	$(CC) -w -g -c time_wheel.c

threadpool.o:threadpool.c
	$(CC) -w -g -c threadpool.c

clean:
	rm *.o