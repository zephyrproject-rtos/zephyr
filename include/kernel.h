/*
 * Copyright (c) 2016, Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file
 *
 * @brief Public kernel APIs.
 */

#ifndef _kernel__h_
#define _kernel__h_

#include <stddef.h>
#include <stdint.h>
#include <toolchain.h>
#include <sections.h>
#include <atomic.h>
#include <errno.h>
#include <misc/__assert.h>
#include <misc/dlist.h>
#include <misc/slist.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_KERNEL_V2_DEBUG
#define K_DEBUG(fmt, ...) printk("[%s]  " fmt, __func__, ##__VA_ARGS__)
#else
#define K_DEBUG(fmt, ...)
#endif

#define K_PRIO_COOP(x) (-(CONFIG_NUM_COOP_PRIORITIES - (x)))
#define K_PRIO_PREEMPT(x) (x)

#define K_FOREVER (-1)
#define K_NO_WAIT 0

#define K_ANY NULL
#define K_END NULL

#if CONFIG_NUM_COOP_PRIORITIES > 0
#define K_HIGHEST_THREAD_PRIO (-CONFIG_NUM_COOP_PRIORITIES)
#else
#define K_HIGHEST_THREAD_PRIO 0
#endif

#if CONFIG_NUM_PREEMPT_PRIORITIES > 0
#define K_LOWEST_THREAD_PRIO CONFIG_NUM_PREEMPT_PRIORITIES
#else
#define K_LOWEST_THREAD_PRIO -1
#endif

#define K_HIGHEST_APPLICATION_THREAD_PRIO (K_HIGHEST_THREAD_PRIO)
#define K_LOWEST_APPLICATION_THREAD_PRIO (K_LOWEST_THREAD_PRIO - 1)

typedef sys_dlist_t _wait_q_t;

#ifdef CONFIG_DEBUG_TRACING_KERNEL_OBJECTS
#define _DEBUG_TRACING_KERNEL_OBJECTS_NEXT_PTR(type) struct type *__next
#define _DEBUG_TRACING_KERNEL_OBJECTS_INIT .__next = NULL,
#else
#define _DEBUG_TRACING_KERNEL_OBJECTS_INIT
#define _DEBUG_TRACING_KERNEL_OBJECTS_NEXT_PTR(type)
#endif

#define k_thread tcs
struct tcs;
struct k_mutex;
struct k_sem;
struct k_event;
struct k_msgq;
struct k_mbox;
struct k_pipe;
struct k_fifo;
struct k_lifo;
struct k_stack;
struct k_mem_map;
struct k_mem_pool;
struct k_timer;

typedef struct k_thread *k_tid_t;

/* threads/scheduler/execution contexts */

enum execution_context_types {
	K_ISR = 0,
	K_COOP_THREAD,
	K_PREEMPT_THREAD,
};

struct k_thread_config {
	char *stack;
	unsigned stack_size;
	unsigned prio;
};

typedef void (*k_thread_entry_t)(void *p1, void *p2, void *p3);
extern k_tid_t k_thread_spawn(char *stack, unsigned stack_size,
			      void (*entry)(void *, void *, void*),
			      void *p1, void *p2, void *p3,
			      int32_t prio, uint32_t options, int32_t delay);

extern void k_sleep(int32_t duration);
extern void k_busy_wait(uint32_t usec_to_wait);
extern void k_yield(void);
extern void k_wakeup(k_tid_t thread);
extern k_tid_t k_current_get(void);
extern int k_thread_cancel(k_tid_t thread);

extern void k_thread_abort(k_tid_t thread);

#define K_THREAD_GROUP_EXE 0x1
#define K_THREAD_GROUP_SYS 0x2
#define K_THREAD_GROUP_FPU 0x4

/* XXX - doesn't work because CONFIG_ARCH is a string */
#if 0
/* arch-specific groups */
#if CONFIG_ARCH == "x86"
#define K_THREAD_GROUP_SSE 0x4
#endif
#endif

#ifdef CONFIG_SYS_CLOCK_EXISTS
#define _THREAD_TIMEOUT_INIT(obj) \
	(obj).nano_timeout = { \
	.node = { {0}, {0} }, \
	.thread = NULL, \
	.wait_q = NULL, \
	.delta_ticks_from_prev = -1, \
	},
#else
#define _THREAD_TIMEOUT_INIT(obj)
#endif

#ifdef CONFIG_ERRNO
#define _THREAD_ERRNO_INIT(obj) (obj).errno_var = 0,
#else
#define _THREAD_ERRNO_INIT(obj)
#endif

struct _static_thread_data {
	uint32_t init_groups;
	int init_prio;
	void (*init_entry)(void *, void *, void *);
	void *init_p1;
	void *init_p2;
	void *init_p3;
	void (*init_abort)(void);
	union {
		char *init_stack;
		struct k_thread *thread;
	};
	unsigned int init_stack_size;
};

#define K_THREAD_INITIALIZER(stack, stack_size, \
			     entry, p1, p2, p3, \
			     abort, prio, groups) \
	{ \
	.init_groups = (groups), \
	.init_prio = (prio), \
	.init_entry = (void (*)(void *, void *, void *))entry, \
	.init_p1 = (void *)p1, \
	.init_p2 = (void *)p2, \
	.init_p3 = (void *)p3, \
	.init_abort = abort, \
	.init_stack = (stack), \
	.init_stack_size = (stack_size), \
	}

/*
 * Define thread initializer object and initialize it
 * NOTE: For thread group functions thread initializers must be organized
 * in array and thus should not have gaps between them.
 * On x86 by default compiler aligns them by 32 byte boundary. To prevent
 * this 32-bit alignment in specified here.
 * _static_thread_data structure sise needs to be kept 32-bit aligned as well
 */
