
#ifndef __HI_LIST_H__
#define __HI_LIST_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "public.h"


/**
 * 双向链表
*/
struct list_head
{
    struct list_head *next,*prev;
};

//头节点的初始化
#define LIST_HEAD_INIT(name)    {&(name),&(name)}
#define LIST_HEAD(name)         \
    struct list_head name = LIST_HEAD_INIT(name)

static inline int list_empty(const struct list_head *head)
{
    return head->next == head;
}

static inline void __list_add(struct list_head *_new,
                              struct list_head *prev,
                              struct list_head *next)
{
    next->prev = _new;
    _new->next = next;
    _new->prev = prev;
    prev->next = _new;
}


//尾插
static inline void list_add_tail(struct list_head *_new, struct list_head *head)
{
    __list_add(_new, head->prev, head);
}

static inline void list_add_head(struct list_head *_new, struct list_head *head)
{
    __list_add(_new, head, head->next);
}


static inline void __list_del(struct list_head *prev, struct list_head *next)
{
    next->prev = prev;
    prev->next = next;
    
}
/**
 * 库里不做内存释放工作，如有需要外部定义指针变量malloc分配，free释放
 * 参考A star算法的使用操作
*/
static inline void list_del(struct list_head *entry)
{
    __list_del(entry->prev, entry->next);
}



#ifdef __cplusplus
}
#endif


#endif

