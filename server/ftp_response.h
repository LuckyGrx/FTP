#ifndef __FTP_RESPONSE_H__
#define __FTP_RESPONSE_H__

#include "head.h"

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

void response_controller(void*);

#endif