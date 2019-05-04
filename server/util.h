#ifndef __UTIL_H__
#define __UTIL_H__

#define LISTENQ 1024

#define DELIM "="
#define BUFFLEN 1024

#define FTP_CONF_ERROR -1
#define FTP_CONF_OK  0

typedef struct ftp_conf {
	int port;
	int threadnum;
	int shutdown;
}ftp_conf_t;

int make_socket_non_blacking(int fd);

int tcp_socket_bind_listen(int);
void tcp_accept(int, int);


int sendn(int sfd, char* buf, int len);
int recvn(int sfd, char* buf, int len);

int read_conf(const char* filename, ftp_conf_t* conf);

int get_file_size(int filefd);

int sendfile_by_mmap(int sockfd, int filefd);

int ftp_daemon();

void handle_for_sigpipe();

#endif