/*
 * Copyright (c) 2016, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Public kernel APIs.
 */

#ifndef _kernel__h_
#define _kernel__h_

#if !defined(_ASMLANGUAGE)
#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <toolchain.h>
#include <sections.h>
#include <atomic.h>
#include <errno.h>
#include <misc/__assert.h>
#include <misc/dlist.h>
#include <misc/slist.h>
#include <misc/util.h>
#include <kernel_version.h>
#include <drivers/rand32.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Kernel APIs
 * @defgroup kernel_apis Kernel APIs
 * @{
 * @}
 */

#ifdef CONFIG_KERNEL_DEBUG
#include <misc/printk.h>
#define K_DEBUG(fmt, ...) printk("[%s]  " fmt, __func__, ##__VA_ARGS__)
#else
#define K_DEBUG(fmt, ...)
#endif

#if defined(CONFIG_COOP_ENABLED) && defined(CONFIG_PREEMPT_ENABLED)
#define _NUM_COOP_PRIO (CONFIG_NUM_COOP_PRIORITIES)
#define _NUM_PREEMPT_PRIO (CONFIG_NUM_PREEMPT_PRIORITIES + 1)
#elif defined(CONFIG_COOP_ENABLED)
#define _NUM_COOP_PRIO (CONFIG_NUM_COOP_PRIORITIES + 1)
#define _NUM_PREEMPT_PRIO (0)
#elif defined(CONFIG_PREEMPT_ENABLED)
#define _NUM_COOP_PRIO (0)
#define _NUM_PREEMPT_PRIO (CONFIG_NUM_PREEMPT_PRIORITIES + 1)
#else
#error "invalid configuration"
#endif

#define K_PRIO_COOP(x) (-(_NUM_COOP_PRIO - (x)))
#define K_PRIO_PREEMPT(x) (x)

#define K_ANY NULL
#define K_END NULL

#if defined(CONFIG_COOP_ENABLED) && defined(CONFIG_PREEMPT_ENABLED)
#define K_HIGHEST_THREAD_PRIO (-CONFIG_NUM_COOP_PRIORITIES)
#elif defined(CONFIG_COOP_ENABLED)
#define K_HIGHEST_THREAD_PRIO (-CONFIG_NUM_COOP_PRIORITIES - 1)
#elif defined(CONFIG_PREEMPT_ENABLED)
#define K_HIGHEST_THREAD_PRIO 0
#else
#error "invalid configuration"
#endif

#ifdef CONFIG_PREEMPT_ENABLED
#define K_LOWEST_THREAD_PRIO CONFIG_NUM_PREEMPT_PRIORITIES
#else
#define K_LOWEST_THREAD_PRIO -1
#endif

#define K_IDLE_PRIO K_LOWEST_THREAD_PRIO

#define K_HIGHEST_APPLICATION_THREAD_PRIO (K_HIGHEST_THREAD_PRIO)
#define K_LOWEST_APPLICATION_THREAD_PRIO (K_LOWEST_THREAD_PRIO - 1)

typedef sys_dlist_t _wait_q_t;

#ifdef CONFIG_OBJECT_TRACING
#define _OBJECT_TRACING_NEXT_PTR(type) struct type *__next
#define _OBJECT_TRACING_INIT .__next = NULL,
#else
#define _OBJECT_TRACING_INIT
#define _OBJECT_TRACING_NEXT_PTR(type)
#endif

#define tcs k_thread
struct k_thread;
struct k_mutex;
struct k_sem;
struct k_alert;
struct k_msgq;
struct k_mbox;
struct k_pipe;
struct k_fifo;
struct k_lifo;
struct k_stack;
struct k_mem_slab;
struct k_mem_pool;
struct k_timer;

typedef struct k_thread *k_tid_t;

enum execution_context_types {
	K_ISR = 0,
	K_COOP_THREAD,
	K_PREEMPT_THREAD,
};

/**
 * @defgroup profiling_apis Profiling APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief Analyze the main, idle, interrupt and system workqueue call stacks
 *
 * This routine calls @ref stack_analyze on the 4 call stacks declared and
 * maintained by the kernel. The sizes of those 4 call stacks are defined by:
 *
 * CONFIG_MAIN_STACK_SIZE
 * CONFIG_IDLE_STACK_SIZE
 * CONFIG_ISR_STACK_SIZE
 * CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE
 *
 * @note CONFIG_INIT_STACKS and CONFIG_PRINTK must be set for this function to
 * produce output.
 *
 * @return N/A
 */
extern void k_call_stacks_analyze(void);

/**
 * @} end defgroup profiling_apis
 */

/**
 * @defgroup thread_apis Thread APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @typedef k_thread_entry_t
 * @brief Thread entry point function type.
 *
 * A thread's entry point function is invoked when the thread starts executing.
 * Up to 3 argument values can be passed to the function.
 *
 * The thread terminates execution permanently if the entry point function
 * returns. The thread is responsible for releasing any shared resources
 * it may own (such as mutexes and dynamically allocated memory), prior to
 * returning.
 *
 * @param p1 First argument.
 * @param p2 Second argument.
 * @param p3 Third argument.
 *
 * @return N/A
 */
typedef void (*k_thread_entry_t)(void *p1, void *p2, void *p3);

#endif /* !_ASMLANGUAGE */


/*
 * Thread user options. May be needed by assembly code. Common part uses low
 * bits, arch-specific use high bits.
 */

/* system thread that must not abort */
#define K_ESSENTIAL (1 << 0)

#if defined(CONFIG_FP_SHARING)
/* thread uses floating point registers */
#define K_FP_REGS (1 << 1)
#endif

#ifdef CONFIG_X86
/* x86 Bitmask definitions for threads user options */

#if defined(CONFIG_FP_SHARING) && defined(CONFIG_SSE)
/* thread uses SSEx (and also FP) registers */
#define K_SSE_REGS (1 << 7)
#endif
#endif

/* end - thread options */

#if !defined(_ASMLANGUAGE)

/**
 * @brief Spawn a thread.
 *
 * This routine initializes a thread, then schedules it for execution.
 *
 * The new thread may be scheduled for immediate execution or a delayed start.
 * If the newly spawned thread does not have a delayed start the kernel
 * scheduler may preempt the current thread to allow the new thread to
 * execute.
 *
 * Thread options are architecture-specific, and can include K_ESSENTIAL,
 * K_FP_REGS, and K_SSE_REGS. Multiple options may be specified by separating
 * them using "|" (the logical OR operator).
 *
 * @param stack Pointer to the stack space.
 * @param stack_size Stack size in bytes.
 * @param entry Thread entry function.
 * @param p1 1st entry point parameter.
 * @param p2 2nd entry point parameter.
 * @param p3 3rd entry point parameter.
 * @param prio Thread priority.
 * @param options Thread options.
 * @param delay Scheduling delay (in milliseconds), or K_NO_WAIT (for no delay).
 *
 * @return ID of new thread.
 */
extern k_tid_t k_thread_spawn(char *stack, size_t stack_size,
			      k_thread_entry_t entry,
			      void *p1, void *p2, void *p3,
			      int prio, uint32_t options, int32_t delay);

/**
 * @brief Put the current thread to sleep.
 *
 * This routine puts the current thread to sleep for @a duration
 * milliseconds.
 *
 * @param duration Number of milliseconds to sleep.
 *
 * @return N/A
 */
extern void k_sleep(int32_t duration);

/**
 * @brief Cause the current thread to busy wait.
 *
 * This routine causes the current thread to execute a "do nothing" loop for
 * @a usec_to_wait microseconds.
 *
 * @return N/A
 */
extern void k_busy_wait(uint32_t usec_to_wait);

/**
 * @brief Yield the current thread.
 *
 * This routine causes the current thread to yield execution to another
 * thread of the same or higher priority. If there are no other ready threads
 * of the same or higher priority, the routine returns immediately.
 *
 * @return N/A
 */
extern void k_yield(void);

/**
 * @brief Wake up a sleeping thread.
 *
 * This routine prematurely wakes up @a thread from sleeping.
 *
 * If @a thread is not currently sleeping, the routine has no effect.
 *
 * @param thread ID of thread to wake.
 *
 * @return N/A
 */
extern void k_wakeup(k_tid_t thread);

/**
 * @brief Get thread ID of the current thread.
 *
 * @return ID of current thread.
 */
extern k_tid_t k_current_get(void);

/**
 * @brief Cancel thread performing a delayed start.
 *
 * This routine prevents @a thread from executing if it has not yet started
 * execution. The thread must be re-spawned before it will execute.
 *
 * @param thread ID of thread to cancel.
 *
 * @retval 0 Thread spawning cancelled.
 * @retval -EINVAL Thread has already started executing.
 */
extern int k_thread_cancel(k_tid_t thread);

/**
 * @brief Abort a thread.
 *
 * This routine permanently stops execution of @a thread. The thread is taken
 * off all kernel queues it is part of (i.e. the ready queue, the timeout
 * queue, or a kernel object wait queue). However, any kernel resources the
 * thread might currently own (such as mutexes or memory blocks) are not
 * released. It is the responsibility of the caller of this routine to ensure
 * all necessary cleanup is performed.
 *
 * @param thread ID of thread to abort.
 *
 * @return N/A
 */
extern void k_thread_abort(k_tid_t thread);

/**
 * @cond INTERNAL_HIDDEN
 */

/* timeout has timed out and is not on _timeout_q anymore */
#define _EXPIRED (-2)

/* timeout is not in use */
#define _INACTIVE (-1)

#ifdef CONFIG_SYS_CLOCK_EXISTS
#define _THREAD_TIMEOUT_INIT(obj) \
	(obj).nano_timeout = { \
	.node = { {0}, {0} }, \
	.thread = NULL, \
	.wait_q = NULL, \
	.delta_ticks_from_prev = _INACTIVE, \
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
	union {
		char *init_stack;
		struct k_thread *thread;
	};
	unsigned int init_stack_size;
	void (*init_entry)(void *, void *, void *);
	void *init_p1;
	void *init_p2;
	void *init_p3;
	int init_prio;
	uint32_t init_options;
	int32_t init_delay;
	void (*init_abort)(void);
	uint32_t init_groups;
};

