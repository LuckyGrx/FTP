#ifndef __TCP_NET_SOCKET_H__
#define __TCP_NET_SOCKET_H__

#include "head.h"

int tcp_connect(const char*, int);

int sendn(int sfd, char* buf, int len);
int recvn(int sfd, char* buf, int len);

int make_socket_non_blacking(int fd);

int command_control(int sockfd);

int command_ctl_puts(int sockfd, int filefd);

int socket_control(int sockfd);
int socket_ctl_puts(int sockfd);
int socket_ctl_gets(int sockfd);
#endif
