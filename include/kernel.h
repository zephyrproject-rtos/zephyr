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

#define K_OBJ(name, size) char name[size] __aligned(4)

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

typedef struct tcs *k_tid_t;
typedef struct k_mem_pool *k_mem_pool_t;

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
extern k_tid_t k_current_get(void);
extern int k_current_priority_get(void);
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

#ifdef CONFIG_NANO_TIMEOUTS
#define _THREAD_TIMEOUT_INIT(obj) \
	(obj).nano_timeout = { \
	.node = { {0}, {0} }, \
	.tcs = NULL, \
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

struct k_thread_static_init {
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
	.init_entry = entry, \
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
 * k_thread_static_init structure sise needs to be kept 32-bit aligned as well
 */
#define K_THREAD_OBJ_DEFINE(name, stack_size, \
			    entry, p1, p2, p3, \
			    abort, prio, groups) \
	extern void entry(void *, void *, void *); \
	char __noinit __stack _k_thread_obj_##name[stack_size]; \
	struct k_thread_static_init _k_thread_init_##name __aligned(4) \
		__in_section(_k_task_list, private, task) = \
		K_THREAD_INITIALIZER(_k_thread_obj_##name, stack_size, \
				     entry, p1, p2, p3, abort, prio, groups)

#define K_THREAD_DEFINE(name, stack_size, entry, p1, p2, p3, \
			abort, prio, groups) \
	K_THREAD_OBJ_DEFINE(name, stack_size, entry, p1, p2, p3, \
			    abort, prio, groups); \
	k_tid_t const name = (k_tid_t)_k_thread_obj_##name

/* extern int k_thread_prio_get(k_tid_t thread); in sched.h */
extern void k_thread_priority_set(k_tid_t thread, int prio);