#define K_THREAD_DEFINE(name, stack_size, \
			    entry, p1, p2, p3, \
			    abort, prio, groups) \
	char __noinit __stack _k_thread_obj_##name[stack_size]; \
	struct _static_thread_data _k_thread_data_##name __aligned(4) \
		__in_section(_k_task_list, private, task) = \
		K_THREAD_INITIALIZER(_k_thread_obj_##name, stack_size, \
				     entry, p1, p2, p3, abort, prio, groups)

extern int  k_thread_priority_get(k_tid_t thread);
extern void k_thread_priority_set(k_tid_t thread, int prio);

extern void k_thread_suspend(k_tid_t thread);
extern void k_thread_resume(k_tid_t thread);
extern void k_thread_abort_handler_set(void (*handler)(void));

extern void k_sched_time_slice_set(int32_t slice, int prio);

extern int k_am_in_isr(void);

extern void k_thread_custom_data_set(void *value);
extern void *k_thread_custom_data_get(void);

/**
 *  kernel timing
 */

#include <sys_clock.h>

/* private internal time manipulation (users should never play with ticks) */

static int64_t __ticks_to_ms(int64_t ticks)
{
#if CONFIG_SYS_CLOCK_EXISTS
	return (MSEC_PER_SEC * (uint64_t)ticks) / sys_clock_ticks_per_sec;
#else
	__ASSERT(ticks == 0, "");
	return 0;
#endif
}


/* timeouts */

struct _timeout;
typedef void (*_timeout_func_t)(struct _timeout *t);

struct _timeout {
	sys_dlist_t node;
	struct k_thread *thread;
	sys_dlist_t *wait_q;
	int32_t delta_ticks_from_prev;
	_timeout_func_t func;
};

/* timers */

struct k_timer {
	/*
	 * _timeout structure must be first here if we want to use
	 * dynamic timer allocation. timeout.node is used in the double-linked
	 * list of free timers
	 */
	struct _timeout timeout;

	/* wait queue for the threads waiting on this timer */
	_wait_q_t wait_q;

	/* runs in ISR context */
	void (*handler)(void *);
	void *handler_arg;

	/* runs in the context of the thread that calls k_timer_stop() */
	void (*stop_handler)(void *);
	void *stop_handler_arg;

	/* timer period */
	int32_t period;

	/* user supplied data pointer returned to the thread*/
	void *user_data;

	/* user supplied data pointer */
	void *user_data_internal;

	_DEBUG_TRACING_KERNEL_OBJECTS_NEXT_PTR(k_timer);
};

#define K_TIMER_INITIALIZER(obj) \
	{ \
	.wait_q = SYS_DLIST_STATIC_INIT(&obj.wait_q), \
	_DEBUG_TRACING_KERNEL_OBJECTS_INIT \
	}

#define K_TIMER_DEFINE(name) \
	struct k_timer name = K_TIMER_INITIALIZER(name)

extern void k_timer_init(struct k_timer *timer, void *data);

extern void k_timer_start(struct k_timer *timer,
			  int32_t duration, int32_t period,
			  void (*handler)(void *), void *handler_arg,
			  void (*stop_handler)(void *), void *stop_handler_arg);
extern void k_timer_restart(struct k_timer *timer, int32_t duration,
			    int32_t period);
extern void k_timer_stop(struct k_timer *timer);
extern int k_timer_test(struct k_timer *timer, void **data, int wait);
extern int32_t k_timer_remaining_get(struct k_timer *timer);


/**
 * @brief Get the time elapsed since the system booted (uptime)
 *
 * @return The current uptime of the system in ms
 */

extern int64_t k_uptime_get(void);

/**
 * @brief Get the lower 32-bit of time elapsed since the system booted (uptime)
 *
 * This function is potentially less onerous in both the time it takes to
 * execute, the interrupt latency it introduces and the amount of 64-bit math
 * it requires than k_uptime_get(), but it only provides an uptime value of
 * 32-bits. The user must handle possible rollovers/spillovers.
 *
 * At a rate of increment of 1000 per second, it rolls over approximately every
 * 50 days.
 *
 * @return The current uptime of the system in ms
 */

extern uint32_t k_uptime_get_32(void);

/**
 * @brief Get the difference between a reference time and the current uptime
 *
 * @param reftime A pointer to a reference time. It is updated with the current
 * uptime upon return.
 *
 * @return The delta between the reference time and the current uptime.
 */

extern int64_t k_uptime_delta(int64_t *reftime);

/**
 * @brief Get the difference between a reference time and the current uptime
 *
 * The 32-bit version of k_uptime_delta(). It has the same perks and issues as
 * k_uptime_get_32().
 *
 * @param reftime A pointer to a reference time. It is updated with the current
 * uptime upon return.
 *
 * @return The delta between the reference time and the current uptime.
 */

extern uint32_t k_uptime_delta_32(int64_t *reftime);

extern uint32_t k_cycle_get_32(void);

/**
 *  data transfers (basic)
 */

/* fifos */

struct k_fifo {
	_wait_q_t wait_q;
	sys_slist_t data_q;

	_DEBUG_TRACING_KERNEL_OBJECTS_NEXT_PTR(k_fifo);
};

extern void k_fifo_init(struct k_fifo *fifo);
extern void k_fifo_put(struct k_fifo *fifo, void *data);
extern void k_fifo_put_list(struct k_fifo *fifo, void *head, void *tail);
extern void k_fifo_put_slist(struct k_fifo *fifo, sys_slist_t *list);
extern void *k_fifo_get(struct k_fifo *fifo, int32_t timeout);

#define K_FIFO_INITIALIZER(obj) \
	{ \
	.wait_q = SYS_DLIST_STATIC_INIT(&obj.wait_q), \
	.data_q = SYS_SLIST_STATIC_INIT(&obj.data_q), \
	_DEBUG_TRACING_KERNEL_OBJECTS_INIT \
	}

