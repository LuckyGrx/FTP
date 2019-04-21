#include "epoll.h"

int ftp_epoll_create(struct epoll_event** events) {
	int epollfd = epoll_create(1);
	*events = (struct epoll_event*)calloc(MAX_EVENT_NUMBER, sizeof(struct epoll_event));
	return epollfd;
}

int ftp_epoll_add(int epollfd, int fd, ftp_request_t* request, int events) {
	struct epoll_event event;
	bzero(&event, sizeof(event));
	event.data.ptr = (void*)request;
	event.events = events;
	int ret = epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
	return ret;
}


int ftp_epoll_mod(int epollfd, int fd, ftp_request_t* request, int events) {
	struct epoll_event event;
	bzero(&event, sizeof(event));
	event.data.ptr = (void*)request;
	event.events = events;
	int ret = epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
	return ret;
}

int ftp_epoll_del(int epollfd, int fd, ftp_request_t* request, int events) {
	struct epoll_event event;
	bzero(&event, sizeof(event));
	event.data.ptr = (void*)request;
	event.events = events;
	int ret = epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, &event);
	return ret;
}