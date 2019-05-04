#include "epoll.h"

struct epoll_event* events;

int ftp_epoll_create() {
	int epollfd = epoll_create(1);
	events = (struct epoll_event*)malloc(MAX_EVENT_NUMBER * sizeof(struct epoll_event));
	return epollfd;
}

int ftp_epoll_add(int epollfd, int fd, ftp_connection_t* connection, int events) {
	struct epoll_event event;
	bzero(&event, sizeof(event));
	event.data.ptr = (void*)connection;
	event.events = events;
	int ret = epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
	return ret;
}

int ftp_epoll_mod(int epollfd, int fd, ftp_connection_t* connection, int events) {
	struct epoll_event event;
	bzero(&event, sizeof(event));
	event.data.ptr = (void*)connection;
	event.events = events;
	int ret = epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
	return ret;
}

int ftp_epoll_del(int epollfd, int fd, ftp_connection_t* connection, int events) {
	struct epoll_event event;
	bzero(&event, sizeof(event));
	event.data.ptr = (void*)connection;
	event.events = events;
	int ret = epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, &event);
	return ret;
}

int ftp_epoll_wait(int epollfd, struct epoll_event *events, int max_events_num) {
	int timeout = EPOLL_TIMEOUT;

	time_t start, end;
begin:
	start = time(NULL);//返回距离1970的秒数

	int events_num = epoll_wait(epollfd, events, max_events_num, timeout);

	if (events_num == 0) {// 说明超时时间到,处理定时任务
		timeout = EPOLL_TIMEOUT;
		time_wheel_tick();
		goto begin;
	} else {// 说明有事件发生
		end = time(NULL);
		timeout -= (end - start) * 1000;
		if (timeout == 0) {// 说明在有事件发生时,刚好超时时间到
			timeout = EPOLL_TIMEOUT;
			time_wheel_tick();
		}
	}
    return events_num;
}

void ftp_handle_events(int epollfd, int listenfd, struct epoll_event* events, 
		int events_num, ftp_threadpool_t* pool) {

	for(int i = 0; i < events_num; ++i) {
		int fd = ((ftp_connection_t*)(events[i].data.ptr))->fd;
		if (fd == listenfd) {
		// 有事件发生的描述符为监听描述符
			tcp_accept(epollfd, listenfd);
		} else {
			/*
			// 排除错误事件
            if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP)
                || (!(events[i].events & EPOLLIN))){
                close(fd);
                continue;
            }
			*/

		// 有事件发生的描述符为连接描述符
			if (events[i].events & EPOLLIN)
				threadpool_add(pool, connection_controller, events[i].data.ptr);
		}
	}
}