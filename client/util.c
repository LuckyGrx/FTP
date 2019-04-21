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
	
	/*
	int optval = 1;
	// 禁用Nagle算法
    if(setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (const void*)&optval, sizeof(int)) == -1) {
		perror("setsockopt");
		close(sockfd);
        return -1;
	}
	*/
	
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
			printf("errno = %d\n", errno);
			return -1;
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
			printf("errno = %d\n", errno);
			return -1;
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

static int omitBlank(const char* line, int begin) {
	int i = begin;
	for (; i < strlen(line) - 1; ++i) {
		if (line[i] != ' ')
			break;
	}
	return i;
}

static int get_file_size(int filefd)
{
	struct stat statbuf;
	fstat(filefd, &statbuf);
	return statbuf.st_size;
}

int command_ctl_puts(int sockfd, int filefd) {
	int file_size = get_file_size(filefd);
	printf("file_size = %d\n", file_size);
	char* file_mmap=(char*)mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, filefd, 0);
	char* p = file_mmap;
	printf("file_mmap = %s\n", file_mmap);
	int one_send_size = 1024;
	int remain = file_size;
	int ret;
	pkg_head_t pkg_head;
	while (remain > 0) {
		bzero(&pkg_head, sizeof(pkg_head));
		pkg_head.pkg_type = htons(file_content);
		if (remain < one_send_size)
			pkg_head.body_len = htons(remain);
		else
			pkg_head.body_len = htons(one_send_size);
		
		// 发送包头
		if ((ret = sendn(sockfd, (char*)&pkg_head, sizeof(pkg_head))) == -1) {
			close(sockfd);
			return -1;
		}

		// 发送包体
		if (remain < one_send_size)
			ret = sendn(sockfd, file_mmap, remain);
		else
			ret = sendn(sockfd, file_mmap, one_send_size);
		
		if (-1 == ret) {
			close(sockfd);
			return -1;
		}

		printf("body ret = %d\n", ret);
		file_mmap += one_send_size;
		remain -= one_send_size;
	}

	ret = munmap(p, file_size);
	if (-1 == ret) {
		perror("munmap");
		return ;
	}
	bzero(&pkg_head, sizeof(pkg_head));
	pkg_head.pkg_type = htons(end_file);
	// 发送包头
	if ((ret = sendn(sockfd, (char*)&pkg_head, sizeof(pkg_head))) == -1) {
		close(sockfd);
		return -1;
	}
	/*
	char buff[2];
	int len, ret;
	while((len = read(filefd, buff, sizeof(buff))) > 0) {
		printf("len = %d\n", len);
		bzero(&pkg_head, sizeof(pkg_head));
		pkg_head.pkg_type = htons(file_content);
		pkg_head.body_len = htons(len);

		printf("body_len = %u\n", len);

		// 发送包头
		if ((ret = sendn(sockfd, (char*)&pkg_head, sizeof(pkg_head))) == -1) {
			close(sockfd);
			return -1;
		}
		printf("head ret = %d\n", ret);
		// 发送包体
		if((ret = sendn(sockfd, buff, len)) == -1) {
			close(sockfd);
			return -1;
		}
		printf("body ret = %d\n", ret);
	}
	*/

}


int command_control(int sockfd) {
	pkg_head_t pkg_head;
	bzero(&pkg_head, sizeof(pkg_head));

	char linebuf[1024];
	bzero(linebuf, sizeof(linebuf));
	read(STDIN_FILENO, linebuf, sizeof(linebuf));
	int i;
	int len = strlen(linebuf) - 1 ; //read包括‘\n'
	char body[1024];
	bzero(body, sizeof(body));
	if (len >= 4 && !strncmp(linebuf, "puts", 4)) {
		pkg_head.pkg_type = htons(command_puts);

		int index = omitBlank(linebuf, strlen("puts"));
		strncpy(body, linebuf + index, strlen(linebuf) - 1 - index);
	}
	pkg_head.body_len = htons(strlen(body));

	int filefd = open(body, O_RDONLY);
	if (-1 == filefd) {
		perror("open");
		return -1;
	}
	
	int ret;
	if ((ret = sendn(sockfd, (char*)&pkg_head, sizeof(pkg_head))) == -1)
		return ret;
	printf("ret = %d\n", ret);

	if ((ret = sendn(sockfd, body, strlen(body))) == -1)
		return ret;
	printf("ret = %d\n", ret);

	command_ctl_puts(sockfd, filefd);
/*
	if (len >= 4 && !strncmp(linebuf, "puts", 4)) {
		int filefd = open(body, O_RDONLY);
		int file_size = get_file_size(body);
		if (-1 == file_size)
			return ;
		printf("file_size = %d\n", file_size);

		bzero(&pkg_header, sizeof(pkg_header));
		pkg_header.pkg_type = htons(command_puts);
		pkg_header.pkg_len = htons(sizeof(pkg_header_t) + file_size);

		if ((ret = sendn(sockfd, (char*)&pkg_header, sizeof(pkg_header))) == -1)
			return ret;

		char buff[1024];
		int ret;
		while ((len = read(filefd, buff, sizeof(buff))) > 0)
			sendn(sockfd, buff, len);
	} else if (len >= 4 && !strncmp(linebuf, "gets", 4)) {

	}
*/
}


int socket_control(int sockfd) {
	//printf("socket_controller\n");
	response_pkg_header_t response_pkg_header;
	bzero(&response_pkg_header, sizeof(response_pkg_header));

	if (-1 == recvn(sockfd, (char*)&response_pkg_header, sizeof(response_pkg_header_t)))
		return ;

	unsigned short command_type = ntohs(response_pkg_header.pkg_type);
	unsigned short result = ntohs(response_pkg_header.pkg_result);

	switch (command_type) {
		case command_puts:
			printf("puts\n");
			break;
		case command_gets:
			printf("gets\n");
			break;
		default:
			break;
	}
}