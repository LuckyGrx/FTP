#include "ftp_request.h"

void init_request_t(ftp_request_t* request, int fd, int epollfd) {
	request->epollfd = epollfd;
	request->fd = fd;
	request->curstate = head_init;
	request->recv_pointer = request->head;
	request->need_recv_len = sizeof(request_pkg_head_t);
}

void request_controller(void* ptr) {
	ftp_request_t* request = (ftp_request_t*)ptr;

	while (1) {
		int reco = recv(request->fd, request->recv_pointer, request->need_recv_len, 0);
		if (0 == reco)
			goto close;
		if (reco < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) 
			break;
		else if (reco < 0)
			goto close;
		
		switch(request->curstate) {
			case head_init:
				if (reco == sizeof(request_pkg_head_t)) // 刚好收到完整包头，拆解包头
					request_head_recv_finish(request);
				else {// 收到的包头不完整
					request->curstate = head_recving;
					request->recv_pointer = request->recv_pointer + reco;
					request->need_recv_len = request->need_recv_len - reco;
				}
				break;
			case head_recving:
				if (request->need_recv_len == reco) // 包头收完整了
					request_head_recv_finish(request);
				else {
					request->recv_pointer = request->recv_pointer + reco;
					request->need_recv_len = request->need_recv_len - reco;
				}
				break;
			case body_init:
				if (request->need_recv_len == reco) // 包体收完整了
					request_body_recv_finish(request);
				else {// 收到的包体不完整
					request->curstate = body_recving;
					request->recv_pointer = request->recv_pointer + reco;
					request->need_recv_len = request->need_recv_len - reco;
				}
				break;
			case body_recving:
				if (request->need_recv_len == reco) // 包体收完整了
					request_body_recv_finish(request);
				else {// 收到的包体不完整
					request->recv_pointer = request->recv_pointer + reco;
					request->need_recv_len = request->need_recv_len - reco;
				}
				break;
			default:
				goto close;
				break;
		}
	}
	ftp_epoll_mod(request->epollfd, request->fd, request, EPOLLIN | EPOLLET | EPOLLONESHOT); 

	return ;

close:
	request_close_conn(request);
} 

void request_close_conn(ftp_request_t* request) {
	ftp_epoll_del(request->epollfd, request->fd, request, EPOLLIN | EPOLLET | EPOLLONESHOT);// 待处理

	close(request->fd);
	free(request->body_pointer);
	request->body_pointer = NULL;
	free(request);
	request = NULL;
}

void request_head_recv_finish(ftp_request_t* request) {
	unsigned short body_len = ntohs(((request_pkg_head_t*)request->head)->body_len);   // 
	request->type = ntohs(((request_pkg_head_t*)request->head)->pkg_type);

	if (body_len > ONE_BODY_MAX) {
		request->curstate = head_init;
		request->body_pointer = NULL;
		request->recv_pointer = request->head;
		request->need_recv_len = sizeof(request_pkg_head_t);
	} else {
		if (0 == body_len)   // 包体为0时，直接处理
			request_body_recv_finish(request);
		else {
			char* temp_buffer = (char*)calloc(body_len, sizeof(char));  // 分配内存 包体长度 

			request->body_pointer = temp_buffer;
			request->curstate = body_init;
			request->recv_pointer = temp_buffer;
			request->need_recv_len = body_len;
			request->body_len = body_len;
		}
	}
}

void request_body_recv_finish(ftp_request_t* request) {
	request_handler(request);

	request->curstate = head_init;
	request->body_pointer = NULL;
	request->recv_pointer = request->head;
	request->need_recv_len = sizeof(request_pkg_head_t);
}

void request_handler(ftp_request_t* request) {
	unsigned short command_type = request->type;
	switch (command_type) {
		case command_puts:
			command_handle_puts(request);
			break;
		case file_content:
			command_handle_file_content(request);
			break;
		case end_file:
			command_handle_end_file(request);
			break;
		default:
			break;
	}
}

int command_handle_puts(ftp_request_t* request) {
	request->filefd = open(request->body_pointer, O_RDWR | O_CREAT, 0666);

	free(request->body_pointer);
	request->body_pointer = NULL;
}

int command_handle_file_content(ftp_request_t* request) {
	// 这里不能用stlen(request->body_pointer)，是存在存储的是字节流的情况
	rio_writen(request->filefd, request->body_pointer, request->body_len);

	free(request->body_pointer);
	request->body_pointer = NULL;
}

int command_handle_end_file(ftp_request_t* request) {
	response_pkg_head_t response_pkg_head;
	bzero(&response_pkg_head, sizeof(response_pkg_head));
	response_pkg_head.pkg_type = htons(command_puts);
	response_pkg_head.handle_result = htons(response_success);

	sendn(request->fd, (char*)&response_pkg_head, sizeof(response_pkg_head_t));
}