#define K_FIFO_DEFINE(name) \
	struct k_fifo name = K_FIFO_INITIALIZER(name)

/* lifos */

struct k_lifo {
	_wait_q_t wait_q;
	void *list;

	_DEBUG_TRACING_KERNEL_OBJECTS_NEXT_PTR(k_lifo);
};

extern void k_lifo_init(struct k_lifo *lifo);
extern void k_lifo_put(struct k_lifo *lifo, void *data);
extern void *k_lifo_get(struct k_lifo *lifo, int32_t timeout);

#define K_LIFO_INITIALIZER(obj) \
	{ \
	.wait_q = SYS_DLIST_STATIC_INIT(&obj.wait_q), \
	.list = NULL, \
	_DEBUG_TRACING_KERNEL_OBJECTS_INIT \
	}

#define K_LIFO_DEFINE(name) \
	struct k_lifo name = K_LIFO_INITIALIZER(name)

/* stacks */

struct k_stack {
	_wait_q_t wait_q;
	uint32_t *base, *next, *top;

	_DEBUG_TRACING_KERNEL_OBJECTS_NEXT_PTR(k_stack);
};

extern void k_stack_init(struct k_stack *stack,
			 uint32_t *buffer, int num_entries);
extern void k_stack_push(struct k_stack *stack, uint32_t data);
extern int k_stack_pop(struct k_stack *stack, uint32_t *data, int32_t timeout);

#define K_STACK_INITIALIZER(obj, stack_num_entries, stack_buffer) \
	{ \
	.wait_q = SYS_DLIST_STATIC_INIT(&obj.wait_q), \
	.base = stack_buffer, \
	.next = stack_buffer, \
	.top = stack_buffer + stack_num_entries, \
	_DEBUG_TRACING_KERNEL_OBJECTS_INIT \
	}

