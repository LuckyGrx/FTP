#include "epoll.h"

int ftp_epoll_add(int epollfd, int fd, int events) {
	struct epoll_event event;
	bzero(&event, sizeof(event));
	event.data.fd = fd;
	event.events = events;
	int ret = epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
	return ret;
}
