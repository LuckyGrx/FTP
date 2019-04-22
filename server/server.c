#include "util.h"
#include "epoll.h"
#include "ftp_request.h"
#include "threadpool.h"

#define DEFAULT_CONFIG "ftp.conf"

int main (int argc, char* argv[]) {
	ftp_conf_t conf;
	read_conf(DEFAULT_CONFIG, &conf);

	int listenfd = tcp_socket_bind_listen(conf.port);
	int rc = make_socket_non_blocking(listenfd);

	struct epoll_event* events = (struct epoll_event*)calloc(1, sizeof(struct epoll_event)); 
	int epollfd = ftp_epoll_create(&events);

	ftp_request_t* request = (ftp_request_t*)calloc(1, sizeof(ftp_request_t));
	init_request_t(request, listenfd, epollfd);
	ftp_epoll_add(epollfd, listenfd, request, EPOLLIN | EPOLLET);

	ftp_threadpool_t* pool = threadpool_init(conf.threadnum);
	for (;;) {
		// 调用epoll_wait函数，返回接收到事件的数量
        int events_num = ftp_epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);

        ftp_handle_events(epollfd, listenfd, events, events_num, pool);
	}
	free(events);
	close(epollfd);
	close(listenfd);
	return 0;
}