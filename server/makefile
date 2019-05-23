CC = gcc -std=gnu99

server:server.o util.o epoll.o ftp_connection.o threadpool.o rio.o timer_list.o
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

timer_list.o:timer_list.c
	$(CC) -w -g -c timer_list.c

threadpool.o:threadpool.c
	$(CC) -w -g -c threadpool.c

clean:
	rm *.o