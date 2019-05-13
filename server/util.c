
#include "util.h"
#include "epoll.h"
#include "ftp_connection.h"
#include "time_wheel.h"

int tcp_socket_bind_listen(int port) {
	int listenfd;
	if (-1 == (listenfd = socket(AF_INET, SOCK_STREAM, 0))) {
		perror("socket");
		exit(-1);
	}

	// 消除bind时"Address already in use"错误
	int optval = 1;
    if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int)) == -1)
        return -1;

	struct sockaddr_in serveraddr;
	bzero(&serveraddr, sizeof(struct sockaddr_in));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(port);
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);// 监听多个网络接口
	if (-1 == bind(listenfd, (struct sockaddr*)&serveraddr, sizeof(struct sockaddr))) {
		perror("bind");
		exit(-1);
	}
	if (-1 == listen(listenfd, LISTENQ)) {
		perror("listen");
		exit(-1);
	}
	return listenfd;
}

int make_socket_non_blocking(int fd) {
	int flag = fcntl(fd, F_GETFL, 0);
    if (-1 == flag)
        return -1;
    flag |= O_NONBLOCK;
    if (-1 == fcntl(fd, F_SETFL, flag))
        return -1;
}

void tcp_accept(int epollfd, int listenfd) {
	struct sockaddr_in sockaddr;
	bzero(&sockaddr, sizeof(sockaddr));
	int addrlen = sizeof(sockaddr);

	int connfd;
	// ET 模式下accept必须用while循环，详见百度
	while ((connfd = accept(listenfd, (struct sockaddr*)&sockaddr, &addrlen)) != -1) {

		int rc = make_socket_non_blocking(connfd);

		ftp_connection_t* connection = (ftp_connection_t*)calloc(1, sizeof(ftp_connection_t));
		init_connection_t(connection, connfd, epollfd);

		time_wheel_add_timer(connection, ftp_connection_shutdown, tw.slot_interval * 20);
		// 文件描述符可以读，边缘触发(Edge Triggered)模式，保证一个socket连接在任一时刻只被一个线程处理
		ftp_epoll_add(epollfd, connfd, connection, EPOLLIN | EPOLLET | EPOLLONESHOT); 

	}
	if (-1 == connfd) {
		if (errno != EAGAIN) {
			perror("accept");
			exit(-1);
		}
	}
}

int sendn(int sfd, char* buf, int len) {
	int total = 0;
	int ret;
	while (total < len) {
		ret = send(sfd, buf + total, len - total, 0);
		if (ret > 0)
			total = total + ret;
		else if (0 == ret) {
			close(sfd);
			exit(-1);
			return 0;
		}
		else {
			if (errno == EAGAIN)
				continue;
		}
	}
	return total;
}

int recvn(int sfd, char* buf, int len) {
	int total = 0;
	int ret;
	while (total < len) {
		ret = recv(sfd, buf + total, len - total, 0);
		if (ret > 0)
			total = total + ret;
		else if (ret == 0) {
			close(sfd);
			exit(1);
			return 0;
		} else {
			if (errno == EAGAIN)
				continue;
		}
	}
	return total;
}


int read_conf(const char* filename, ftp_conf_t* conf) {
	FILE* fp = fopen(filename, "r");

	char buff[BUFFLEN];
	int buff_len = BUFFLEN;
	char* curr_pos = buff;

	char* delim_pos = NULL;
	int pos = 0;
	int line_len = 0;
	while (fgets(curr_pos, buff_len - pos, fp) != NULL) {
		delim_pos = strstr(curr_pos, DELIM);
		if (NULL == delim_pos)
			return FTP_CONF_ERROR;
		// 得到port
		if (0 == strncmp("port", curr_pos, 4))
			conf->port = atoi(delim_pos + 1);
		// 得到threadnum
		if (0 == strncmp("threadnum", curr_pos, 9))
			conf->threadnum = atoi(delim_pos + 1);
		// 得到关机模式
		if (0 == strncmp("shutdown", curr_pos, 8))
			conf->shutdown = atoi(delim_pos + 1);

		// line_len得到当前行行长
		line_len = strlen(curr_pos);
		// 当前位置跳转至下一行首部
		curr_pos += line_len;
	}
	fclose(fp);
	return FTP_CONF_OK;
}

int get_file_size(int filefd) {
	struct stat statbuf;
	fstat(filefd, &statbuf);
	return statbuf.st_size;
}

int sendfile_by_mmap(int sockfd, int filefd) {  
	printf("%s\n", __func__);

	int file_size = get_file_size(filefd);
	char* file_mmap=(char*)mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, filefd, 0);
	char* p = file_mmap;
	int one_send_size = ONE_BODY_MAX;
	int remain = file_size;
	int ret;
	response_pkg_head_t pkg_head;
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


int ftp_daemon() {
	pid_t pid = fork();
	if (pid < 0) {
		return -1;
	} else if (pid > 0)
		exit(0);

	umask(0);

	pid_t sid = setsid(); // 使子进程成为进程组组长
	if (sid < 0)
		return -1;

	pid = fork();    // 使子进程不是进程组组长，这样该子进程就不能打开控制终端
	if (pid < 0)
		return -1;
	else if (pid > 0)
		exit(0);


	int fd = open("/dev/null", O_RDWR);

	if (-1 == fd) {
		return -1;
	}

	if (dup2(fd, STDIN_FILENO) == -1)
		return -1;
	if (dup2(fd, STDOUT_FILENO) == -1)
		return -1;
	if (dup2(fd, STDERR_FILENO) == -1)
		return -1;

	if (fd > STDERR_FILENO) {
		if (close(fd) == -1)
			return -1;
	}

	return 1;
}

void handle_for_sigpipe() {
    struct sigaction siga;
	bzero(&siga, sizeof(siga));
    siga.sa_handler = SIG_IGN;
    siga.sa_flags = 0;
    if(sigaction(SIGPIPE, &siga, NULL))
        return;
}