#include "util.h"

int tcp_connect(const char* ip, int port) {
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == sockfd) {
		perror("socket");
		exit(1);
	}
	struct sockaddr_in serveraddr;
	bzero(&serveraddr, sizeof(struct sockaddr_in));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(port);
	serveraddr.sin_addr.s_addr = inet_addr(ip);
	int ret = connect(sockfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	if (-1 == ret) {
		perror("connect");
		close(sockfd);
		exit(1);
	}
	return sockfd;
}

int sendn(int sockfd, char* buf, int len) {
	int total = 0;
	int ret;
	while (total < len) {
		ret = send(sockfd, buf + total, len - total, 0);
		if (ret < 0) {
			if (errno == EAGAIN)
				continue;
		} else if (ret == 0) {
			close(sockfd);
			return 0;
		} else
			total = total + ret;
	}
	return total;
}

int recvn(int sockfd, char* buf, int len) {
	int total = 0;
	int ret;
	while (total < len) {
		ret = recv(sockfd, buf + total, len - total, 0);
		if (ret < 0) {
			if (errno == EAGAIN)
				continue;
		} else if (ret == 0) {
			close(sockfd);
			exit(1);
			return 0;
		} else
			total = total + ret;
	}
	return total;
}

int make_socket_non_blocking(int fd) {
	int flag = fcntl(fd, F_GETFL, 0);
    if (-1 == flag)
        return -1;

    flag |= O_NONBLOCK;
    if (-1 == fcntl(fd, F_SETFL, flag))
        return -1;
}

static int omit_blank(const char* line, int begin) {
	int i = begin;
	for (; i < strlen(line) - 1; ++i) {
		if (line[i] != ' ')
			break;
	}
	return i;
}

int get_file_size(int filefd) {
	struct stat statbuf;
	fstat(filefd, &statbuf);
	return statbuf.st_size;
}

int get_command_line(command_line_t* command_line) {
	char linebuf[1024];
	bzero(linebuf, sizeof(linebuf));
	read(STDIN_FILENO, linebuf, sizeof(linebuf));
	int len = strlen(linebuf) - 1 ; //read包括‘\n'
	char commands[][10] = {"puts"};

	for (int i = 0; i < sizeof(commands) / sizeof(commands[0]); ++i) {
		if (len >= strlen(commands[i]) && !strncmp(linebuf, commands[i], strlen(commands[i]))) {
			strcpy(command_line->command, commands[i]);
			int index = omit_blank(linebuf, strlen(commands[i]));
			strncpy(command_line->file_name, linebuf + index, strlen(linebuf) - 1 - index);
			return 0;
		}
	}	
	return -1;
}

int request_control(int sockfd) {
	command_line_t command_line;
	if (-1 == get_command_line(&command_line))
		return -1;

	if (!strcmp(command_line.command, "puts"))
		request_control_puts(sockfd, &command_line);
}

int request_control_puts(int sockfd, command_line_t* command_line) {
	request_pkg_head_t pkg_head;
	bzero(&pkg_head, sizeof(pkg_head));

	pkg_head.pkg_type = htons(command_puts);
	pkg_head.body_len = htons(strlen(command_line->file_name));
	int filefd;
	if ((filefd = open(command_line->file_name, O_RDONLY)) == -1) {
		perror("open");
		return -1;
	}

	sendn(sockfd, (char*)&pkg_head, sizeof(pkg_head));
	sendn(sockfd, command_line->file_name, strlen(command_line->file_name));
	sendfile_by_mmap(sockfd, filefd);
}

int sendfile_by_mmap(int sockfd, int filefd) {
	int file_size = get_file_size(filefd);
	char* file_mmap=(char*)mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, filefd, 0);
	char* p = file_mmap;
	int one_send_size = ONE_BODY_MAX;
	int remain = file_size;
	int ret;
	request_pkg_head_t pkg_head;
	while (remain > 0) {
		bzero(&pkg_head, sizeof(pkg_head));
		pkg_head.pkg_type = htons(file_content);
		if (remain < one_send_size)
			pkg_head.body_len = htons(remain);
		else
			pkg_head.body_len = htons(one_send_size);
		
		// 发送包头
		sendn(sockfd, (char*)&pkg_head, sizeof(pkg_head));
		// 发送包体
		if (remain < one_send_size)
			sendn(sockfd, file_mmap, remain);
		else
			sendn(sockfd, file_mmap, one_send_size);
		
		file_mmap += one_send_size;
		remain -= one_send_size;
	}

	ret = munmap(p, file_size);
	if (-1 == ret) {
		perror("munmap");
		return -1;
	}

	bzero(&pkg_head, sizeof(pkg_head));
	pkg_head.pkg_type = htons(end_file);
	// 发送文件结束标志
	sendn(sockfd, (char*)&pkg_head, sizeof(pkg_head));
}

int response_control(int sockfd) {
	response_pkg_head_t response_pkg_head;
	bzero(&response_pkg_head, sizeof(response_pkg_head));
	recvn(sockfd, (char*)&response_pkg_head, sizeof(response_pkg_head_t));
	unsigned short command_type = ntohs(response_pkg_head.pkg_type);

	switch (command_type) {
		case command_puts:
			response_control_puts(&response_pkg_head);
			break;
		default:
			break;
	}
}

int response_control_puts(response_pkg_head_t* response_pkg_head) {
	printf("%s\n", __func__);
	unsigned short handle_result = ntohs(response_pkg_head->handle_result);
	if (handle_result == response_success)
		printf("上传成功\n");
	else if (handle_result == response_failed)
		printf("上传失败\n");
}