#define _THREAD_INITIALIZER(stack, stack_size,                   \
			    entry, p1, p2, p3,                   \
			    prio, options, delay, abort, groups) \
	{                                                        \
	.init_stack = (stack),                                   \
	.init_stack_size = (stack_size),                         \
	.init_entry = (void (*)(void *, void *, void *))entry,   \
	.init_p1 = (void *)p1,                                   \
	.init_p2 = (void *)p2,                                   \
	.init_p3 = (void *)p3,                                   \
	.init_prio = (prio),                                     \
	.init_options = (options),                               \
	.init_delay = (delay),                                   \
	.init_abort = (abort),                                   \
	.init_groups = (groups),                                 \
	}

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @brief Statically define and initialize a thread.
 *
 * The thread may be scheduled for immediate execution or a delayed start.
 *
 * Thread options are architecture-specific, and can include K_ESSENTIAL,
 * K_FP_REGS, and K_SSE_REGS. Multiple options may be specified by separating
 * them using "|" (the logical OR operator).
 *
 * The ID of the thread can be accessed using:
 *
 * @code extern const k_tid_t <name>; @endcode
 *
 * @param name Name of the thread.
 * @param stack_size Stack size in bytes.
 * @param entry Thread entry function.
 * @param p1 1st entry point parameter.
 * @param p2 2nd entry point parameter.
 * @param p3 3rd entry point parameter.
 * @param prio Thread priority.
 * @param options Thread options.
 * @param delay Scheduling delay (in milliseconds), or K_NO_WAIT (for no delay).
 *
 * @internal It has been observed that the x86 compiler by default aligns
 * these _static_thread_data structures to 32-byte boundaries, thereby
 * wasting space. To work around this, force a 4-byte alignment.
 */
#define K_THREAD_DEFINE(name, stack_size,                                \
			entry, p1, p2, p3,                               \
			prio, options, delay)                            \
	char __noinit __stack _k_thread_obj_##name[stack_size];          \
	struct _static_thread_data _k_thread_data_##name __aligned(4)    \
		__in_section(_static_thread_data, static, name) =        \
		_THREAD_INITIALIZER(_k_thread_obj_##name, stack_size,    \
				entry, p1, p2, p3, prio, options, delay, \
				NULL, 0); \
	const k_tid_t name = (k_tid_t)_k_thread_obj_##name

/**
 * @brief Get a thread's priority.
 *
 * This routine gets the priority of @a thread.
 *
 * @param thread ID of thread whose priority is needed.
 *
 * @return Priority of @a thread.
 */
extern int  k_thread_priority_get(k_tid_t thread);

/**
 * @brief Set a thread's priority.
 *
 * This routine immediately changes the priority of @a thread.
 *
 * Rescheduling can occur immediately depending on the priority @a thread is
 * set to:
 *
 * - If its priority is raised above the priority of the caller of this
 * function, and the caller is preemptible, @a thread will be scheduled in.
 *
 * - If the caller operates on itself, it lowers its priority below that of
 * other threads in the system, and the caller is preemptible, the thread of
 * highest priority will be scheduled in.
 *
 * Priority can be assigned in the range of -CONFIG_NUM_COOP_PRIORITIES to
 * CONFIG_NUM_PREEMPT_PRIORITIES-1, where -CONFIG_NUM_COOP_PRIORITIES is the
 * highest priority.
 *
 * @param thread ID of thread whose priority is to be set.
 * @param prio New priority.
 *
 * @warning Changing the priority of a thread currently involved in mutex
 * priority inheritance may result in undefined behavior.
 *
 * @return N/A
 */
extern void k_thread_priority_set(k_tid_t thread, int prio);

/**
 * @brief Suspend a thread.
 *
 * This routine prevents the kernel scheduler from making @a thread the
 * current thread. All other internal operations on @a thread are still
 * performed; for example, any timeout it is waiting on keeps ticking,
 * kernel objects it is waiting on are still handed to it, etc.
 *
 * If @a thread is already suspended, the routine has no effect.
 *
 * @param thread ID of thread to suspend.
 *
 * @return N/A
 */
extern void k_thread_suspend(k_tid_t thread);

/**
 * @brief Resume a suspended thread.
 *
 * This routine allows the kernel scheduler to make @a thread the current
 * thread, when it is next eligible for that role.
 *
 * If @a thread is not currently suspended, the routine has no effect.
 *
 * @param thread ID of thread to resume.
 *
 * @return N/A
 */
extern void k_thread_resume(k_tid_t thread);

/**
 * @brief Set time-slicing period and scope.
 *
 * This routine specifies how the scheduler will perform time slicing of
 * preemptible threads.
 *
 * To enable time slicing, @a slice must be non-zero. The scheduler
 * ensures that no thread runs for more than the specified time limit
 * before other threads of that priority are given a chance to execute.
 * Any thread whose priority is higher than @a prio is exempted, and may
 * execute as long as desired without being pre-empted due to time slicing.
 *
 * Time slicing only limits the maximum amount of time a thread may continuously
 * execute. Once the scheduler selects a thread for execution, there is no
 * minimum guaranteed time the thread will execute before threads of greater or
 * equal priority are scheduled.
 *
 * When the current thread is the only one of that priority eligible
 * for execution, this routine has no effect; the thread is immediately
 * rescheduled after the slice period expires.
 *
 * To disable timeslicing, set both @a slice and @a prio to zero.
 *
 * @param slice Maximum time slice length (in milliseconds).
 * @param prio Highest thread priority level eligible for time slicing.
 *
 * @return N/A
 */
extern void k_sched_time_slice_set(int32_t slice, int prio);

/**
 * @} end defgroup thread_apis
 */

/**
 * @addtogroup isr_apis
 * @{
 */

/**
 * @brief Determine if code is running at interrupt level.
 *
 * This routine allows the caller to customize its actions, depending on
 * whether it is a thread or an ISR.
 *
 * @note Can be called by ISRs.
 *
 * @return 0 if invoked by a thread.
 * @return Non-zero if invoked by an ISR.
 */
extern int k_is_in_isr(void);

/**
 * @brief Determine if code is running in a preemptible thread.
 *
 * This routine allows the caller to customize its actions, depending on
 * whether it can be preempted by another thread. The routine returns a 'true'
 * value if all of the following conditions are met:
 *
 * - The code is running in a thread, not at ISR.
 * - The thread's priority is in the preemptible range.
 * - The thread has not locked the scheduler.
 *
 * @note Can be called by ISRs.
 *
 * @return 0 if invoked by an ISR or by a cooperative thread.
 * @return Non-zero if invoked by a preemptible thread.
 */
extern int k_is_preempt_thread(void);

/**
 * @} end addtogroup isr_apis
 */

/**
 * @addtogroup thread_apis
 * @{
 */

/**
 * @brief Lock the scheduler.
 *
 * This routine prevents the current thread from being preempted by another
 * thread by instructing the scheduler to treat it as a cooperative thread.
 * If the thread subsequently performs an operation that makes it unready,
 * it will be context switched out in the normal manner. When the thread
 * again becomes the current thread, its non-preemptible status is maintained.
 *
 * This routine can be called recursively.
 *
 * @note k_sched_lock() and k_sched_unlock() should normally be used
 * when the operation being performed can be safely interrupted by ISRs.
 * However, if the amount of processing involved is very small, better
 * performance may be obtained by using irq_lock() and irq_unlock().
 *
 * @return N/A
 */
extern void k_sched_lock(void);

/**
 * @brief Unlock the scheduler.
 *
 * This routine reverses the effect of a previous call to k_sched_lock().
 * A thread must call the routine once for each time it called k_sched_lock()
 * before the thread becomes preemptible.
 *
 * @return N/A
 */
extern void k_sched_unlock(void);

/**
 * @brief Set current thread's custom data.
 *
 * This routine sets the custom data for the current thread to @ value.
 *
 * Custom data is not used by the kernel itself, and is freely available
 * for a thread to use as it sees fit. It can be used as a framework
 * upon which to build thread-local storage.
 *
 * @param value New custom data value.
 *
 * @return N/A
 */
extern void k_thread_custom_data_set(void *value);

/**
 * @brief Get current thread's custom data.
 *
 * This routine returns the custom data for the current thread.
 *
 * @return Current custom data value.
 */
extern void *k_thread_custom_data_get(void);

/**
 * @} end addtogroup thread_apis
 */

#include <sys_clock.h>

/**
 * @addtogroup clock_apis
 * @{
 */

/**
 * @brief Generate null timeout delay.
 *
 * This macro generates a timeout delay that that instructs a kernel API
 * not to wait if the requested operation cannot be performed immediately.
 *
 * @return Timeout delay value.
 */
#define K_NO_WAIT 0

/**
 * @brief Generate timeout delay from milliseconds.
 *
 * This macro generates a timeout delay that that instructs a kernel API
 * to wait up to @a ms milliseconds to perform the requested operation.
 *
 * @param ms Duration in milliseconds.
 *
 * @return Timeout delay value.
 */
#define K_MSEC(ms)     (ms)

/**
 * @brief Generate timeout delay from seconds.
 *
 * This macro generates a timeout delay that that instructs a kernel API
 * to wait up to @a s seconds to perform the requested operation.
 *
 * @param s Duration in seconds.
 *
 * @return Timeout delay value.
 */
#define K_SECONDS(s)   K_MSEC((s) * MSEC_PER_SEC)

/**
 * @brief Generate timeout delay from minutes.
 *
 * This macro generates a timeout delay that that instructs a kernel API
 * to wait up to @a m minutes to perform the requested operation.
 *
 * @param m Duration in minutes.
 *
 * @return Timeout delay value.
 */
#define K_MINUTES(m)   K_SECONDS((m) * 60)

/**
 * @brief Generate timeout delay from hours.
 *
 * This macro generates a timeout delay that that instructs a kernel API
 * to wait up to @a h hours to perform the requested operation.
 *
 * @param h Duration in hours.
 *
 * @return Timeout delay value.
 */
#define K_HOURS(h)     K_MINUTES((h) * 60)

/**
 * @brief Generate infinite timeout delay.
 *
 * This macro generates a timeout delay that that instructs a kernel API
 * to wait as long as necessary to perform the requested operation.
 *
 * @return Timeout delay value.
 */
#define K_FOREVER (-1)

/**
 * @} end addtogroup clock_apis
 */

/**
 * @cond INTERNAL_HIDDEN
 */

/* kernel clocks */

