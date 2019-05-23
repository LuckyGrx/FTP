#ifndef __TIMER_LIST_H__
#define __TIMER_LIST_H__
#include "head.h"
#include "ftp_connection.h"

#define DEFAULT_TICK_TIME 10  // 每隔10秒运行心搏函数

// 函数指针，负责超时处理， add_timer时指定处理函数
typedef int (*timer_handler_pt)(ftp_connection_t* connection);

typedef struct tl_timer {
    time_t          expire;             // 任务的超时时间,使用绝对时间
    timer_handler_pt handler;           // 定时器回调函数
    ftp_connection_t*   connection;     // 客户数据

    struct tl_timer* next;            // 指向下一个定时器
    struct tl_timer* prev;            // 指向上一个定时器
}tl_timer_t;

typedef struct timer_list {
    struct tl_timer*  head;                // 链表头指针     
    struct tl_timer*  tail;                // 链表尾指针
    pthread_spinlock_t spin_lock;          // 自旋锁
}timer_list_t;

volatile timer_list_t tl;

int timer_list_init();

int timer_list_add_timer(ftp_connection_t* connnection, timer_handler_pt handler, int timeout);

int timer_list_del_timer(ftp_connection_t* connection);

int timer_list_destroy();

int timer_list_tick();

#endif