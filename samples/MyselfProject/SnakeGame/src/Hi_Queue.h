
/**
 * 单链表的队列实现
 * 同样的,队列则是链表的尾插法使用
*/

#ifndef __HI_QUEUE_H__
#define __HI_QUEUE_H__

#ifdef __cplusplus
extern "C" {
#endif


#include "public.h"

struct queue_head
{
    struct queue_head *hook;
};
#define QUEUE_HEAD_INIT()       {NULL}
#define QUEUE_HEAD(name)            \
    struct queue_head name = QUEUE_HEAD_INIT()

/**
 * @brief 数据入队
 * @param _new      需要入队的新节点
 * @param head      队列的头节点
*/
static inline void queue_push(struct queue_head *_new,
                              struct queue_head *head)
{
    struct queue_head *temp = head;
    while(NULL != temp->hook){
        temp = temp->hook;
    }
    temp->hook = _new;
    _new->hook = NULL;
}

/**
 * @brief           数据出队
 * @param head      队列的头节点
 * @return          返回出队数据的节点
*/
static inline struct queue_head* queue_pop(struct queue_head *head)
{
    struct queue_head* temp = head->hook;
    if(NULL == temp){
        printf("queue is null\n");
        return NULL;
    }
    head->hook = temp->hook;

    return temp;
}

/**
 * @brief           队列判空
 * @param head      队列的头节点
 * @return          1:队列空 0:队列非空
*/
static inline int queue_empty(const struct queue_head *head)
{
    return head->hook == NULL;
}




#ifdef __cplusplus
}
#endif


#endif