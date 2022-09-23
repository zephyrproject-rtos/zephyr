/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_TRACING_TRACKING_H_
#define ZEPHYR_INCLUDE_TRACING_TRACKING_H_

#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>

#if defined(CONFIG_TRACING_OBJECT_TRACKING) || defined(__DOXYGEN__)

/**
 * @brief Object tracking
 *
 * Object tracking provides lists to kernel objects, so their
 * existence and current status can be tracked.
 *
 * The following global variables are the heads of available lists:
 * - _track_list_k_timer
 * - _track_list_k_mem_slab
 * - _track_list_k_sem
 * - _track_list_k_mutex
 * - _track_list_k_stack
 * - _track_list_k_msgq
 * - _track_list_k_mbox
 * - _track_list_k_pipe
 * - _track_list_k_queue
 *
 * @defgroup subsys_tracing_object_tracking Object tracking
 * @ingroup subsys_tracing
 * @{
 */

extern struct k_timer *_track_list_k_timer;
extern struct k_mem_slab *_track_list_k_mem_slab;
extern struct k_sem *_track_list_k_sem;
extern struct k_mutex *_track_list_k_mutex;
extern struct k_stack *_track_list_k_stack;
extern struct k_msgq *_track_list_k_msgq;
extern struct k_mbox *_track_list_k_mbox;
extern struct k_pipe *_track_list_k_pipe;
extern struct k_queue *_track_list_k_queue;

/**
 * @brief Gets node's next element in a object tracking list.
 *
 * @param list Node to get next element from.
 */
#define SYS_PORT_TRACK_NEXT(list)((list)->_obj_track_next)

/** @cond INTERNAL_HIDDEN */

#define sys_port_track_k_thread_start(thread)
#define sys_port_track_k_thread_create(new_thread)
#define sys_port_track_k_thread_sched_ready(thread)
#define sys_port_track_k_thread_wakeup(thread)
#define sys_port_track_k_thread_sched_priority_set(thread, prio)
#define sys_port_track_k_work_delayable_init(dwork)
#define sys_port_track_k_work_queue_init(queue)
#define sys_port_track_k_work_init(work)
#define sys_port_track_k_mutex_init(mutex, ret) \
	sys_track_k_mutex_init(mutex)
#define sys_port_track_k_timer_stop(timer)
#define sys_port_track_k_timer_start(timer, duration, period)
#define sys_port_track_k_timer_init(timer) \
	sys_track_k_timer_init(timer)
#define sys_port_track_k_queue_peek_tail(queue, ret)
#define sys_port_track_k_queue_peek_head(queue, ret)
#define sys_port_track_k_queue_cancel_wait(queue)
#define sys_port_track_k_queue_init(queue) \
	sys_track_k_queue_init(queue)
#define sys_port_track_k_pipe_init(pipe) \
	sys_track_k_pipe_init(pipe)
#define sys_port_track_k_condvar_init(condvar, ret)
#define sys_port_track_k_stack_init(stack) \
	sys_track_k_stack_init(stack)
#define sys_port_track_k_thread_name_set(thread, ret)
#define sys_port_track_k_sem_reset(sem)
#define sys_port_track_k_sem_init(sem, ret) \
	sys_track_k_sem_init(sem)
#define sys_port_track_k_msgq_purge(msgq)
#define sys_port_track_k_msgq_peek(msgq, ret)
#define sys_port_track_k_msgq_init(msgq) \
	sys_track_k_msgq_init(msgq)
#define sys_port_track_k_mbox_init(mbox) \
	sys_track_k_mbox_init(mbox)
#define sys_port_track_k_mem_slab_init(slab, rc) \
	sys_track_k_mem_slab_init(slab)
#define sys_port_track_k_heap_free(h)
#define sys_port_track_k_heap_init(h)

void sys_track_k_timer_init(struct k_timer *timer);
void sys_track_k_mem_slab_init(struct k_mem_slab *slab);
void sys_track_k_sem_init(struct k_sem *sem);
void sys_track_k_mutex_init(struct k_mutex *mutex);
void sys_track_k_stack_init(struct k_stack *stack);
void sys_track_k_msgq_init(struct k_msgq *msgq);
void sys_track_k_mbox_init(struct k_mbox *mbox);
void sys_track_k_pipe_init(struct k_pipe *pipe);
void sys_track_k_queue_init(struct k_queue *queue);

/** @endcond */

/** @} */ /* end of subsys_tracing_object_tracking */

#else

#define sys_port_track_k_thread_start(thread)
#define sys_port_track_k_thread_create(new_thread)
#define sys_port_track_k_thread_sched_ready(thread)
#define sys_port_track_k_thread_wakeup(thread)
#define sys_port_track_k_thread_sched_priority_set(thread, prio)
#define sys_port_track_k_work_delayable_init(dwork)
#define sys_port_track_k_work_queue_init(queue)
#define sys_port_track_k_work_init(work)
#define sys_port_track_k_mutex_init(mutex, ret)
#define sys_port_track_k_timer_stop(timer)
#define sys_port_track_k_timer_start(timer, duration, period)
#define sys_port_track_k_timer_init(timer)
#define sys_port_track_k_queue_peek_tail(queue, ret)
#define sys_port_track_k_queue_peek_head(queue, ret)
#define sys_port_track_k_queue_cancel_wait(queue)
#define sys_port_track_k_queue_init(queue)
#define sys_port_track_k_pipe_init(pipe)
#define sys_port_track_k_condvar_init(condvar, ret)
#define sys_port_track_k_stack_init(stack)
#define sys_port_track_k_thread_name_set(thread, ret)
#define sys_port_track_k_sem_reset(sem)
#define sys_port_track_k_sem_init(sem, ret)
#define sys_port_track_k_msgq_purge(msgq)
#define sys_port_track_k_msgq_peek(msgq, ret)
#define sys_port_track_k_msgq_init(msgq)
#define sys_port_track_k_mbox_init(mbox)
#define sys_port_track_k_mem_slab_init(slab, rc)
#define sys_port_track_k_heap_free(h)
#define sys_port_track_k_heap_init(h)

#endif

#endif /* ZEPHYR_INCLUDE_TRACING_TRACKING_H_ */
