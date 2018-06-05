/*
 * Copyright (c) 2013-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <toolchain.h>

#ifdef TEST_min

/* INTENTIONALLY LEFT EMPTY (I.E. NO APPLICATION THREADS, OR ISRS) */

#else

#include <misc/printk.h>
#include <stdio.h>

#define IRQ_LINE          10  /* just some random value w/o driver conflicts */
#define IRQ_PRIORITY      3
#define TEST_SOFT_INT	  64

#ifdef TEST_max
#define MESSAGE "Running maximal kernel configuration %p\n"
#else
#define MESSAGE "Running regular kernel configuration %p\n"
#endif /* TEST_max */

typedef void* (*pfunc) (void *);

/* variables */

volatile int i;	/* counter used by foreground task */

static pfunc func_array[] = {
	/* mutexes */
	(pfunc)k_mutex_init,
	(pfunc)k_mutex_lock,
	(pfunc)k_mutex_unlock,

	/* semaphores */
	(pfunc)k_sem_init,
	(pfunc)k_sem_take,
	(pfunc)k_sem_give,
	(pfunc)k_sem_reset,
	(pfunc)k_sem_count_get,

	/* queues */
	(pfunc)k_queue_init,
	(pfunc)k_queue_append,
	(pfunc)k_queue_prepend,
	(pfunc)k_queue_append_list,
	(pfunc)k_queue_merge_slist,
	(pfunc)k_queue_get,

	/* mem slabs */
	(pfunc)k_mem_slab_init,
	(pfunc)k_mem_slab_alloc,
	(pfunc)k_mem_slab_free,
	(pfunc)k_mem_slab_num_used_get,
	(pfunc)k_mem_slab_num_free_get,

#ifdef TEST_max
	/* alerts */
	(pfunc)k_alert_init,
	(pfunc)k_alert_send,
	(pfunc)k_alert_recv,

	/* message queues */
	(pfunc)k_msgq_init,
	(pfunc)k_msgq_put,
	(pfunc)k_msgq_get,
	(pfunc)k_msgq_purge,
	(pfunc)k_msgq_num_free_get,
	(pfunc)k_msgq_num_used_get,

	/* stacks */
	(pfunc)k_stack_init,
	(pfunc)k_stack_push,
	(pfunc)k_stack_pop,

	/* workqueues */
	(pfunc)k_work_init,
	(pfunc)k_work_submit_to_queue,
	(pfunc)k_work_pending,
	(pfunc)k_work_q_start,
	(pfunc)k_delayed_work_init,
	(pfunc)k_delayed_work_submit_to_queue,
	(pfunc)k_delayed_work_cancel,
	(pfunc)k_work_submit,
	(pfunc)k_delayed_work_submit,

	/* mailboxes */
	(pfunc)k_mbox_init,
	(pfunc)k_mbox_put,
	(pfunc)k_mbox_async_put,
	(pfunc)k_mbox_get,
	(pfunc)k_mbox_data_get,
	(pfunc)k_mbox_data_block_get,

	/* pipes */
	(pfunc)k_pipe_init,
	(pfunc)k_pipe_put,
	(pfunc)k_pipe_get,
	(pfunc)k_pipe_block_put,

	/* mem pools */
	(pfunc)k_mem_pool_alloc,
	(pfunc)k_mem_pool_free,
	(pfunc)k_malloc,
	(pfunc)k_free,

	/* timers */
	(pfunc)k_timer_init,
	(pfunc)k_timer_stop,
	(pfunc)k_timer_status_get,
	(pfunc)k_timer_status_sync,
	(pfunc)k_timer_remaining_get,
	(pfunc)k_uptime_get,
	(pfunc)k_uptime_get_32,
	(pfunc)k_uptime_delta,
	(pfunc)k_uptime_delta_32,

	/* thread stuff */
	(pfunc)k_thread_create,
	(pfunc)k_sleep,
	(pfunc)k_busy_wait,
	(pfunc)k_yield,
	(pfunc)k_wakeup,
	(pfunc)k_current_get,
	(pfunc)k_thread_abort,
	(pfunc)k_thread_priority_get,
	(pfunc)k_thread_priority_set,
	(pfunc)k_thread_suspend,
	(pfunc)k_thread_resume,
	(pfunc)k_sched_time_slice_set,
	(pfunc)k_is_in_isr,
	(pfunc)k_thread_custom_data_set,
	(pfunc)k_thread_custom_data_get,
#endif
};

/**
 *
 * @brief Dummy ISR
 *
 * @return N/A
 */
void dummy_isr(void *unused)
{
	ARG_UNUSED(unused);
}

/**
 *
 * @brief Entry function for foreground task
 *
 * This routine prints a message, then simply increments a global counter.
 * (Gdb can be used to observe the counter as it increases.)
 *
 * @return N/A
 */
void main(void)
{
	i = 0;
#ifdef TEST_reg
	IRQ_CONNECT(IRQ_LINE, IRQ_PRIORITY, dummy_isr, NULL, 0);
#endif
	/* note: referencing "func_array" ensures it isn't optimized out */
#ifdef TEST_max
	printf((char *)MESSAGE, func_array);
#else
	printk((char *)MESSAGE, func_array);
#endif /* TEST_max */

	while (1) {
		i++;
	}
}

#endif /* TEST_min */
