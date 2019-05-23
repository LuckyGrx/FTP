#include "timer_list.h"

int timer_list_init() {
    tl.head = tl.tail = NULL;
    pthread_spin_init(&(tl.spin_lock), PTHREAD_PROCESS_SHARED);
    return 0;
}

int timer_list_destroy() {
    tl_timer_t* tmp = tl.head;
    while (tmp) {
        tl.head = tmp->next;
        free(tmp);
        tmp = tl.head;
    }
	pthread_spin_destroy(&(tl.spin_lock));
}

// 将目标定时器timer添加到节点list_head之后的部分链表中
static void add_timer(tl_timer_t* timer, tl_timer_t* list_head) {
    tl_timer_t* prev = list_head;
    tl_timer_t* tmp = prev->next;

    // 遍历 list_head节点之后的部分链表,直到找到一个超时时间大于目标定时器的超时
    // 时间的节点,并将目标定时器插入该节点之前
    while (tmp) {
        if (timer->expire < tmp->expire) {
            prev->next = timer;
            timer->next = tmp;
            tmp->prev = timer;
            timer->prev = prev;
            break;
        }
        prev = tmp;
        tmp = tmp->next;
    }
    // 如果遍历完list_head节点之后的部分链表,仍未找到超时时间大于目标定时器的超时时间的节点,
    // 则将目标定时器插入链表尾部,并把它设置为链表新的尾节点
    if (!tmp) {
        prev->next = timer;
        timer->prev = prev;
        timer->next = NULL;
        tl.tail = timer;
    }
}

// 将目标定时器添加到链表中(目标定时器的超时时间为timeout)
int timer_list_add_timer(ftp_connection_t* connection, timer_handler_pt handler, int timeout) {
    pthread_spin_lock(&(tl.spin_lock));

    tl_timer_t* timer = (tl_timer_t*)calloc(1, sizeof(tl_timer_t));
    timer->expire = time(NULL) + timeout;
    timer->prev = NULL;
    timer->next = NULL;
    timer->handler = handler;
    timer->connection = connection;

    connection->timer = timer; // 之后好进行删除操作

    if (tl.head == NULL) {
        tl.head = tl.tail = timer;
        goto done;
    }
    // 如果目标定时器的超时时间小于当前链表中所有定时器的超时时间,则把该定时器插入链表头部,作为新的头节点,
    // 否则调用add_timer,把目标定时器插入链表中合适的位置,保证链表的升序特性
    if (timer->expire < tl.head->expire) {
        timer->next = tl.head;
        tl.head->prev = timer;
        tl.head = timer;
        goto done;
    }
    add_timer(timer, tl.head);

done:
    pthread_spin_unlock(&(tl.spin_lock));
    return 0;
}


int timer_list_del_timer(ftp_connection_t* connection) {
    if (connection == NULL)
        return -1;
    pthread_spin_lock(&(tl.spin_lock));

    tl_timer_t* timer = (tl_timer_t*)connection->timer;

    if (timer != NULL) {
        // 表示链表中只有一个定时器,即目标定时器
        if ((timer == tl.head) && (timer == tl.tail)) {
            tl.head = NULL;
            tl.tail = NULL;
        // 如果链表中至少有两个定时器,且目标定时器是链表的头结点,则将链表的头结点重置为原头结点的下一个节点,
        // 然后删除目标定时器
        } else if (timer == tl.head) {
            tl.head = tl.head->next;
            tl.head->prev = NULL;
        // 如果链表中至少有两个定时器,且目标定时器是链表的尾结点,则将链表的尾结点重置为原尾结点的前一个节点,
        // 然后删除目标定时器
        } else if (timer == tl.tail) {
            tl.tail = tl.tail->prev;
            tl.tail->next = NULL;
        // 如果目标定时器位于链表的中间,则把它前后的定时器串联起来,然后删除目标定时器
        } else {
            timer->prev->next = timer->next;
            timer->next->prev = timer->prev;
        }
        free(timer);
        connection->timer = NULL; // 修复野指针问题
    }

    pthread_spin_unlock(&(tl.spin_lock));
}

int timer_list_tick() {
    pthread_spin_lock(&(tl.spin_lock));

    time_t cur = time(NULL);    // 获得系统当前的时间
    tl_timer_t* tmp = tl.head;
    // 从头结点开始依次处理每个定时器,直到遇到一个尚未到期的定时器,这就是定时器的核心逻辑
    while (tmp) {
        // 因为每个定时器都使用绝对时间作为超时值,所以我们可以把定时器的超时值和系统当前时间进行比较,
        // 以判断定时器是否到期
        if (cur < tmp->expire)
            break;
        
        // 调用定时器的回调函数,以执行定时任务
        tmp->handler(tmp->connection); 
        // 执行完定时器中的定时任务之后,就将它从链表中删除,并重置链表头节点
        tl.head = tmp->next;

        if (tl.head)
            tl.head->prev = NULL;
        free(tmp);
        tmp = tl.head;
    }

    pthread_spin_unlock(&(tl.spin_lock));
}