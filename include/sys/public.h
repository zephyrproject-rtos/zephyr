
/**
 * 队列和栈部分公用内容
 * 这一部分是linux内核里一种高级用法
*/




#ifndef __PUBLIC_H__
#define __PUBLIC_H__

#include "stdlib.h"


#define     u8_t        unsigned char
#define     u16_t       unsigned int


#define CONTAINER_OF(ptr,type,member) ({\
(type*)((char*)ptr-(size_t)(&((type*)0)->member));\
})


#define list_entry(ptr, type, member)   \
    CONTAINER_OF(ptr, type, member)

//加括号是因为传进来的参数是一个取地址，防止出错
#define list_for_each_entry(pos, head, member)      \
    for(pos = list_entry((head)->next, typeof(*pos), member); \
        &pos->member != (head);                                   \
        pos = list_entry(pos->member.next, typeof(*pos), member))


#endif 