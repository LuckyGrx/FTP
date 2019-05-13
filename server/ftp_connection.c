#include "ftp_connection.h"
#include "time_wheel.h"

void init_connection_t(ftp_connection_t* connection, int fd, int epollfd) {
	connection->epollfd = epollfd;
	connection->fd = fd;
	connection->curstate = head_init;
	connection->recv_pointer = connection->head;
	connection->need_recv_len = sizeof(request_pkg_head_t);

	connection->filefd = -1;
	connection->body_pointer = NULL;
	connection->timer = NULL;
}

void connection_controller(void* ptr) {
	ftp_connection_t* connection = (ftp_connection_t*)ptr;

	// 删除定时器
	time_wheel_del_timer(connection);

	while (1) {
		int reco = recv(connection->fd, connection->recv_pointer, connection->need_recv_len, 0);
		if (0 == reco)
			goto close;
		if (reco < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) 
			break;
		else if (reco < 0)
			goto close;
		
		switch(connection->curstate) {
			case head_init:
				if (connection->need_recv_len == reco) // 刚好收到完整包头，拆解包头
					connection_head_recv_finish(connection);
				else {// 收到的包头不完整
					connection->curstate = head_recving;
					connection->recv_pointer = connection->recv_pointer + reco;
					connection->need_recv_len = connection->need_recv_len - reco;
				}
				break;
			case head_recving:
				if (connection->need_recv_len == reco) // 包头收完整了
					connection_head_recv_finish(connection);
				else {
					connection->recv_pointer = connection->recv_pointer + reco;
					connection->need_recv_len = connection->need_recv_len - reco;
				}
				break;
			case body_init:
				if (connection->need_recv_len == reco) // 包体收完整了
					connection_body_recv_finish(connection);
				else {// 收到的包体不完整
					connection->curstate = body_recving;
					connection->recv_pointer = connection->recv_pointer + reco;
					connection->need_recv_len = connection->need_recv_len - reco;
				}
				break;
			case body_recving:
				if (connection->need_recv_len == reco) // 包体收完整了
					connection_body_recv_finish(connection);
				else {// 收到的包体不完整
					connection->recv_pointer = connection->recv_pointer + reco;
					connection->need_recv_len = connection->need_recv_len - reco;
				}
				break;
			default:
				goto close;
				break;
		}
	}
	// 添加定时器
	time_wheel_add_timer(connection, ftp_connection_shutdown, tw.slot_interval * 10);

	ftp_epoll_mod(connection->epollfd, connection->fd, connection, EPOLLIN | EPOLLET | EPOLLONESHOT); 

	return ;

close:
	ftp_connection_close(connection);
} 

void ftp_connection_close(ftp_connection_t* connection) {
	if (NULL == connection)
		return ;
	ftp_epoll_del(connection->epollfd, connection->fd, connection, EPOLLIN | EPOLLET | EPOLLONESHOT);// 待处理
	close(connection->fd);
	close(connection->filefd); // 关闭文件描述符,否则文件描述符不够用
	free(connection);
	connection = NULL;
}

void ftp_connection_shutdown(ftp_connection_t* connection) {
	if (NULL == connection)
		return ;
	// 关闭套接字写的一半(将发送缓冲区内的数据发完,收到对端的ACK后,再执行4次挥手过程)
	shutdown(connection->fd, SHUT_WR); //关闭套接字写的一半,说明对端还可以继续往服务器发送数据
}

void connection_head_recv_finish(ftp_connection_t* connection) {
	unsigned short body_len = ntohs(((request_pkg_head_t*)connection->head)->body_len);   // 
	connection->type = ntohs(((request_pkg_head_t*)connection->head)->pkg_type);

	if (body_len > ONE_BODY_MAX) {
		connection->curstate = head_init;
		connection->body_pointer = NULL;
		connection->recv_pointer = connection->head;
		connection->need_recv_len = sizeof(request_pkg_head_t);
	} else {
		if (0 == body_len)   // 包体为0时，直接处理
			connection_body_recv_finish(connection);
		else {
			char* temp_buffer = (char*)calloc(body_len, sizeof(char));  // 分配内存 包体长度 

			connection->body_pointer = temp_buffer;
			connection->curstate = body_init;
			connection->recv_pointer = temp_buffer;
			connection->need_recv_len = body_len;
			connection->body_len = body_len;
		}
	}
}

void connection_body_recv_finish(ftp_connection_t* connection) {
	connection_handler(connection);

	connection->curstate = head_init;
	connection->body_pointer = NULL;
	connection->recv_pointer = connection->head;
	connection->need_recv_len = sizeof(request_pkg_head_t);
}

void connection_handler(ftp_connection_t* connection) {
	unsigned short command_type = connection->type;
	switch (command_type) {
		case command_puts:
			command_handle_puts(connection);
			break;
		case file_content:
			command_handle_file_content(connection);
			break;
		case end_file:
			command_handle_end_file(connection);
			break;
		default:
			break;
	}
}

int command_handle_puts(ftp_connection_t* connection) {
	if (-1 == connection->filefd)
		connection->filefd = open(connection->body_pointer, O_RDWR | O_CREAT, 0666);
	else {
		close(connection->filefd);
		connection->filefd = open(connection->body_pointer, O_RDWR | O_CREAT, 0666);
	}

	free(connection->body_pointer);
	connection->body_pointer = NULL;
}

int command_handle_file_content(ftp_connection_t* connection) {
	// 这里不能用stlen(connection->body_pointer)，是存在存储的是字节流的情况
	rio_writen(connection->filefd, connection->body_pointer, connection->body_len);

	free(connection->body_pointer);
	connection->body_pointer = NULL;
}

int command_handle_end_file(ftp_connection_t* connection) {
	response_pkg_head_t response_pkg_head;
	bzero(&response_pkg_head, sizeof(response_pkg_head));
	response_pkg_head.pkg_type = htons(command_puts);
	response_pkg_head.handle_result = htons(response_success);

	sendn(connection->fd, (char*)&response_pkg_head, sizeof(response_pkg_head_t));
}