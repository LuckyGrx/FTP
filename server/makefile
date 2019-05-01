CC = gcc -std=gnu99

server:server.o util.o epoll.o ftp_connection.o threadpool.o rio.o time_wheel.o
	$(CC) -w -o server *.o -lpthread

server.o:server.c
	$(CC) -w -c server.c

epoll.o:epoll.c
	$(CC) -w -c epoll.c

util.o:util.c
	$(CC) -w -c util.c

ftp_connection.o:ftp_connection.c
	$(CC) -w -c ftp_connection.c

rio.o:rio.c
	$(CC) -w -c rio.c

time_wheel.o:time_wheel.c
	$(CC) -w -c time_wheel.c

threadpool.o:threadpool.c
	$(CC) -w -c threadpool.c

clean:
	rm *.o