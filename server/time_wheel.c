#include "time_wheel.h"

int time_wheel_init() {
    tw.cur_slot = 0;
    tw.slot_num = SLOT_NUM;
    tw.slot_interval = 1;
    for (int i = 0; i < tw.slot_num; ++i)
        tw.slots[i] = NULL;

    pthread_spin_init(&(tw.spin_lock), PTHREAD_PROCESS_SHARED);
    return 0;
}

int time_wheel_destroy() {
    for (int i = 0; i < tw.slot_num; ++i)
        free(tw.slots[i]);
	pthread_spin_destroy(&(tw.spin_lock));
}

int time_wheel_add_timer(ftp_connection_t* connection, timer_handler_pt handler, int timeout) {
    pthread_spin_lock(&(tw.spin_lock));

    int ticks = 0;        // 转动几个槽触发
    int rotation = 0;     // 计算待插入的定时器在时间轮转动多少圈后被触发
    int slot = 0;         // 距离当前槽相差几个槽

    ticks = timeout / tw.slot_interval;

    if (0 == ticks)
        ticks = 1;
    
    rotation = ticks / tw.slot_num;

    slot = (tw.cur_slot + (ticks % tw.slot_num)) % tw.slot_num;

    tw_timer_t* timer = (tw_timer_t*)calloc(1, sizeof(tw_timer_t));
    timer->rotation = rotation;
    timer->slot = slot;
    timer->handler = handler;
    timer->connection = connection;
    timer->next = NULL;
    timer->prev = NULL;

    connection->timer = timer; // 之后好进行删除操作

    if (tw.slots[slot] == NULL) {
        tw.slots[slot] = timer;
    } else {//头插法
        timer->next = tw.slots[slot];
        tw.slots[slot]->prev = timer;
        tw.slots[slot] = timer;
    }

    pthread_spin_unlock(&(tw.spin_lock));
    return 0;
}

int time_wheel_del_timer(ftp_connection_t* connection) {
    if (connection == NULL)
        return -1;
    pthread_spin_lock(&(tw.spin_lock));

    tw_timer_t* timer = (tw_timer_t*)connection->timer;

    if (timer != NULL) {	
        if (timer == tw.slots[timer->slot]) {	
            tw.slots[timer->slot] = timer->next;	
            if (tw.slots[timer->slot])	
                tw.slots[timer->slot]->prev = NULL;	
        } else {	
            timer->prev->next = timer->next;	
            if (timer->next)	
                timer->next->prev = timer->prev;	
        }	
        free(timer);	
        connection->timer = NULL; // 修复timer->slot的值会大于9的情况(32515)
    }

    pthread_spin_unlock(&(tw.spin_lock));
}

void time_wheel_alarm_handler(int sig) {
    time_wheel_tick();

    alarm(DEFAULT_TICK_TIME);
}

int time_wheel_tick() {
    pthread_spin_lock(&(tw.spin_lock));

    tw_timer_t* tmp = tw.slots[tw.cur_slot];

    while (tmp != NULL) {
        if (tmp->rotation > 0) {
            --(tmp->rotation);
            tmp = tmp->next;
        } else { // 否则，说明定时器已经到期，于是执行定时任务，然后删除该定时器
            //(DEFAULT_CONNECTION_TIMEOUT / time_wheel.slot_interval) * EPOLL_TIMEOUT = 大约50 s
            tmp->handler(tmp->connection); //  执行定时任务

            if (tmp == tw.slots[tw.cur_slot]) {
                tw.slots[tw.cur_slot] = tmp->next;
                free(tmp);

                if (tw.slots[tw.cur_slot])
                    tw.slots[tw.cur_slot]->prev = NULL;
                
                tmp = tw.slots[tw.cur_slot];

            } else {
                tmp->prev->next = tmp->next;
                if (tmp->next)
                    tmp->next->prev = tmp->prev;

                tw_timer_t* tmp2 = tmp->next;
                free(tmp);
                tmp = tmp2;
            }
        }
    }
    tw.cur_slot = (++(tw.cur_slot)) % tw.slot_num;

    pthread_spin_unlock(&(tw.spin_lock));
}