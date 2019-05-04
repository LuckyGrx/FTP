#ifndef __EPOLL_H__
#define __EPOLL_H__

#include "head.h"
#include "ftp_connection.h"
#include "threadpool.h"
#include "time_wheel.h"

#define MAX_EVENT_NUMBER 1024
#define EPOLL_TIMEOUT 10000  //毫秒,ms

int ftp_epoll_create();

int ftp_epoll_add(int epollfd, int fd, ftp_connection_t* connection, int events);

int ftp_epoll_mod(int epollfd, int fd, ftp_connection_t* connection, int events);

int ftp_epoll_del(int epollfd, int fd, ftp_connection_t* connection, int events);

int ftp_epoll_wait(int epollfd, struct epoll_event *events, int max_events);

void ftp_handle_events(int epollfd, int listenfd, struct epoll_event* events,
                      int events_num, ftp_threadpool_t* pool);

#endif