#if	(sys_clock_ticks_per_sec == 1000) || \
	(sys_clock_ticks_per_sec == 500)  || \
	(sys_clock_ticks_per_sec == 250)  || \
	(sys_clock_ticks_per_sec == 125)  || \
	(sys_clock_ticks_per_sec == 100)  || \
	(sys_clock_ticks_per_sec == 50)   || \
	(sys_clock_ticks_per_sec == 25)   || \
	(sys_clock_ticks_per_sec == 20)   || \
	(sys_clock_ticks_per_sec == 10)   || \
	(sys_clock_ticks_per_sec == 1)

	#define _ms_per_tick (MSEC_PER_SEC / sys_clock_ticks_per_sec)
#else
	/* yields horrible 64-bit math on many architectures: try to avoid */
	#define _NON_OPTIMIZED_TICKS_PER_SEC
#endif

#ifdef _NON_OPTIMIZED_TICKS_PER_SEC
extern int32_t _ms_to_ticks(int32_t ms);
#else
static ALWAYS_INLINE int32_t _ms_to_ticks(int32_t ms)
{
	return (int32_t)ceiling_fraction((uint32_t)ms, _ms_per_tick);
}
#endif

/* added tick needed to account for tick in progress */
#define _TICK_ALIGN 1

static inline int64_t __ticks_to_ms(int64_t ticks)
{
#ifdef CONFIG_SYS_CLOCK_EXISTS

#ifdef _NON_OPTIMIZED_TICKS_PER_SEC
	return (MSEC_PER_SEC * (uint64_t)ticks) / sys_clock_ticks_per_sec;
#else
	return (uint64_t)ticks * _ms_per_tick;
#endif

#else
	__ASSERT(ticks == 0, "");
	return 0;
#endif
}

/* timeouts */

struct _timeout;
typedef void (*_timeout_func_t)(struct _timeout *t);

struct _timeout {
	sys_dnode_t node;
	struct k_thread *thread;
	sys_dlist_t *wait_q;
	int32_t delta_ticks_from_prev;
	_timeout_func_t func;
};

extern int32_t _timeout_remaining_get(struct _timeout *timeout);

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @cond INTERNAL_HIDDEN
 */

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

	/* user-specific data, also used to support legacy features */
	void *user_data;

	_OBJECT_TRACING_NEXT_PTR(k_timer);
};

#define K_TIMER_INITIALIZER(obj, expiry, stop) \
	{ \
	.timeout.delta_ticks_from_prev = _INACTIVE, \
	.timeout.wait_q = NULL, \
	.timeout.thread = NULL, \
	.timeout.func = _timer_expiration_handler, \
	.wait_q = SYS_DLIST_STATIC_INIT(&obj.wait_q), \
	.expiry_fn = expiry, \
	.stop_fn = stop, \
	.status = 0, \
	.user_data = 0, \
	_OBJECT_TRACING_INIT \
	}

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @defgroup timer_apis Timer APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @typedef k_timer_expiry_t
 * @brief Timer expiry function type.
 *
 * A timer's expiry function is executed by the system clock interrupt handler
 * each time the timer expires. The expiry function is optional, and is only
 * invoked if the timer has been initialized with one.
 *
 * @param timer     Address of timer.
 *
 * @return N/A
 */
typedef void (*k_timer_expiry_t)(struct k_timer *timer);

/**
 * @typedef k_timer_stop_t
 * @brief Timer stop function type.
 *
 * A timer's stop function is executed if the timer is stopped prematurely.
 * The function runs in the context of the thread that stops the timer.
 * The stop function is optional, and is only invoked if the timer has been
 * initialized with one.
 *
 * @param timer     Address of timer.
 *
 * @return N/A
 */
typedef void (*k_timer_stop_t)(struct k_timer *timer);

/**
 * @brief Statically define and initialize a timer.
 *
 * The timer can be accessed outside the module where it is defined using:
 *
 * @code extern struct k_timer <name>; @endcode
 *
 * @param name Name of the timer variable.
 * @param expiry_fn Function to invoke each time the timer expires.
 * @param stop_fn   Function to invoke if the timer is stopped while running.
 */
#define K_TIMER_DEFINE(name, expiry_fn, stop_fn) \
	struct k_timer name \
		__in_section(_k_timer, static, name) = \
		K_TIMER_INITIALIZER(name, expiry_fn, stop_fn)

/**
 * @brief Initialize a timer.
 *
 * This routine initializes a timer, prior to its first use.
 *
 * @param timer     Address of timer.
 * @param expiry_fn Function to invoke each time the timer expires.
 * @param stop_fn   Function to invoke if the timer is stopped while running.
 *
 * @return N/A
 */
extern void k_timer_init(struct k_timer *timer,
			 k_timer_expiry_t expiry_fn,
			 k_timer_stop_t stop_fn);

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
 * effect on the timer.
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
 * @brief Get time remaining before a timer next expires.
 *
 * This routine computes the (approximate) time remaining before a running
 * timer next expires. If the timer is not running, it returns zero.
 *
 * @param timer     Address of timer.
 *
 * @return Remaining time (in milliseconds).
 */
static inline int32_t k_timer_remaining_get(struct k_timer *timer)
{
	return _timeout_remaining_get(&timer->timeout);
}

/**
 * @brief Associate user-specific data with a timer.
 *
 * This routine records the @a user_data with the @a timer, to be retrieved
 * later.
 *
 * It can be used e.g. in a timer handler shared across multiple subsystems to
 * retrieve data specific to the subsystem this timer is associated with.
 *
 * @param timer     Address of timer.
 * @param user_data User data to associate with the timer.
 *
 * @return N/A
 */
static inline void k_timer_user_data_set(struct k_timer *timer,
					 void *user_data)
{
	timer->user_data = user_data;
}

/**
 * @brief Retrieve the user-specific data from a timer.
 *
 * @param timer     Address of timer.
 *
 * @return The user data.
 */
static inline void *k_timer_user_data_get(struct k_timer *timer)
{
	return timer->user_data;
}

/**
 * @} end defgroup timer_apis
 */

/**
 * @addtogroup clock_apis
 * @{
 */

/**
 * @brief Get system uptime.
 *
 * This routine returns the elapsed time since the system booted,
 * in milliseconds.
 *
 * @return Current uptime.
 */
extern int64_t k_uptime_get(void);

/**
 * @brief Get system uptime (32-bit version).
 *
 * This routine returns the lower 32-bits of the elapsed time since the system
 * booted, in milliseconds.
 *
 * This routine can be more efficient than k_uptime_get(), as it reduces the
 * need for interrupt locking and 64-bit math. However, the 32-bit result
 * cannot hold a system uptime time larger than approximately 50 days, so the
 * caller must handle possible rollovers.
 *
 * @return Current uptime.
 */
extern uint32_t k_uptime_get_32(void);

/**
 * @brief Get elapsed time.
 *
 * This routine computes the elapsed time between the current system uptime
 * and an earlier reference time, in milliseconds.
 *
 * @param reftime Pointer to a reference time, which is updated to the current
 *                uptime upon return.
 *
 * @return Elapsed time.
 */
extern int64_t k_uptime_delta(int64_t *reftime);

/**
 * @brief Get elapsed time (32-bit version).
 *
 * This routine computes the elapsed time between the current system uptime
 * and an earlier reference time, in milliseconds.
 *
 * This routine can be more efficient than k_uptime_delta(), as it reduces the
 * need for interrupt locking and 64-bit math. However, the 32-bit result
 * cannot hold an elapsed time larger than approximately 50 days, so the
 * caller must handle possible rollovers.
 *
 * @param reftime Pointer to a reference time, which is updated to the current
 *                uptime upon return.
 *
 * @return Elapsed time.
 */
extern uint32_t k_uptime_delta_32(int64_t *reftime);

/**
 * @brief Read the hardware clock.
 *
 * This routine returns the current time, as measured by the system's hardware
 * clock.
 *
 * @return Current hardware clock up-counter (in cycles).
 */
extern uint32_t k_cycle_get_32(void);

/**
 * @} end addtogroup clock_apis
 */

/**
 * @cond INTERNAL_HIDDEN
 */

struct k_fifo {
	_wait_q_t wait_q;
	sys_slist_t data_q;

	_OBJECT_TRACING_NEXT_PTR(k_fifo);
};

#define K_FIFO_INITIALIZER(obj) \
	{ \
	.wait_q = SYS_DLIST_STATIC_INIT(&obj.wait_q), \
	.data_q = SYS_SLIST_STATIC_INIT(&obj.data_q), \
	_OBJECT_TRACING_INIT \
	}

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @defgroup fifo_apis Fifo APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief Initialize a fifo.
 *
 * This routine initializes a fifo object, prior to its first use.
 *
 * @param fifo Address of the fifo.
 *
 * @return N/A
 */
extern void k_fifo_init(struct k_fifo *fifo);

/**
 * @brief Add an element to a fifo.
 *
 * This routine adds a data item to @a fifo. A fifo data item must be
 * aligned on a 4-byte boundary, and the first 32 bits of the item are
 * reserved for the kernel's use.
 *
 * @note Can be called by ISRs.
 *
 * @param fifo Address of the fifo.
 * @param data Address of the data item.
 *
 * @return N/A
 */
extern void k_fifo_put(struct k_fifo *fifo, void *data);

/**
 * @brief Atomically add a list of elements to a fifo.
 *
 * This routine adds a list of data items to @a fifo in one operation.
 * The data items must be in a singly-linked list, with the first 32 bits
 * each data item pointing to the next data item; the list must be
 * NULL-terminated.
 *
 * @note Can be called by ISRs.
 *
 * @param fifo Address of the fifo.
 * @param head Pointer to first node in singly-linked list.
 * @param tail Pointer to last node in singly-linked list.
 *
 * @return N/A
 */
extern void k_fifo_put_list(struct k_fifo *fifo, void *head, void *tail);

/**
 * @brief Atomically add a list of elements to a fifo.
 *
 * This routine adds a list of data items to @a fifo in one operation.
 * The data items must be in a singly-linked list implemented using a
 * sys_slist_t object. Upon completion, the sys_slist_t object is invalid
 * and must be re-initialized via sys_slist_init().
 *
 * @note Can be called by ISRs.
 *
 * @param fifo Address of the fifo.
 * @param list Pointer to sys_slist_t object.
 *
 * @return N/A
 */
extern void k_fifo_put_slist(struct k_fifo *fifo, sys_slist_t *list);

