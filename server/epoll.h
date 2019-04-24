#ifndef __EPOLL_H__
#define __EPOLL_H__

#include "head.h"
#include "ftp_request.h"
#include "threadpool.h"

#define MAX_EVENT_NUMBER 1024

int ftp_epoll_create(struct epoll_event** events);

int ftp_epoll_add(int epollfd, int fd, ftp_request_t* request, int events);

int ftp_epoll_mod(int epollfd, int fd, ftp_request_t* request, int events);

int ftp_epoll_del(int epollfd, int fd, ftp_request_t* request, int events);

int ftp_epoll_wait(int epollfd, struct epoll_event *events, int max_events, int timeout);

void ftp_handle_events(int epollfd, int listenfd, struct epoll_event* events,
                      int events_num, ftp_threadpool_t* pool);

#endif