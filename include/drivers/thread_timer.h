/**
 * 线程定时器
*/
#include "sys/dlist.h"

struct thread_timer;
typedef void (*thread_timer_expiry_t)(struct thread_timer *ttimer,
                                      void *expiry_fn_arg);
struct thread_timer{
    sys_dlist_t node;
    int duration;
    int period;
    int expiry_time;
    void *tid;
    thread_timer_expiry_t expiry_fn;
    void *expiry_fn_arg;
};

extern void thread_timer_init(struct thread_timer *ttimer,
                        thread_timer_expiry_t expiry_fn,
                        void *expiry_fn_arg);
extern void thread_timer_start(struct thread_timer *ttimer, int duration, int period);
extern void thread_timer_stop(struct thread_timer *ttimer);
extern bool thread_timer_is_running(struct thread_timer *ttimer);
extern void thread_timer_handle_expired(void);