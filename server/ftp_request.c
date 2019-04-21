#include "ftp_request.h"

void init_request_t(ftp_request_t* request, int fd, int epollfd) {
	request->epollfd = epollfd;
	request->fd = fd;
	request->curstate = head_init;
	request->recv_pointer = request->head;
	request->need_recv_len = sizeof(pkg_head_t);
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
				if (reco == sizeof(pkg_head_t)) // 刚好收到完整包头，拆解包头
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
				else {// 收到的宽度小于要收的宽度
					request->curstate = body_recving;
					request->recv_pointer = request->recv_pointer + reco;
					request->need_recv_len = request->need_recv_len - reco;
				}
				break;
			case body_recving:
				if (request->need_recv_len == reco) // 包体收完整了
					request_body_recv_finish(request);
				else {// 包体没收完整，继续收
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
	close(request->fd);
	free(request->body_pointer);
	request->body_pointer = NULL;
	free(request);
	request = NULL;
}


void request_head_recv_finish(ftp_request_t* request) {
	unsigned short body_len = ntohs(((pkg_head_t*)request->head)->body_len);   // 
	request->type = ntohs(((pkg_head_t*)request->head)->pkg_type);

	printf("body_len = %u\n", body_len);

	char* temp_buffer = (char*)calloc(body_len, sizeof(char));  // 分配内存 包体长度 

	request->body_pointer = temp_buffer;
	request->curstate = body_init;
	request->recv_pointer = temp_buffer;
	request->need_recv_len = body_len;

	request->body_len = body_len;
}

void request_body_recv_finish(ftp_request_t* request) {
	request_handler(request);

	request->curstate = head_init;
	request->body_pointer = NULL;
	request->recv_pointer = request->head;
	request->need_recv_len = sizeof(pkg_head_t);
}

void request_handler(ftp_request_t* request) {
	unsigned short command_type = request->type;
	printf("command_type = %u\n", command_type);
	switch (command_type) {
		case command_gets:
			command_handle_gets(request);
			break;
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

int command_handle_end_file(ftp_request_t* request) {
	int connfd = request->fd;
	char buf[20] = "I am here";
	sendn(connfd, buf, strlen(buf));
}

int command_handle_file_content(ftp_request_t* request) {
	printf("%s\n", __func__);
	// 这里不能用stlen(request->body_pointer)，是存在存储的是字节流的情况
	write(request->filefd, request->body_pointer, request->body_len);
	free(request->body_pointer);
	request->body_pointer = NULL;
}

int command_handle_puts(ftp_request_t* request) {
	printf("%s\n", __func__);
	printf("%s\n", request->body_pointer);
	request->filefd = open(request->body_pointer, O_RDWR | O_CREAT, 0666);

	free(request->body_pointer);
	request->body_pointer = NULL;
}

int command_handle_gets(ftp_request_t* request) {

}