/**
 * @brief Get an element from a fifo.
 *
 * This routine removes a data item from @a fifo in a "first in, first out"
 * manner. The first 32 bits of the data item are reserved for the kernel's use.
 *
 * @note Can be called by ISRs, but @a timeout must be set to K_NO_WAIT.
 *
 * @param fifo Address of the fifo.
 * @param timeout Waiting period to obtain a data item (in milliseconds),
 *                or one of the special values K_NO_WAIT and K_FOREVER.
 *
 * @return Address of the data item if successful; NULL if returned
 * without waiting, or waiting period timed out.
 */
extern void *k_fifo_get(struct k_fifo *fifo, int32_t timeout);

/**
 * @brief Statically define and initialize a fifo.
 *
 * The fifo can be accessed outside the module where it is defined using:
 *
 * @code extern struct k_fifo <name>; @endcode
 *
 * @param name Name of the fifo.
 */
#define K_FIFO_DEFINE(name) \
	struct k_fifo name \
		__in_section(_k_fifo, static, name) = \
		K_FIFO_INITIALIZER(name)

/**
 * @} end defgroup fifo_apis
 */

/**
 * @cond INTERNAL_HIDDEN
 */

struct k_lifo {
	_wait_q_t wait_q;
	void *list;

	_OBJECT_TRACING_NEXT_PTR(k_lifo);
};

#define K_LIFO_INITIALIZER(obj) \
	{ \
	.wait_q = SYS_DLIST_STATIC_INIT(&obj.wait_q), \
	.list = NULL, \
	_OBJECT_TRACING_INIT \
	}

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @defgroup lifo_apis Lifo APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief Initialize a lifo.
 *
 * This routine initializes a lifo object, prior to its first use.
 *
 * @param lifo Address of the lifo.
 *
 * @return N/A
 */
extern void k_lifo_init(struct k_lifo *lifo);

/**
 * @brief Add an element to a lifo.
 *
 * This routine adds a data item to @a lifo. A lifo data item must be
 * aligned on a 4-byte boundary, and the first 32 bits of the item are
 * reserved for the kernel's use.
 *
 * @note Can be called by ISRs.
 *
 * @param lifo Address of the lifo.
 * @param data Address of the data item.
 *
 * @return N/A
 */
extern void k_lifo_put(struct k_lifo *lifo, void *data);

/**
 * @brief Get an element from a lifo.
 *
 * This routine removes a data item from @a lifo in a "last in, first out"
 * manner. The first 32 bits of the data item are reserved for the kernel's use.
 *
 * @note Can be called by ISRs, but @a timeout must be set to K_NO_WAIT.
 *
 * @param lifo Address of the lifo.
 * @param timeout Waiting period to obtain a data item (in milliseconds),
 *                or one of the special values K_NO_WAIT and K_FOREVER.
 *
 * @return Address of the data item if successful; NULL if returned
 * without waiting, or waiting period timed out.
 */
extern void *k_lifo_get(struct k_lifo *lifo, int32_t timeout);

/**
 * @brief Statically define and initialize a lifo.
 *
 * The lifo can be accessed outside the module where it is defined using:
 *
 * @code extern struct k_lifo <name>; @endcode
 *
 * @param name Name of the fifo.
 */
#define K_LIFO_DEFINE(name) \
	struct k_lifo name \
		__in_section(_k_lifo, static, name) = \
		K_LIFO_INITIALIZER(name)

/**
 * @} end defgroup lifo_apis
 */

/**
 * @cond INTERNAL_HIDDEN
 */

struct k_stack {
	_wait_q_t wait_q;
	uint32_t *base, *next, *top;

	_OBJECT_TRACING_NEXT_PTR(k_stack);
};

#define K_STACK_INITIALIZER(obj, stack_buffer, stack_num_entries) \
	{ \
	.wait_q = SYS_DLIST_STATIC_INIT(&obj.wait_q), \
	.base = stack_buffer, \
	.next = stack_buffer, \
	.top = stack_buffer + stack_num_entries, \
	_OBJECT_TRACING_INIT \
	}

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @defgroup stack_apis Stack APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief Initialize a stack.
 *
 * This routine initializes a stack object, prior to its first use.
 *
 * @param stack Address of the stack.
 * @param buffer Address of array used to hold stacked values.
 * @param num_entries Maximum number of values that can be stacked.
 *
 * @return N/A
 */
extern void k_stack_init(struct k_stack *stack,
			 uint32_t *buffer, int num_entries);

/**
 * @brief Push an element onto a stack.
 *
 * This routine adds a 32-bit value @a data to @a stack.
 *
 * @note Can be called by ISRs.
 *
 * @param stack Address of the stack.
 * @param data Value to push onto the stack.
 *
 * @return N/A
 */
extern void k_stack_push(struct k_stack *stack, uint32_t data);

/**
 * @brief Pop an element from a stack.
 *
 * This routine removes a 32-bit value from @a stack in a "last in, first out"
 * manner and stores the value in @a data.
 *
 * @note Can be called by ISRs, but @a timeout must be set to K_NO_WAIT.
 *
 * @param stack Address of the stack.
 * @param data Address of area to hold the value popped from the stack.
 * @param timeout Waiting period to obtain a value (in milliseconds),
 *                or one of the special values K_NO_WAIT and K_FOREVER.
 *
 * @retval 0 Element popped from stack.
 * @retval -EBUSY Returned without waiting.
 * @retval -EAGAIN Waiting period timed out.
 */
extern int k_stack_pop(struct k_stack *stack, uint32_t *data, int32_t timeout);

/**
 * @brief Statically define and initialize a stack
 *
 * The stack can be accessed outside the module where it is defined using:
 *
 * @code extern struct k_stack <name>; @endcode
 *
 * @param name Name of the stack.
 * @param stack_num_entries Maximum number of values that can be stacked.
 */
#define K_STACK_DEFINE(name, stack_num_entries)                \
	uint32_t __noinit                                      \
		_k_stack_buf_##name[stack_num_entries];        \
	struct k_stack name                                    \
		__in_section(_k_stack, static, name) =    \
		K_STACK_INITIALIZER(name, _k_stack_buf_##name, \
				    stack_num_entries)

/**
 * @} end defgroup stack_apis
 */

struct k_work;

/**
 * @defgroup workqueue_apis Workqueue Thread APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @typedef k_work_handler_t
 * @brief Work item handler function type.
 *
 * A work item's handler function is executed by a workqueue's thread
 * when the work item is processed by the workqueue.
 *
 * @param work Address of the work item.
 *
 * @return N/A
 */
typedef void (*k_work_handler_t)(struct k_work *work);

/**
 * @cond INTERNAL_HIDDEN
 */

struct k_work_q {
	struct k_fifo fifo;
};

enum {
	K_WORK_STATE_PENDING,	/* Work item pending state */
};

struct k_work {
	void *_reserved;		/* Used by k_fifo implementation. */
	k_work_handler_t handler;
	atomic_t flags[1];
};

struct k_delayed_work {
	struct k_work work;
	struct _timeout timeout;
	struct k_work_q *work_q;
};

extern struct k_work_q k_sys_work_q;

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @brief Initialize a statically-defined work item.
 *
 * This macro can be used to initialize a statically-defined workqueue work
 * item, prior to its first use. For example,
 *
 * @code struct k_work <work> = K_WORK_INITIALIZER(<work_handler>); @endcode
 *
 * @param work_handler Function to invoke each time work item is processed.
 */
#define K_WORK_INITIALIZER(work_handler) \
	{ \
	._reserved = NULL, \
	.handler = work_handler, \
	.flags = { 0 } \
	}

/**
 * @brief Initialize a work item.
 *
 * This routine initializes a workqueue work item, prior to its first use.
 *
 * @param work Address of work item.
 * @param handler Function to invoke each time work item is processed.
 *
 * @return N/A
 */
static inline void k_work_init(struct k_work *work, k_work_handler_t handler)
{
	atomic_clear_bit(work->flags, K_WORK_STATE_PENDING);
	work->handler = handler;
}

/**
 * @brief Submit a work item.
 *
 * This routine submits work item @a work to be processed by workqueue
 * @a work_q. If the work item is already pending in the workqueue's queue
 * as a result of an earlier submission, this routine has no effect on the
 * work item. If the work item has already been processed, or is currently
 * being processed, its work is considered complete and the work item can be
 * resubmitted.
 *
 * @warning
 * A submitted work item must not be modified until it has been processed
 * by the workqueue.
 *
 * @note Can be called by ISRs.
 *
 * @param work_q Address of workqueue.
 * @param work Address of work item.
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
 * @brief Check if a work item is pending.
 *
 * This routine indicates if work item @a work is pending in a workqueue's
 * queue.
 *
 * @note Can be called by ISRs.
 *
 * @param work Address of work item.
 *
 * @return 1 if work item is pending, or 0 if it is not pending.
 */
static inline int k_work_pending(struct k_work *work)
{
	return atomic_test_bit(work->flags, K_WORK_STATE_PENDING);
}

/**
 * @brief Start a workqueue.
 *
 * This routine starts workqueue @a work_q. The workqueue spawns its work
 * processing thread, which runs forever.
 *
 * @param work_q Address of workqueue.
 * @param stack Pointer to work queue thread's stack space.
 * @param stack_size Size of the work queue thread's stack (in bytes).
 * @param prio Priority of the work queue's thread.
 *
 * @return N/A
 */
extern void k_work_q_start(struct k_work_q *work_q, char *stack,
			   size_t stack_size, int prio);

/**
 * @brief Initialize a delayed work item.
 *
 * This routine initializes a workqueue delayed work item, prior to
 * its first use.
 *
 * @param work Address of delayed work item.
 * @param handler Function to invoke each time work item is processed.
 *
 * @return N/A
 */
extern void k_delayed_work_init(struct k_delayed_work *work,
				k_work_handler_t handler);

