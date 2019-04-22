#ifndef __HEAD_H__
#define __HEAD_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <errno.h>
#include <time.h>
#include <shadow.h>
#include <crypt.h>
#include <mysql/mysql.h>
#include <netinet/tcp.h>

#define ONE_BODY_MAX 1024
#define BUFF_SIZE 1024

int register_user(int,char*,char*);
void print_register(char**,char**);
void print_choose(char**);
int login_user(int);

enum type {
	command_puts,
	command_gets,
	file_content,
	end_file
};
#pragma pack(1)
typedef struct request_pkg_head {
	unsigned short body_len;   // 包体长度
	unsigned short pkg_type;   // 报文类型
}request_pkg_head_t;
#pragma pack()

#pragma pack(1)
typedef struct response_pkg_head {
	unsigned short body_len;         // 包体长度
	unsigned short pkg_type;         // 报文类型
	unsigned short handle_result;    // 结果码
}response_pkg_head_t;
#pragma pack()

enum response {
	response_success,
	response_failed
};

#endif
