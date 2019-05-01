#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

typedef struct client_data {
    time_t tt;
    void* data;
}client_data;

typedef struct tw_timer {
    int rotation;                     // 记录定时器在时间轮转多少圈后生效
    int slot;                         // 记录定时器属于时间轮上哪个槽
 
    void* (*cb_func)(void* param);    // 定时器回调函数
    struct client_data c_data;        // 客户数据

    struct tw_timer* next;           // 指向下一个定时器
    struct tw_timer* prev;           // 指向上一个定时器
}tw_timer;

#define SLOT_NUM 10


typedef struct time_wheel {
    int cur_slot;                      // 时间轮的当前槽
    int slot_num;                      // 时间轮上槽的数目
    int slot_interval;                 // 槽间隔时间
    struct tw_timer* slots[SLOT_NUM];  // 时间轮的槽，其中每个元素指向一个定时器链表，链表无序
}time_wheel_t;


int init_time_wheel_t(time_wheel_t* time_wheel, int slot_interval) {
    time_wheel->cur_slot = 0;
    time_wheel->slot_num = SLOT_NUM;
    time_wheel->slot_interval = slot_interval;
    int i;
    for (i = 0; i < time_wheel->slot_num; ++i)
        time_wheel->slots[i] = NULL;

    return 0;
}

void* ontime_func(void* param) {
    client_data* data = (client_data*)param;

    time_t tt = time(NULL);

    printf("-------------------------\n");

    printf("\t old time:%s", ctime(&data->tt));
    printf("\t current time:%s", ctime(&tt));

    printf("-------------------------\n");
    return NULL;
}


int add_timer(time_wheel_t* time_wheel, int timeout) {
    if (timeout < 0)
        return -1;
    int ticks = 0;        // 转动几个槽触发
    int rotation = 0;    // 计算待插入的定时器在时间轮转动多少圈后被触发
    int slot = 0;        // 距离当前槽相差几个槽

    ticks = timeout / time_wheel->slot_interval;
    
    rotation = ticks / time_wheel->slot_num;

    slot = (time_wheel->cur_slot + (ticks % time_wheel->slot_num)) % time_wheel->slot_num;

    tw_timer* timer = (tw_timer*)calloc(1, sizeof(tw_timer));
    timer->rotation = rotation;
    timer->slot = slot;
    timer->cb_func = ontime_func;

    timer->c_data.tt = time(NULL);


    if (time_wheel->slots[slot] == NULL) {
        time_wheel->slots[slot] = timer;
        timer->next = NULL;
        timer->prev = NULL;
    } else {//头插法
        timer->next = time_wheel->slots[slot];
        time_wheel->slots[slot]->prev = timer;
        time_wheel->slots[slot] = timer;
    }
    return 0;
}


int tick(time_wheel_t* time_wheel) {
    if (time_wheel == NULL)
        return -1;

    tw_timer* tmp = time_wheel->slots[time_wheel->cur_slot];

    printf("current slot is %d\n", time_wheel->cur_slot);
    time_t tt = time(NULL);
    printf("\t current time:%s", ctime(&tt));

    while (tmp != NULL) {
        if (tmp->rotation > 0) {
            --(tmp->rotation);
            tmp = tmp->next;
        } else { // 否则，说明定时器已经到期，于是执行定时任务，然后删除该定时器
            tmp->cb_func(&(tmp->c_data));

            if (tmp == time_wheel->slots[time_wheel->cur_slot]) {
                time_wheel->slots[time_wheel->cur_slot] = tmp->next;
                free(tmp);

                if (time_wheel->slots[time_wheel->cur_slot])
                    time_wheel->slots[time_wheel->cur_slot]->prev = NULL;
                
                tmp = time_wheel->slots[time_wheel->cur_slot];

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
    time_wheel->cur_slot = ++(time_wheel->cur_slot) % time_wheel->slot_num;
}


time_wheel_t time_wheel;

void alarm_handler(int sig) {

    int ret = tick(&time_wheel);

    alarm(time_wheel.slot_interval);

}

int main() {
    signal(SIGALRM, alarm_handler);

    init_time_wheel_t(&time_wheel, 1);

    add_timer(&time_wheel, 1);
    add_timer(&time_wheel, 5);
    add_timer(&time_wheel, 6);
    add_timer(&time_wheel, 10);
    add_timer(&time_wheel, 11);
    add_timer(&time_wheel, 19);
    add_timer(&time_wheel, 21);


    alarm(time_wheel.slot_interval);


    while (1)
        sleep(5);

    return 0;
}