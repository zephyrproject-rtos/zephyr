/**
 * 优先队列,可以使用单向/双向链表
 * 为了实现多态，用户自定义的结构数据的要求
 * 
 * 优先队列除了入队操作使用自己的,其他操作共用普通队列
*/
#ifndef __HI_PRIORITYQUEUE_H__
#define __HI_PRIORITYQUEUE_H__

#ifdef __cplusplus
extern "C" {
#endif


#include "Hi_list.h"

struct priority_queue_info
{
    struct list_head prior_hook;
    int prior;

    char* name;//用户自定义的数据放在下面
};

typedef struct priority_queue_info prio_queue_info;


static inline void __queue_node_inset(struct list_head *_new, struct list_head *_pos)
{
    _new->next = _pos;
    _new->prev = _pos->prev;
    _pos->prev->next = _new;
    _pos->prev = _new;
}

/**
 * @brief 优先队列的数据入队
 * @param _new      需要入队的新节点
 * @param head      队列的头节点
*/
static inline void priority_queue_push(struct priority_queue_info *_new,
                                       struct list_head *head)
{
    struct priority_queue_info *temp=NULL;
    list_for_each_entry(temp,head,prior_hook){
        if(_new->prior < temp->prior){
            __queue_node_inset(&_new->prior_hook,&temp->prior_hook);
            return;
        }
    }
    //小丑竟是我自己,就尾插最后
    list_add_tail(&(_new->prior_hook),head);
}


#ifdef __cplusplus
}
#endif

#endif