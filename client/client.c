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
	struct epoll_event event, events[MAX_EVENT_NUMBER];

	ftp_epoll_add(epollfd, sockfd, EPOLLIN);
	ftp_epoll_add(epollfd, STDIN_FILENO, EPOLLIN);

	int result;
	for (;;) {
		result = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
		for(int i = 0; i < result; ++i) {
			int fd = events[i].data.fd;
			if (STDIN_FILENO == fd)
				request_control(sockfd);
			if (sockfd == fd)
				response_control(sockfd);
		}
	}
	close(epollfd);
	close(sockfd);
	return 0;
}
