#ifndef __EPOLL_H__
#define __EPOLL_H__

#include "head.h"

int ftp_epoll_add(int epollfd, int fd, int events);

int ftp_epoll_mod(int epollfd, int fd, int events);

int ftp_epoll_del(int epollfd, int fd, int events);

#endif