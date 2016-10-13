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
	int32_t init_delay;
};

/*
 * Common macro used by both K_THREAD_INITIALIZER()
 * and _MDEF_THREAD_INITIALIZER().
 */
#define _THREAD_INITIALIZER(stack, stack_size,                   \
			    entry, p1, p2, p3,                   \
			    abort, prio)                         \
	.init_prio = (prio),                                     \
	.init_entry = (void (*)(void *, void *, void *))entry,   \
	.init_p1 = (void *)p1,                                   \
	.init_p2 = (void *)p2,                                   \
	.init_p3 = (void *)p3,                                   \
	.init_abort = abort,                                     \
	.init_stack = (stack),                                   \
	.init_stack_size = (stack_size),

/**
 * @brief Thread initializer macro
 *
 * This macro is to only be used with statically defined threads that were not
 * defined in the MDEF file. As such the associated threads can not belong to
 * any thread group.
 */
#define K_THREAD_INITIALIZER(stack, stack_size,  \
			     entry, p1, p2, p3,  \
			     abort, prio, delay) \
	{                                        \
	_THREAD_INITIALIZER(stack, stack_size,   \
			    entry, p1, p2, p3,   \
			    abort, prio)         \
	.init_groups = 0,                        \
	.init_delay = (delay),                   \
	}

/**
 * @brief Thread initializer macro
 *
 * This macro is to only be used with statically defined threads that were
 * defined with legacy APIs (including the MDEF file). As such the associated
 * threads may belong to one or more thread groups.
 */
#define _MDEF_THREAD_INITIALIZER(stack, stack_size,   \
				  entry, p1, p2, p3,   \
				  abort, prio, groups) \
	{                                              \
	_THREAD_INITIALIZER(stack, stack_size,         \
			    entry, p1, p2, p3,         \
			    abort, prio)               \
	.init_groups = (groups),                       \
	.init_delay = K_FOREVER,                       \
	}

/**
 * @brief Define thread initializer and initialize it.
 *
 * @internal It has been observed that the x86 compiler by default aligns
 * these _static_thread_data structures to 32-byte boundaries, thereby
 * wasting space. To work around this, force a 4-byte alignment.
 */
#define K_THREAD_DEFINE(name, stack_size,                               \
			entry, p1, p2, p3,                              \
			abort, prio, delay)                             \
	char __noinit __stack _k_thread_obj_##name[stack_size];         \
	struct _static_thread_data _k_thread_data_##name __aligned(4)   \
		__in_section(_k_task_list, private, task) =             \
		K_THREAD_INITIALIZER(_k_thread_obj_##name, stack_size,  \
				     entry, p1, p2, p3, abort, prio, delay)

/**
 * @brief Define thread initializer for MDEF defined thread and initialize it.
 *
 * @ref K_THREAD_DEFINE
 */
#define _MDEF_THREAD_DEFINE(name, stack_size,                              \
			     entry, p1, p2, p3,                             \
			     abort, prio, groups)                           \
	char __noinit __stack _k_thread_obj_##name[stack_size];             \
	struct _static_thread_data _k_thread_data_##name __aligned(4)       \
		__in_section(_k_task_list, private, task) =                 \
		_MDEF_THREAD_INITIALIZER(_k_thread_obj_##name, stack_size, \
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

/* added tick needed to account for tick in progress */
#define _TICK_ALIGN 1

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

	/* wait queue for the (single) thread waiting on this timer */
	_wait_q_t wait_q;

	/* runs in ISR context */
	void (*expiry_fn)(struct k_timer *);

	/* runs in the context of the thread that calls k_timer_stop() */
	void (*stop_fn)(struct k_timer *);

	/* timer period */
	int32_t period;

	/* timer status */
	uint32_t status;

	/* used to support legacy timer APIs */
	void *_legacy_data;

	_DEBUG_TRACING_KERNEL_OBJECTS_NEXT_PTR(k_timer);
};