/**
 * @brief Submit a delayed work item.
 *
 * This routine schedules work item @a work to be processed by workqueue
 * @a work_q after a delay of @a delay milliseconds. The routine initiates
 * an asychronous countdown for the work item and then returns to the caller.
 * Only when the countdown completes is the work item actually submitted to
 * the workqueue and becomes pending.
 *
 * Submitting a previously submitted delayed work item that is still
 * counting down cancels the existing submission and restarts the countdown
 * using the new delay. If the work item is currently pending on the
 * workqueue's queue because the countdown has completed it is too late to
 * resubmit the item, and resubmission fails without impacting the work item.
 * If the work item has already been processed, or is currently being processed,
 * its work is considered complete and the work item can be resubmitted.
 *
 * @warning
 * A delayed work item must not be modified until it has been processed
 * by the workqueue.
 *
 * @note Can be called by ISRs.
 *
 * @param work_q Address of workqueue.
 * @param work Address of delayed work item.
 * @param delay Delay before submitting the work item (in milliseconds).
 *
 * @retval 0 Work item countdown started.
 * @retval -EINPROGRESS Work item is already pending.
 * @retval -EINVAL Work item is being processed or has completed its work.
 * @retval -EADDRINUSE Work item is pending on a different workqueue.
 */
extern int k_delayed_work_submit_to_queue(struct k_work_q *work_q,
					  struct k_delayed_work *work,
					  int32_t delay);

/**
 * @brief Cancel a delayed work item.
 *
 * This routine cancels the submission of delayed work item @a work.
 * A delayed work item can only be cancelled while its countdown is still
 * underway.
 *
 * @note Can be called by ISRs.
 *
 * @param work Address of delayed work item.
 *
 * @retval 0 Work item countdown cancelled.
 * @retval -EINPROGRESS Work item is already pending.
 * @retval -EINVAL Work item is being processed or has completed its work.
 */
extern int k_delayed_work_cancel(struct k_delayed_work *work);

/**
 * @brief Submit a work item to the system workqueue.
 *
 * This routine submits work item @a work to be processed by the system
 * workqueue. If the work item is already pending in the workqueue's queue
 * as a result of an earlier submission, this routine has no effect on the
 * work item. If the work item has already been processed, or is currently
 * being processed, its work is considered complete and the work item can be
 * resubmitted.
 *
 * @warning
 * Work items submitted to the system workqueue should avoid using handlers
 * that block or yield since this may prevent the system workqueue from
 * processing other work items in a timely manner.
 *
 * @note Can be called by ISRs.
 *
 * @param work Address of work item.
 *
 * @return N/A
 */
static inline void k_work_submit(struct k_work *work)
{
	k_work_submit_to_queue(&k_sys_work_q, work);
}

/**
 * @brief Submit a delayed work item to the system workqueue.
 *
 * This routine schedules work item @a work to be processed by the system
 * workqueue after a delay of @a delay milliseconds. The routine initiates
 * an asychronous countdown for the work item and then returns to the caller.
 * Only when the countdown completes is the work item actually submitted to
 * the workqueue and becomes pending.
 *
 * Submitting a previously submitted delayed work item that is still
 * counting down cancels the existing submission and restarts the countdown
 * using the new delay. If the work item is currently pending on the
 * workqueue's queue because the countdown has completed it is too late to
 * resubmit the item, and resubmission fails without impacting the work item.
 * If the work item has already been processed, or is currently being processed,
 * its work is considered complete and the work item can be resubmitted.
 *
 * @warning
 * Work items submitted to the system workqueue should avoid using handlers
 * that block or yield since this may prevent the system workqueue from
 * processing other work items in a timely manner.
 *
 * @note Can be called by ISRs.
 *
 * @param work Address of delayed work item.
 * @param delay Delay before submitting the work item (in milliseconds).
 *
 * @retval 0 Work item countdown started.
 * @retval -EINPROGRESS Work item is already pending.
 * @retval -EINVAL Work item is being processed or has completed its work.
 * @retval -EADDRINUSE Work item is pending on a different workqueue.
 */
static inline int k_delayed_work_submit(struct k_delayed_work *work,
					int32_t delay)
{
	return k_delayed_work_submit_to_queue(&k_sys_work_q, work, delay);
}

/**
 * @brief Get time remaining before a delayed work gets scheduled.
 *
 * This routine computes the (approximate) time remaining before a
 * delayed work gets executed. If the delayed work is not waiting to be
 * schedules, it returns zero.
 *
 * @param work     Delayed work item.
 *
 * @return Remaining time (in milliseconds).
 */
static inline int32_t k_delayed_work_remaining_get(struct k_delayed_work *work)
{
	return _timeout_remaining_get(&work->timeout);
}

/**
 * @} end defgroup workqueue_apis
 */

/**
 * @cond INTERNAL_HIDDEN
 */

struct k_mutex {
	_wait_q_t wait_q;
	struct k_thread *owner;
	uint32_t lock_count;
	int owner_orig_prio;
#ifdef CONFIG_OBJECT_MONITOR
	int num_lock_state_changes;
	int num_conflicts;
#endif

	_OBJECT_TRACING_NEXT_PTR(k_mutex);
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
	_OBJECT_TRACING_INIT \
	}

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @defgroup mutex_apis Mutex APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief Statically define and initialize a mutex.
 *
 * The mutex can be accessed outside the module where it is defined using:
 *
 * @code extern struct k_mutex <name>; @endcode
 *
 * @param name Name of the mutex.
 */
#define K_MUTEX_DEFINE(name) \
	struct k_mutex name \
		__in_section(_k_mutex, static, name) = \
		K_MUTEX_INITIALIZER(name)

/**
 * @brief Initialize a mutex.
 *
 * This routine initializes a mutex object, prior to its first use.
 *
 * Upon completion, the mutex is available and does not have an owner.
 *
 * @param mutex Address of the mutex.
 *
 * @return N/A
 */
extern void k_mutex_init(struct k_mutex *mutex);

/**
 * @brief Lock a mutex.
 *
 * This routine locks @a mutex. If the mutex is locked by another thread,
 * the calling thread waits until the mutex becomes available or until
 * a timeout occurs.
 *
 * A thread is permitted to lock a mutex it has already locked. The operation
 * completes immediately and the lock count is increased by 1.
 *
 * @param mutex Address of the mutex.
 * @param timeout Waiting period to lock the mutex (in milliseconds),
 *                or one of the special values K_NO_WAIT and K_FOREVER.
 *
 * @retval 0 Mutex locked.
 * @retval -EBUSY Returned without waiting.
 * @retval -EAGAIN Waiting period timed out.
 */
extern int k_mutex_lock(struct k_mutex *mutex, int32_t timeout);

/**
 * @brief Unlock a mutex.
 *
 * This routine unlocks @a mutex. The mutex must already be locked by the
 * calling thread.
 *
 * The mutex cannot be claimed by another thread until it has been unlocked by
 * the calling thread as many times as it was previously locked by that
 * thread.
 *
 * @param mutex Address of the mutex.
 *
 * @return N/A
 */
extern void k_mutex_unlock(struct k_mutex *mutex);

/**
 * @} end defgroup mutex_apis
 */

/**
 * @cond INTERNAL_HIDDEN
 */

struct k_sem {
	_wait_q_t wait_q;
	unsigned int count;
	unsigned int limit;

	_OBJECT_TRACING_NEXT_PTR(k_sem);
};

#define K_SEM_INITIALIZER(obj, initial_count, count_limit) \
	{ \
	.wait_q = SYS_DLIST_STATIC_INIT(&obj.wait_q), \
	.count = initial_count, \
	.limit = count_limit, \
	_OBJECT_TRACING_INIT \
	}

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @defgroup semaphore_apis Semaphore APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief Initialize a semaphore.
 *
 * This routine initializes a semaphore object, prior to its first use.
 *
 * @param sem Address of the semaphore.
 * @param initial_count Initial semaphore count.
 * @param limit Maximum permitted semaphore count.
 *
 * @return N/A
 */
extern void k_sem_init(struct k_sem *sem, unsigned int initial_count,
			unsigned int limit);

/**
 * @brief Take a semaphore.
 *
 * This routine takes @a sem.
 *
 * @note Can be called by ISRs, but @a timeout must be set to K_NO_WAIT.
 *
 * @param sem Address of the semaphore.
 * @param timeout Waiting period to take the semaphore (in milliseconds),
 *                or one of the special values K_NO_WAIT and K_FOREVER.
 *
 * @note When porting code from the nanokernel legacy API to the new API, be
 * careful with the return value of this function. The return value is the
 * reverse of the one of nano_sem_take family of APIs: 0 means success, and
 * non-zero means failure, while the nano_sem_take family returns 1 for success
 * and 0 for failure.
 *
 * @retval 0 Semaphore taken.
 * @retval -EBUSY Returned without waiting.
 * @retval -EAGAIN Waiting period timed out.
 */
extern int k_sem_take(struct k_sem *sem, int32_t timeout);

/**
 * @brief Give a semaphore.
 *
 * This routine gives @a sem, unless the semaphore is already at its maximum
 * permitted count.
 *
 * @note Can be called by ISRs.
 *
 * @param sem Address of the semaphore.
 *
 * @return N/A
 */
extern void k_sem_give(struct k_sem *sem);

/**
 * @brief Reset a semaphore's count to zero.
 *
 * This routine sets the count of @a sem to zero.
 *
 * @param sem Address of the semaphore.
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
 * This routine returns the current count of @a sem.
 *
 * @param sem Address of the semaphore.
 *
 * @return Current semaphore count.
 */
static inline unsigned int k_sem_count_get(struct k_sem *sem)
{
	return sem->count;
}

/**
 * @brief Statically define and initialize a semaphore.
 *
 * The semaphore can be accessed outside the module where it is defined using:
 *
 * @code extern struct k_sem <name>; @endcode
 *
 * @param name Name of the semaphore.
 * @param initial_count Initial semaphore count.
 * @param count_limit Maximum permitted semaphore count.
 */
#define K_SEM_DEFINE(name, initial_count, count_limit) \
	struct k_sem name \
		__in_section(_k_sem, static, name) = \
		K_SEM_INITIALIZER(name, initial_count, count_limit)

/**
 * @} end defgroup semaphore_apis
 */

/**
 * @defgroup alert_apis Alert APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @typedef k_alert_handler_t
 * @brief Alert handler function type.
 *
 * An alert's alert handler function is invoked by the system workqueue
 * when the alert is signalled. The alert handler function is optional,
 * and is only invoked if the alert has been initialized with one.
 *
 * @param alert Address of the alert.
 *
 * @return 0 if alert has been consumed; non-zero if alert should pend.
 */
typedef int (*k_alert_handler_t)(struct k_alert *alert);

/**
 * @} end defgroup alert_apis
 */

