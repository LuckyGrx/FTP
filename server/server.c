#include "util.h"
#include "epoll.h"
#include "ftp_connection.h"
#include "threadpool.h"
#include "time_wheel.h"

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

	// 
	signal(SIGALRM, time_wheel_alarm_handler);
	// 初始化时间轮
	time_wheel_init();
	// 发送定时信号
	alarm(DEFAULT_TICK_TIME);
	for (;;) {

		// 调用epoll_wait函数，返回接收到事件的数量
        int events_num = ftp_epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);

		// 遍历events数组
        ftp_handle_events(epollfd, listenfd, events, events_num, pool);
	}
	
	// 销毁线程池（平滑停机模式）
	threadpool_destroy(pool, conf.shutdown);
	// 销毁时间轮
	time_wheel_destroy();

	free(connection);
	free(events);
	close(epollfd);
	close(listenfd);
	return 0;
}