

#include "drivers/thread_timer.h"
#include "sys/__assert.h"
#include "string.h"
#include "kernel_structs.h"
#include "kernel.h"

#define compare_time(a,b) ((int)((unsigned int)(a) - (unsigned int)(b)))
static void __thread_timer_insert(struct k_thread *thread,
                                  struct thread_timer *ttimer,
                                  unsigned int expiry_time)
{
    sys_dlist_t *thread_timer_q = &thread->thread_timer_q;
    struct thread_timer *in_q;
    int irq_lock;
    irq_lock = irq_lock();
    SYS_DLIST_FOR_EACH_CONTAINER(thread_timer_q,in_q,node){
        if(compare_time(ttimer->expiry_time,in_q->expiry_time)<0){
            sys_dlist_insert(&in_q->node,&ttimer->node);
            irq_unlock(irq_lock);
            return;
        }
    }
    sys_dlist_append(thread_timer_q,&ttimer->node);
    irq_unlock(irq_lock);
}




static void __thread_timer_remove(struct k_thread *thread,
                                  struct thread_timer *ttimer)
{
    struct thread_timer *in_q, *next;
    int irq_flag;
    irq_flag = irq_lock();
    SYS_DLIST_FOR_EACH_CONTAINER_SAFE(&thread->thread_timer_q, in_q, next, node){
        if(ttimer == in_q){
            sys_dlist_remove(&in_q->node);
            irq_unlock(irq_flag);
            return;
        }
    }
    irq_unlock(irq_flag);
}



void thread_timer_init(struct thread_timer *ttimer,
                        thread_timer_expiry_t expiry_fn,
                        void *expiry_fn_arg)
{
    __ASSERT(ttimer != NULL,"ttimer is null~");

    __thread_timer_remove(_current, ttimer);

    memset(ttimer, 0, sizeof(struct thread_timer));
    ttimer->expiry_fn = expiry_fn;
    ttimer->expiry_fn_arg = expiry_fn_arg;
    ttimer->tid = _current;
    sys_dlist_init(&ttimer->node);//先指向自己
}

void thread_timer_start(struct thread_timer *ttimer, int duration, int period)
{
    unsigned int cur_time;
    __ASSERT((ttimer!=NULL) && (ttimer->expiry_fn!=NULL),"ttimer expiry_fn is null~");

    __thread_timer_remove(_current, ttimer);

    cur_time = k_uptime_get_32();
    ttimer->expiry_time = cur_time + duration;
    ttimer->period = period;
    ttimer->duration = duration;
    __thread_timer_insert(_current, ttimer, ttimer->expiry_time);
}

void thread_timer_stop(struct thread_timer *ttimer)
{
    __ASSERT(ttimer != NULL,"ttimer is null");
    __thread_timer_remove(_current,ttimer);
}

bool thread_timer_is_running(struct thread_timer *ttimer)
{
    struct thread_timer *in_q;
    SYS_DLIST_FOR_EACH_CONTAINER(&_current->thread_timer_q,in_q,node){
        if(ttimer == in_q)
            return true;
    }
    return false;
}

void thread_timer_handle_expired(void)
{
    int irq_flag;
    struct thread_timer *ttimer;
    unsigned int cur_time = k_uptime_get_32();
    do{
        irq_flag = irq_lock();
        ttimer = SYS_DLIST_PEEK_HEAD_CONTAINER(
            &_current->thread_timer_q,ttimer,node
        );
        if(!ttimer || (compare_time(ttimer->expiry_time,cur_time)>0)){
            irq_unlock(irq_flag);//还没有到期的就出去
            break;
        }
        //从链表里移除到期的任务
        sys_dlist_remove(&ttimer->node);
        irq_unlock(irq_flag);
        if(ttimer->expiry_fn){
            if(ttimer->period){
                thread_timer_start(ttimer,ttimer->duration,ttimer->period);
            }
            ttimer->expiry_fn(ttimer,ttimer->expiry_fn_arg);
        }

    }while(1);
}