#define K_STACK_DEFINE(name, stack_num_entries) \
	uint32_t __noinit _k_stack_buf_##name[stack_num_entries]; \
	struct k_stack name = \
		K_STACK_INITIALIZER(name, stack_num_entries, \
				    _k_stack_buf_##name); \

#define K_STACK_SIZE(stack_num_entries) \
	(sizeof(struct k_stack) + (stack_num_entries * sizeof(uint32_t)))

/**
 *  workqueues
 */

struct k_work;

typedef void (*k_work_handler_t)(struct k_work *);

/**
 * A workqueue is a fiber that executes @ref k_work items that are
 * queued to it.  This is useful for drivers which need to schedule
 * execution of code which might sleep from ISR context.  The actual
 * fiber identifier is not stored in the structure in order to save
 * space.
 */
struct k_work_q {
	struct k_fifo fifo;
};

/**
 * @brief Work flags.
 */
enum {
	K_WORK_STATE_PENDING,	/* Work item pending state */
};

/**
 * @brief An item which can be scheduled on a @ref k_work_q.
 */
struct k_work {
	void *_reserved;		/* Used by k_fifo implementation. */
	k_work_handler_t handler;
	atomic_t flags[1];
};

/**
 * @brief Statically initialize work item
 */
#define K_WORK_INITIALIZER(work_handler) \
	{ \
	._reserved = NULL, \
	.handler = work_handler, \
	.flags = { 0 } \
	}

/**
 * @brief Dynamically initialize work item
 */
static inline void k_work_init(struct k_work *work, k_work_handler_t handler)
{
	atomic_clear_bit(work->flags, K_WORK_STATE_PENDING);
	work->handler = handler;
}

/**
 * @brief Submit a work item to a workqueue.
 *
 * This procedure schedules a work item to be processed.
 * In the case where the work item has already been submitted and is pending
 * execution, calling this function will result in a no-op. In this case, the
 * work item must not be modified externally (e.g. by the caller of this
 * function), since that could cause the work item to be processed in a
 * corrupted state.
 *
 * @param work_q to schedule the work item
 * @param work work item
 *
 * @return N/A
 */
static inline void k_work_submit_to_queue(struct k_work_q *work_q,
					  struct k_work *work)
{
	if (!atomic_test_and_set_bit(work->flags, K_WORK_STATE_PENDING)) {
		k_fifo_put(&work_q->fifo, work);
	}
}

/**
 * @brief Check if work item is pending.
 */
static inline int k_work_pending(struct k_work *work)
{
	return atomic_test_bit(work->flags, K_WORK_STATE_PENDING);
}

/**
 * @brief Start a new workqueue.  This routine can be called from either
 * fiber or task context.
 */
extern void k_work_q_start(struct k_work_q *work_q,
				 const struct k_thread_config *config);

#if defined(CONFIG_SYS_CLOCK_EXISTS)

 /*
 * @brief An item which can be scheduled on a @ref k_work_q with a
 * delay.
 */
struct k_delayed_work {
	struct k_work work;
	struct _timeout timeout;
	struct k_work_q *work_q;
};

/**
 * @brief Initialize delayed work
 */
extern void k_delayed_work_init(struct k_delayed_work *work,
				k_work_handler_t handler);

/**
 * @brief Submit a delayed work item to a workqueue.
 *
 * This procedure schedules a work item to be processed after a delay.
 * Once the delay has passed, the work item is submitted to the work queue:
 * at this point, it is no longer possible to cancel it. Once the work item's
 * handler is about to be executed, the work is considered complete and can be
 * resubmitted.
 *
 * Care must be taken if the handler blocks or yield as there is no implicit
 * mutual exclusion mechanism. Such usage is not recommended and if necessary,
 * it should be explicitly done between the submitter and the handler.
 *
 * @param work_q to schedule the work item
 * @param work Delayed work item
 * @param ticks Ticks to wait before scheduling the work item
 *
 * @return 0 in case of success or negative value in case of error.
 */
extern int k_delayed_work_submit_to_queue(struct k_work_q *work_q,
					  struct k_delayed_work *work,
					  int32_t ticks);

/**
 * @brief Cancel a delayed work item
 *
 * This procedure cancels a scheduled work item. If the work has been completed
 * or is idle, this will do nothing. The only case where this can fail is when
 * the work has been submitted to the work queue, but the handler has not run
 * yet.
 *
 * @param work Delayed work item to be canceled
 *
 * @return 0 in case of success or negative value in case of error.
 */
extern int k_delayed_work_cancel(struct k_delayed_work *work);

#endif /* CONFIG_SYS_CLOCK_EXISTS */

#if defined(CONFIG_SYSTEM_WORKQUEUE)

extern struct k_work_q k_sys_work_q;

/*
 * @brief Submit a work item to the system workqueue.
 *
 * @ref k_work_submit_to_queue
 *
 * When using the system workqueue it is not recommended to block or yield
 * on the handler since its fiber is shared system wide it may cause
 * unexpected behavior.
 */
static inline void k_work_submit(struct k_work *work)
{
	k_work_submit_to_queue(&k_sys_work_q, work);
}

#if defined(CONFIG_SYS_CLOCK_EXISTS)
/*
 * @brief Submit a delayed work item to the system workqueue.
 *
 * @ref k_delayed_work_submit_to_queue
 *
 * When using the system workqueue it is not recommended to block or yield
 * on the handler since its fiber is shared system wide it may cause
 * unexpected behavior.
 */
static inline int k_delayed_work_submit(struct k_delayed_work *work,
					   int ticks)
{
	return k_delayed_work_submit_to_queue(&k_sys_work_q, work, ticks);
}

#endif /* CONFIG_SYS_CLOCK_EXISTS */
#endif /* CONFIG_SYSTEM_WORKQUEUE */

/**
 * synchronization
 */

/* mutexes */

struct k_mutex {
	_wait_q_t wait_q;
	struct k_thread *owner;
	uint32_t lock_count;
	int owner_orig_prio;
#ifdef CONFIG_OBJECT_MONITOR
	int num_lock_state_changes;
	int num_conflicts;
#endif

	_DEBUG_TRACING_KERNEL_OBJECTS_NEXT_PTR(k_mutex);
};

#ifdef CONFIG_OBJECT_MONITOR
#define _MUTEX_INIT_OBJECT_MONITOR \
	.num_lock_state_changes = 0, .num_conflicts = 0,
#else
#define _MUTEX_INIT_OBJECT_MONITOR
#endif

#define K_MUTEX_INITIALIZER(obj) \
	{ \
	.wait_q = SYS_DLIST_STATIC_INIT(&obj.wait_q), \
	.owner = NULL, \
	.lock_count = 0, \
	.owner_orig_prio = K_LOWEST_THREAD_PRIO, \
	_MUTEX_INIT_OBJECT_MONITOR \
	_DEBUG_TRACING_KERNEL_OBJECTS_INIT \
	}

#define K_MUTEX_DEFINE(name) \
	struct k_mutex name = K_MUTEX_INITIALIZER(name)

extern void k_mutex_init(struct k_mutex *mutex);
extern int k_mutex_lock(struct k_mutex *mutex, int32_t timeout);
extern void k_mutex_unlock(struct k_mutex *mutex);

/* semaphores */

struct k_sem {
	_wait_q_t wait_q;
	unsigned int count;
	unsigned int limit;

	_DEBUG_TRACING_KERNEL_OBJECTS_NEXT_PTR(k_sem);
};

extern void k_sem_init(struct k_sem *sem, unsigned int initial_count,
			unsigned int limit);
extern int k_sem_take(struct k_sem *sem, int32_t timeout);
extern void k_sem_give(struct k_sem *sem);

static inline void k_sem_reset(struct k_sem *sem)
{
	sem->count = 0;
}

static inline unsigned int k_sem_count_get(struct k_sem *sem)
{
	return sem->count;
}

#ifdef CONFIG_SEMAPHORE_GROUPS
/**
 * @brief Take the first available semaphore
 *
 * Given a list of semaphore pointers, this routine will attempt to take one
 * of them, waiting up to a maximum of @a timeout ms to do so. The taken
 * semaphore is identified by @a sem (set to NULL on error).
 *
 * Be aware that the more semaphores specified in the group, the more stack
 * space is required by the waiting thread.
 *
 * @param sem_array Array of semaphore pointers terminated by a K_END entry
 * @param sem Identifies the semaphore that was taken
 * @param timeout Maximum number of milliseconds to wait
 *
 * @retval 0 A semaphore was successfully taken
 * @retval -EBUSY No semaphore was available (@a timeout = K_NO_WAIT)
 * @retval -EAGAIN Time out occurred while waiting for semaphore
 */

extern int k_sem_group_take(struct k_sem *sem_array[], struct k_sem **sem,
			    int32_t timeout);

/**
 * @brief Give all the semaphores in the group
 *
 * This routine will give each semaphore in the array of semaphore pointers.
 *
 * @param sem_array Array of semaphore pointers terminated by a K_END entry
 *
 * @return N/A
 */
extern void k_sem_group_give(struct k_sem *sem_array[]);

/**
 * @brief Reset the count to zero on each semaphore in the array
 *
 * This routine resets the count of each semaphore in the group to zero.
 * Note that it does NOT have any impact on any thread that might have
 * been previously pending on any of the semaphores.
 *
 * @param sem_array Array of semaphore pointers terminated by a K_END entry
 *
 * @return N/A
 */
extern void k_sem_group_reset(struct k_sem *sem_array[]);
#endif

#define K_SEM_INITIALIZER(obj, initial_count, count_limit) \
	{ \
	.wait_q = SYS_DLIST_STATIC_INIT(&obj.wait_q), \
	.count = initial_count, \
	.limit = count_limit, \
	_DEBUG_TRACING_KERNEL_OBJECTS_INIT \
	}

#define K_SEM_DEFINE(name, initial_count, count_limit) \
	struct k_sem name = \
		K_SEM_INITIALIZER(name, initial_count, count_limit)

/* events */

#define K_EVT_DEFAULT NULL
#define K_EVT_IGNORE ((void *)(-1))

typedef int (*k_event_handler_t)(struct k_event *);

struct k_event {
	k_event_handler_t handler;
	atomic_t send_count;
	struct k_work work_item;
	struct k_sem sem;

	_DEBUG_TRACING_KERNEL_OBJECTS_NEXT_PTR(k_event);
};

extern void _k_event_deliver(struct k_work *work);

#define K_EVENT_INITIALIZER(obj, event_handler) \
	{ \
	.handler = (k_event_handler_t)event_handler, \
	.send_count = ATOMIC_INIT(0), \
	.work_item = K_WORK_INITIALIZER(_k_event_deliver), \
	.sem = K_SEM_INITIALIZER(obj.sem, 0, 1), \
	_DEBUG_TRACING_KERNEL_OBJECTS_INIT \
	}

#define K_EVENT_DEFINE(name, event_handler) \
	struct k_event name \
		__in_section(_k_event_list, event, name) = \
		K_EVENT_INITIALIZER(name, event_handler)

extern void k_event_init(struct k_event *event, k_event_handler_t handler);
extern int k_event_recv(struct k_event *event, int32_t timeout);
extern void k_event_send(struct k_event *event);

/**
 *  data transfers (complex)
 */

/* message queues */

struct k_msgq {
	_wait_q_t wait_q;
	uint32_t msg_size;
	uint32_t max_msgs;
	char *buffer_start;
	char *buffer_end;
	char *read_ptr;
	char *write_ptr;
	uint32_t used_msgs;

	_DEBUG_TRACING_KERNEL_OBJECTS_NEXT_PTR(k_msgq);
};

#define K_MSGQ_INITIALIZER(obj, q_depth, q_width, q_buffer) \
	{ \
	.wait_q = SYS_DLIST_STATIC_INIT(&obj.wait_q), \
	.max_msgs = q_depth, \
	.msg_size = q_width, \
	.buffer_start = q_buffer, \
	.buffer_end = q_buffer + (q_depth * q_width), \
	.read_ptr = q_buffer, \
	.write_ptr = q_buffer, \
	.used_msgs = 0, \
	_DEBUG_TRACING_KERNEL_OBJECTS_INIT \
	}

#define K_MSGQ_DEFINE(name, q_depth, q_width) \
	static char __noinit _k_fifo_buf_##name[(q_depth) * (q_width)]; \
	struct k_msgq name = \
	       K_MSGQ_INITIALIZER(name, q_depth, q_width, _k_fifo_buf_##name)

#define K_MSGQ_SIZE(q_depth, q_width) \
	((sizeof(struct k_msgq)) + ((q_width) * (q_depth)))

extern void k_msgq_init(struct k_msgq *q, uint32_t msg_size,
			uint32_t max_msgs, char *buffer);
extern int k_msgq_put(struct k_msgq *q, void *data, int32_t timeout);
extern int k_msgq_get(struct k_msgq *q, void *data, int32_t timeout);
extern void k_msgq_purge(struct k_msgq *q);

static inline int k_msgq_num_used_get(struct k_msgq *q)
{
	return q->used_msgs;
}

struct k_mem_block {
	struct k_mem_pool *pool_id;
	void *addr_in_pool;
	void *data;
	uint32_t req_size;
};

/* mailboxes */

struct k_mbox_msg {
	/** internal use only - needed for legacy API support */
	uint32_t _mailbox;
	/** size of message (in bytes) */
	uint32_t size;
	/** application-defined information value */
	uint32_t info;
	/** sender's message data buffer */
	void *tx_data;
	/** internal use only - needed for legacy API support */
	void *_rx_data;
	/** message data block descriptor */
	struct k_mem_block tx_block;
	/** source thread id */
	k_tid_t rx_source_thread;
	/** target thread id */
	k_tid_t tx_target_thread;
	/** internal use only - thread waiting on send (may be a dummy) */
	k_tid_t _syncing_thread;
#if (CONFIG_NUM_MBOX_ASYNC_MSGS > 0)
	/** internal use only - semaphore used during asynchronous send */
	struct k_sem *_async_sem;
#endif
};

struct k_mbox {
	_wait_q_t tx_msg_queue;
	_wait_q_t rx_msg_queue;

	_DEBUG_TRACING_KERNEL_OBJECTS_NEXT_PTR(k_mbox);
};

#define K_MBOX_INITIALIZER(obj) \
	{ \
	.tx_msg_queue = SYS_DLIST_STATIC_INIT(&obj.tx_msg_queue), \
	.rx_msg_queue = SYS_DLIST_STATIC_INIT(&obj.rx_msg_queue), \
	_DEBUG_TRACING_KERNEL_OBJECTS_INIT \
	}

#define K_MBOX_DEFINE(name) \
	struct k_mbox name = \
		K_MBOX_INITIALIZER(name) \

extern void k_mbox_init(struct k_mbox *mbox);

extern int k_mbox_put(struct k_mbox *mbox, struct k_mbox_msg *msg,
		      int32_t timeout);
extern void k_mbox_async_put(struct k_mbox *mbox, struct k_mbox_msg *msg,
			     struct k_sem *sem);

extern int k_mbox_get(struct k_mbox *mbox, struct k_mbox_msg *msg,
		      void *buffer, int32_t timeout);
extern void k_mbox_data_get(struct k_mbox_msg *msg, void *buffer);
extern int k_mbox_data_block_get(struct k_mbox_msg *msg,
				 struct k_mem_pool *pool,
				 struct k_mem_block *block, int32_t timeout);

/* pipes */

struct k_pipe {
	unsigned char *buffer;          /* Pipe buffer: may be NULL */
	size_t         size;            /* Buffer size */
	size_t         bytes_used;      /* # bytes used in buffer */
	size_t         read_index;      /* Where in buffer to read from */
	size_t         write_index;     /* Where in buffer to write */

	struct {
		_wait_q_t      readers; /* Reader wait queue */
		_wait_q_t      writers; /* Writer wait queue */
	} wait_q;

	_DEBUG_TRACING_KERNEL_OBJECTS_NEXT_PTR(k_pipe);
};

#define K_PIPE_INITIALIZER(obj, pipe_buffer, pipe_buffer_size)        \
	{                                                             \
	.buffer = pipe_buffer,                                        \
	.size = pipe_buffer_size,                                     \
	.bytes_used = 0,                                              \
	.read_index = 0,                                              \
	.write_index = 0,                                             \
	.wait_q.writers = SYS_DLIST_STATIC_INIT(&obj.wait_q.writers), \
	.wait_q.readers = SYS_DLIST_STATIC_INIT(&obj.wait_q.readers), \
	_DEBUG_TRACING_KERNEL_OBJECTS_INIT                            \
	}

#define K_PIPE_DEFINE(name, pipe_buffer_size, pipe_align)     \
	static unsigned char __noinit __aligned(pipe_align)   \
		_k_pipe_buf_##name[pipe_buffer_size];         \
	struct k_pipe name =                                  \
		K_PIPE_INITIALIZER(name, _k_pipe_buf_##name, pipe_buffer_size)

/**
 * @brief Runtime initialization of a pipe
 *
 * @param pipe Pointer to pipe to initialize
 * @param buffer Pointer to buffer to use for pipe's ring buffer
 * @param size Size of the pipe's ring buffer
 *
 * @return N/A
 */
extern void k_pipe_init(struct k_pipe *pipe, unsigned char *buffer,
			size_t size);

/**
 * @brief Put a message into the specified pipe
 *
 * This routine synchronously adds a message into the pipe specified by
 * @a pipe. It will wait up to @a timeout for the pipe to accept
 * @a bytes_to_write bytes of data. If by @a timeout, the pipe could not
 * accept @a min_xfer bytes of data, it fails. Fewer than @a min_xfer will
 * only ever be written to the pipe if K_NO_WAIT < @a timeout < K_FOREVER.
 *
 * @param pipe Pointer to the pipe
 * @param data Data to put into the pipe
 * @param bytes_to_write Desired number of bytes to put into the pipe
 * @param bytes_written Number of bytes the pipe accepted
 * @param min_xfer Minimum number of bytes accepted for success
 * @param timeout Maximum number of milliseconds to wait
 *
 * @retval 0 At least @a min_xfer were sent
 * @retval -EIO Request can not be satisfied (@a timeout is K_NO_WAIT)
 * @retval -EAGAIN Fewer than @a min_xfer were sent
 */
extern int k_pipe_put(struct k_pipe *pipe, void *data,
		      size_t bytes_to_write, size_t *bytes_written,
		      size_t min_xfer, int32_t timeout);

/**
 * @brief Get a message from the specified pipe
 *
 * This routine synchronously retrieves a message from the pipe specified by
 * @a pipe. It will wait up to @a timeout to retrieve @a bytes_to_read
 * bytes of data from the pipe. If by @a timeout, the pipe could not retrieve
 * @a min_xfer bytes of data, it fails. Fewer than @a min_xfer will
 * only ever be retrieved from the pipe if K_NO_WAIT < @a timeout < K_FOREVER.
 *
 * @param pipe Pointer to the pipe
 * @param data Location to place retrieved data
 * @param bytes_to_read Desired number of bytes to retrieve from the pipe
 * @param bytes_read Number of bytes retrieved from the pipe
 * @param min_xfer Minimum number of bytes retrieved for success
 * @param timeout Maximum number of milliseconds to wait
 *
 * @retval 0 At least @a min_xfer were transferred
 * @retval -EIO Request can not be satisfied (@a timeout is K_NO_WAIT)
 * @retval -EAGAIN Fewer than @a min_xfer were retrieved
 */
extern int k_pipe_get(struct k_pipe *pipe, void *data,
		      size_t bytes_to_read, size_t *bytes_read,
		      size_t min_xfer, int32_t timeout);

/**
 * @brief Send a message to the specified pipe
 *
 * This routine asynchronously sends a message from the pipe specified by
 * @a pipe. Once all @a size bytes have been accepted by the pipe, it will
 * free the memory block @a block and give the semaphore @a sem (if specified).
 * Up to CONFIG_NUM_PIPE_ASYNC_MSGS asynchronous pipe messages can be in-flight
 * at any given time.
 *
 * @param pipe Pointer to the pipe
 * @param block Memory block containing data to send
 * @param size Number of data bytes in memory block to send
 * @param sem Semaphore to signal upon completion (else NULL)
 *
 * @retval N/A
 */
extern void k_pipe_block_put(struct k_pipe *pipe, struct k_mem_block *block,
			     size_t size, struct k_sem *sem);

/**
 *  memory management
 */

/* memory maps */

struct k_mem_map {
	_wait_q_t wait_q;
	int num_blocks;
	int block_size;
	char *buffer;
	char *free_list;
	int num_used;

	_DEBUG_TRACING_KERNEL_OBJECTS_NEXT_PTR(k_mem_map);
};

#define K_MEM_MAP_INITIALIZER(obj, map_num_blocks, map_block_size, \
			      map_buffer) \
	{ \
	.wait_q = SYS_DLIST_STATIC_INIT(&obj.wait_q), \
	.num_blocks = map_num_blocks, \
	.block_size = map_block_size, \
	.buffer = map_buffer, \
	.free_list = NULL, \
	.num_used = 0, \
	_DEBUG_TRACING_KERNEL_OBJECTS_INIT \
	}

#define K_MEM_MAP_DEFINE(name, map_num_blocks, map_block_size) \
	char _k_mem_map_buf_##name[(map_num_blocks) * (map_block_size)]; \
	struct k_mem_map name \
		__in_section(_k_mem_map_ptr, private, mem_map) = \
		K_MEM_MAP_INITIALIZER(name, map_num_blocks, \
				      map_block_size, _k_mem_map_buf_##name)

#define K_MEM_MAP_SIZE(map_num_blocks, map_block_size) \
	(sizeof(struct k_mem_map) + ((map_num_blocks) * (map_block_size)))

extern void k_mem_map_init(struct k_mem_map *map, int num_blocks,
			   int block_size, void *buffer);
extern int k_mem_map_alloc(struct k_mem_map *map, void **mem, int32_t timeout);
extern void k_mem_map_free(struct k_mem_map *map, void **mem);

static inline int k_mem_map_num_used_get(struct k_mem_map *map)
{
	return map->num_used;
}

/* memory pools */

/*
 * Memory pool requires a buffer and two arrays of structures for the
 * memory block accounting:
 * A set of arrays of k_mem_pool_quad_block structures where each keeps a
 * status of four blocks of memory.
 */
struct k_mem_pool_quad_block {
	char *mem_blocks; /* pointer to the first of four memory blocks */
	uint32_t mem_status; /* four bits. If bit is set, memory block is
				allocated */
};
/*
 * Memory pool mechanism uses one array of k_mem_pool_quad_block for accounting
 * blocks of one size. Block sizes go from maximal to minimal. Next memory
 * block size is 4 times less than the previous one and thus requires 4 times
 * bigger array of k_mem_pool_quad_block structures to keep track of the
 * memory blocks.
 */

/*
 * The array of k_mem_pool_block_set keeps the information of each array of
 * k_mem_pool_quad_block structures
 */
struct k_mem_pool_block_set {
	int block_size; /* memory block size */
	int nr_of_entries; /* nr of quad block structures in the array */
	struct k_mem_pool_quad_block *quad_block;
	int count;
};

/* Memory pool descriptor */
struct k_mem_pool {
	int max_block_size;
	int min_block_size;
	int nr_of_maxblocks;
	int nr_of_block_sets;
	struct k_mem_pool_block_set *block_set;
	char *bufblock;
	_wait_q_t wait_q;
	_DEBUG_TRACING_KERNEL_OBJECTS_NEXT_PTR(k_mem_pool);
};

#ifdef CONFIG_ARM
#define _SECTION_TYPE_SIGN "%"
#else
#define _SECTION_TYPE_SIGN "@"
#endif

/*
 * Static memory pool initialization
 */
/*
 * Use .altmacro to be able to recalculate values and pass them as string
 * arguments when calling assembler macros resursively
 */
__asm__(".altmacro\n\t");

/*
 * Recursively calls a macro
 * The followig global symbols need to be initialized:
 * __memory_pool_max_block_size - maximal size of the memory block
 * __memory_pool_min_block_size - minimal size of the memory block
 * Notes:
 * Global symbols are used due the fact that assembler macro allows only
 * one argument be passed with the % conversion
 * Some assemblers do not get division operation ("/"). To avoid it >> 2
 * is used instead of / 4.
 * n_max argument needs to go first in the invoked macro, as some
 * assemblers concatenate \name and %(\n_max * 4) arguments
 * if \name goes first
 */
__asm__(".macro __do_recurse macro_name, name, n_max\n\t"
	".ifge __memory_pool_max_block_size >> 2 -"
	" __memory_pool_min_block_size\n\t\t"
	"__memory_pool_max_block_size = __memory_pool_max_block_size >> 2\n\t\t"
	"\\macro_name %(\\n_max * 4) \\name\n\t"
	".endif\n\t"
	".endm\n");

/*
 * Build quad blocks
 * Macro allocates space in memory for the array of k_mem_pool_quad_block
 * structures and recursively calls itself for the next array, 4 times
 * larger.
 * The followig global symbols need to be initialized:
 * __memory_pool_max_block_size - maximal size of the memory block
 * __memory_pool_min_block_size - minimal size of the memory block
 * __memory_pool_quad_block_size - sizeof(struct k_mem_pool_quad_block)
 */
__asm__(".macro _build_quad_blocks n_max, name\n\t"
	"_mem_pool_quad_blocks_\\name\\()_\\n_max:\n\t"
	".skip __memory_pool_quad_block_size * \\n_max >> 2\n\t"
	".if \\n_max % 4\n\t\t"
	".skip __memory_pool_quad_block_size\n\t"
	".endif\n\t"
	"__do_recurse _build_quad_blocks \\name \\n_max\n\t"
	".endm\n");

/*
 * Build block sets and initialize them
 * Macro initializes the k_mem_pool_block_set structure and
 * recursively calls itself for the next one.
 * The followig global symbols need to be initialized:
 * __memory_pool_max_block_size - maximal size of the memory block
 * __memory_pool_min_block_size - minimal size of the memory block
 * __memory_pool_block_set_count, the number of the elements in the
 * block set array must be set to 0. Macro calculates it's real
 * value.
 * Since the macro initializes pointers to an array of k_mem_pool_quad_block
 * structures, _build_quad_blocks must be called prior it.
 */
__asm__(".macro _build_block_set n_max, name\n\t"
	".int __memory_pool_max_block_size\n\t" /* block_size */
	".if \\n_max % 4\n\t\t"
	".int \\n_max >> 2 + 1\n\t" /* nr_of_entries */
	".else\n\t\t"
	".int \\n_max >> 2\n\t"
	".endif\n\t"
	".int _mem_pool_quad_blocks_\\name\\()_\\n_max\n\t" /* quad_block */
	".int 0\n\t" /* count */
	"__memory_pool_block_set_count = __memory_pool_block_set_count + 1\n\t"
	"__do_recurse _build_block_set \\name \\n_max\n\t"
	".endm\n");

/*
 * Build a memory pool structure and initialize it
 * Macro uses __memory_pool_block_set_count global symbol,
 * block set addresses and buffer address, it may be called only after
 * _build_block_set
 */
__asm__(".macro _build_mem_pool name, min_size, max_size, n_max\n\t"
	".pushsection ._k_memory_pool,\"aw\","
	_SECTION_TYPE_SIGN "progbits\n\t"
	".globl \\name\n\t"
	"\\name:\n\t"
	".int \\max_size\n\t" /* max_block_size */
	".int \\min_size\n\t" /* min_block_size */
	".int \\n_max\n\t" /* nr_of_maxblocks */
	".int __memory_pool_block_set_count\n\t" /* nr_of_block_sets */
	".int _mem_pool_block_sets_\\name\n\t" /* block_set */
	".int _mem_pool_buffer_\\name\n\t" /* bufblock */
	".int 0\n\t" /* wait_q->head */
	".int 0\n\t" /* wait_q->next */
	".popsection\n\t"
	".endm\n");

#define _MEMORY_POOL_QUAD_BLOCK_DEFINE(name, min_size, max_size, n_max) \
	__asm__(".pushsection ._k_memory_pool.struct,\"aw\","		\
		_SECTION_TYPE_SIGN "progbits\n\t");			\
	__asm__("__memory_pool_min_block_size = " STRINGIFY(min_size) "\n\t"); \
	__asm__("__memory_pool_max_block_size = " STRINGIFY(max_size) "\n\t"); \
	__asm__("_build_quad_blocks " STRINGIFY(n_max) " "		\
		STRINGIFY(name) "\n\t");				\
	__asm__(".popsection\n\t")

#define _MEMORY_POOL_BLOCK_SETS_DEFINE(name, min_size, max_size, n_max) \
	__asm__("__memory_pool_block_set_count = 0\n\t");		\
	__asm__("__memory_pool_max_block_size = " STRINGIFY(max_size) "\n\t"); \
	__asm__(".pushsection ._k_memory_pool.struct,\"aw\","		\
		_SECTION_TYPE_SIGN "progbits\n\t");			\
	__asm__("_mem_pool_block_sets_" STRINGIFY(name) ":\n\t");	\
	__asm__("_build_block_set " STRINGIFY(n_max) " "		\
		STRINGIFY(name) "\n\t");				\
	__asm__("_mem_pool_block_set_count_" STRINGIFY(name) ":\n\t");	\
	__asm__(".int __memory_pool_block_set_count\n\t");		\
	__asm__(".popsection\n\t");					\
	extern uint32_t _mem_pool_block_set_count_##name;		\
	extern struct k_mem_pool_block_set _mem_pool_block_sets_##name[]

#define _MEMORY_POOL_BUFFER_DEFINE(name, max_size, n_max)	\
	char __noinit _mem_pool_buffer_##name[(max_size) * (n_max)]

#define K_MEMORY_POOL_DEFINE(name, min_size, max_size, n_max)		\
	_MEMORY_POOL_QUAD_BLOCK_DEFINE(name, min_size, max_size, n_max); \
	_MEMORY_POOL_BLOCK_SETS_DEFINE(name, min_size, max_size, n_max); \
	_MEMORY_POOL_BUFFER_DEFINE(name, max_size, n_max);		\
	__asm__("_build_mem_pool " STRINGIFY(name) " " STRINGIFY(min_size) " " \
	       STRINGIFY(max_size) " " STRINGIFY(n_max) "\n\t");	\
	extern struct k_mem_pool name

/*
 * Dummy function that assigns the value of sizeof(struct k_mem_pool_quad_block)
 * to __memory_pool_quad_block_size absolute symbol.
 * This function does not get called, but compiler calculates the value and
 * assigns it to the absolute symbol, that, in turn is used by assembler macros.
 */
static void __attribute__ ((used)) __k_mem_pool_quad_block_size_define(void)
{
	__asm__(".globl __memory_pool_quad_block_size\n\t"
	    "__memory_pool_quad_block_size = %c0\n\t"
	    :
	    : "n"(sizeof(struct k_mem_pool_quad_block)));
}

#define K_MEM_POOL_SIZE(max_block_size, num_max_blocks) \
	(sizeof(struct k_mem_pool) + ((max_block_size) * (num_max_blocks)))

extern int k_mem_pool_alloc(struct k_mem_pool *pool, struct k_mem_block *block,
				int size, int32_t timeout);
extern void k_mem_pool_free(struct k_mem_block *block);
extern void k_mem_pool_defrag(struct k_mem_pool *pool);
extern void *k_malloc(uint32_t size);
extern void k_free(void *p);

/*
 * legacy.h must be before arch/cpu.h to allow the ioapic/loapic drivers to
 * hook into the device subsystem, which itself uses nanokernel semaphores,
 * and thus currently requires the definition of nano_sem.
 */
#include <legacy.h>
#include <arch/cpu.h>

/*
 * private APIs that are utilized by one or more public APIs
 */

extern int _is_thread_essential(void);
extern void _init_static_threads(void);

#ifdef __cplusplus
}
#endif

#endif /* _kernel__h_ */