#define K_TIMER_INITIALIZER(obj) \
	{ \
	.wait_q = SYS_DLIST_STATIC_INIT(&obj.wait_q), \
	_DEBUG_TRACING_KERNEL_OBJECTS_INIT \
	}

#define K_TIMER_DEFINE(name) \
	struct k_timer name = K_TIMER_INITIALIZER(name)

/**
 * @brief Initialize a timer.
 *
 * This routine must be called before the timer is used.
 *
 * @param timer     Address of timer.
 * @param expiry_fn Function to invoke each time timer expires.
 * @param stop_fn   Function to invoke if timer is stopped while running.
 *
 * @return N/A
 */
extern void k_timer_init(struct k_timer *timer,
			 void (*expiry_fn)(struct k_timer *),
			 void (*stop_fn)(struct k_timer *));

/**
 * @brief Start a timer.
 *
 * This routine starts a timer, and resets its status to zero. The timer
 * begins counting down using the specified duration and period values.
 *
 * Attempting to start a timer that is already running is permitted.
 * The timer's status is reset to zero and the timer begins counting down
 * using the new duration and period values.
 *
 * @param timer     Address of timer.
 * @param duration  Initial timer duration (in milliseconds).
 * @param period    Timer period (in milliseconds).
 *
 * @return N/A
 */
extern void k_timer_start(struct k_timer *timer,
			  int32_t duration, int32_t period);

/**
 * @brief Stop a timer.
 *
 * This routine stops a running timer prematurely. The timer's stop function,
 * if one exists, is invoked by the caller.
 *
 * Attempting to stop a timer that is not running is permitted, but has no
 * effect on the timer since it is already stopped.
 *
 * @param timer     Address of timer.
 *
 * @return N/A
 */
extern void k_timer_stop(struct k_timer *timer);

/**
 * @brief Read timer status.
 *
 * This routine reads the timer's status, which indicates the number of times
 * it has expired since its status was last read.
 *
 * Calling this routine resets the timer's status to zero.
 *
 * @param timer     Address of timer.
 *
 * @return Timer status.
 */
extern uint32_t k_timer_status_get(struct k_timer *timer);

/**
 * @brief Synchronize thread to timer expiration.
 *
 * This routine blocks the calling thread until the timer's status is non-zero
 * (indicating that it has expired at least once since it was last examined)
 * or the timer is stopped. If the timer status is already non-zero,
 * or the timer is already stopped, the caller continues without waiting.
 *
 * Calling this routine resets the timer's status to zero.
 *
 * This routine must not be used by interrupt handlers, since they are not
 * allowed to block.
 *
 * @param timer     Address of timer.
 *
 * @return Timer status.
 */
extern uint32_t k_timer_status_sync(struct k_timer *timer);

/**
 * @brief Get timer remaining before next timer expiration.
 *
 * This routine computes the (approximate) time remaining before a running
 * timer next expires. If the timer is not running, it returns zero.
 *
 * @param timer     Address of timer.
 *
 * @return Remaining time (in milliseconds).
 */

extern int32_t k_timer_remaining_get(struct k_timer *timer);


/* kernel clocks */

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
 * @param work_q Workqueue to schedule the work item
 * @param work Delayed work item
 * @param delay Delay before scheduling the work item (in milliseconds)
 *
 * @return 0 in case of success or negative value in case of error.
 */
