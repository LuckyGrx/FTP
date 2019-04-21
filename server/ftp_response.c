#include "ftp_response.h"
#include "ftp_request.h"

void response_controller(void* ptr) {
    ftp_request_t* response = (ftp_request_t*)ptr;
    int fd = response->fd;

	unsigned short command_type = response->type;

	response_pkg_head_t response_pkg_head;
	bzero(&response_pkg_head, sizeof(response_pkg_head));
	response_pkg_head.pkg_type = htons(response->type);
	response_pkg_head.body_len = htons(response->body_len);
	response_pkg_head.handle_result = htons(response->handle_result);
	int ret;
	if ((ret = sendn(response->fd, (char*)&response_pkg_head, sizeof(response_pkg_head_t))) == -1)
		return ;
	printf("ret = %d\n", ret);

	//free(response->pnewpointer);

	//epoll_mod(response->epollfd, response->fd, response, EPOLLIN | EPOLLONESHOT);
}