/**
 * @cond INTERNAL_HIDDEN
 */

#define K_ALERT_DEFAULT NULL
#define K_ALERT_IGNORE ((void *)(-1))

struct k_alert {
	k_alert_handler_t handler;
	atomic_t send_count;
	struct k_work work_item;
	struct k_sem sem;

	_OBJECT_TRACING_NEXT_PTR(k_alert);
};

extern void _alert_deliver(struct k_work *work);

#define K_ALERT_INITIALIZER(obj, alert_handler, max_num_pending_alerts) \
	{ \
	.handler = (k_alert_handler_t)alert_handler, \
	.send_count = ATOMIC_INIT(0), \
	.work_item = K_WORK_INITIALIZER(_alert_deliver), \
	.sem = K_SEM_INITIALIZER(obj.sem, 0, max_num_pending_alerts), \
	_OBJECT_TRACING_INIT \
	}

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @addtogroup alert_apis
 * @{
 */

/**
 * @brief Statically define and initialize an alert.
 *
 * The alert can be accessed outside the module where it is defined using:
 *
 * @code extern struct k_alert <name>; @endcode
 *
 * @param name Name of the alert.
 * @param alert_handler Action to take when alert is sent. Specify either
 *        the address of a function to be invoked by the system workqueue
 *        thread, K_ALERT_IGNORE (which causes the alert to be ignored), or
 *        K_ALERT_DEFAULT (which causes the alert to pend).
 * @param max_num_pending_alerts Maximum number of pending alerts.
 */
#define K_ALERT_DEFINE(name, alert_handler, max_num_pending_alerts) \
	struct k_alert name \
		__in_section(_k_alert, static, name) = \
		K_ALERT_INITIALIZER(name, alert_handler, \
				    max_num_pending_alerts)

/**
 * @brief Initialize an alert.
 *
 * This routine initializes an alert object, prior to its first use.
 *
 * @param alert Address of the alert.
 * @param handler Action to take when alert is sent. Specify either the address
 *                of a function to be invoked by the system workqueue thread,
 *                K_ALERT_IGNORE (which causes the alert to be ignored), or
 *                K_ALERT_DEFAULT (which causes the alert to pend).
 * @param max_num_pending_alerts Maximum number of pending alerts.
 *
 * @return N/A
 */
extern void k_alert_init(struct k_alert *alert, k_alert_handler_t handler,
			 unsigned int max_num_pending_alerts);

/**
 * @brief Receive an alert.
 *
 * This routine receives a pending alert for @a alert.
 *
 * @note Can be called by ISRs, but @a timeout must be set to K_NO_WAIT.
 *
 * @param alert Address of the alert.
 * @param timeout Waiting period to receive the alert (in milliseconds),
 *                or one of the special values K_NO_WAIT and K_FOREVER.
 *
 * @retval 0 Alert received.
 * @retval -EBUSY Returned without waiting.
 * @retval -EAGAIN Waiting period timed out.
 */
extern int k_alert_recv(struct k_alert *alert, int32_t timeout);

/**
 * @brief Signal an alert.
 *
 * This routine signals @a alert. The action specified for @a alert will
 * be taken, which may trigger the execution of an alert handler function
 * and/or cause the alert to pend (assuming the alert has not reached its
 * maximum number of pending alerts).
 *
 * @note Can be called by ISRs.
 *
 * @param alert Address of the alert.
 *
 * @return N/A
 */
extern void k_alert_send(struct k_alert *alert);

/**
 * @} end addtogroup alert_apis
 */

/**
 * @cond INTERNAL_HIDDEN
 */

struct k_msgq {
	_wait_q_t wait_q;
	size_t msg_size;
	uint32_t max_msgs;
	char *buffer_start;
	char *buffer_end;
	char *read_ptr;
	char *write_ptr;
	uint32_t used_msgs;

	_OBJECT_TRACING_NEXT_PTR(k_msgq);
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
	_OBJECT_TRACING_INIT \
	}

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @defgroup msgq_apis Message Queue APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief Statically define and initialize a message queue.
 *
 * The message queue's ring buffer contains space for @a q_max_msgs messages,
 * each of which is @a q_msg_size bytes long. The buffer is aligned to a
 * @a q_align -byte boundary, which must be a power of 2. To ensure that each
 * message is similarly aligned to this boundary, @a q_msg_size must also be
 * a multiple of @a q_align.
 *
 * The message queue can be accessed outside the module where it is defined
 * using:
 *
 * @code extern struct k_msgq <name>; @endcode
 *
 * @param q_name Name of the message queue.
 * @param q_msg_size Message size (in bytes).
 * @param q_max_msgs Maximum number of messages that can be queued.
 * @param q_align Alignment of the message queue's ring buffer.
 */
#define K_MSGQ_DEFINE(q_name, q_msg_size, q_max_msgs, q_align)      \
	static char __noinit __aligned(q_align)                     \
		_k_fifo_buf_##q_name[(q_max_msgs) * (q_msg_size)];  \
	struct k_msgq q_name                                        \
		__in_section(_k_msgq, static, q_name) =        \
	       K_MSGQ_INITIALIZER(q_name, _k_fifo_buf_##q_name,     \
				  q_msg_size, q_max_msgs)

/**
 * @brief Initialize a message queue.
 *
 * This routine initializes a message queue object, prior to its first use.
 *
 * The message queue's ring buffer must contain space for @a max_msgs messages,
 * each of which is @a msg_size bytes long. The buffer must be aligned to an
 * N-byte boundary, where N is a power of 2 (i.e. 1, 2, 4, ...). To ensure
 * that each message is similarly aligned to this boundary, @a q_msg_size
 * must also be a multiple of N.
 *
 * @param q Address of the message queue.
 * @param buffer Pointer to ring buffer that holds queued messages.
 * @param msg_size Message size (in bytes).
 * @param max_msgs Maximum number of messages that can be queued.
 *
 * @return N/A
 */
extern void k_msgq_init(struct k_msgq *q, char *buffer,
			size_t msg_size, uint32_t max_msgs);

/**
 * @brief Send a message to a message queue.
 *
 * This routine sends a message to message queue @a q.
 *
 * @note Can be called by ISRs.
 *
 * @param q Address of the message queue.
 * @param data Pointer to the message.
 * @param timeout Waiting period to add the message (in milliseconds),
 *                or one of the special values K_NO_WAIT and K_FOREVER.
 *
 * @retval 0 Message sent.
 * @retval -ENOMSG Returned without waiting or queue purged.
 * @retval -EAGAIN Waiting period timed out.
 */
extern int k_msgq_put(struct k_msgq *q, void *data, int32_t timeout);

/**
 * @brief Receive a message from a message queue.
 *
 * This routine receives a message from message queue @a q in a "first in,
 * first out" manner.
 *
 * @note Can be called by ISRs, but @a timeout must be set to K_NO_WAIT.
 *
 * @param q Address of the message queue.
 * @param data Address of area to hold the received message.
 * @param timeout Waiting period to receive the message (in milliseconds),
 *                or one of the special values K_NO_WAIT and K_FOREVER.
 *
 * @retval 0 Message received.
 * @retval -ENOMSG Returned without waiting.
 * @retval -EAGAIN Waiting period timed out.
 */
extern int k_msgq_get(struct k_msgq *q, void *data, int32_t timeout);

/**
 * @brief Purge a message queue.
 *
 * This routine discards all unreceived messages in a message queue's ring
 * buffer. Any threads that are blocked waiting to send a message to the
 * message queue are unblocked and see an -ENOMSG error code.
 *
 * @param q Address of the message queue.
 *
 * @return N/A
 */
extern void k_msgq_purge(struct k_msgq *q);

/**
 * @brief Get the amount of free space in a message queue.
 *
 * This routine returns the number of unused entries in a message queue's
 * ring buffer.
 *
 * @param q Address of the message queue.
 *
 * @return Number of unused ring buffer entries.
 */
static inline uint32_t k_msgq_num_free_get(struct k_msgq *q)
{
	return q->max_msgs - q->used_msgs;
}

/**
 * @brief Get the number of messages in a message queue.
 *
 * This routine returns the number of messages in a message queue's ring buffer.
 *
 * @param q Address of the message queue.
 *
 * @return Number of messages.
 */
static inline uint32_t k_msgq_num_used_get(struct k_msgq *q)
{
	return q->used_msgs;
}

/**
 * @} end defgroup msgq_apis
 */

/**
 * @defgroup mem_pool_apis Memory Pool APIs
 * @ingroup kernel_apis
 * @{
 */

struct k_mem_block {
	struct k_mem_pool *pool_id;
	void *addr_in_pool;
	void *data;
	size_t req_size;
};

/**
 * @} end defgroup mem_pool_apis
 */

/**
 * @defgroup mailbox_apis Mailbox APIs
 * @ingroup kernel_apis
 * @{
 */

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

/**
 * @cond INTERNAL_HIDDEN
 */

struct k_mbox {
	_wait_q_t tx_msg_queue;
	_wait_q_t rx_msg_queue;

	_OBJECT_TRACING_NEXT_PTR(k_mbox);
};

#define K_MBOX_INITIALIZER(obj) \
	{ \
	.tx_msg_queue = SYS_DLIST_STATIC_INIT(&obj.tx_msg_queue), \
	.rx_msg_queue = SYS_DLIST_STATIC_INIT(&obj.rx_msg_queue), \
	_OBJECT_TRACING_INIT \
	}

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @brief Statically define and initialize a mailbox.
 *
 * The mailbox is to be accessed outside the module where it is defined using:
 *
 * @code extern struct k_mbox <name>; @endcode
 *
 * @param name Name of the mailbox.
 */
#define K_MBOX_DEFINE(name) \
	struct k_mbox name \
		__in_section(_k_mbox, static, name) = \
		K_MBOX_INITIALIZER(name) \

/**
 * @brief Initialize a mailbox.
 *
 * This routine initializes a mailbox object, prior to its first use.
 *
 * @param mbox Address of the mailbox.
 *
 * @return N/A
 */
extern void k_mbox_init(struct k_mbox *mbox);

/**
 * @brief Send a mailbox message in a synchronous manner.
 *
 * This routine sends a message to @a mbox and waits for a receiver to both
 * receive and process it. The message data may be in a buffer, in a memory
 * pool block, or non-existent (i.e. an empty message).
 *
 * @param mbox Address of the mailbox.
 * @param tx_msg Address of the transmit message descriptor.
 * @param timeout Waiting period for the message to be received (in
 *                milliseconds), or one of the special values K_NO_WAIT
 *                and K_FOREVER. Once the message has been received,
 *                this routine waits as long as necessary for the message
 *                to be completely processed.
 *
 * @retval 0 Message sent.
 * @retval -ENOMSG Returned without waiting.
 * @retval -EAGAIN Waiting period timed out.
 */
extern int k_mbox_put(struct k_mbox *mbox, struct k_mbox_msg *tx_msg,
		      int32_t timeout);

/**
 * @brief Send a mailbox message in an asynchronous manner.
 *
 * This routine sends a message to @a mbox without waiting for a receiver
 * to process it. The message data may be in a buffer, in a memory pool block,
 * or non-existent (i.e. an empty message). Optionally, the semaphore @a sem
 * will be given when the message has been both received and completely
 * processed by the receiver.
 *
 * @param mbox Address of the mailbox.
 * @param tx_msg Address of the transmit message descriptor.
 * @param sem Address of a semaphore, or NULL if none is needed.
 *
 * @return N/A
 */
extern void k_mbox_async_put(struct k_mbox *mbox, struct k_mbox_msg *tx_msg,
			     struct k_sem *sem);

/**
 * @brief Receive a mailbox message.
 *
 * This routine receives a message from @a mbox, then optionally retrieves
 * its data and disposes of the message.
 *
 * @param mbox Address of the mailbox.
 * @param rx_msg Address of the receive message descriptor.
 * @param buffer Address of the buffer to receive data, or NULL to defer data
 *               retrieval and message disposal until later.
 * @param timeout Waiting period for a message to be received (in
 *                milliseconds), or one of the special values K_NO_WAIT
 *                and K_FOREVER.
 *
 * @retval 0 Message received.
 * @retval -ENOMSG Returned without waiting.
 * @retval -EAGAIN Waiting period timed out.
 */
extern int k_mbox_get(struct k_mbox *mbox, struct k_mbox_msg *rx_msg,
		      void *buffer, int32_t timeout);

/**
 * @brief Retrieve mailbox message data into a buffer.
 *
 * This routine completes the processing of a received message by retrieving
 * its data into a buffer, then disposing of the message.
 *
 * Alternatively, this routine can be used to dispose of a received message
 * without retrieving its data.
 *
 * @param rx_msg Address of the receive message descriptor.
 * @param buffer Address of the buffer to receive data, or NULL to discard
 *               the data.
 *
 * @return N/A
 */
extern void k_mbox_data_get(struct k_mbox_msg *rx_msg, void *buffer);

/**
 * @brief Retrieve mailbox message data into a memory pool block.
 *
 * This routine completes the processing of a received message by retrieving
 * its data into a memory pool block, then disposing of the message.
 * The memory pool block that results from successful retrieval must be
 * returned to the pool once the data has been processed, even in cases
 * where zero bytes of data are retrieved.
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
 * @param rx_msg Address of a receive message descriptor.
 * @param pool Address of memory pool, or NULL to discard data.
 * @param block Address of the area to hold memory pool block info.
 * @param timeout Waiting period to wait for a memory pool block (in
 *                milliseconds), or one of the special values K_NO_WAIT
 *                and K_FOREVER.
 *
 * @retval 0 Data retrieved.
 * @retval -ENOMEM Returned without waiting.
 * @retval -EAGAIN Waiting period timed out.
 */
extern int k_mbox_data_block_get(struct k_mbox_msg *rx_msg,
				 struct k_mem_pool *pool,
				 struct k_mem_block *block, int32_t timeout);

/**
 * @} end defgroup mailbox_apis
 */

/**
 * @cond INTERNAL_HIDDEN
 */

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

	_OBJECT_TRACING_NEXT_PTR(k_pipe);
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
	_OBJECT_TRACING_INIT                            \
	}

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @defgroup pipe_apis Pipe APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief Statically define and initialize a pipe.
 *
 * The pipe can be accessed outside the module where it is defined using:
 *
 * @code extern struct k_pipe <name>; @endcode
 *
 * @param name Name of the pipe.
 * @param pipe_buffer_size Size of the pipe's ring buffer (in bytes),
 *                         or zero if no ring buffer is used.
 * @param pipe_align Alignment of the pipe's ring buffer (power of 2).
 */
