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
#include <pthread.h>
#define ONE_BODY_MAX 1024

#pragma pack(1)
typedef struct request_pkg_head {
	unsigned short body_len;   // 包体长度
	unsigned short pkg_type;   // 报文类型
}request_pkg_head_t;
#pragma pack()

enum type {
	command_puts,
	file_content,
	end_file
};

int get_file_size(int filefd) {
	struct stat statbuf;
	fstat(filefd, &statbuf);
	return statbuf.st_size;
}

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

int request_control_puts(int sockfd, char* file_name) {
	static int file_index = 1;
	request_pkg_head_t pkg_head;
	bzero(&pkg_head, sizeof(pkg_head));

	char file_name_send[15];
	strcpy(file_name_send, file_name);

	sprintf(file_name_send, "%s%d", file_name, file_index++);
	printf("%s\n", file_name_send);

	pkg_head.pkg_type = htons(command_puts);
	pkg_head.body_len = htons(strlen(file_name_send));
	int filefd;
	if ((filefd = open(file_name, O_RDONLY)) == -1) {
		perror("open");
		return -1;
	}

	send(sockfd, (char*)&pkg_head, sizeof(pkg_head), 0);
	send(sockfd, file_name_send, strlen(file_name_send), 0);
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
		send(sockfd, (char*)&pkg_head, sizeof(pkg_head), 0);
		// 发送包体
		if (remain < one_send_size)
			send(sockfd, file_mmap, remain, 0);
		else
			send(sockfd, file_mmap, one_send_size, 0);
		
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
	send(sockfd, (char*)&pkg_head, sizeof(pkg_head), 0);
}

void* threadfunc(void* p) {
	char* file_name = (char*)p;
	printf("%s\n", file_name);
	int sockfd = tcp_connect("127.0.0.1", atoi("8080"));
	request_control_puts(sockfd, file_name);
	close(file_name);
}


int main(int argc, char** argv) {
	if (argc != 3) {
		printf("param error\n");
		return -1;
	}
	int pthreadnum = atoi(argv[1]);
	pthread_t* pidnums = (pthread_t*)calloc(pthreadnum, sizeof(pthread_t));
	int i;
	char file_name[15];
	strcpy(file_name, argv[2]);
	for (i = 0; i < pthreadnum; ++i)
		pthread_create(&(pidnums[i]), NULL, threadfunc, file_name);

	for (i = 0; i < pthreadnum; ++i) {
		pthread_join(pidnums[i], NULL);
	}
	return 0;
}