#if 0
extern int k_thread_suspend(k_tid_t thread);
extern int k_thread_resume(k_tid_t thread);
extern int k_thread_entry_set(k_tid_t thread,
				void (*entry)(void*, void*, void*);
extern int k_thread_abort_handler_set(k_tid_t thread,
					 void (*handler)(void));
#endif

extern void k_sched_time_slice_set(int32_t slice, int prio);
extern int k_workload_get(void);
extern void k_workload_time_slice_set(int32_t slice);

extern int k_am_in_isr(void);

extern void k_thread_custom_data_set(void *value);
extern void *k_thread_custom_data_get(void);

/**
 *  kernel timing
 */

/* timeouts */

struct _timeout;
typedef void (*_timeout_func_t)(struct _timeout *t);

struct _timeout {
	sys_dlist_t node;
	struct tcs *tcs;
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
extern struct k_timer *k_timer_alloc(void);
extern void k_timer_free(struct k_timer *timer);
extern void k_timer_start(struct k_timer *timer,
			  int32_t duration, int32_t period,
			  void (*handler)(void *), void *handler_arg,
			  void (*stop_handler)(void *), void *stop_handler_arg);
extern void k_timer_restart(struct k_timer *timer, int32_t duration,
			    int32_t period);
extern void k_timer_stop(struct k_timer *timer);
extern int k_timer_test(struct k_timer *timer, void **data, int wait);
extern int32_t k_timer_remaining_get(struct k_timer *timer);
extern int64_t k_uptime_get(void);
extern int64_t k_uptime_delta(int64_t *reftime);
extern bool k_timer_pool_is_empty(void);

extern uint32_t k_cycle_get_32(void);

#if (CONFIG_NUM_DYNAMIC_TIMERS > 0)
extern void _k_dyamic_timer_init(void);
#else
#define _k_dyamic_timer_init()
#endif

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
	.data_q = SYS_DLIST_STATIC_INIT(&obj.data_q), \
	_DEBUG_TRACING_KERNEL_OBJECTS_INIT \
	}

#define K_FIFO_DEFINE(name) \
	struct k_fifo _k_fifo_obj_##name = \
		K_FIFO_INITIALIZER(_k_fifo_obj_##name); \
	struct k_fifo * const name = &_k_fifo_obj_##name

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
	struct k_lifo _k_lifo_obj_##name = \
		K_LIFO_INITIALIZER(_k_lifo_obj_##name); \
	struct k_lifo * const name = &_k_lifo_obj_##name

/* stacks */

struct k_stack {
	_wait_q_t wait_q;
	uint32_t *base, *next, *top;

	_DEBUG_TRACING_KERNEL_OBJECTS_NEXT_PTR(k_stack);
};

extern void k_stack_init(struct k_stack *stack, int num_entries);
extern void k_stack_init_with_buffer(struct k_stack *stack, int num_entries,
				     uint32_t *buffer);
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
	struct k_stack _k_stack_obj_##name = \
		K_STACK_INITIALIZER(_k_stack_obj_##name, stack_num_entries, \
				    _k_stack_buf_##name); \
	struct k_stack * const name = &_k_stack_obj_##name

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
	K_WORK_STATE_IDLE,		/* Work item idle state */
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
	.flags = { 1 } \
	}

/**
 * @brief Dynamically initialize work item
 */
static inline void k_work_init(struct k_work *work, k_work_handler_t handler)
{
	atomic_set_bit(work->flags, K_WORK_STATE_IDLE);
	work->handler = handler;
}

/**
 * @brief Submit a work item to a workqueue.
 */
static inline void k_work_submit_to_queue(struct k_work_q *work_q,
					  struct k_work *work)
{
	if (!atomic_test_and_clear_bit(work->flags, K_WORK_STATE_IDLE)) {
		__ASSERT_NO_MSG(0);
	} else {
		k_fifo_put(&work_q->fifo, work);
	}
}

/**
 * @brief Start a new workqueue.  This routine can be called from either
 * fiber or task context.
 */
extern void k_work_q_start(struct k_work_q *work_q,
				 const struct k_thread_config *config);

#if defined(CONFIG_NANO_TIMEOUTS)

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
void k_delayed_work_init(struct k_delayed_work *work,
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
int k_delayed_work_submit_to_queue(struct k_work_q *work_q,
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
int k_delayed_work_cancel(struct k_delayed_work *work);

#endif /* CONFIG_NANO_TIMEOUTS */

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

#if defined(CONFIG_NANO_TIMEOUTS)
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

#endif /* CONFIG_NANO_TIMEOUTS */
#endif /* CONFIG_SYSTEM_WORKQUEUE */

/**
 * synchronization
 */

/* mutexes */

struct k_mutex {
	_wait_q_t wait_q;
	struct tcs *owner;
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

static inline int k_sem_reset(struct k_sem *sem)
{
	sem->count = 0;

	return 0;
}

static inline int k_sem_count_get(struct k_sem *sem)
{
	return sem->count;
}

extern struct k_sem *k_sem_group_take(struct k_sem **sem_array,
				      int32_t timeout);
extern void k_sem_group_give(struct k_sem **sem_array);
extern void k_sem_group_reset(struct k_sem **sem_array);

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

void k_msgq_init(struct k_msgq *q, uint32_t msg_size, uint32_t max_msgs,
		 char *buffer);
extern int k_msgq_put(struct k_msgq *q, void *data, int32_t timeout);
extern int k_msgq_get(struct k_msgq *q, void *data, int32_t timeout);
extern void k_msgq_purge(struct k_msgq *q);

static inline int k_msgq_num_used_get(struct k_msgq *q)
{
	return q->used_msgs;
}

struct k_mem_block {
	k_mem_pool_t pool_id;
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

#if (CONFIG_NUM_MBOX_ASYNC_MSGS > 0)
extern void _k_mbox_init(void);
#else
#define _k_mbox_init()
#endif

extern void k_mbox_init(struct k_mbox *mbox);

extern int k_mbox_put(struct k_mbox *mbox, struct k_mbox_msg *msg,
		      int32_t timeout);
extern void k_mbox_async_put(struct k_mbox *mbox, struct k_mbox_msg *msg,
			     struct k_sem *sem);

extern int k_mbox_get(struct k_mbox *mbox, struct k_mbox_msg *msg,
		      void *buffer, int32_t timeout);
extern void k_mbox_data_get(struct k_mbox_msg *msg, void *buffer);
extern int k_mbox_data_block_get(struct k_mbox_msg *msg, k_mem_pool_t pool,
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

#define K_PIPE_INITIALIZER(obj, pipe_buffer_size, pipe_buffer)        \
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

#define K_PIPE_DEFINE(name, pipe_buffer_size)                           \
	static unsigned char __noinit _k_pipe_buf_##name[pipe_buffer_size]; \
	struct k_pipe name =                                  \
		K_PIPE_INITIALIZER(name, pipe_buffer_size, _k_pipe_buf_##name)

#define K_PIPE_SIZE(buffer_size) (sizeof(struct k_pipe) + buffer_size)

#if (CONFIG_NUM_PIPE_ASYNC_MSGS > 0)
extern void _k_pipes_init(void);
#else
#define _k_pipes_init()  do { } while (0)
#endif

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
 * @a num_bytes_to_write bytes of data. If by @a timeout, the pipe could not
 * accept @a min_bytes bytes of data, it fails. Fewer than @a min_bytes will
 * only ever be written to the pipe if K_NO_WAIT < @a timeout < K_FOREVER.
 *
 * @param pipe Pointer to the pipe
 * @param buffer Data to put into the pipe
 * @param num_bytes_to_write Desired number of bytes to put into the pipe
 * @param num_bytes_written Number of bytes the pipe accepted
 * @param min_bytes Minimum number of bytes accepted for success
 * @param timeout Maximum number of milliseconds to wait
 *
 * @retval 0 At least @a min_bytes were sent
 * @retval -EIO Request can not be satisfied (@a timeout is K_NO_WAIT)
 * @retval -EAGAIN Fewer than @a min_bytes were sent
 */
extern int k_pipe_put(struct k_pipe *pipe, void *buffer,
		      size_t num_bytes_to_write, size_t *num_bytes_written,
		      size_t min_bytes, int32_t timeout);

/**
 * @brief Get a message from the specified pipe
 *
 * This routine synchronously retrieves a message from the pipe specified by
 * @a pipe. It will wait up to @a timeout to retrieve @a num_bytes_to_read
 * bytes of data from the pipe. If by @a timeout, the pipe could not retrieve
 * @a min_bytes bytes of data, it fails. Fewer than @a min_bytes will
 * only ever be retrieved from the pipe if K_NO_WAIT < @a timeout < K_FOREVER.
 *
 * @param pipe Pointer to the pipe
 * @param buffer Location to place retrieved data
 * @param num_bytes_to_read Desired number of bytes to retrieve from the pipe
 * @param num_bytes_read Number of bytes retrieved from the pipe
 * @param min_bytes Minimum number of bytes retrieved for success
 * @param timeout Maximum number of milliseconds to wait
 *
 * @retval 0 At least @a min_bytes were transferred
 * @retval -EIO Request can not be satisfied (@a timeout is K_NO_WAIT)
 * @retval -EAGAIN Fewer than @a min_bytes were retrieved
 */
extern int k_pipe_get(struct k_pipe *pipe, void *buffer,
		      size_t num_bytes_to_read, size_t *num_bytes_read,
		      size_t min_bytes, int32_t timeout);

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

extern void _k_mem_map_init(void);

extern void k_mem_map_init(struct k_mem_map *map, int num_blocks,
			   int block_size, void *buffer);
extern int k_mem_map_alloc(struct k_mem_map *map, void **mem, int32_t timeout);
extern void k_mem_map_free(struct k_mem_map *map, void **mem);

static inline int k_mem_map_num_used_get(struct k_mem_map *map)
{
	return map->num_used;
}

/* memory pools */

struct k_mem_pool {
	_wait_q_t wait_q;
	int max_block_size;
	int num_max_blocks;

	_DEBUG_TRACING_KERNEL_OBJECTS_NEXT_PTR(k_mem_pool);
};

/* cannot initialize pools statically */

/* XXX - review this computation */
#define K_MEM_POOL_SIZE(max_block_size, num_max_blocks) \
	(sizeof(struct k_mem_pool) + ((max_block_size) * (num_max_blocks)))

extern void k_mem_pool_init(struct k_mem_pool *mem, int max_block_size,
				int num_max_blocks);
extern int k_mem_pool_alloc(k_mem_pool_t id, struct k_mem_block *block,
				int size, int32_t timeout);
extern void k_mem_pool_free(struct k_mem_block *block);
extern void k_mem_pool_defrag(k_mem_pool_t id);
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

extern struct k_thread_static_init _k_task_list_start[];
extern struct k_thread_static_init _k_task_list_end[];

#define _FOREACH_STATIC_THREAD(thread_init) \
	for (struct k_thread_static_init *thread_init = _k_task_list_start; \
	     thread_init < _k_task_list_end; thread_init++)

extern int _is_thread_essential(void);
static inline int is_in_any_group(struct k_thread_static_init *thread_init,
				  uint32_t groups)
{
	return !!(thread_init->init_groups & groups);
}
extern void _init_static_threads(void);

#ifdef __cplusplus
}
#endif

#endif /* _kernel__h_ */