#define K_PIPE_DEFINE(name, pipe_buffer_size, pipe_align)     \
	static unsigned char __noinit __aligned(pipe_align)   \
		_k_pipe_buf_##name[pipe_buffer_size];         \
	struct k_pipe name                                    \
		__in_section(_k_pipe, static, name) =    \
		K_PIPE_INITIALIZER(name, _k_pipe_buf_##name, pipe_buffer_size)

/**
 * @brief Initialize a pipe.
 *
 * This routine initializes a pipe object, prior to its first use.
 *
 * @param pipe Address of the pipe.
 * @param buffer Address of the pipe's ring buffer, or NULL if no ring buffer
 *               is used.
 * @param size Size of the pipe's ring buffer (in bytes), or zero if no ring
 *             buffer is used.
 *
 * @return N/A
 */
extern void k_pipe_init(struct k_pipe *pipe, unsigned char *buffer,
			size_t size);

/**
 * @brief Write data to a pipe.
 *
 * This routine writes up to @a bytes_to_write bytes of data to @a pipe.
 *
 * @param pipe Address of the pipe.
 * @param data Address of data to write.
 * @param bytes_to_write Size of data (in bytes).
 * @param bytes_written Address of area to hold the number of bytes written.
 * @param min_xfer Minimum number of bytes to write.
 * @param timeout Waiting period to wait for the data to be written (in
 *                milliseconds), or one of the special values K_NO_WAIT
 *                and K_FOREVER.
 *
 * @retval 0 At least @a min_xfer bytes of data were written.
 * @retval -EIO Returned without waiting; zero data bytes were written.
 * @retval -EAGAIN Waiting period timed out; between zero and @a min_xfer
 *                 minus one data bytes were written.
 */
extern int k_pipe_put(struct k_pipe *pipe, void *data,
		      size_t bytes_to_write, size_t *bytes_written,
		      size_t min_xfer, int32_t timeout);

/**
 * @brief Read data from a pipe.
 *
 * This routine reads up to @a bytes_to_read bytes of data from @a pipe.
 *
 * @param pipe Address of the pipe.
 * @param data Address to place the data read from pipe.
 * @param bytes_to_read Maximum number of data bytes to read.
 * @param bytes_read Address of area to hold the number of bytes read.
 * @param min_xfer Minimum number of data bytes to read.
 * @param timeout Waiting period to wait for the data to be read (in
 *                milliseconds), or one of the special values K_NO_WAIT
 *                and K_FOREVER.
 *
 * @retval 0 At least @a min_xfer bytes of data were read.
 * @retval -EIO Returned without waiting; zero data bytes were read.
 * @retval -EAGAIN Waiting period timed out; between zero and @a min_xfer
 *                 minus one data bytes were read.
 */
extern int k_pipe_get(struct k_pipe *pipe, void *data,
		      size_t bytes_to_read, size_t *bytes_read,
		      size_t min_xfer, int32_t timeout);

/**
 * @brief Write memory block to a pipe.
 *
 * This routine writes the data contained in a memory block to @a pipe.
 * Once all of the data in the block has been written to the pipe, it will
 * free the memory block @a block and give the semaphore @a sem (if specified).
 *
 * @param pipe Address of the pipe.
 * @param block Memory block containing data to send
 * @param size Number of data bytes in memory block to send
 * @param sem Semaphore to signal upon completion (else NULL)
 *
 * @return N/A
 */
extern void k_pipe_block_put(struct k_pipe *pipe, struct k_mem_block *block,
			     size_t size, struct k_sem *sem);

/**
 * @} end defgroup pipe_apis
 */

/**
 * @cond INTERNAL_HIDDEN
 */

struct k_mem_slab {
	_wait_q_t wait_q;
	uint32_t num_blocks;
	size_t block_size;
	char *buffer;
	char *free_list;
	uint32_t num_used;

	_OBJECT_TRACING_NEXT_PTR(k_mem_slab);
};

#define K_MEM_SLAB_INITIALIZER(obj, slab_buffer, slab_block_size, \
			       slab_num_blocks) \
	{ \
	.wait_q = SYS_DLIST_STATIC_INIT(&obj.wait_q), \
	.num_blocks = slab_num_blocks, \
	.block_size = slab_block_size, \
	.buffer = slab_buffer, \
	.free_list = NULL, \
	.num_used = 0, \
	_OBJECT_TRACING_INIT \
	}

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @defgroup mem_slab_apis Memory Slab APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief Statically define and initialize a memory slab.
 *
 * The memory slab's buffer contains @a slab_num_blocks memory blocks
 * that are @a slab_block_size bytes long. The buffer is aligned to a
 * @a slab_align -byte boundary. To ensure that each memory block is similarly
 * aligned to this boundary, @a slab_block_size must also be a multiple of
 * @a slab_align.
 *
 * The memory slab can be accessed outside the module where it is defined
 * using:
 *
 * @code extern struct k_mem_slab <name>; @endcode
 *
 * @param name Name of the memory slab.
 * @param slab_block_size Size of each memory block (in bytes).
 * @param slab_num_blocks Number memory blocks.
 * @param slab_align Alignment of the memory slab's buffer (power of 2).
 */
#define K_MEM_SLAB_DEFINE(name, slab_block_size, slab_num_blocks, slab_align) \
	char __noinit __aligned(slab_align) \
		_k_mem_slab_buf_##name[(slab_num_blocks) * (slab_block_size)]; \
	struct k_mem_slab name \
		__in_section(_k_mem_slab, static, name) = \
		K_MEM_SLAB_INITIALIZER(name, _k_mem_slab_buf_##name, \
				      slab_block_size, slab_num_blocks)

/**
 * @brief Initialize a memory slab.
 *
 * Initializes a memory slab, prior to its first use.
 *
 * The memory slab's buffer contains @a slab_num_blocks memory blocks
 * that are @a slab_block_size bytes long. The buffer must be aligned to an
 * N-byte boundary, where N is a power of 2 larger than 2 (i.e. 4, 8, 16, ...).
 * To ensure that each memory block is similarly aligned to this boundary,
 * @a slab_block_size must also be a multiple of N.
 *
 * @param slab Address of the memory slab.
 * @param buffer Pointer to buffer used for the memory blocks.
 * @param block_size Size of each memory block (in bytes).
 * @param num_blocks Number of memory blocks.
 *
 * @return N/A
 */
