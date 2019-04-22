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

int ftp_epoll_wait(int epollfd, struct epoll_event *events, int max_events_num, int timeout) {
	int event_count = epoll_wait(epollfd, events, max_events_num, timeout);
    return event_count;
}

void ftp_handle_events(int epollfd, int listenfd, struct epoll_event* events, 
		int events_num, ftp_threadpool_t* pool) {

	for(int i = 0; i < events_num; ++i) {
		int fd = ((ftp_request_t*)(events[i].data.ptr))->fd;
		if (fd == listenfd) {
		// 有事件发生的描述符为监听描述符
			tcp_accept(epollfd, listenfd);
		} else {
		// 有事件发生的描述符为连接描述符
			if (events[i].events & EPOLLIN)
				threadpool_add(pool, request_controller, events[i].data.ptr);
		}
	}
}