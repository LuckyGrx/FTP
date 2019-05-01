#include "time_wheel.h"



int time_wheel_init() {
    time_wheel.cur_slot = 0;
    time_wheel.slot_num = SLOT_NUM;
    time_wheel.slot_interval = 1;
    int i;
    for (i = 0; i < time_wheel.slot_num; ++i)
        time_wheel.slots[i] = NULL;

	pthread_mutex_init(&(time_wheel.mutex), NULL);
    return 0;
}

int time_wheel_add_timer(ftp_connection_t* connection, timer_handler_pt handler) {
	pthread_mutex_lock(&(time_wheel.mutex));

    int ticks = 0;        // 转动几个槽触发
    int rotation = 0;     // 计算待插入的定时器在时间轮转动多少圈后被触发
    int slot = 0;         // 距离当前槽相差几个槽

    ticks = DEFAULT_TIMEOUT / time_wheel.slot_interval;
    
    rotation = ticks / time_wheel.slot_num;

    slot = (time_wheel.cur_slot + (ticks % time_wheel.slot_num)) % time_wheel.slot_num;

    tw_timer* timer = (tw_timer*)calloc(1, sizeof(tw_timer));
    timer->rotation = rotation;
    timer->slot = slot;
    timer->handler = handler;
    timer->connection = connection;

    connection->timer = timer; // 之后好进行删除操作

    if (time_wheel.slots[slot] == NULL) {
        time_wheel.slots[slot] = timer;
        timer->next = NULL;
        timer->prev = NULL;
    } else {//头插法
        timer->next = time_wheel.slots[slot];
        time_wheel.slots[slot]->prev = timer;
        time_wheel.slots[slot] = timer;
    }

	pthread_mutex_unlock(&(time_wheel.mutex));
    return 0;
}

int time_wheel_del_timer(ftp_connection_t* connection) {
	pthread_mutex_lock(&(time_wheel.mutex));

    tw_timer* timer = (tw_timer*)connection->timer;


    if (timer != NULL) {
        if (timer == time_wheel.slots[timer->slot]) {
            time_wheel.slots[timer->slot] = time_wheel.slots[timer->slot]->next;
            if (time_wheel.slots[timer->slot])
                time_wheel.slots[timer->slot]->prev = NULL;
        } else {
            timer->prev->next = timer->next;
            if (timer->next)
                timer->next->prev = timer->prev;
        }
        free(timer);
    }

	pthread_mutex_unlock(&(time_wheel.mutex));
}

void time_wheel_alarm_handler(int sig) {
    int ret = time_wheel_tick();

    alarm(time_wheel.slot_interval);
}

int time_wheel_tick() {
	pthread_mutex_lock(&(time_wheel.mutex));

    tw_timer* tmp = time_wheel.slots[time_wheel.cur_slot];

    time_t tt = time(NULL);

    while (tmp != NULL) {
        if (tmp->rotation > 0) {
            --(tmp->rotation);
            tmp = tmp->next;
        } else { // 否则，说明定时器已经到期，于是执行定时任务，然后删除该定时器
            tmp->handler(tmp->connection);

            if (tmp == time_wheel.slots[time_wheel.cur_slot]) {
                time_wheel.slots[time_wheel.cur_slot] = tmp->next;
                free(tmp);

                if (time_wheel.slots[time_wheel.cur_slot])
                    time_wheel.slots[time_wheel.cur_slot]->prev = NULL;
                
                tmp = time_wheel.slots[time_wheel.cur_slot];

            } else {
                tmp->prev->next = tmp->next;
                if (tmp->next)
                    tmp->next->prev = tmp->prev;

                tw_timer* tmp2 = tmp->next;
                free(tmp);
                tmp = tmp2;
            }
        }
    }
    time_wheel.cur_slot = ++(time_wheel.cur_slot) % time_wheel.slot_num;

	pthread_mutex_unlock(&(time_wheel.mutex));
}