extern int k_delayed_work_submit_to_queue(struct k_work_q *work_q,
					  struct k_delayed_work *work,
					  int32_t delay);

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
					   int32_t delay)
{
	return k_delayed_work_submit_to_queue(&k_sys_work_q, work, delay);
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

/**
 * @brief Initialize a semaphore object.
 *
 * An initial count and a count limit can be specified. The count will never go
 * over the count limit if the semaphore is given multiple times without being
 * taken.
 *
 * Cannot be called from ISR.
 *
 * @param sem Pointer to a semaphore object.
 * @param initial_count Initial count.
 * @param limit Highest value the count can take during operation.
 *
 * @return N/A
 */
extern void k_sem_init(struct k_sem *sem, unsigned int initial_count,
			unsigned int limit);

/**
 * @brief Take a semaphore, possibly pending if not available.
 *
 * The current execution context tries to obtain the semaphore. If the
 * semaphore is unavailable and a timeout other than K_NO_WAIT is specified,
 * the context will pend.
 *
 * @param sem Pointer to a semaphore object.
 * @param timeout Number of milliseconds to wait if semaphore is unavailable,
 *                or one of the special values K_NO_WAIT and K_FOREVER.
 *
 * @warning If it is called from the context of an ISR, then the only legal
 * value for @a timeout is K_NO_WAIT.
 *
 * @retval 0 When semaphore is obtained successfully.
 * @retval -EAGAIN When timeout expires.
 * @retval -EBUSY When unavailable and the timeout is K_NO_WAIT.
 *
 * @sa K_NO_WAIT, K_FOREVER
 */
extern int k_sem_take(struct k_sem *sem, int32_t timeout);

/**
 * @brief Give a semaphore.
 *
 * Increase the semaphore's internal count by 1, up to its limit, if no thread
 * is waiting on the semaphore; otherwise, wake up the first thread in the
 * semaphore's waiting queue.
 *
 * If the latter case, and if the current context is preemptible, the thread
 * that is taken off the wait queue will be scheduled in and will preempt the
 * current thread.
 *
 * @param sem Pointer to a semaphore object.
 *
 * @return N/A
 */
extern void k_sem_give(struct k_sem *sem);

/**
 * @brief Reset a semaphore's count to zero.
 *
 * The only effect is that the count is set to zero. There is no other
 * side-effect to calling this function.
 *
 * @param sem Pointer to a semaphore object.
 *
 * @return N/A
 */
static inline void k_sem_reset(struct k_sem *sem)
{
	sem->count = 0;
}

/**
 * @brief Get a semaphore's count.
 *
 * Note there is no guarantee the count has not changed by the time this
 * function returns.
 *
 * @param sem Pointer to a semaphore object.
 *
 * @return The current semaphore count.
 */
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
 * @param timeout Number of milliseconds to wait if semaphores are unavailable,
 *                or one of the special values K_NO_WAIT and K_FOREVER.
 *
 * @retval 0 A semaphore was successfully taken
 * @retval -EBUSY No semaphore was available (@a timeout = K_NO_WAIT)
 * @retval -EAGAIN Time out occurred while waiting for semaphore
 *
 * @sa K_NO_WAIT, K_FOREVER
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

/**
 * @def K_SEM_DEFINE
 *
 * @brief Statically define and initialize a global semaphore.
 *
 * Create a global semaphore named @name. It is initialized as if k_sem_init()
 * was called on it. If the semaphore is to be accessed outside the module
 * where it is defined, it can be declared via
 *
 *    extern struct k_sem @name;
 *
 * @param name Name of the semaphore variable.
 * @param initial_count Initial count.
 * @param count_limit Highest value the count can take during operation.
 */
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
	size_t msg_size;
	uint32_t max_msgs;
	char *buffer_start;
	char *buffer_end;
	char *read_ptr;
	char *write_ptr;
	uint32_t used_msgs;

	_DEBUG_TRACING_KERNEL_OBJECTS_NEXT_PTR(k_msgq);
};

#define K_MSGQ_INITIALIZER(obj, q_buffer, q_msg_size, q_max_msgs) \
	{ \
	.wait_q = SYS_DLIST_STATIC_INIT(&obj.wait_q), \
	.max_msgs = q_max_msgs, \
	.msg_size = q_msg_size, \
	.buffer_start = q_buffer, \
	.buffer_end = q_buffer + (q_max_msgs * q_msg_size), \
	.read_ptr = q_buffer, \
	.write_ptr = q_buffer, \
	.used_msgs = 0, \
	_DEBUG_TRACING_KERNEL_OBJECTS_INIT \
	}

