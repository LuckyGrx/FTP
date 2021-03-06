#include "util.h"
#include "epoll.h"
#include "ftp_connection.h"
#include "threadpool.h"

extern struct epoll_event *events;
#define DEFAULT_CONFIG "ftp.conf"

int main (int argc, char* argv[]) {
	// 开启守护进程
	//ftp_daemon();

	ftp_conf_t conf;
	read_conf(DEFAULT_CONFIG, &conf);
	
	handle_for_sigpipe();

	int listenfd = tcp_socket_bind_listen(conf.port);
	int rc = make_socket_non_blocking(listenfd);

	int epollfd = ftp_epoll_create();

	ftp_connection_t* connection = (ftp_connection_t*)calloc(1, sizeof(ftp_connection_t));
	init_connection_t(connection, listenfd, epollfd);
	ftp_epoll_add(epollfd, listenfd, connection, EPOLLIN | EPOLLET);

	// 初始化线程池
	ftp_threadpool_t* pool = threadpool_init(conf.threadnum);
	// 初始化定时器链表
	timer_list_init();
	// epoll_wait 超时时间
	int timeout = 1000 * 5; // 每5秒运行一次心搏函数
	for (;;) {

		// 调用epoll_wait函数，返回接收到事件的数量
        int events_num = ftp_epoll_wait(epollfd, events, MAX_EVENT_NUMBER, &timeout);

		// 遍历events数组
        ftp_handle_events(epollfd, listenfd, events, events_num, pool);
	}
	
	// 销毁线程池（平滑停机模式）
	threadpool_destroy(pool, conf.shutdown);
	// 销毁定时器链表
	timer_wlist_destroy();

	free(connection);
	free(events);
	close(epollfd);
	close(listenfd);
	return 0;
}