#include "head.h"
#define MAX_EVENT_NUMBER 1024

int main(int argc,char* argv[]){
	if (argc != 3) {
		printf("args error\n");
		return -1;
	}
	int sockfd = tcp_connect(argv[1], atoi(argv[2]));
	int rc = make_socket_non_blocking(sockfd);

	int epollfd = epoll_create(1);
	struct epoll_event event, evs[MAX_EVENT_NUMBER];

	epoll_add(epollfd, sockfd, EPOLLIN);
	epoll_add(epollfd, STDIN_FILENO, EPOLLIN);

	int result, i;
	for (;;) {
		result = epoll_wait(epollfd, evs, MAX_EVENT_NUMBER, -1);
		for(i = 0; i < result; ++i) {
			int fd = evs[i].data.fd;
			// 两个监听是并行的
			if (STDIN_FILENO == fd) {
				//printf("This is stdinfd\n");
				command_control(sockfd);
			}
			if (sockfd == fd) {
				//printf("This is sockfd\n");
				socket_control(sockfd);
			}
		}
	}
	close(epollfd);
	close(sockfd);
	return 0;
}