/**
 * @brief Define a message queue
 *
 * This declares and initializes a message queue whose buffer is aligned to
 * a @a q_align -byte boundary. The new message queue can be passed to the
 * kernel's message queue functions.
 *
 * Note that for each of the mesages in the message queue to be aligned to
 * @a q_align bytes, then @a q_msg_size must be a multiple of @a q_align.
 *
 * @param q_name Name of the message queue
 * @param q_msg_size The size in bytes of each message
 * @param q_max_msgs Maximum number of messages the queue can hold
 * @param q_align Alignment of the message queue's buffer (power of 2)
 */
#define K_MSGQ_DEFINE(q_name, q_msg_size, q_max_msgs, q_align)      \
	static char __noinit __aligned(q_align)                     \
		_k_fifo_buf_##q_name[(q_max_msgs) * (q_msg_size)];  \
	struct k_msgq q_name =                                      \
	       K_MSGQ_INITIALIZER(q_name, _k_fifo_buf_##q_name,     \
				  q_msg_size, q_max_msgs)

/**
 * @brief Initialize a message queue.
 *
 * @param q Pointer to the message queue object.
 * @param buffer Pointer to memory area that holds queued messages.
 * @param msg_size Message size, in bytes.
 * @param max_msgs Maximum number of messages that can be queued.
 *
 * @return N/A
 */
extern void k_msgq_init(struct k_msgq *q, char *buffer,
			size_t msg_size, uint32_t max_msgs);

/**
 * @brief Add a message to a message queue.
 *
 * This routine adds an item to the message queue. When the message queue is
 * full, the routine will wait either for space to become available, or until
 * the specified time limit is reached.
 *
 * @param q Pointer to the message queue object.
 * @param data Pointer to message data area.
 * @param timeout Number of milliseconds to wait until space becomes available
 *                to add the message into the message queue, or one of the
 *                special values K_NO_WAIT and K_FOREVER.
 *
 * @return 0 if successful, -ENOMSG if failed immediately or after queue purge,
 *         -EAGAIN if timed out
 *
 * @sa K_NO_WAIT, K_FOREVER
 */
extern int k_msgq_put(struct k_msgq *q, void *data, int32_t timeout);

/**
 * @brief Obtain a message from a message queue.
 *
 * This routine fetches the oldest item from the message queue. When the message
 * queue is found empty, the routine will wait either until an item is added to
 * the message queue or until the specified time limit is reached.
 *
 * @param q Pointer to the message queue object.
 * @param data Pointer to message data area.
 * @param timeout Number of milliseconds to wait to obtain message, or one of
 *                the special values K_NO_WAIT and K_FOREVER.
 *
 * @return 0 if successful, -ENOMSG if failed immediately, -EAGAIN if timed out
 *
 * @sa K_NO_WAIT, K_FOREVER
 */
extern int k_msgq_get(struct k_msgq *q, void *data, int32_t timeout);

/**
 * @brief Purge contents of a message queue.
 *
 * Discards all messages currently in the message queue, and cancels
 * any "add message" operations initiated by waiting threads.
 *
 * @param q Pointer to the message queue object.
 *
 * @return N/A
 */
extern void k_msgq_purge(struct k_msgq *q);

/**
 * @brief Get the number of unused messages
 *
 * @param q Message queue to query
 *
 * @return Number of unused messages
 */
static inline uint32_t k_msgq_num_free_get(struct k_msgq *q)
{
	return q->max_msgs - q->used_msgs;
}

/**
 * @brief Get the number of used messages
 *
 * @param q Message queue to query
 *
 * @return Number of used messages
 */