extern void k_mem_slab_init(struct k_mem_slab *slab, void *buffer,
			   size_t block_size, uint32_t num_blocks);

/**
 * @brief Allocate memory from a memory slab.
 *
 * This routine allocates a memory block from a memory slab.
 *
 * @param slab Address of the memory slab.
 * @param mem Pointer to block address area.
 * @param timeout Maximum time to wait for operation to complete
 *        (in milliseconds). Use K_NO_WAIT to return without waiting,
 *        or K_FOREVER to wait as long as necessary.
 *
 * @retval 0 Memory allocated. The block address area pointed at by @a mem
 *         is set to the starting address of the memory block.
 * @retval -ENOMEM Returned without waiting.
 * @retval -EAGAIN Waiting period timed out.
 */
extern int k_mem_slab_alloc(struct k_mem_slab *slab, void **mem,
			    int32_t timeout);

/**
 * @brief Free memory allocated from a memory slab.
 *
 * This routine releases a previously allocated memory block back to its
 * associated memory slab.
 *
 * @param slab Address of the memory slab.
 * @param mem Pointer to block address area (as set by k_mem_slab_alloc()).
 *
 * @return N/A
 */
extern void k_mem_slab_free(struct k_mem_slab *slab, void **mem);

/**
 * @brief Get the number of used blocks in a memory slab.
 *
 * This routine gets the number of memory blocks that are currently
 * allocated in @a slab.
 *
 * @param slab Address of the memory slab.
 *
 * @return Number of allocated memory blocks.
 */
static inline uint32_t k_mem_slab_num_used_get(struct k_mem_slab *slab)
{
	return slab->num_used;
}

/**
 * @brief Get the number of unused blocks in a memory slab.
 *
 * This routine gets the number of memory blocks that are currently
 * unallocated in @a slab.
 *
 * @param slab Address of the memory slab.
 *
 * @return Number of unallocated memory blocks.
 */
static inline uint32_t k_mem_slab_num_free_get(struct k_mem_slab *slab)
{
	return slab->num_blocks - slab->num_used;
}

/**
 * @} end defgroup mem_slab_apis
 */

/**
 * @cond INTERNAL_HIDDEN
 */

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
	size_t block_size; /* memory block size */
	uint32_t nr_of_entries; /* nr of quad block structures in the array */
	struct k_mem_pool_quad_block *quad_block;
	int count;
};

/* Memory pool descriptor */
struct k_mem_pool {
	size_t max_block_size;
	size_t min_block_size;
	uint32_t nr_of_maxblocks;
	uint32_t nr_of_block_sets;
	struct k_mem_pool_block_set *block_set;
	char *bufblock;
	_wait_q_t wait_q;
	_OBJECT_TRACING_NEXT_PTR(k_mem_pool);
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
	".balign 4\n\t"
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
	".pushsection ._k_mem_pool.static.\\name,\"aw\","
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
	__asm__(".balign 4\n\t");			\
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

/*
 * Dummy function that assigns the value of sizeof(struct k_mem_pool_quad_block)
 * to __memory_pool_quad_block_size absolute symbol.
 * This function does not get called, but compiler calculates the value and
 * assigns it to the absolute symbol, that, in turn is used by assembler macros.
 */
static void __attribute__ ((used)) __k_mem_pool_quad_block_size_define(void)
{
	__asm__(".globl __memory_pool_quad_block_size\n\t"
#ifdef CONFIG_NIOS2
	    "__memory_pool_quad_block_size = %0\n\t"
#else
	    "__memory_pool_quad_block_size = %c0\n\t"
#endif
	    :
	    : "n"(sizeof(struct k_mem_pool_quad_block)));
}

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @addtogroup mem_pool_apis
 * @{
 */

/**
 * @brief Statically define and initialize a memory pool.
 *
 * The memory pool's buffer contains @a n_max blocks that are @a max_size bytes
 * long. The memory pool allows blocks to be repeatedly partitioned into
 * quarters, down to blocks of @a min_size bytes long. The buffer is aligned
 * to a @a align -byte boundary. To ensure that the minimum sized blocks are
 * similarly aligned to this boundary, @a min_size must also be a multiple of
 * @a align.
 *
 * If the pool is to be accessed outside the module where it is defined, it
 * can be declared via
 *
 * @code extern struct k_mem_pool <name>; @endcode
 *
 * @param name Name of the memory pool.
 * @param min_size Size of the smallest blocks in the pool (in bytes).
 * @param max_size Size of the largest blocks in the pool (in bytes).
 * @param n_max Number of maximum sized blocks in the pool.
 * @param align Alignment of the pool's buffer (power of 2).
 */
#define K_MEM_POOL_DEFINE(name, min_size, max_size, n_max, align)     \
	_MEMORY_POOL_QUAD_BLOCK_DEFINE(name, min_size, max_size, n_max); \
	_MEMORY_POOL_BLOCK_SETS_DEFINE(name, min_size, max_size, n_max); \
	_MEMORY_POOL_BUFFER_DEFINE(name, max_size, n_max, align);        \
	__asm__("_build_mem_pool " STRINGIFY(name) " " STRINGIFY(min_size) " " \
	       STRINGIFY(max_size) " " STRINGIFY(n_max) "\n\t");	\
	extern struct k_mem_pool name

/**
 * @brief Allocate memory from a memory pool.
 *
 * This routine allocates a memory block from a memory pool.
 *
 * @param pool Address of the memory pool.
 * @param block Pointer to block descriptor for the allocated memory.
 * @param size Amount of memory to allocate (in bytes).
 * @param timeout Maximum time to wait for operation to complete
 *        (in milliseconds). Use K_NO_WAIT to return without waiting,
 *        or K_FOREVER to wait as long as necessary.
 *
 * @retval 0 Memory allocated. The @a data field of the block descriptor
 *         is set to the starting address of the memory block.
 * @retval -ENOMEM Returned without waiting.
 * @retval -EAGAIN Waiting period timed out.
 */
extern int k_mem_pool_alloc(struct k_mem_pool *pool, struct k_mem_block *block,
			    size_t size, int32_t timeout);

/**
 * @brief Free memory allocated from a memory pool.
 *
 * This routine releases a previously allocated memory block back to its
 * memory pool.
 *
 * @param block Pointer to block descriptor for the allocated memory.
 *
 * @return N/A
 */
extern void k_mem_pool_free(struct k_mem_block *block);

/**
 * @brief Defragment a memory pool.
 *
 * This routine instructs a memory pool to concatenate unused memory blocks
 * into larger blocks wherever possible. Manually defragmenting the memory
 * pool may speed up future allocations of memory blocks by eliminating the
 * need for the memory pool to perform an automatic partial defragmentation.
 *
 * @param pool Address of the memory pool.
 *
 * @return N/A
 */
extern void k_mem_pool_defrag(struct k_mem_pool *pool);

/**
 * @} end addtogroup mem_pool_apis
 */

/**
 * @defgroup heap_apis Heap Memory Pool APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief Allocate memory from heap.
 *
 * This routine provides traditional malloc() semantics. Memory is
 * allocated from the heap memory pool.
 *
 * @param size Amount of memory requested (in bytes).
 *
 * @return Address of the allocated memory if successful; otherwise NULL.
 */
extern void *k_malloc(size_t size);

/**
 * @brief Free memory allocated from heap.
 *
 * This routine provides traditional free() semantics. The memory being
 * returned must have been allocated from the heap memory pool.
 *
 * If @a ptr is NULL, no operation is performed.
 *
 * @param ptr Pointer to previously allocated memory.
 *
 * @return N/A
 */
extern void k_free(void *ptr);

/**
 * @} end defgroup heap_apis
 */

/**
 * @brief Make the CPU idle.
 *
 * This function makes the CPU idle until an event wakes it up.
 *
 * In a regular system, the idle thread should be the only thread responsible
 * for making the CPU idle and triggering any type of power management.
 * However, in some more constrained systems, such as a single-threaded system,
 * the only thread would be responsible for this if needed.
 *
 * @return N/A
 */
extern void k_cpu_idle(void);

/**
 * @brief Make the CPU idle in an atomic fashion.
 *
 * Similar to k_cpu_idle(), but called with interrupts locked if operations
 * must be done atomically before making the CPU idle.
 *
 * @param key Interrupt locking key obtained from irq_lock().
 *
 * @return N/A
 */
extern void k_cpu_atomic_idle(unsigned int key);

extern void _sys_power_save_idle_exit(int32_t ticks);

/* Include legacy APIs */
#if defined(CONFIG_LEGACY_KERNEL)
#include <legacy.h>
#endif
#include <arch/cpu.h>

/*
 * private APIs that are utilized by one or more public APIs
 */

#ifdef CONFIG_MULTITHREADING
extern void _init_static_threads(void);
#else
#define _init_static_threads() do { } while ((0))
#endif

extern int _is_thread_essential(void);
extern void _timer_expiration_handler(struct _timeout *t);

#ifdef __cplusplus
}
#endif

#if defined(CONFIG_CPLUSPLUS) && defined(__cplusplus)
/*
 * Define new and delete operators.
 * At this moment, the operators do nothing since objects are supposed
 * to be statically allocated.
 */
inline void operator delete(void *ptr)
{
	(void)ptr;
}

inline void operator delete[](void *ptr)
{
	(void)ptr;
}

inline void *operator new(size_t size)
{
	(void)size;
	return NULL;
}

inline void *operator new[](size_t size)
{
	(void)size;
	return NULL;
}

/* Placement versions of operator new and delete */
inline void operator delete(void *ptr1, void *ptr2)
{
	(void)ptr1;
	(void)ptr2;
}

inline void operator delete[](void *ptr1, void *ptr2)
{
	(void)ptr1;
	(void)ptr2;
}

inline void *operator new(size_t size, void *ptr)
{
	(void)size;
	return ptr;
}

inline void *operator new[](size_t size, void *ptr)
{
	(void)size;
	return ptr;
}

#endif /* defined(CONFIG_CPLUSPLUS) && defined(__cplusplus) */

#endif /* !_ASMLANGUAGE */

#endif /* _kernel__h_ */
