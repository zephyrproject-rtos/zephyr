/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/slist.h>
#include <zephyr/tracing/tracking.h>

struct k_timer *_track_list_k_timer;
struct k_spinlock _track_list_k_timer_lock;

struct k_mem_slab *_track_list_k_mem_slab;
struct k_spinlock _track_list_k_mem_slab_lock;

struct k_sem *_track_list_k_sem;
struct k_spinlock _track_list_k_sem_lock;

struct k_mutex *_track_list_k_mutex;
struct k_spinlock _track_list_k_mutex_lock;

struct k_stack *_track_list_k_stack;
struct k_spinlock _track_list_k_stack_lock;

struct k_msgq *_track_list_k_msgq;
struct k_spinlock _track_list_k_msgq_lock;

struct k_mbox *_track_list_k_mbox;
struct k_spinlock _track_list_k_mbox_lock;

#ifdef CONFIG_PIPES
struct k_pipe *_track_list_k_pipe;
struct k_spinlock _track_list_k_pipe_lock;
#endif

struct k_queue *_track_list_k_queue;
struct k_spinlock _track_list_k_queue_lock;

#ifdef CONFIG_EVENTS
struct k_event *_track_list_k_event;
struct k_spinlock _track_list_k_event_lock;
#endif

#define SYS_TRACK_LIST_PREPEND(list, obj) \
	do { \
		k_spinlock_key_t key = k_spin_lock(&list ## _lock); \
		obj->_obj_track_next = list; \
		list = obj; \
		k_spin_unlock(&list ## _lock, key); \
	} while (false)

#define SYS_TRACK_STATIC_INIT(type, ...) \
	do { \
		STRUCT_SECTION_FOREACH(type, obj) \
			_SYS_PORT_TRACKING_OBJ_INIT(type)(obj, ##__VA_ARGS__); \
	} while (false)


void sys_track_k_timer_init(struct k_timer *timer)
{
	SYS_PORT_TRACING_TYPE_MASK(k_timer,
			SYS_TRACK_LIST_PREPEND(_track_list_k_timer, timer));
}

void sys_track_k_mem_slab_init(struct k_mem_slab *slab)
{
	SYS_PORT_TRACING_TYPE_MASK(k_mem_slab,
			SYS_TRACK_LIST_PREPEND(_track_list_k_mem_slab, slab));
}

void sys_track_k_sem_init(struct k_sem *sem)
{
	if (sem) {
		SYS_PORT_TRACING_TYPE_MASK(k_sem,
				SYS_TRACK_LIST_PREPEND(_track_list_k_sem, sem));
	}
}

void sys_track_k_mutex_init(struct k_mutex *mutex)
{
	SYS_PORT_TRACING_TYPE_MASK(k_mutex,
			SYS_TRACK_LIST_PREPEND(_track_list_k_mutex, mutex));
}

void sys_track_k_stack_init(struct k_stack *stack)
{
	SYS_PORT_TRACING_TYPE_MASK(k_stack,
			SYS_TRACK_LIST_PREPEND(_track_list_k_stack, stack));
}

void sys_track_k_msgq_init(struct k_msgq *msgq)
{
	SYS_PORT_TRACING_TYPE_MASK(k_msgq,
			SYS_TRACK_LIST_PREPEND(_track_list_k_msgq, msgq));
}

void sys_track_k_mbox_init(struct k_mbox *mbox)
{
	SYS_PORT_TRACING_TYPE_MASK(k_mbox,
			SYS_TRACK_LIST_PREPEND(_track_list_k_mbox, mbox));
}

#ifdef CONFIG_PIPES
void sys_track_k_pipe_init(struct k_pipe *pipe)
{
	SYS_PORT_TRACING_TYPE_MASK(k_pipe,
			SYS_TRACK_LIST_PREPEND(_track_list_k_pipe, pipe));
}
#endif

void sys_track_k_queue_init(struct k_queue *queue)
{
	SYS_PORT_TRACING_TYPE_MASK(k_queue,
			SYS_TRACK_LIST_PREPEND(_track_list_k_queue, queue));
}

#ifdef CONFIG_EVENTS
void sys_track_k_event_init(struct k_event *event)
{
	SYS_PORT_TRACING_TYPE_MASK(k_event,
			SYS_TRACK_LIST_PREPEND(_track_list_k_event, event));
}
#endif

static int sys_track_static_init(void)
{

	SYS_PORT_TRACING_TYPE_MASK(k_timer,
			SYS_TRACK_STATIC_INIT(k_timer));

	SYS_PORT_TRACING_TYPE_MASK(k_mem_slab,
			SYS_TRACK_STATIC_INIT(k_mem_slab, 0));

	SYS_PORT_TRACING_TYPE_MASK(k_sem,
			SYS_TRACK_STATIC_INIT(k_sem, 0));

	SYS_PORT_TRACING_TYPE_MASK(k_mutex,
			SYS_TRACK_STATIC_INIT(k_mutex, 0));

	SYS_PORT_TRACING_TYPE_MASK(k_stack,
			SYS_TRACK_STATIC_INIT(k_stack));

	SYS_PORT_TRACING_TYPE_MASK(k_msgq,
			SYS_TRACK_STATIC_INIT(k_msgq));

	SYS_PORT_TRACING_TYPE_MASK(k_mbox,
			SYS_TRACK_STATIC_INIT(k_mbox));

#ifdef CONFIG_PIPES
	SYS_PORT_TRACING_TYPE_MASK(k_pipe,
			SYS_TRACK_STATIC_INIT(k_pipe));
#endif

	SYS_PORT_TRACING_TYPE_MASK(k_queue,
			SYS_TRACK_STATIC_INIT(k_queue));

#ifdef CONFIG_EVENTS
	SYS_PORT_TRACING_TYPE_MASK(k_event,
			SYS_TRACK_STATIC_INIT(k_event));
#endif

	return 0;
}

SYS_INIT(sys_track_static_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);