static inline uint32_t k_msgq_num_used_get(struct k_msgq *q)
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
	size_t size;
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

/**
 * @brief Define a mailbox
 *
 * This declares and initializes a mailbox. The new mailbox can be passed to
 * the kernel's mailbox functions.
 *
 * @param name Name of the mailbox
 */
#define K_MBOX_DEFINE(name) \
	struct k_mbox name = \
		K_MBOX_INITIALIZER(name) \

/**
 * @brief Initialize a mailbox.
 *
 * @param mbox Pointer to the mailbox object
 *
 * @return N/A
 */
extern void k_mbox_init(struct k_mbox *mbox);

/**
 * @brief Send a mailbox message in a synchronous manner.
 *
 * Sends a message to a mailbox and waits for a receiver to process it.
 * The message data may be in a buffer, in a memory pool block, or non-existent
 * (i.e. empty message).
 *
 * @param mbox Pointer to the mailbox object.
 * @param tx_msg Pointer to transmit message descriptor.
 * @param timeout Maximum time (milliseconds) to wait for the message to be
 *        received (although not necessarily completely processed).
 *        Use K_NO_WAIT to return immediately, or K_FOREVER to wait as long
 *        as necessary.
 *
 * @return 0 if successful, -ENOMSG if failed immediately, -EAGAIN if timed out
 */
extern int k_mbox_put(struct k_mbox *mbox, struct k_mbox_msg *tx_msg,
		      int32_t timeout);

#if (CONFIG_NUM_MBOX_ASYNC_MSGS > 0)
/**
 * @brief Send a mailbox message in an asynchronous manner.
 *
 * Sends a message to a mailbox without waiting for a receiver to process it.
 * The message data may be in a buffer, in a memory pool block, or non-existent
 * (i.e. an empty message). Optionally, the specified semaphore will be given
 * by the mailbox when the message has been both received and disposed of
 * by the receiver.
 *
 * @param mbox Pointer to the mailbox object.
 * @param tx_msg Pointer to transmit message descriptor.
 * @param sem Semaphore identifier, or NULL if none specified.
 *
 * @return N/A
 */
extern void k_mbox_async_put(struct k_mbox *mbox, struct k_mbox_msg *tx_msg,
			     struct k_sem *sem);
#endif

/**
 * @brief Receive a mailbox message.
 *
 * Receives a message from a mailbox, then optionally retrieves its data
 * and disposes of the message.
 *
 * @param mbox Pointer to the mailbox object.
 * @param rx_msg Pointer to receive message descriptor.
 * @param buffer Pointer to buffer to receive data.
 *        (Use NULL to defer data retrieval and message disposal until later.)
 * @param timeout Maximum time (milliseconds) to wait for a message.
 *        Use K_NO_WAIT to return immediately, or K_FOREVER to wait as long as
 *        necessary.
 *
 * @return 0 if successful, -ENOMSG if failed immediately, -EAGAIN if timed out
 */
extern int k_mbox_get(struct k_mbox *mbox, struct k_mbox_msg *rx_msg,
		      void *buffer, int32_t timeout);

/**
 * @brief Retrieve mailbox message data into a buffer.
 *
 * Completes the processing of a received message by retrieving its data
 * into a buffer, then disposing of the message.
 *
 * Alternatively, this routine can be used to dispose of a received message
 * without retrieving its data.
 *
 * @param rx_msg Pointer to receive message descriptor.
 * @param buffer Pointer to buffer to receive data. (Use NULL to discard data.)
 *
 * @return N/A
 */
extern void k_mbox_data_get(struct k_mbox_msg *rx_msg, void *buffer);

