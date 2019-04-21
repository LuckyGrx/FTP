#include "util.h"
#include "epoll.h"
#include "ftp_request.h"
#include "ftp_response.h"
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

	int result;
	threadpool_t* pool = threadpool_init(conf.threadnum);
	for (;;) {
		result = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
		for(int i = 0; i < result; ++i) {
			int fd = ((ftp_request_t*)(events[i].data.ptr))->fd;
			if (fd == listenfd) {
			// 有事件发生的描述符为监听描述符
				tcp_accept(epollfd, listenfd);
			} else {

/*
				// 排除错误事件
            if ((evs[i].events & EPOLLERR) || (evs[i].events & EPOLLHUP)){
                close(fd);
                continue;
            }
*/
			// 有事件发生的描述符为连接描述符
				if (events[i].events & EPOLLIN) {
					//printf("This is EPOLLIN\n");
					threadpool_add(pool, request_controller, events[i].data.ptr);
				}

				/*
				if (events[i].events & EPOLLOUT) {
					printf("This is EPOLLOUT\n");
					threadpool_add(pool, response_controller, events[i].data.ptr);
				}
				*/
			}
		}
	}
	free(events);
	close(epollfd);
	close(listenfd);
	return 0;
}