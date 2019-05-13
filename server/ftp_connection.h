#ifndef __FTP_REQUEST_H__
#define __FTP_REQUEST_H__

#include "head.h"
#include "util.h"
#include "rio.h"

#define ONE_BODY_MAX 1024
#define BUFF_SIZE 1024

#pragma pack(1)
typedef struct request_pkg_head {
	unsigned short body_len;   // 包体长度
	unsigned short pkg_type;   // 报文类型
}request_pkg_head_t;
#pragma pack()

#pragma pack(1)
typedef struct response_pkg_head {
	unsigned short body_len;           // 包体长度
	unsigned short pkg_type;           // 报文类型
	unsigned short handle_result;      // 结果类型
}response_pkg_head_t;
#pragma pack()

enum response {
	response_success,
	response_failed
};

enum state {
	head_init,          // 初始状态，准备接收数据包头
	head_recving,       // 结束包头中，包头不完整，继续接受中
	body_init,          // 包头刚好收完，准备接收包体
	body_recving        // 接收包体中，包体不完整，继续接收中，处理后直接回到head_init
};

enum type {
	command_puts,
	file_content,
	end_file
};

typedef struct ftp_connection {
	int              epollfd;
	int              fd;
	unsigned short   type;
	enum state       curstate;
	unsigned short   handle_result;                          // 

	// 收包相关
    char             head[sizeof(request_pkg_head_t)];       // 存放包头信息
    char*            recv_pointer;         	 			     // 接收数据的缓冲区的头指针
    unsigned int     need_recv_len;                          // 还要收到多少数据
	char*            body_pointer;                           //
	unsigned short   body_len;                               // 记录body的长度

	int              filefd;                                 // 

	void*            timer;
}ftp_connection_t;

void init_connection_t(ftp_connection_t* connection, int fd, int epollfd);

void connection_controller(void*);

void connection_head_recv_finish(ftp_connection_t* connection);

void connection_body_recv_finish(ftp_connection_t* connection);

void connection_handler(ftp_connection_t* connection);

int command_handle_puts(ftp_connection_t* connection);

int command_handle_file_content(ftp_connection_t* connection);

int command_handle_end_file(ftp_connection_t* connection);

void ftp_connection_close(ftp_connection_t* connection);

void ftp_connection_shutdown(ftp_connection_t* connection);

#endif