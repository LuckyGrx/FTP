#ifndef __EPOLL_H__
#define __EPOLL_H__

#include "head.h"

int ftp_epoll_add(int epollfd, int fd, int events);

#endif