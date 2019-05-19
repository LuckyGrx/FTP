#ifndef __TIME_WHEEL_H__
#define __TIME_WHEEL_H__

#include "head.h"
#include "ftp_connection.h"

#define SLOT_NUM 10

#define DEFAULT_TICK_TIME 10  // 每隔10秒运行心搏函数

// 函数指针，负责超时处理， add_timer时指定处理函数
typedef int (*timer_handler_pt)(ftp_connection_t* request);

typedef struct tw_timer {
    int rotation;                     // 记录定时器在时间轮转多少圈后生效
    int slot;                         // 记录定时器属于时间轮上哪个槽
 
    timer_handler_pt handler;         // 定时器回调函数
    ftp_connection_t*   connection;   // 客户数据

    struct tw_timer* next;            // 指向下一个定时器
    struct tw_timer* prev;            // 指向上一个定时器
}tw_timer_t;

typedef struct time_wheel {
    int cur_slot;                      // 时间轮的当前槽
    int slot_num;                      // 时间轮上槽的数目
    int slot_interval;                 // 槽间隔时间
    tw_timer_t* slots[SLOT_NUM];  // 时间轮的槽，其中每个元素指向一个双向定时器链表，链表无序

    pthread_spinlock_t spin_lock;       // 自旋锁
}time_wheel_t;

volatile time_wheel_t tw;

int time_wheel_init();

int time_wheel_add_timer(ftp_connection_t* connnection, timer_handler_pt handler, int timeout);

int time_wheel_del_timer(ftp_connection_t* connection);

void time_wheel_alarm_handler(int sig);

int time_wheel_destroy();

int time_wheel_tick();

#endif