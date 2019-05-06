#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/stat.h>
#include <time.h>
#define ONE_BODY_MAX 1024


long long all_request = 0;
long long all_response = 0;

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

int get_file_size(int filefd) {
	struct stat statbuf;
	fstat(filefd, &statbuf);
	return statbuf.st_size;
}

int sendn(int sockfd, char* buf, int len) {
	int total = 0;
	int ret;
	while (total < len) {
		ret = send(sockfd, buf + total, len - total, 0);
		if (ret < 0) {
			if (errno == EAGAIN)
				continue;
			else {
				printf("sendn errno = %d\n", errno);
				break;
			}
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
			if (errno == EAGAIN) {
				printf("hhh\n");
				continue;
			} else {
				printf("recvn errno = %d\n", errno);
				break;
			}
		} else if (ret == 0) {
			close(sockfd);
			return 0;
		} else
			total = total + ret;
	}
	return total;
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
	++all_request;
}

int client2server(int sockfd, char* file_name) {
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

	sendn(sockfd, (char*)&pkg_head, sizeof(pkg_head));
	sendn(sockfd, file_name_send, strlen(file_name_send));
	sendfile_by_mmap(sockfd, filefd);
	close(filefd);
}

int response_control_puts(response_pkg_head_t* response_pkg_head, int sockfd) {
	printf("%s\n", __func__);
	unsigned short handle_result = ntohs(response_pkg_head->handle_result);
	if (handle_result == response_success)
		printf("%d: 上传成功\n", sockfd);
	else if (handle_result == response_failed)
		printf("%d: 上传失败\n", sockfd);
	++all_response;
}

int server2client(int sockfd) {
	response_pkg_head_t response_pkg_head;
	bzero(&response_pkg_head, sizeof(response_pkg_head));
	recvn(sockfd, (char*)&response_pkg_head, sizeof(response_pkg_head_t));
	unsigned short command_type = ntohs(response_pkg_head.pkg_type);

	switch (command_type) {
		case command_puts:
			response_control_puts(&response_pkg_head, sockfd);
			break;
		default:
			break;
	}
}

int setnonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}


void addfd(int epoll_fd, int fd) {
    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLOUT | EPOLLET | EPOLLERR;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

// 向服务器发起num个TCP连接,我们可以通过改变num来调整测试压力
void start_conn(int epoll_fd, int num, const char* ip, int port) {
    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);


    for (int i = 0; i < num; ++i) {
        int sockfd = socket(PF_INET, SOCK_STREAM, 0);
		printf("sockfd = %d\n", sockfd);
        if (sockfd < 0)
            continue;
        if (connect(sockfd, (struct sockaddr*)&address, sizeof(address)) == 0) {
            printf("build connection %d\n", i);
            addfd(epoll_fd, sockfd);
        }
    }
}

void close_conn(int epoll_fd, int sockfd) {
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, sockfd, 0);
    close(sockfd);
}


int main(int argc, char* argv[]) {
    assert(argc == 5);
    int epoll_fd = epoll_create(1);
    start_conn(epoll_fd, atoi(argv[3]), argv[1], atoi(argv[2]));
    struct epoll_event events[10000];


	time_t start = time(NULL), end;
    while (1) {
		end = time(NULL);
		if (end - start > 60) {//测试60s内的结果
			printf("request = %lld, response = %lld, failed = %lld\n", all_request, all_response, all_request - all_response);
			break;
		}
        int fds = epoll_wait(epoll_fd, events, 10000, -1);
        for (int i = 0; i < fds; i++) {
            int sockfd = events[i].data.fd;

			printf("sockfd = %d\n", sockfd);

            if (events[i].events & EPOLLIN) {

                server2client(sockfd);

                struct epoll_event event;
                event.events = EPOLLOUT | EPOLLET | EPOLLERR;
                event.data.fd = sockfd;
                epoll_ctl(epoll_fd, EPOLL_CTL_MOD, sockfd, &event);

            } else if (events[i].events & EPOLLOUT) {

                client2server(sockfd, argv[4]);

                struct epoll_event event;
                event.events = EPOLLIN | EPOLLET | EPOLLERR;
                event.data.fd = sockfd;
                epoll_ctl(epoll_fd, EPOLL_CTL_MOD, sockfd, &event);
			} else if (events[i].events & EPOLLERR) {
				close_conn(epoll_fd, sockfd);
			}
        }
    }


    return 0;
}