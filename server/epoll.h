#ifndef __EPOLL_H__
#define __EPOLL_H__

#include "head.h"
#include "ftp_request.h"

#define MAX_EVENT_NUMBER 1024

int ftp_epoll_create(struct epoll_event** events);

int ftp_epoll_add(int epollfd, int fd, ftp_request_t* request, int events);

int ftp_epoll_mod(int epollfd, int fd, ftp_request_t* request, int events);

int ftp_epoll_del(int epollfd, int fd, ftp_request_t* request, int events);


#endif