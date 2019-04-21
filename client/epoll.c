#include "epoll.h"

int epoll_add(int epollfd, int fd, int events) {
	struct epoll_event event;
	bzero(&event, sizeof(event));
	event.data.fd = fd;
	event.events = events;
	int ret = epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
}


int epoll_mod(int epollfd, int fd, int events) {
	struct epoll_event event;
	bzero(&event, sizeof(event));
	event.data.fd = fd;
	event.events = events;
	int ret = epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

int epoll_del(int epollfd, int fd, int events) {
	struct epoll_event event;
	bzero(&event, sizeof(event));
	event.data.fd = fd;
	event.events = events;
	int ret = epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, &event);
}