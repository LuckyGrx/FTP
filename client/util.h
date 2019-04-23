#ifndef __UTIL_H__
#define __UTIL_H__

#include "head.h"

int tcp_connect(const char*, int);

int sendn(int sfd, char* buf, int len);
int recvn(int sfd, char* buf, int len);

int make_socket_non_blacking(int fd);

int request_control(int sockfd);
int request_control_puts(int sockfd, command_line_t* command_line);
int request_control_gets(int sockfd, command_line_t* command_line);

int sendfile_by_mmap(int sockfd, int filefd);

int response_control(int sockfd);
int response_control_puts(response_pkg_head_t* response_pkg_head);
int response_control_gets(response_pkg_head_t* response_pkg_head);
int response_control_file_content(response_pkg_head_t* response_pkg_head);

int get_file_size(int filefd);
#endif