/**
 * @brief Retrieve mailbox message data into a memory pool block.
 *
 * Completes the processing of a received message by retrieving its data
 * into a memory pool block, then disposing of the message. The memory pool
 * block that results from successful retrieval must be returned to the pool
 * once the data has been processed, even in cases where zero bytes of data
 * are retrieved.
 *
 * Alternatively, this routine can be used to dispose of a received message
 * without retrieving its data. In this case there is no need to return a
 * memory pool block to the pool.
 *
 * This routine allocates a new memory pool block for the data only if the
 * data is not already in one. If a new block cannot be allocated, the routine
 * returns a failure code and the received message is left unchanged. This
 * permits the caller to reattempt data retrieval at a later time or to dispose
 * of the received message without retrieving its data.
 *
 * @param rx_msg Pointer to receive message descriptor.
 * @param pool Memory pool identifier. (Use NULL to discard data.)
 * @param block Pointer to area to hold memory pool block info.
 * @param timeout Maximum time (milliseconds) to wait for a memory pool block.
 *        Use K_NO_WAIT to return immediately, or K_FOREVER to wait as long as
 *        necessary.
 *
 * @return 0 if successful, -ENOMEM if failed immediately, -EAGAIN if timed out
 */
extern int k_mbox_data_block_get(struct k_mbox_msg *rx_msg,
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

#if (CONFIG_NUM_PIPE_ASYNC_MSGS > 0)
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
#endif

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

#define K_MEM_MAP_INITIALIZER(obj, map_buffer, map_block_size, map_num_blocks) \
	{ \
	.wait_q = SYS_DLIST_STATIC_INIT(&obj.wait_q), \
	.num_blocks = map_num_blocks, \
	.block_size = map_block_size, \
	.buffer = map_buffer, \
	.free_list = NULL, \
	.num_used = 0, \
	_DEBUG_TRACING_KERNEL_OBJECTS_INIT \
	}

/**
 * @brief Define a memory map
 *
 * This declares and initializes a memory map whose buffer is aligned to
 * a @a map_align -byte boundary. The new memory map can be passed to the
 * kernel's memory map functions.
 *
 * Note that for each of the blocks in the memory map to be aligned to
 * @a map_align bytes, then @a map_block_size must be a multiple of
 * @a map_align.
 *
 * @param name Name of the memory map
 * @param map_block_size Size of each block in the buffer (in bytes)
 * @param map_num_blocks Number blocks in the buffer
 * @param map_align Alignment of the memory map's buffer (power of 2)
 */
#define K_MEM_MAP_DEFINE(name, map_block_size, map_num_blocks, map_align)   \
	char __aligned(map_align)                                           \
		_k_mem_map_buf_##name[(map_num_blocks) * (map_block_size)]; \
	struct k_mem_map name                                               \
		__in_section(_k_mem_map_ptr, private, mem_map) =            \
		K_MEM_MAP_INITIALIZER(name, _k_mem_map_buf_##name,          \
				      map_block_size, map_num_blocks)

/**
 * @brief Initialize a memory map.
 *
 * Initializes the memory map and creates its list of free blocks.
 *
 * @param map Pointer to the memory map object
 * @param buffer Pointer to buffer used for the blocks.
 * @param block_size Size of each block, in bytes.
 * @param num_blocks Number of blocks.
 *
 * @return N/A
 */
extern void k_mem_map_init(struct k_mem_map *map, void *buffer,
			   int block_size, int num_blocks);

/**
 * @brief Allocate a memory map block.
 *
 * Takes a block from the list of unused blocks.
 *
 * @param map Pointer to memory map object.
 * @param mem Pointer to area to receive block address.
 * @param timeout Maximum time (milliseconds) to wait for allocation to
 *        complete.  Use K_NO_WAIT to return immediately, or K_FOREVER to wait
 *        as long as necessary.
 *
 * @return 0 if successful, -ENOMEM if failed immediately, -EAGAIN if timed out
 */
extern int k_mem_map_alloc(struct k_mem_map *map, void **mem, int32_t timeout);

/**
 * @brief Free a memory map block.
 *
 * Gives block to a waiting thread if there is one, otherwise returns it to
 * the list of unused blocks.
 *
 * @param map Pointer to memory map object.
 * @param mem Pointer to area to containing block address.
 *
 * @return N/A
 */
extern void k_mem_map_free(struct k_mem_map *map, void **mem);

/**
 * @brief Get the number of used memory blocks
 *
 * This routine gets the current number of used memory blocks in the
 * specified pool. It should be used for stats purposes only as that
 * value may potentially be out-of-date by the time it is used.
 *
 * @param map Memory map to query
 *
 * @return Number of used memory blocks
 */
static inline int k_mem_map_num_used_get(struct k_mem_map *map)
{
	return map->num_used;
}

/**
 * @brief Get the number of unused memory blocks
 *
 * This routine gets the current number of unused memory blocks in the
 * specified pool. It should be used for stats purposes only as that value
 * may potentially be out-of-date by the time it is used.
 *
 * @param map Memory map to query
 *
 * @return Number of unused memory blocks
 */
static inline int k_mem_map_num_free_get(struct k_mem_map *map)
{
	return map->num_blocks - map->num_used;
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

#define _MEMORY_POOL_BUFFER_DEFINE(name, max_size, n_max, align)	\
	char __noinit __aligned(align)                                  \
		_mem_pool_buffer_##name[(max_size) * (n_max)]

/**
 * @brief Define a memory pool
 *
 * This declares and initializes a memory pool whose buffer is aligned to
 * a @a align -byte boundary. The new memory pool can be passed to the
 * kernel's memory pool functions.
 *
 * Note that for each of the minimum sized blocks to be aligned to @a align
 * bytes, then @a min_size must be a multiple of @a align.
 *
 * @param name Name of the memory pool
 * @param min_size Minimum block size in the pool
 * @param max_size Maximum block size in the pool
 * @param n_max Number of maximum sized blocks in the pool
 * @param align Alignment of the memory pool's buffer
 */
#define K_MEM_POOL_DEFINE(name, min_size, max_size, n_max, align)     \
	_MEMORY_POOL_QUAD_BLOCK_DEFINE(name, min_size, max_size, n_max); \
	_MEMORY_POOL_BLOCK_SETS_DEFINE(name, min_size, max_size, n_max); \
	_MEMORY_POOL_BUFFER_DEFINE(name, max_size, n_max, align);        \
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

/**
 * @brief Allocate memory from a memory pool
 *
 * @param pool Pointer to the memory pool object
 * @param block Pointer to the allocated memory's block descriptor
 * @param size Minimum number of bytes to allocate
 * @param timeout Maximum time (milliseconds) to wait for operation to
 *        complete. Use K_NO_WAIT to return immediately, or K_FOREVER
 *        to wait as long as necessary.
 *
 * @return 0 on success, -ENOMEM on failure
 */
extern int k_mem_pool_alloc(struct k_mem_pool *pool, struct k_mem_block *block,
			    int size, int32_t timeout);

/**
 * @brief Return previously allocated memory to its memory pool
 *
 * @param block Pointer to allocated memory's block descriptor
 *
 * @return N/A
 */
extern void k_mem_pool_free(struct k_mem_block *block);

/**
 * @brief Defragment the specified memory pool
 *
 * @param pool Pointer to the memory pool object
 *
 * @return N/A
 */
extern void k_mem_pool_defrag(struct k_mem_pool *pool);

/**
 * @brief Allocate memory from heap pool
 *
 * This routine provides traditional malloc semantics; internally it uses
 * the memory pool APIs on a dedicated HEAP pool
 *
 * @param size Size of memory requested by the caller (in bytes)
 *
 * @return Address of the allocated memory on success; otherwise NULL
 */
extern void *k_malloc(uint32_t size);

/**
 * @brief Free memory allocated through k_malloc()
 *
 * @param ptr Pointer to previously allocated memory
 *
 * @return N/A
 */
extern void k_free(void *ptr);

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
