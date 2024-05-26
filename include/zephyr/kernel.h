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

#ifndef ZEPHYR_INCLUDE_KERNEL_H_
#define ZEPHYR_INCLUDE_KERNEL_H_

#if !defined(_ASMLANGUAGE)
#include <zephyr/kernel_includes.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <zephyr/toolchain.h>
#include <zephyr/tracing/tracing_macros.h>
#include <zephyr/sys/mem_stats.h>
#include <zephyr/sys/iterable_sections.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Zephyr currently assumes the size of a couple standard types to simplify
 * print string formats. Let's make sure this doesn't change without notice.
 */
BUILD_ASSERT(sizeof(int32_t) == sizeof(int));
BUILD_ASSERT(sizeof(int64_t) == sizeof(long long));
BUILD_ASSERT(sizeof(intptr_t) == sizeof(long));

/**
 * @brief Kernel APIs
 * @defgroup kernel_apis Kernel APIs
 * @since 1.0
 * @version 1.0.0
 * @{
 * @}
 */

#define K_ANY NULL

#if CONFIG_NUM_COOP_PRIORITIES + CONFIG_NUM_PREEMPT_PRIORITIES == 0
#error Zero available thread priorities defined!
#endif

#define K_PRIO_COOP(x) (-(CONFIG_NUM_COOP_PRIORITIES - (x)))
#define K_PRIO_PREEMPT(x) (x)

#define K_HIGHEST_THREAD_PRIO (-CONFIG_NUM_COOP_PRIORITIES)
#define K_LOWEST_THREAD_PRIO CONFIG_NUM_PREEMPT_PRIORITIES
#define K_IDLE_PRIO K_LOWEST_THREAD_PRIO
#define K_HIGHEST_APPLICATION_THREAD_PRIO (K_HIGHEST_THREAD_PRIO)
#define K_LOWEST_APPLICATION_THREAD_PRIO (K_LOWEST_THREAD_PRIO - 1)

#ifdef CONFIG_POLL
#define Z_POLL_EVENT_OBJ_INIT(obj) \
	.poll_events = SYS_DLIST_STATIC_INIT(&obj.poll_events),
#define Z_DECL_POLL_EVENT sys_dlist_t poll_events;
#else
#define Z_POLL_EVENT_OBJ_INIT(obj)
#define Z_DECL_POLL_EVENT
#endif

struct k_thread;
struct k_mutex;
struct k_sem;
struct k_msgq;
struct k_mbox;
struct k_pipe;
struct k_queue;
struct k_fifo;
struct k_lifo;
struct k_stack;
struct k_mem_slab;
struct k_timer;
struct k_poll_event;
struct k_poll_signal;
struct k_mem_domain;
struct k_mem_partition;
struct k_futex;
struct k_event;

enum execution_context_types {
	K_ISR = 0,
	K_COOP_THREAD,
	K_PREEMPT_THREAD,
};

/* private, used by k_poll and k_work_poll */
struct k_work_poll;
typedef int (*_poller_cb_t)(struct k_poll_event *event, uint32_t state);

/**
 * @addtogroup thread_apis
 * @{
 */

typedef void (*k_thread_user_cb_t)(const struct k_thread *thread,
				   void *user_data);

/**
 * @brief Iterate over all the threads in the system.
 *
 * This routine iterates over all the threads in the system and
 * calls the user_cb function for each thread.
 *
 * @param user_cb Pointer to the user callback function.
 * @param user_data Pointer to user data.
 *
 * @note @kconfig{CONFIG_THREAD_MONITOR} must be set for this function
 * to be effective.
 * @note This API uses @ref k_spin_lock to protect the _kernel.threads
 * list which means creation of new threads and terminations of existing
 * threads are blocked until this API returns.
 */
void k_thread_foreach(k_thread_user_cb_t user_cb, void *user_data);

/**
 * @brief Iterate over all the threads in the system without locking.
 *
 * This routine works exactly the same like @ref k_thread_foreach
 * but unlocks interrupts when user_cb is executed.
 *
 * @param user_cb Pointer to the user callback function.
 * @param user_data Pointer to user data.
 *
 * @note @kconfig{CONFIG_THREAD_MONITOR} must be set for this function
 * to be effective.
 * @note This API uses @ref k_spin_lock only when accessing the _kernel.threads
 * queue elements. It unlocks it during user callback function processing.
 * If a new task is created when this @c foreach function is in progress,
 * the added new task would not be included in the enumeration.
 * If a task is aborted during this enumeration, there would be a race here
 * and there is a possibility that this aborted task would be included in the
 * enumeration.
 * @note If the task is aborted and the memory occupied by its @c k_thread
 * structure is reused when this @c k_thread_foreach_unlocked is in progress
 * it might even lead to the system behave unstable.
 * This function may never return, as it would follow some @c next task
 * pointers treating given pointer as a pointer to the k_thread structure
 * while it is something different right now.
 * Do not reuse the memory that was occupied by k_thread structure of aborted
 * task if it was aborted after this function was called in any context.
 */
void k_thread_foreach_unlocked(
	k_thread_user_cb_t user_cb, void *user_data);

/** @} */

/**
 * @defgroup thread_apis Thread APIs
 * @ingroup kernel_apis
 * @{
 */

#endif /* !_ASMLANGUAGE */


/*
 * Thread user options. May be needed by assembly code. Common part uses low
 * bits, arch-specific use high bits.
 */

/**
 * @brief system thread that must not abort
 * */
#define K_ESSENTIAL (BIT(0))

/**
 * @brief FPU registers are managed by context switch
 *
 * @details
 * This option indicates that the thread uses the CPU's floating point
 * registers. This instructs the kernel to take additional steps to save
 * and restore the contents of these registers when scheduling the thread.
 * No effect if @kconfig{CONFIG_FPU_SHARING} is not enabled.
 */
#define K_FP_IDX 1
#define K_FP_REGS (BIT(K_FP_IDX))

/**
 * @brief user mode thread
 *
 * This thread has dropped from supervisor mode to user mode and consequently
 * has additional restrictions
 */
#define K_USER (BIT(2))

/**
 * @brief Inherit Permissions
 *
 * @details
 * Indicates that the thread being created should inherit all kernel object
 * permissions from the thread that created it. No effect if
 * @kconfig{CONFIG_USERSPACE} is not enabled.
 */
#define K_INHERIT_PERMS (BIT(3))

/**
 * @brief Callback item state
 *
 * @details
 * This is a single bit of state reserved for "callback manager"
 * utilities (p4wq initially) who need to track operations invoked
 * from within a user-provided callback they have been invoked.
 * Effectively it serves as a tiny bit of zero-overhead TLS data.
 */
#define K_CALLBACK_STATE (BIT(4))

/**
 * @brief DSP registers are managed by context switch
 *
 * @details
 * This option indicates that the thread uses the CPU's DSP registers.
 * This instructs the kernel to take additional steps to save and
 * restore the contents of these registers when scheduling the thread.
 * No effect if @kconfig{CONFIG_DSP_SHARING} is not enabled.
 */
#define K_DSP_IDX 6
#define K_DSP_REGS (BIT(K_DSP_IDX))

/**
 * @brief AGU registers are managed by context switch
 *
 * @details
 * This option indicates that the thread uses the ARC processor's XY
 * memory and DSP feature. Often used with @kconfig{CONFIG_ARC_AGU_SHARING}.
 * No effect if @kconfig{CONFIG_ARC_AGU_SHARING} is not enabled.
 */
#define K_AGU_IDX 7
#define K_AGU_REGS (BIT(K_AGU_IDX))

/**
 * @brief FP and SSE registers are managed by context switch on x86
 *
 * @details
 * This option indicates that the thread uses the x86 CPU's floating point
 * and SSE registers. This instructs the kernel to take additional steps to
 * save and restore the contents of these registers when scheduling
 * the thread. No effect if @kconfig{CONFIG_X86_SSE} is not enabled.
 */
#define K_SSE_REGS (BIT(7))

/* end - thread options */

#if !defined(_ASMLANGUAGE)
/**
 * @brief Dynamically allocate a thread stack.
 *
 * Relevant stack creation flags include:
 * - @ref K_USER allocate a userspace thread (requires `CONFIG_USERSPACE=y`)
 *
 * @param size Stack size in bytes.
 * @param flags Stack creation flags, or 0.
 *
 * @retval the allocated thread stack on success.
 * @retval NULL on failure.
 *
 * @see CONFIG_DYNAMIC_THREAD
 */
__syscall k_thread_stack_t *k_thread_stack_alloc(size_t size, int flags);

/**
 * @brief Free a dynamically allocated thread stack.
 *
 * @param stack Pointer to the thread stack.
 *
 * @retval 0 on success.
 * @retval -EBUSY if the thread stack is in use.
 * @retval -EINVAL if @p stack is invalid.
 * @retval -ENOSYS if dynamic thread stack allocation is disabled
 *
 * @see CONFIG_DYNAMIC_THREAD
 */
__syscall int k_thread_stack_free(k_thread_stack_t *stack);

/**
 * @brief Create a thread.
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
 * Stack objects passed to this function must be originally defined with
 * either of these macros in order to be portable:
 *
 * - K_THREAD_STACK_DEFINE() - For stacks that may support either user or
 *   supervisor threads.
 * - K_KERNEL_STACK_DEFINE() - For stacks that may support supervisor
 *   threads only. These stacks use less memory if CONFIG_USERSPACE is
 *   enabled.
 *
 * The stack_size parameter has constraints. It must either be:
 *
 * - The original size value passed to K_THREAD_STACK_DEFINE() or
 *   K_KERNEL_STACK_DEFINE()
 * - The return value of K_THREAD_STACK_SIZEOF(stack) if the stack was
 *   defined with K_THREAD_STACK_DEFINE()
 * - The return value of K_KERNEL_STACK_SIZEOF(stack) if the stack was
 *   defined with K_KERNEL_STACK_DEFINE().
 *
 * Using other values, or sizeof(stack) may produce undefined behavior.
 *
 * @param new_thread Pointer to uninitialized struct k_thread
 * @param stack Pointer to the stack space.
 * @param stack_size Stack size in bytes.
 * @param entry Thread entry function.
 * @param p1 1st entry point parameter.
 * @param p2 2nd entry point parameter.
 * @param p3 3rd entry point parameter.
 * @param prio Thread priority.
 * @param options Thread options.
 * @param delay Scheduling delay, or K_NO_WAIT (for no delay).
 *
 * @return ID of new thread.
 *
 */
__syscall k_tid_t k_thread_create(struct k_thread *new_thread,
				  k_thread_stack_t *stack,
				  size_t stack_size,
				  k_thread_entry_t entry,
				  void *p1, void *p2, void *p3,
				  int prio, uint32_t options, k_timeout_t delay);

/**
 * @brief Drop a thread's privileges permanently to user mode
 *
 * This allows a supervisor thread to be re-used as a user thread.
 * This function does not return, but control will transfer to the provided
 * entry point as if this was a new user thread.
 *
 * The implementation ensures that the stack buffer contents are erased.
 * Any thread-local storage will be reverted to a pristine state.
 *
 * Memory domain membership, resource pool assignment, kernel object
 * permissions, priority, and thread options are preserved.
 *
 * A common use of this function is to re-use the main thread as a user thread
 * once all supervisor mode-only tasks have been completed.
 *
 * @param entry Function to start executing from
 * @param p1 1st entry point parameter
 * @param p2 2nd entry point parameter
 * @param p3 3rd entry point parameter
 */
FUNC_NORETURN void k_thread_user_mode_enter(k_thread_entry_t entry,
						   void *p1, void *p2,
						   void *p3);

/**
 * @brief Grant a thread access to a set of kernel objects
 *
 * This is a convenience function. For the provided thread, grant access to
 * the remaining arguments, which must be pointers to kernel objects.
 *
 * The thread object must be initialized (i.e. running). The objects don't
 * need to be.
 * Note that NULL shouldn't be passed as an argument.
 *
 * @param thread Thread to grant access to objects
 * @param ... list of kernel object pointers
 */
#define k_thread_access_grant(thread, ...) \
	FOR_EACH_FIXED_ARG(k_object_access_grant, (;), thread, __VA_ARGS__)

/**
 * @brief Assign a resource memory pool to a thread
 *
 * By default, threads have no resource pool assigned unless their parent
 * thread has a resource pool, in which case it is inherited. Multiple
 * threads may be assigned to the same memory pool.
 *
 * Changing a thread's resource pool will not migrate allocations from the
 * previous pool.
 *
 * @param thread Target thread to assign a memory pool for resource requests.
 * @param heap Heap object to use for resources,
 *             or NULL if the thread should no longer have a memory pool.
 */
static inline void k_thread_heap_assign(struct k_thread *thread,
					struct k_heap *heap)
{
	thread->resource_pool = heap;
}

#if defined(CONFIG_INIT_STACKS) && defined(CONFIG_THREAD_STACK_INFO)
/**
 * @brief Obtain stack usage information for the specified thread
 *
 * User threads will need to have permission on the target thread object.
 *
 * Some hardware may prevent inspection of a stack buffer currently in use.
 * If this API is called from supervisor mode, on the currently running thread,
 * on a platform which selects @kconfig{CONFIG_NO_UNUSED_STACK_INSPECTION}, an
 * error will be generated.
 *
 * @param thread Thread to inspect stack information
 * @param unused_ptr Output parameter, filled in with the unused stack space
 *	of the target thread in bytes.
 * @return 0 on success
 * @return -EBADF Bad thread object (user mode only)
 * @return -EPERM No permissions on thread object (user mode only)
 * #return -ENOTSUP Forbidden by hardware policy
 * @return -EINVAL Thread is uninitialized or exited (user mode only)
 * @return -EFAULT Bad memory address for unused_ptr (user mode only)
 */
__syscall int k_thread_stack_space_get(const struct k_thread *thread,
				       size_t *unused_ptr);
#endif

#if (K_HEAP_MEM_POOL_SIZE > 0)
/**
 * @brief Assign the system heap as a thread's resource pool
 *
 * Similar to k_thread_heap_assign(), but the thread will use
 * the kernel heap to draw memory.
 *
 * Use with caution, as a malicious thread could perform DoS attacks on the
 * kernel heap.
 *
 * @param thread Target thread to assign the system heap for resource requests
 *
 */
void k_thread_system_pool_assign(struct k_thread *thread);
#endif /* (K_HEAP_MEM_POOL_SIZE > 0) */

/**
 * @brief Sleep until a thread exits
 *
 * The caller will be put to sleep until the target thread exits, either due
 * to being aborted, self-exiting, or taking a fatal error. This API returns
 * immediately if the thread isn't running.
 *
 * This API may only be called from ISRs with a K_NO_WAIT timeout,
 * where it can be useful as a predicate to detect when a thread has
 * aborted.
 *
 * @param thread Thread to wait to exit
 * @param timeout upper bound time to wait for the thread to exit.
 * @retval 0 success, target thread has exited or wasn't running
 * @retval -EBUSY returned without waiting
 * @retval -EAGAIN waiting period timed out
 * @retval -EDEADLK target thread is joining on the caller, or target thread
 *                  is the caller
 */
__syscall int k_thread_join(struct k_thread *thread, k_timeout_t timeout);

/**
 * @brief Put the current thread to sleep.
 *
 * This routine puts the current thread to sleep for @a duration,
 * specified as a k_timeout_t object.
 *
 * @note if @a timeout is set to K_FOREVER then the thread is suspended.
 *
 * @param timeout Desired duration of sleep.
 *
 * @return Zero if the requested time has elapsed or if the thread was woken up
 * by the \ref k_wakeup call, the time left to sleep rounded up to the nearest
 * millisecond.
 */
__syscall int32_t k_sleep(k_timeout_t timeout);

/**
 * @brief Put the current thread to sleep.
 *
 * This routine puts the current thread to sleep for @a duration milliseconds.
 *
 * @param ms Number of milliseconds to sleep.
 *
 * @return Zero if the requested time has elapsed or if the thread was woken up
 * by the \ref k_wakeup call, the time left to sleep rounded up to the nearest
 * millisecond.
 */
static inline int32_t k_msleep(int32_t ms)
{
	return k_sleep(Z_TIMEOUT_MS(ms));
}

/**
 * @brief Put the current thread to sleep with microsecond resolution.
 *
 * This function is unlikely to work as expected without kernel tuning.
 * In particular, because the lower bound on the duration of a sleep is
 * the duration of a tick, @kconfig{CONFIG_SYS_CLOCK_TICKS_PER_SEC} must be
 * adjusted to achieve the resolution desired. The implications of doing
 * this must be understood before attempting to use k_usleep(). Use with
 * caution.
 *
 * @param us Number of microseconds to sleep.
 *
 * @return Zero if the requested time has elapsed or if the thread was woken up
 * by the \ref k_wakeup call, the time left to sleep rounded up to the nearest
 * microsecond.
 */
__syscall int32_t k_usleep(int32_t us);

/**
 * @brief Cause the current thread to busy wait.
 *
 * This routine causes the current thread to execute a "do nothing" loop for
 * @a usec_to_wait microseconds.
 *
 * @note The clock used for the microsecond-resolution delay here may
 * be skewed relative to the clock used for system timeouts like
 * k_sleep().  For example k_busy_wait(1000) may take slightly more or
 * less time than k_sleep(K_MSEC(1)), with the offset dependent on
 * clock tolerances.
 *
 * @note In case when @kconfig{CONFIG_SYSTEM_CLOCK_SLOPPY_IDLE} and
 * @kconfig{CONFIG_PM} options are enabled, this function may not work.
 * The timer/clock used for delay processing may be disabled/inactive.
 */
__syscall void k_busy_wait(uint32_t usec_to_wait);

/**
 * @brief Check whether it is possible to yield in the current context.
 *
 * This routine checks whether the kernel is in a state where it is possible to
 * yield or call blocking API's. It should be used by code that needs to yield
 * to perform correctly, but can feasibly be called from contexts where that
 * is not possible. For example in the PRE_KERNEL initialization step, or when
 * being run from the idle thread.
 *
 * @return True if it is possible to yield in the current context, false otherwise.
 */
bool k_can_yield(void);

/**
 * @brief Yield the current thread.
 *
 * This routine causes the current thread to yield execution to another
 * thread of the same or higher priority. If there are no other ready threads
 * of the same or higher priority, the routine returns immediately.
 */
__syscall void k_yield(void);

/**
 * @brief Wake up a sleeping thread.
 *
 * This routine prematurely wakes up @a thread from sleeping.
 *
 * If @a thread is not currently sleeping, the routine has no effect.
 *
 * @param thread ID of thread to wake.
 */
__syscall void k_wakeup(k_tid_t thread);

/**
 * @brief Query thread ID of the current thread.
 *
 * This unconditionally queries the kernel via a system call.
 *
 * @note Use k_current_get() unless absolutely sure this is necessary.
 *       This should only be used directly where the thread local
 *       variable cannot be used or may contain invalid values
 *       if thread local storage (TLS) is enabled. If TLS is not
 *       enabled, this is the same as k_current_get().
 *
 * @return ID of current thread.
 */
__attribute_const__
__syscall k_tid_t k_sched_current_thread_query(void);

/**
 * @brief Get thread ID of the current thread.
 *
 * @return ID of current thread.
 *
 */
__attribute_const__
static inline k_tid_t k_current_get(void)
{
#ifdef CONFIG_CURRENT_THREAD_USE_TLS

	/* Thread-local cache of current thread ID, set in z_thread_entry() */
	extern __thread k_tid_t z_tls_current;

	return z_tls_current;
#else
	return k_sched_current_thread_query();
#endif
}

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
 * After k_thread_abort() returns, the thread is guaranteed not to be
 * running or to become runnable anywhere on the system.  Normally
 * this is done via blocking the caller (in the same manner as
 * k_thread_join()), but in interrupt context on SMP systems the
 * implementation is required to spin for threads that are running on
 * other CPUs.
 *
 * @param thread ID of thread to abort.
 */
__syscall void k_thread_abort(k_tid_t thread);


/**
 * @brief Start an inactive thread
 *
 * If a thread was created with K_FOREVER in the delay parameter, it will
 * not be added to the scheduling queue until this function is called
 * on it.
 *
 * @param thread thread to start
 */
__syscall void k_thread_start(k_tid_t thread);

k_ticks_t z_timeout_expires(const struct _timeout *timeout);
k_ticks_t z_timeout_remaining(const struct _timeout *timeout);

#ifdef CONFIG_SYS_CLOCK_EXISTS

/**
 * @brief Get time when a thread wakes up, in system ticks
 *
 * This routine computes the system uptime when a waiting thread next
 * executes, in units of system ticks.  If the thread is not waiting,
 * it returns current system time.
 */
__syscall k_ticks_t k_thread_timeout_expires_ticks(const struct k_thread *thread);

static inline k_ticks_t z_impl_k_thread_timeout_expires_ticks(
						const struct k_thread *thread)
{
	return z_timeout_expires(&thread->base.timeout);
}

/**
 * @brief Get time remaining before a thread wakes up, in system ticks
 *
 * This routine computes the time remaining before a waiting thread
 * next executes, in units of system ticks.  If the thread is not
 * waiting, it returns zero.
 */
__syscall k_ticks_t k_thread_timeout_remaining_ticks(const struct k_thread *thread);

static inline k_ticks_t z_impl_k_thread_timeout_remaining_ticks(
						const struct k_thread *thread)
{
	return z_timeout_remaining(&thread->base.timeout);
}

#endif /* CONFIG_SYS_CLOCK_EXISTS */

/**
 * @cond INTERNAL_HIDDEN
 */

struct _static_thread_data {
	struct k_thread *init_thread;
	k_thread_stack_t *init_stack;
	unsigned int init_stack_size;
	k_thread_entry_t init_entry;
	void *init_p1;
	void *init_p2;
	void *init_p3;
	int init_prio;
	uint32_t init_options;
	const char *init_name;
#ifdef CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME
	int32_t init_delay_ms;
#else
	k_timeout_t init_delay;
#endif
};

#ifdef CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME
#define Z_THREAD_INIT_DELAY_INITIALIZER(ms) .init_delay_ms = (ms)
#define Z_THREAD_INIT_DELAY(thread) SYS_TIMEOUT_MS((thread)->init_delay_ms)
#else
#define Z_THREAD_INIT_DELAY_INITIALIZER(ms) .init_delay = SYS_TIMEOUT_MS(ms)
#define Z_THREAD_INIT_DELAY(thread) (thread)->init_delay
#endif

#define Z_THREAD_INITIALIZER(thread, stack, stack_size,           \
			    entry, p1, p2, p3,                   \
			    prio, options, delay, tname)         \
	{                                                        \
	.init_thread = (thread),				 \
	.init_stack = (stack),					 \
	.init_stack_size = (stack_size),                         \
	.init_entry = (k_thread_entry_t)entry,			 \
	.init_p1 = (void *)p1,                                   \
	.init_p2 = (void *)p2,                                   \
	.init_p3 = (void *)p3,                                   \
	.init_prio = (prio),                                     \
	.init_options = (options),                               \
	.init_name = STRINGIFY(tname),                           \
	Z_THREAD_INIT_DELAY_INITIALIZER(delay)			 \
	}

/*
 * Refer to K_THREAD_DEFINE() and K_KERNEL_THREAD_DEFINE() for
 * information on arguments.
 */
#define Z_THREAD_COMMON_DEFINE(name, stack_size,			\
			       entry, p1, p2, p3,			\
			       prio, options, delay)			\
	struct k_thread _k_thread_obj_##name;				\
	STRUCT_SECTION_ITERABLE(_static_thread_data,			\
				_k_thread_data_##name) =		\
		Z_THREAD_INITIALIZER(&_k_thread_obj_##name,		\
				     _k_thread_stack_##name, stack_size,\
				     entry, p1, p2, p3, prio, options,	\
				     delay, name);			\
	const k_tid_t name = (k_tid_t)&_k_thread_obj_##name

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
 * @param delay Scheduling delay (in milliseconds), zero for no delay.
 *
 * @note Static threads with zero delay should not normally have
 * MetaIRQ priority levels.  This can preempt the system
 * initialization handling (depending on the priority of the main
 * thread) and cause surprising ordering side effects.  It will not
 * affect anything in the OS per se, but consider it bad practice.
 * Use a SYS_INIT() callback if you need to run code before entrance
 * to the application main().
 */
#define K_THREAD_DEFINE(name, stack_size,                                \
			entry, p1, p2, p3,                               \
			prio, options, delay)                            \
	K_THREAD_STACK_DEFINE(_k_thread_stack_##name, stack_size);	 \
	Z_THREAD_COMMON_DEFINE(name, stack_size, entry, p1, p2, p3,	 \
			       prio, options, delay)

/**
 * @brief Statically define and initialize a thread intended to run only in kernel mode.
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
 * @note Threads defined by this can only run in kernel mode, and cannot be
 *       transformed into user thread via k_thread_user_mode_enter().
 *
 * @warning Depending on the architecture, the stack size (@p stack_size)
 *          may need to be multiples of CONFIG_MMU_PAGE_SIZE (if MMU)
 *          or in power-of-two size (if MPU).
 *
 * @param name Name of the thread.
 * @param stack_size Stack size in bytes.
 * @param entry Thread entry function.
 * @param p1 1st entry point parameter.
 * @param p2 2nd entry point parameter.
 * @param p3 3rd entry point parameter.
 * @param prio Thread priority.
 * @param options Thread options.
 * @param delay Scheduling delay (in milliseconds), zero for no delay.
 */
#define K_KERNEL_THREAD_DEFINE(name, stack_size,			\
			       entry, p1, p2, p3,			\
			       prio, options, delay)			\
	K_KERNEL_STACK_DEFINE(_k_thread_stack_##name, stack_size);	\
	Z_THREAD_COMMON_DEFINE(name, stack_size, entry, p1, p2, p3,	\
			       prio, options, delay)

/**
 * @brief Get a thread's priority.
 *
 * This routine gets the priority of @a thread.
 *
 * @param thread ID of thread whose priority is needed.
 *
 * @return Priority of @a thread.
 */
__syscall int k_thread_priority_get(k_tid_t thread);

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
 */
__syscall void k_thread_priority_set(k_tid_t thread, int prio);


#ifdef CONFIG_SCHED_DEADLINE
/**
 * @brief Set deadline expiration time for scheduler
 *
 * This sets the "deadline" expiration as a time delta from the
 * current time, in the same units used by k_cycle_get_32().  The
 * scheduler (when deadline scheduling is enabled) will choose the
 * next expiring thread when selecting between threads at the same
 * static priority.  Threads at different priorities will be scheduled
 * according to their static priority.
 *
 * @note Deadlines are stored internally using 32 bit unsigned
 * integers.  The number of cycles between the "first" deadline in the
 * scheduler queue and the "last" deadline must be less than 2^31 (i.e
 * a signed non-negative quantity).  Failure to adhere to this rule
 * may result in scheduled threads running in an incorrect deadline
 * order.
 *
 * @note Despite the API naming, the scheduler makes no guarantees
 * the thread WILL be scheduled within that deadline, nor does it take
 * extra metadata (like e.g. the "runtime" and "period" parameters in
 * Linux sched_setattr()) that allows the kernel to validate the
 * scheduling for achievability.  Such features could be implemented
 * above this call, which is simply input to the priority selection
 * logic.
 *
 * @note You should enable @kconfig{CONFIG_SCHED_DEADLINE} in your project
 * configuration.
 *
 * @param thread A thread on which to set the deadline
 * @param deadline A time delta, in cycle units
 *
 */
__syscall void k_thread_deadline_set(k_tid_t thread, int deadline);
#endif

#ifdef CONFIG_SCHED_CPU_MASK
/**
 * @brief Sets all CPU enable masks to zero
 *
 * After this returns, the thread will no longer be schedulable on any
 * CPUs.  The thread must not be currently runnable.
 *
 * @note You should enable @kconfig{CONFIG_SCHED_CPU_MASK} in your project
 * configuration.
 *
 * @param thread Thread to operate upon
 * @return Zero on success, otherwise error code
 */
int k_thread_cpu_mask_clear(k_tid_t thread);

/**
 * @brief Sets all CPU enable masks to one
 *
 * After this returns, the thread will be schedulable on any CPU.  The
 * thread must not be currently runnable.
 *
 * @note You should enable @kconfig{CONFIG_SCHED_CPU_MASK} in your project
 * configuration.
 *
 * @param thread Thread to operate upon
 * @return Zero on success, otherwise error code
 */
int k_thread_cpu_mask_enable_all(k_tid_t thread);

/**
 * @brief Enable thread to run on specified CPU
 *
 * The thread must not be currently runnable.
 *
 * @note You should enable @kconfig{CONFIG_SCHED_CPU_MASK} in your project
 * configuration.
 *
 * @param thread Thread to operate upon
 * @param cpu CPU index
 * @return Zero on success, otherwise error code
 */
int k_thread_cpu_mask_enable(k_tid_t thread, int cpu);

/**
 * @brief Prevent thread to run on specified CPU
 *
 * The thread must not be currently runnable.
 *
 * @note You should enable @kconfig{CONFIG_SCHED_CPU_MASK} in your project
 * configuration.
 *
 * @param thread Thread to operate upon
 * @param cpu CPU index
 * @return Zero on success, otherwise error code
 */
int k_thread_cpu_mask_disable(k_tid_t thread, int cpu);

/**
 * @brief Pin a thread to a CPU
 *
 * Pin a thread to a CPU by first clearing the cpu mask and then enabling the
 * thread on the selected CPU.
 *
 * @param thread Thread to operate upon
 * @param cpu CPU index
 * @return Zero on success, otherwise error code
 */
int k_thread_cpu_pin(k_tid_t thread, int cpu);
#endif

/**
 * @brief Suspend a thread.
 *
 * This routine prevents the kernel scheduler from making @a thread
 * the current thread. All other internal operations on @a thread are
 * still performed; for example, kernel objects it is waiting on are
 * still handed to it.  Note that any existing timeouts
 * (e.g. k_sleep(), or a timeout argument to k_sem_take() et. al.)
 * will be canceled.  On resume, the thread will begin running
 * immediately and return from the blocked call.
 *
 * When the target thread is active on another CPU, the caller will block until
 * the target thread is halted (suspended or aborted).  But if the caller is in
 * an interrupt context, it will spin waiting for that target thread active on
 * another CPU to halt.
 *
 * If @a thread is already suspended, the routine has no effect.
 *
 * @param thread ID of thread to suspend.
 */
__syscall void k_thread_suspend(k_tid_t thread);

/**
 * @brief Resume a suspended thread.
 *
 * This routine allows the kernel scheduler to make @a thread the current
 * thread, when it is next eligible for that role.
 *
 * If @a thread is not currently suspended, the routine has no effect.
 *
 * @param thread ID of thread to resume.
 */
__syscall void k_thread_resume(k_tid_t thread);

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
 * execute as long as desired without being preempted due to time slicing.
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
 */
void k_sched_time_slice_set(int32_t slice, int prio);

/**
 * @brief Set thread time slice
 *
 * As for k_sched_time_slice_set, but (when
 * CONFIG_TIMESLICE_PER_THREAD=y) sets the timeslice for a specific
 * thread.  When non-zero, this timeslice will take precedence over
 * the global value.
 *
 * When such a thread's timeslice expires, the configured callback
 * will be called before the thread is removed/re-added to the run
 * queue.  This callback will occur in interrupt context, and the
 * specified thread is guaranteed to have been preempted by the
 * currently-executing ISR.  Such a callback is free to, for example,
 * modify the thread priority or slice time for future execution,
 * suspend the thread, etc...
 *
 * @note Unlike the older API, the time slice parameter here is
 * specified in ticks, not milliseconds.  Ticks have always been the
 * internal unit, and not all platforms have integer conversions
 * between the two.
 *
 * @note Threads with a non-zero slice time set will be timesliced
 * always, even if they are higher priority than the maximum timeslice
 * priority set via k_sched_time_slice_set().
 *
 * @note The callback notification for slice expiration happens, as it
 * must, while the thread is still "current", and thus it happens
 * before any registered timeouts at this tick.  This has the somewhat
 * confusing side effect that the tick time (c.f. k_uptime_get()) does
 * not yet reflect the expired ticks.  Applications wishing to make
 * fine-grained timing decisions within this callback should use the
 * cycle API, or derived facilities like k_thread_runtime_stats_get().
 *
 * @param th A valid, initialized thread
 * @param slice_ticks Maximum timeslice, in ticks
 * @param expired Callback function called on slice expiration
 * @param data Parameter for the expiration handler
 */
void k_thread_time_slice_set(struct k_thread *th, int32_t slice_ticks,
			     k_thread_timeslice_fn_t expired, void *data);

/** @} */

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
 * @funcprops \isr_ok
 *
 * @return false if invoked by a thread.
 * @return true if invoked by an ISR.
 */
bool k_is_in_isr(void);

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
 * @funcprops \isr_ok
 *
 * @return 0 if invoked by an ISR or by a cooperative thread.
 * @return Non-zero if invoked by a preemptible thread.
 */
__syscall int k_is_preempt_thread(void);

/**
 * @brief Test whether startup is in the before-main-task phase.
 *
 * This routine allows the caller to customize its actions, depending on
 * whether it being invoked before the kernel is fully active.
 *
 * @funcprops \isr_ok
 *
 * @return true if invoked before post-kernel initialization
 * @return false if invoked during/after post-kernel initialization
 */
static inline bool k_is_pre_kernel(void)
{
	extern bool z_sys_post_kernel; /* in init.c */

	return !z_sys_post_kernel;
}

/**
 * @}
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
 * Owing to clever implementation details, scheduler locks are
 * extremely fast for non-userspace threads (just one byte
 * inc/decrement in the thread struct).
 *
 * @note This works by elevating the thread priority temporarily to a
 * cooperative priority, allowing cheap synchronization vs. other
 * preemptible or cooperative threads running on the current CPU.  It
 * does not prevent preemption or asynchrony of other types.  It does
 * not prevent threads from running on other CPUs when CONFIG_SMP=y.
 * It does not prevent interrupts from happening, nor does it prevent
 * threads with MetaIRQ priorities from preempting the current thread.
 * In general this is a historical API not well-suited to modern
 * applications, use with care.
 */
void k_sched_lock(void);

/**
 * @brief Unlock the scheduler.
 *
 * This routine reverses the effect of a previous call to k_sched_lock().
 * A thread must call the routine once for each time it called k_sched_lock()
 * before the thread becomes preemptible.
 */
void k_sched_unlock(void);

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
 */
__syscall void k_thread_custom_data_set(void *value);

/**
 * @brief Get current thread's custom data.
 *
 * This routine returns the custom data for the current thread.
 *
 * @return Current custom data value.
 */
__syscall void *k_thread_custom_data_get(void);

/**
 * @brief Set current thread name
 *
 * Set the name of the thread to be used when @kconfig{CONFIG_THREAD_MONITOR}
 * is enabled for tracing and debugging.
 *
 * @param thread Thread to set name, or NULL to set the current thread
 * @param str Name string
 * @retval 0 on success
 * @retval -EFAULT Memory access error with supplied string
 * @retval -ENOSYS Thread name configuration option not enabled
 * @retval -EINVAL Thread name too long
 */
__syscall int k_thread_name_set(k_tid_t thread, const char *str);

/**
 * @brief Get thread name
 *
 * Get the name of a thread
 *
 * @param thread Thread ID
 * @retval Thread name, or NULL if configuration not enabled
 */
const char *k_thread_name_get(k_tid_t thread);

/**
 * @brief Copy the thread name into a supplied buffer
 *
 * @param thread Thread to obtain name information
 * @param buf Destination buffer
 * @param size Destination buffer size
 * @retval -ENOSPC Destination buffer too small
 * @retval -EFAULT Memory access error
 * @retval -ENOSYS Thread name feature not enabled
 * @retval 0 Success
 */
__syscall int k_thread_name_copy(k_tid_t thread, char *buf,
				 size_t size);

/**
 * @brief Get thread state string
 *
 * This routine generates a human friendly string containing the thread's
 * state, and copies as much of it as possible into @a buf.
 *
 * @param thread_id Thread ID
 * @param buf Buffer into which to copy state strings
 * @param buf_size Size of the buffer
 *
 * @retval Pointer to @a buf if data was copied, else a pointer to "".
 */
const char *k_thread_state_str(k_tid_t thread_id, char *buf, size_t buf_size);

/**
 * @}
 */

/**
 * @addtogroup clock_apis
 * @{
 */

/**
 * @brief Generate null timeout delay.
 *
 * This macro generates a timeout delay that instructs a kernel API
 * not to wait if the requested operation cannot be performed immediately.
 *
 * @return Timeout delay value.
 */
#define K_NO_WAIT Z_TIMEOUT_NO_WAIT

/**
 * @brief Generate timeout delay from nanoseconds.
 *
 * This macro generates a timeout delay that instructs a kernel API to
 * wait up to @a t nanoseconds to perform the requested operation.
 * Note that timer precision is limited to the tick rate, not the
 * requested value.
 *
 * @param t Duration in nanoseconds.
 *
 * @return Timeout delay value.
 */
#define K_NSEC(t)     Z_TIMEOUT_NS(t)

/**
 * @brief Generate timeout delay from microseconds.
 *
 * This macro generates a timeout delay that instructs a kernel API
 * to wait up to @a t microseconds to perform the requested operation.
 * Note that timer precision is limited to the tick rate, not the
 * requested value.
 *
 * @param t Duration in microseconds.
 *
 * @return Timeout delay value.
 */
#define K_USEC(t)     Z_TIMEOUT_US(t)

/**
 * @brief Generate timeout delay from cycles.
 *
 * This macro generates a timeout delay that instructs a kernel API
 * to wait up to @a t cycles to perform the requested operation.
 *
 * @param t Duration in cycles.
 *
 * @return Timeout delay value.
 */
#define K_CYC(t)     Z_TIMEOUT_CYC(t)

/**
 * @brief Generate timeout delay from system ticks.
 *
 * This macro generates a timeout delay that instructs a kernel API
 * to wait up to @a t ticks to perform the requested operation.
 *
 * @param t Duration in system ticks.
 *
 * @return Timeout delay value.
 */
#define K_TICKS(t)     Z_TIMEOUT_TICKS(t)

/**
 * @brief Generate timeout delay from milliseconds.
 *
 * This macro generates a timeout delay that instructs a kernel API
 * to wait up to @a ms milliseconds to perform the requested operation.
 *
 * @param ms Duration in milliseconds.
 *
 * @return Timeout delay value.
 */
#define K_MSEC(ms)     Z_TIMEOUT_MS(ms)

/**
 * @brief Generate timeout delay from seconds.
 *
 * This macro generates a timeout delay that instructs a kernel API
 * to wait up to @a s seconds to perform the requested operation.
 *
 * @param s Duration in seconds.
 *
 * @return Timeout delay value.
 */
#define K_SECONDS(s)   K_MSEC((s) * MSEC_PER_SEC)

/**
 * @brief Generate timeout delay from minutes.

 * This macro generates a timeout delay that instructs a kernel API
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
 * This macro generates a timeout delay that instructs a kernel API
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
 * This macro generates a timeout delay that instructs a kernel API
 * to wait as long as necessary to perform the requested operation.
 *
 * @return Timeout delay value.
 */
#define K_FOREVER Z_FOREVER

#ifdef CONFIG_TIMEOUT_64BIT

/**
 * @brief Generates an absolute/uptime timeout value from system ticks
 *
 * This macro generates a timeout delay that represents an expiration
 * at the absolute uptime value specified, in system ticks.  That is, the
 * timeout will expire immediately after the system uptime reaches the
 * specified tick count.
 *
 * @param t Tick uptime value
 * @return Timeout delay value
 */
#define K_TIMEOUT_ABS_TICKS(t) \
	Z_TIMEOUT_TICKS(Z_TICK_ABS((k_ticks_t)MAX(t, 0)))

/**
 * @brief Generates an absolute/uptime timeout value from milliseconds
 *
 * This macro generates a timeout delay that represents an expiration
 * at the absolute uptime value specified, in milliseconds.  That is,
 * the timeout will expire immediately after the system uptime reaches
 * the specified tick count.
 *
 * @param t Millisecond uptime value
 * @return Timeout delay value
 */
#define K_TIMEOUT_ABS_MS(t) K_TIMEOUT_ABS_TICKS(k_ms_to_ticks_ceil64(t))

/**
 * @brief Generates an absolute/uptime timeout value from microseconds
 *
 * This macro generates a timeout delay that represents an expiration
 * at the absolute uptime value specified, in microseconds.  That is,
 * the timeout will expire immediately after the system uptime reaches
 * the specified time.  Note that timer precision is limited by the
 * system tick rate and not the requested timeout value.
 *
 * @param t Microsecond uptime value
 * @return Timeout delay value
 */
#define K_TIMEOUT_ABS_US(t) K_TIMEOUT_ABS_TICKS(k_us_to_ticks_ceil64(t))

/**
 * @brief Generates an absolute/uptime timeout value from nanoseconds
 *
 * This macro generates a timeout delay that represents an expiration
 * at the absolute uptime value specified, in nanoseconds.  That is,
 * the timeout will expire immediately after the system uptime reaches
 * the specified time.  Note that timer precision is limited by the
 * system tick rate and not the requested timeout value.
 *
 * @param t Nanosecond uptime value
 * @return Timeout delay value
 */
#define K_TIMEOUT_ABS_NS(t) K_TIMEOUT_ABS_TICKS(k_ns_to_ticks_ceil64(t))

/**
 * @brief Generates an absolute/uptime timeout value from system cycles
 *
 * This macro generates a timeout delay that represents an expiration
 * at the absolute uptime value specified, in cycles.  That is, the
 * timeout will expire immediately after the system uptime reaches the
 * specified time.  Note that timer precision is limited by the system
 * tick rate and not the requested timeout value.
 *
 * @param t Cycle uptime value
 * @return Timeout delay value
 */
#define K_TIMEOUT_ABS_CYC(t) K_TIMEOUT_ABS_TICKS(k_cyc_to_ticks_ceil64(t))

#endif

/**
 * @}
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
	void (*expiry_fn)(struct k_timer *timer);

	/* runs in the context of the thread that calls k_timer_stop() */
	void (*stop_fn)(struct k_timer *timer);

	/* timer period */
	k_timeout_t period;

	/* timer status */
	uint32_t status;

	/* user-specific data, also used to support legacy features */
	void *user_data;

	SYS_PORT_TRACING_TRACKING_FIELD(k_timer)

#ifdef CONFIG_OBJ_CORE_TIMER
	struct k_obj_core  obj_core;
#endif
};

#define Z_TIMER_INITIALIZER(obj, expiry, stop) \
	{ \
	.timeout = { \
		.node = {},\
		.fn = z_timer_expiration_handler, \
		.dticks = 0, \
	}, \
	.wait_q = Z_WAIT_Q_INIT(&obj.wait_q), \
	.expiry_fn = expiry, \
	.stop_fn = stop, \
	.status = 0, \
	.user_data = 0, \
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
 */
typedef void (*k_timer_expiry_t)(struct k_timer *timer);

/**
 * @typedef k_timer_stop_t
 * @brief Timer stop function type.
 *
 * A timer's stop function is executed if the timer is stopped prematurely.
 * The function runs in the context of call that stops the timer.  As
 * k_timer_stop() can be invoked from an ISR, the stop function must be
 * callable from interrupt context (isr-ok).
 *
 * The stop function is optional, and is only invoked if the timer has been
 * initialized with one.
 *
 * @param timer     Address of timer.
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
	STRUCT_SECTION_ITERABLE(k_timer, name) = \
		Z_TIMER_INITIALIZER(name, expiry_fn, stop_fn)

/**
 * @brief Initialize a timer.
 *
 * This routine initializes a timer, prior to its first use.
 *
 * @param timer     Address of timer.
 * @param expiry_fn Function to invoke each time the timer expires.
 * @param stop_fn   Function to invoke if the timer is stopped while running.
 */
void k_timer_init(struct k_timer *timer,
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
 * @param duration  Initial timer duration.
 * @param period    Timer period.
 */
__syscall void k_timer_start(struct k_timer *timer,
			     k_timeout_t duration, k_timeout_t period);

/**
 * @brief Stop a timer.
 *
 * This routine stops a running timer prematurely. The timer's stop function,
 * if one exists, is invoked by the caller.
 *
 * Attempting to stop a timer that is not running is permitted, but has no
 * effect on the timer.
 *
 * @note The stop handler has to be callable from ISRs if @a k_timer_stop is to
 * be called from ISRs.
 *
 * @funcprops \isr_ok
 *
 * @param timer     Address of timer.
 */
__syscall void k_timer_stop(struct k_timer *timer);

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
__syscall uint32_t k_timer_status_get(struct k_timer *timer);

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
__syscall uint32_t k_timer_status_sync(struct k_timer *timer);

#ifdef CONFIG_SYS_CLOCK_EXISTS

/**
 * @brief Get next expiration time of a timer, in system ticks
 *
 * This routine returns the future system uptime reached at the next
 * time of expiration of the timer, in units of system ticks.  If the
 * timer is not running, current system time is returned.
 *
 * @param timer The timer object
 * @return Uptime of expiration, in ticks
 */
__syscall k_ticks_t k_timer_expires_ticks(const struct k_timer *timer);

static inline k_ticks_t z_impl_k_timer_expires_ticks(
				       const struct k_timer *timer)
{
	return z_timeout_expires(&timer->timeout);
}

/**
 * @brief Get time remaining before a timer next expires, in system ticks
 *
 * This routine computes the time remaining before a running timer
 * next expires, in units of system ticks.  If the timer is not
 * running, it returns zero.
 */
__syscall k_ticks_t k_timer_remaining_ticks(const struct k_timer *timer);

static inline k_ticks_t z_impl_k_timer_remaining_ticks(
				       const struct k_timer *timer)
{
	return z_timeout_remaining(&timer->timeout);
}

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
static inline uint32_t k_timer_remaining_get(struct k_timer *timer)
{
	return k_ticks_to_ms_floor32(k_timer_remaining_ticks(timer));
}

#endif /* CONFIG_SYS_CLOCK_EXISTS */

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
 */
__syscall void k_timer_user_data_set(struct k_timer *timer, void *user_data);

/**
 * @internal
 */
static inline void z_impl_k_timer_user_data_set(struct k_timer *timer,
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
__syscall void *k_timer_user_data_get(const struct k_timer *timer);

static inline void *z_impl_k_timer_user_data_get(const struct k_timer *timer)
{
	return timer->user_data;
}

/** @} */

/**
 * @addtogroup clock_apis
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief Get system uptime, in system ticks.
 *
 * This routine returns the elapsed time since the system booted, in
 * ticks (c.f. @kconfig{CONFIG_SYS_CLOCK_TICKS_PER_SEC}), which is the
 * fundamental unit of resolution of kernel timekeeping.
 *
 * @return Current uptime in ticks.
 */
__syscall int64_t k_uptime_ticks(void);

/**
 * @brief Get system uptime.
 *
 * This routine returns the elapsed time since the system booted,
 * in milliseconds.
 *
 * @note
 *    While this function returns time in milliseconds, it does
 *    not mean it has millisecond resolution. The actual resolution depends on
 *    @kconfig{CONFIG_SYS_CLOCK_TICKS_PER_SEC} config option.
 *
 * @return Current uptime in milliseconds.
 */
static inline int64_t k_uptime_get(void)
{
	return k_ticks_to_ms_floor64(k_uptime_ticks());
}

/**
 * @brief Get system uptime (32-bit version).
 *
 * This routine returns the lower 32 bits of the system uptime in
 * milliseconds.
 *
 * Because correct conversion requires full precision of the system
 * clock there is no benefit to using this over k_uptime_get() unless
 * you know the application will never run long enough for the system
 * clock to approach 2^32 ticks.  Calls to this function may involve
 * interrupt blocking and 64-bit math.
 *
 * @note
 *    While this function returns time in milliseconds, it does
 *    not mean it has millisecond resolution. The actual resolution depends on
 *    @kconfig{CONFIG_SYS_CLOCK_TICKS_PER_SEC} config option
 *
 * @return The low 32 bits of the current uptime, in milliseconds.
 */
static inline uint32_t k_uptime_get_32(void)
{
	return (uint32_t)k_uptime_get();
}

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
static inline int64_t k_uptime_delta(int64_t *reftime)
{
	int64_t uptime, delta;

	uptime = k_uptime_get();
	delta = uptime - *reftime;
	*reftime = uptime;

	return delta;
}

/**
 * @brief Read the hardware clock.
 *
 * This routine returns the current time, as measured by the system's hardware
 * clock.
 *
 * @return Current hardware clock up-counter (in cycles).
 */
static inline uint32_t k_cycle_get_32(void)
{
	return arch_k_cycle_get_32();
}

/**
 * @brief Read the 64-bit hardware clock.
 *
 * This routine returns the current time in 64-bits, as measured by the
 * system's hardware clock, if available.
 *
 * @see CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER
 *
 * @return Current hardware clock up-counter (in cycles).
 */
static inline uint64_t k_cycle_get_64(void)
{
	if (!IS_ENABLED(CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER)) {
		__ASSERT(0, "64-bit cycle counter not enabled on this platform. "
			    "See CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER");
		return 0;
	}

	return arch_k_cycle_get_64();
}

/**
 * @}
 */

struct k_queue {
	sys_sflist_t data_q;
	struct k_spinlock lock;
	_wait_q_t wait_q;

	Z_DECL_POLL_EVENT

	SYS_PORT_TRACING_TRACKING_FIELD(k_queue)
};

/**
 * @cond INTERNAL_HIDDEN
 */

#define Z_QUEUE_INITIALIZER(obj) \
	{ \
	.data_q = SYS_SFLIST_STATIC_INIT(&obj.data_q), \
	.lock = { }, \
	.wait_q = Z_WAIT_Q_INIT(&obj.wait_q),	\
	Z_POLL_EVENT_OBJ_INIT(obj)		\
	}

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @defgroup queue_apis Queue APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief Initialize a queue.
 *
 * This routine initializes a queue object, prior to its first use.
 *
 * @param queue Address of the queue.
 */
__syscall void k_queue_init(struct k_queue *queue);

/**
 * @brief Cancel waiting on a queue.
 *
 * This routine causes first thread pending on @a queue, if any, to
 * return from k_queue_get() call with NULL value (as if timeout expired).
 * If the queue is being waited on by k_poll(), it will return with
 * -EINTR and K_POLL_STATE_CANCELLED state (and per above, subsequent
 * k_queue_get() will return NULL).
 *
 * @funcprops \isr_ok
 *
 * @param queue Address of the queue.
 */
__syscall void k_queue_cancel_wait(struct k_queue *queue);

/**
 * @brief Append an element to the end of a queue.
 *
 * This routine appends a data item to @a queue. A queue data item must be
 * aligned on a word boundary, and the first word of the item is reserved
 * for the kernel's use.
 *
 * @funcprops \isr_ok
 *
 * @param queue Address of the queue.
 * @param data Address of the data item.
 */
void k_queue_append(struct k_queue *queue, void *data);

/**
 * @brief Append an element to a queue.
 *
 * This routine appends a data item to @a queue. There is an implicit memory
 * allocation to create an additional temporary bookkeeping data structure from
 * the calling thread's resource pool, which is automatically freed when the
 * item is removed. The data itself is not copied.
 *
 * @funcprops \isr_ok
 *
 * @param queue Address of the queue.
 * @param data Address of the data item.
 *
 * @retval 0 on success
 * @retval -ENOMEM if there isn't sufficient RAM in the caller's resource pool
 */
__syscall int32_t k_queue_alloc_append(struct k_queue *queue, void *data);

/**
 * @brief Prepend an element to a queue.
 *
 * This routine prepends a data item to @a queue. A queue data item must be
 * aligned on a word boundary, and the first word of the item is reserved
 * for the kernel's use.
 *
 * @funcprops \isr_ok
 *
 * @param queue Address of the queue.
 * @param data Address of the data item.
 */
void k_queue_prepend(struct k_queue *queue, void *data);

/**
 * @brief Prepend an element to a queue.
 *
 * This routine prepends a data item to @a queue. There is an implicit memory
 * allocation to create an additional temporary bookkeeping data structure from
 * the calling thread's resource pool, which is automatically freed when the
 * item is removed. The data itself is not copied.
 *
 * @funcprops \isr_ok
 *
 * @param queue Address of the queue.
 * @param data Address of the data item.
 *
 * @retval 0 on success
 * @retval -ENOMEM if there isn't sufficient RAM in the caller's resource pool
 */
__syscall int32_t k_queue_alloc_prepend(struct k_queue *queue, void *data);

/**
 * @brief Inserts an element to a queue.
 *
 * This routine inserts a data item to @a queue after previous item. A queue
 * data item must be aligned on a word boundary, and the first word of
 * the item is reserved for the kernel's use.
 *
 * @funcprops \isr_ok
 *
 * @param queue Address of the queue.
 * @param prev Address of the previous data item.
 * @param data Address of the data item.
 */
void k_queue_insert(struct k_queue *queue, void *prev, void *data);

/**
 * @brief Atomically append a list of elements to a queue.
 *
 * This routine adds a list of data items to @a queue in one operation.
 * The data items must be in a singly-linked list, with the first word
 * in each data item pointing to the next data item; the list must be
 * NULL-terminated.
 *
 * @funcprops \isr_ok
 *
 * @param queue Address of the queue.
 * @param head Pointer to first node in singly-linked list.
 * @param tail Pointer to last node in singly-linked list.
 *
 * @retval 0 on success
 * @retval -EINVAL on invalid supplied data
 *
 */
int k_queue_append_list(struct k_queue *queue, void *head, void *tail);

/**
 * @brief Atomically add a list of elements to a queue.
 *
 * This routine adds a list of data items to @a queue in one operation.
 * The data items must be in a singly-linked list implemented using a
 * sys_slist_t object. Upon completion, the original list is empty.
 *
 * @funcprops \isr_ok
 *
 * @param queue Address of the queue.
 * @param list Pointer to sys_slist_t object.
 *
 * @retval 0 on success
 * @retval -EINVAL on invalid data
 */
int k_queue_merge_slist(struct k_queue *queue, sys_slist_t *list);

/**
 * @brief Get an element from a queue.
 *
 * This routine removes first data item from @a queue. The first word of the
 * data item is reserved for the kernel's use.
 *
 * @note @a timeout must be set to K_NO_WAIT if called from ISR.
 *
 * @funcprops \isr_ok
 *
 * @param queue Address of the queue.
 * @param timeout Waiting period to obtain a data item, or one of the special
 *                values K_NO_WAIT and K_FOREVER.
 *
 * @return Address of the data item if successful; NULL if returned
 * without waiting, or waiting period timed out.
 */
__syscall void *k_queue_get(struct k_queue *queue, k_timeout_t timeout);

/**
 * @brief Remove an element from a queue.
 *
 * This routine removes data item from @a queue. The first word of the
 * data item is reserved for the kernel's use. Removing elements from k_queue
 * rely on sys_slist_find_and_remove which is not a constant time operation.
 *
 * @note @a timeout must be set to K_NO_WAIT if called from ISR.
 *
 * @funcprops \isr_ok
 *
 * @param queue Address of the queue.
 * @param data Address of the data item.
 *
 * @return true if data item was removed
 */
bool k_queue_remove(struct k_queue *queue, void *data);

/**
 * @brief Append an element to a queue only if it's not present already.
 *
 * This routine appends data item to @a queue. The first word of the data
 * item is reserved for the kernel's use. Appending elements to k_queue
 * relies on sys_slist_is_node_in_list which is not a constant time operation.
 *
 * @funcprops \isr_ok
 *
 * @param queue Address of the queue.
 * @param data Address of the data item.
 *
 * @return true if data item was added, false if not
 */
bool k_queue_unique_append(struct k_queue *queue, void *data);

/**
 * @brief Query a queue to see if it has data available.
 *
 * Note that the data might be already gone by the time this function returns
 * if other threads are also trying to read from the queue.
 *
 * @funcprops \isr_ok
 *
 * @param queue Address of the queue.
 *
 * @return Non-zero if the queue is empty.
 * @return 0 if data is available.
 */
__syscall int k_queue_is_empty(struct k_queue *queue);

static inline int z_impl_k_queue_is_empty(struct k_queue *queue)
{
	return (int)sys_sflist_is_empty(&queue->data_q);
}

/**
 * @brief Peek element at the head of queue.
 *
 * Return element from the head of queue without removing it.
 *
 * @param queue Address of the queue.
 *
 * @return Head element, or NULL if queue is empty.
 */
__syscall void *k_queue_peek_head(struct k_queue *queue);

/**
 * @brief Peek element at the tail of queue.
 *
 * Return element from the tail of queue without removing it.
 *
 * @param queue Address of the queue.
 *
 * @return Tail element, or NULL if queue is empty.
 */
__syscall void *k_queue_peek_tail(struct k_queue *queue);

/**
 * @brief Statically define and initialize a queue.
 *
 * The queue can be accessed outside the module where it is defined using:
 *
 * @code extern struct k_queue <name>; @endcode
 *
 * @param name Name of the queue.
 */
#define K_QUEUE_DEFINE(name) \
	STRUCT_SECTION_ITERABLE(k_queue, name) = \
		Z_QUEUE_INITIALIZER(name)

/** @} */

#ifdef CONFIG_USERSPACE
/**
 * @brief futex structure
 *
 * A k_futex is a lightweight mutual exclusion primitive designed
 * to minimize kernel involvement. Uncontended operation relies
 * only on atomic access to shared memory. k_futex are tracked as
 * kernel objects and can live in user memory so that any access
 * bypasses the kernel object permission management mechanism.
 */
struct k_futex {
	atomic_t val;
};

/**
 * @brief futex kernel data structure
 *
 * z_futex_data are the helper data structure for k_futex to complete
 * futex contended operation on kernel side, structure z_futex_data
 * of every futex object is invisible in user mode.
 */
struct z_futex_data {
	_wait_q_t wait_q;
	struct k_spinlock lock;
};

#define Z_FUTEX_DATA_INITIALIZER(obj) \
	{ \
	.wait_q = Z_WAIT_Q_INIT(&obj.wait_q) \
	}

/**
 * @defgroup futex_apis FUTEX APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief Pend the current thread on a futex
 *
 * Tests that the supplied futex contains the expected value, and if so,
 * goes to sleep until some other thread calls k_futex_wake() on it.
 *
 * @param futex Address of the futex.
 * @param expected Expected value of the futex, if it is different the caller
 *		   will not wait on it.
 * @param timeout Waiting period on the futex, or one of the special values
 *                K_NO_WAIT or K_FOREVER.
 * @retval -EACCES Caller does not have read access to futex address.
 * @retval -EAGAIN If the futex value did not match the expected parameter.
 * @retval -EINVAL Futex parameter address not recognized by the kernel.
 * @retval -ETIMEDOUT Thread woke up due to timeout and not a futex wakeup.
 * @retval 0 if the caller went to sleep and was woken up. The caller
 *	     should check the futex's value on wakeup to determine if it needs
 *	     to block again.
 */
__syscall int k_futex_wait(struct k_futex *futex, int expected,
			   k_timeout_t timeout);

/**
 * @brief Wake one/all threads pending on a futex
 *
 * Wake up the highest priority thread pending on the supplied futex, or
 * wakeup all the threads pending on the supplied futex, and the behavior
 * depends on wake_all.
 *
 * @param futex Futex to wake up pending threads.
 * @param wake_all If true, wake up all pending threads; If false,
 *                 wakeup the highest priority thread.
 * @retval -EACCES Caller does not have access to the futex address.
 * @retval -EINVAL Futex parameter address not recognized by the kernel.
 * @retval Number of threads that were woken up.
 */
__syscall int k_futex_wake(struct k_futex *futex, bool wake_all);

/** @} */
#endif

/**
 * @defgroup event_apis Event APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * Event Structure
 * @ingroup event_apis
 */

struct k_event {
	_wait_q_t         wait_q;
	uint32_t          events;
	struct k_spinlock lock;

	SYS_PORT_TRACING_TRACKING_FIELD(k_event)

#ifdef CONFIG_OBJ_CORE_EVENT
	struct k_obj_core obj_core;
#endif

};

#define Z_EVENT_INITIALIZER(obj) \
	{ \
	.wait_q = Z_WAIT_Q_INIT(&obj.wait_q), \
	.events = 0 \
	}

/**
 * @brief Initialize an event object
 *
 * This routine initializes an event object, prior to its first use.
 *
 * @param event Address of the event object.
 */
__syscall void k_event_init(struct k_event *event);

/**
 * @brief Post one or more events to an event object
 *
 * This routine posts one or more events to an event object. All tasks waiting
 * on the event object @a event whose waiting conditions become met by this
 * posting immediately unpend.
 *
 * Posting differs from setting in that posted events are merged together with
 * the current set of events tracked by the event object.
 *
 * @param event Address of the event object
 * @param events Set of events to post to @a event
 *
 * @retval Previous value of the events in @a event
 */
__syscall uint32_t k_event_post(struct k_event *event, uint32_t events);

/**
 * @brief Set the events in an event object
 *
 * This routine sets the events stored in event object to the specified value.
 * All tasks waiting on the event object @a event whose waiting conditions
 * become met by this immediately unpend.
 *
 * Setting differs from posting in that set events replace the current set of
 * events tracked by the event object.
 *
 * @param event Address of the event object
 * @param events Set of events to set in @a event
 *
 * @retval Previous value of the events in @a event
 */
__syscall uint32_t k_event_set(struct k_event *event, uint32_t events);

/**
 * @brief Set or clear the events in an event object
 *
 * This routine sets the events stored in event object to the specified value.
 * All tasks waiting on the event object @a event whose waiting conditions
 * become met by this immediately unpend. Unlike @ref k_event_set, this routine
 * allows specific event bits to be set and cleared as determined by the mask.
 *
 * @param event Address of the event object
 * @param events Set of events to set/clear in @a event
 * @param events_mask Mask to be applied to @a events
 *
 * @retval Previous value of the events in @a events_mask
 */
__syscall uint32_t k_event_set_masked(struct k_event *event, uint32_t events,
				  uint32_t events_mask);

/**
 * @brief Clear the events in an event object
 *
 * This routine clears (resets) the specified events stored in an event object.
 *
 * @param event Address of the event object
 * @param events Set of events to clear in @a event
 *
 * @retval Previous value of the events in @a event
 */
__syscall uint32_t k_event_clear(struct k_event *event, uint32_t events);

/**
 * @brief Wait for any of the specified events
 *
 * This routine waits on event object @a event until any of the specified
 * events have been delivered to the event object, or the maximum wait time
 * @a timeout has expired. A thread may wait on up to 32 distinctly numbered
 * events that are expressed as bits in a single 32-bit word.
 *
 * @note The caller must be careful when resetting if there are multiple threads
 * waiting for the event object @a event.
 *
 * @param event Address of the event object
 * @param events Set of desired events on which to wait
 * @param reset If true, clear the set of events tracked by the event object
 *              before waiting. If false, do not clear the events.
 * @param timeout Waiting period for the desired set of events or one of the
 *                special values K_NO_WAIT and K_FOREVER.
 *
 * @retval set of matching events upon success
 * @retval 0 if matching events were not received within the specified time
 */
__syscall uint32_t k_event_wait(struct k_event *event, uint32_t events,
				bool reset, k_timeout_t timeout);

/**
 * @brief Wait for all of the specified events
 *
 * This routine waits on event object @a event until all of the specified
 * events have been delivered to the event object, or the maximum wait time
 * @a timeout has expired. A thread may wait on up to 32 distinctly numbered
 * events that are expressed as bits in a single 32-bit word.
 *
 * @note The caller must be careful when resetting if there are multiple threads
 * waiting for the event object @a event.
 *
 * @param event Address of the event object
 * @param events Set of desired events on which to wait
 * @param reset If true, clear the set of events tracked by the event object
 *              before waiting. If false, do not clear the events.
 * @param timeout Waiting period for the desired set of events or one of the
 *                special values K_NO_WAIT and K_FOREVER.
 *
 * @retval set of matching events upon success
 * @retval 0 if matching events were not received within the specified time
 */
__syscall uint32_t k_event_wait_all(struct k_event *event, uint32_t events,
				    bool reset, k_timeout_t timeout);

/**
 * @brief Test the events currently tracked in the event object
 *
 * @param event Address of the event object
 * @param events_mask Set of desired events to test
 *
 * @retval Current value of events in @a events_mask
 */
static inline uint32_t k_event_test(struct k_event *event, uint32_t events_mask)
{
	return k_event_wait(event, events_mask, false, K_NO_WAIT);
}

/**
 * @brief Statically define and initialize an event object
 *
 * The event can be accessed outside the module where it is defined using:
 *
 * @code extern struct k_event <name>; @endcode
 *
 * @param name Name of the event object.
 */
#define K_EVENT_DEFINE(name)                                   \
	STRUCT_SECTION_ITERABLE(k_event, name) =               \
		Z_EVENT_INITIALIZER(name);

/** @} */

struct k_fifo {
	struct k_queue _queue;
#ifdef CONFIG_OBJ_CORE_FIFO
	struct k_obj_core  obj_core;
#endif
};

/**
 * @cond INTERNAL_HIDDEN
 */
#define Z_FIFO_INITIALIZER(obj) \
	{ \
	._queue = Z_QUEUE_INITIALIZER(obj._queue) \
	}

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @defgroup fifo_apis FIFO APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief Initialize a FIFO queue.
 *
 * This routine initializes a FIFO queue, prior to its first use.
 *
 * @param fifo Address of the FIFO queue.
 */
#define k_fifo_init(fifo)                                    \
	({                                                   \
	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_fifo, init, fifo); \
	k_queue_init(&(fifo)->_queue);                       \
	K_OBJ_CORE_INIT(K_OBJ_CORE(fifo), _obj_type_fifo);   \
	K_OBJ_CORE_LINK(K_OBJ_CORE(fifo));                   \
	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_fifo, init, fifo);  \
	})

/**
 * @brief Cancel waiting on a FIFO queue.
 *
 * This routine causes first thread pending on @a fifo, if any, to
 * return from k_fifo_get() call with NULL value (as if timeout
 * expired).
 *
 * @funcprops \isr_ok
 *
 * @param fifo Address of the FIFO queue.
 */
#define k_fifo_cancel_wait(fifo) \
	({ \
	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_fifo, cancel_wait, fifo); \
	k_queue_cancel_wait(&(fifo)->_queue); \
	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_fifo, cancel_wait, fifo); \
	})

/**
 * @brief Add an element to a FIFO queue.
 *
 * This routine adds a data item to @a fifo. A FIFO data item must be
 * aligned on a word boundary, and the first word of the item is reserved
 * for the kernel's use.
 *
 * @funcprops \isr_ok
 *
 * @param fifo Address of the FIFO.
 * @param data Address of the data item.
 */
#define k_fifo_put(fifo, data) \
	({ \
	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_fifo, put, fifo, data); \
	k_queue_append(&(fifo)->_queue, data); \
	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_fifo, put, fifo, data); \
	})

/**
 * @brief Add an element to a FIFO queue.
 *
 * This routine adds a data item to @a fifo. There is an implicit memory
 * allocation to create an additional temporary bookkeeping data structure from
 * the calling thread's resource pool, which is automatically freed when the
 * item is removed. The data itself is not copied.
 *
 * @funcprops \isr_ok
 *
 * @param fifo Address of the FIFO.
 * @param data Address of the data item.
 *
 * @retval 0 on success
 * @retval -ENOMEM if there isn't sufficient RAM in the caller's resource pool
 */
#define k_fifo_alloc_put(fifo, data) \
	({ \
	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_fifo, alloc_put, fifo, data); \
	int fap_ret = k_queue_alloc_append(&(fifo)->_queue, data); \
	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_fifo, alloc_put, fifo, data, fap_ret); \
	fap_ret; \
	})

/**
 * @brief Atomically add a list of elements to a FIFO.
 *
 * This routine adds a list of data items to @a fifo in one operation.
 * The data items must be in a singly-linked list, with the first word of
 * each data item pointing to the next data item; the list must be
 * NULL-terminated.
 *
 * @funcprops \isr_ok
 *
 * @param fifo Address of the FIFO queue.
 * @param head Pointer to first node in singly-linked list.
 * @param tail Pointer to last node in singly-linked list.
 */
#define k_fifo_put_list(fifo, head, tail) \
	({ \
	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_fifo, put_list, fifo, head, tail); \
	k_queue_append_list(&(fifo)->_queue, head, tail); \
	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_fifo, put_list, fifo, head, tail); \
	})

/**
 * @brief Atomically add a list of elements to a FIFO queue.
 *
 * This routine adds a list of data items to @a fifo in one operation.
 * The data items must be in a singly-linked list implemented using a
 * sys_slist_t object. Upon completion, the sys_slist_t object is invalid
 * and must be re-initialized via sys_slist_init().
 *
 * @funcprops \isr_ok
 *
 * @param fifo Address of the FIFO queue.
 * @param list Pointer to sys_slist_t object.
 */
#define k_fifo_put_slist(fifo, list) \
	({ \
	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_fifo, put_slist, fifo, list); \
	k_queue_merge_slist(&(fifo)->_queue, list); \
	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_fifo, put_slist, fifo, list); \
	})

/**
 * @brief Get an element from a FIFO queue.
 *
 * This routine removes a data item from @a fifo in a "first in, first out"
 * manner. The first word of the data item is reserved for the kernel's use.
 *
 * @note @a timeout must be set to K_NO_WAIT if called from ISR.
 *
 * @funcprops \isr_ok
 *
 * @param fifo Address of the FIFO queue.
 * @param timeout Waiting period to obtain a data item,
 *                or one of the special values K_NO_WAIT and K_FOREVER.
 *
 * @return Address of the data item if successful; NULL if returned
 * without waiting, or waiting period timed out.
 */
#define k_fifo_get(fifo, timeout) \
	({ \
	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_fifo, get, fifo, timeout); \
	void *fg_ret = k_queue_get(&(fifo)->_queue, timeout); \
	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_fifo, get, fifo, timeout, fg_ret); \
	fg_ret; \
	})

/**
 * @brief Query a FIFO queue to see if it has data available.
 *
 * Note that the data might be already gone by the time this function returns
 * if other threads is also trying to read from the FIFO.
 *
 * @funcprops \isr_ok
 *
 * @param fifo Address of the FIFO queue.
 *
 * @return Non-zero if the FIFO queue is empty.
 * @return 0 if data is available.
 */
#define k_fifo_is_empty(fifo) \
	k_queue_is_empty(&(fifo)->_queue)

/**
 * @brief Peek element at the head of a FIFO queue.
 *
 * Return element from the head of FIFO queue without removing it. A usecase
 * for this is if elements of the FIFO object are themselves containers. Then
 * on each iteration of processing, a head container will be peeked,
 * and some data processed out of it, and only if the container is empty,
 * it will be completely remove from the FIFO queue.
 *
 * @param fifo Address of the FIFO queue.
 *
 * @return Head element, or NULL if the FIFO queue is empty.
 */
#define k_fifo_peek_head(fifo) \
	({ \
	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_fifo, peek_head, fifo); \
	void *fph_ret = k_queue_peek_head(&(fifo)->_queue); \
	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_fifo, peek_head, fifo, fph_ret); \
	fph_ret; \
	})

/**
 * @brief Peek element at the tail of FIFO queue.
 *
 * Return element from the tail of FIFO queue (without removing it). A usecase
 * for this is if elements of the FIFO queue are themselves containers. Then
 * it may be useful to add more data to the last container in a FIFO queue.
 *
 * @param fifo Address of the FIFO queue.
 *
 * @return Tail element, or NULL if a FIFO queue is empty.
 */
#define k_fifo_peek_tail(fifo) \
	({ \
	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_fifo, peek_tail, fifo); \
	void *fpt_ret = k_queue_peek_tail(&(fifo)->_queue); \
	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_fifo, peek_tail, fifo, fpt_ret); \
	fpt_ret; \
	})

/**
 * @brief Statically define and initialize a FIFO queue.
 *
 * The FIFO queue can be accessed outside the module where it is defined using:
 *
 * @code extern struct k_fifo <name>; @endcode
 *
 * @param name Name of the FIFO queue.
 */
#define K_FIFO_DEFINE(name) \
	STRUCT_SECTION_ITERABLE(k_fifo, name) = \
		Z_FIFO_INITIALIZER(name)

/** @} */

struct k_lifo {
	struct k_queue _queue;
#ifdef CONFIG_OBJ_CORE_LIFO
	struct k_obj_core  obj_core;
#endif
};

/**
 * @cond INTERNAL_HIDDEN
 */

#define Z_LIFO_INITIALIZER(obj) \
	{ \
	._queue = Z_QUEUE_INITIALIZER(obj._queue) \
	}

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @defgroup lifo_apis LIFO APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief Initialize a LIFO queue.
 *
 * This routine initializes a LIFO queue object, prior to its first use.
 *
 * @param lifo Address of the LIFO queue.
 */
#define k_lifo_init(lifo)                                    \
	({                                                   \
	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_lifo, init, lifo); \
	k_queue_init(&(lifo)->_queue);                       \
	K_OBJ_CORE_INIT(K_OBJ_CORE(lifo), _obj_type_lifo);   \
	K_OBJ_CORE_LINK(K_OBJ_CORE(lifo));                   \
	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_lifo, init, lifo);  \
	})

/**
 * @brief Add an element to a LIFO queue.
 *
 * This routine adds a data item to @a lifo. A LIFO queue data item must be
 * aligned on a word boundary, and the first word of the item is
 * reserved for the kernel's use.
 *
 * @funcprops \isr_ok
 *
 * @param lifo Address of the LIFO queue.
 * @param data Address of the data item.
 */
#define k_lifo_put(lifo, data) \
	({ \
	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_lifo, put, lifo, data); \
	k_queue_prepend(&(lifo)->_queue, data); \
	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_lifo, put, lifo, data); \
	})

/**
 * @brief Add an element to a LIFO queue.
 *
 * This routine adds a data item to @a lifo. There is an implicit memory
 * allocation to create an additional temporary bookkeeping data structure from
 * the calling thread's resource pool, which is automatically freed when the
 * item is removed. The data itself is not copied.
 *
 * @funcprops \isr_ok
 *
 * @param lifo Address of the LIFO.
 * @param data Address of the data item.
 *
 * @retval 0 on success
 * @retval -ENOMEM if there isn't sufficient RAM in the caller's resource pool
 */
#define k_lifo_alloc_put(lifo, data) \
	({ \
	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_lifo, alloc_put, lifo, data); \
	int lap_ret = k_queue_alloc_prepend(&(lifo)->_queue, data); \
	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_lifo, alloc_put, lifo, data, lap_ret); \
	lap_ret; \
	})

/**
 * @brief Get an element from a LIFO queue.
 *
 * This routine removes a data item from @a LIFO in a "last in, first out"
 * manner. The first word of the data item is reserved for the kernel's use.
 *
 * @note @a timeout must be set to K_NO_WAIT if called from ISR.
 *
 * @funcprops \isr_ok
 *
 * @param lifo Address of the LIFO queue.
 * @param timeout Waiting period to obtain a data item,
 *                or one of the special values K_NO_WAIT and K_FOREVER.
 *
 * @return Address of the data item if successful; NULL if returned
 * without waiting, or waiting period timed out.
 */
#define k_lifo_get(lifo, timeout) \
	({ \
	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_lifo, get, lifo, timeout); \
	void *lg_ret = k_queue_get(&(lifo)->_queue, timeout); \
	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_lifo, get, lifo, timeout, lg_ret); \
	lg_ret; \
	})

/**
 * @brief Statically define and initialize a LIFO queue.
 *
 * The LIFO queue can be accessed outside the module where it is defined using:
 *
 * @code extern struct k_lifo <name>; @endcode
 *
 * @param name Name of the fifo.
 */
#define K_LIFO_DEFINE(name) \
	STRUCT_SECTION_ITERABLE(k_lifo, name) = \
		Z_LIFO_INITIALIZER(name)

/** @} */

/**
 * @cond INTERNAL_HIDDEN
 */
#define K_STACK_FLAG_ALLOC	((uint8_t)1)	/* Buffer was allocated */

typedef uintptr_t stack_data_t;

struct k_stack {
	_wait_q_t wait_q;
	struct k_spinlock lock;
	stack_data_t *base, *next, *top;

	uint8_t flags;

	SYS_PORT_TRACING_TRACKING_FIELD(k_stack)

#ifdef CONFIG_OBJ_CORE_STACK
	struct k_obj_core  obj_core;
#endif
};

#define Z_STACK_INITIALIZER(obj, stack_buffer, stack_num_entries) \
	{ \
	.wait_q = Z_WAIT_Q_INIT(&obj.wait_q),	\
	.base = stack_buffer, \
	.next = stack_buffer, \
	.top = stack_buffer + stack_num_entries, \
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
 */
void k_stack_init(struct k_stack *stack,
		  stack_data_t *buffer, uint32_t num_entries);


/**
 * @brief Initialize a stack.
 *
 * This routine initializes a stack object, prior to its first use. Internal
 * buffers will be allocated from the calling thread's resource pool.
 * This memory will be released if k_stack_cleanup() is called, or
 * userspace is enabled and the stack object loses all references to it.
 *
 * @param stack Address of the stack.
 * @param num_entries Maximum number of values that can be stacked.
 *
 * @return -ENOMEM if memory couldn't be allocated
 */

__syscall int32_t k_stack_alloc_init(struct k_stack *stack,
				   uint32_t num_entries);

/**
 * @brief Release a stack's allocated buffer
 *
 * If a stack object was given a dynamically allocated buffer via
 * k_stack_alloc_init(), this will free it. This function does nothing
 * if the buffer wasn't dynamically allocated.
 *
 * @param stack Address of the stack.
 * @retval 0 on success
 * @retval -EAGAIN when object is still in use
 */
int k_stack_cleanup(struct k_stack *stack);

/**
 * @brief Push an element onto a stack.
 *
 * This routine adds a stack_data_t value @a data to @a stack.
 *
 * @funcprops \isr_ok
 *
 * @param stack Address of the stack.
 * @param data Value to push onto the stack.
 *
 * @retval 0 on success
 * @retval -ENOMEM if stack is full
 */
__syscall int k_stack_push(struct k_stack *stack, stack_data_t data);

/**
 * @brief Pop an element from a stack.
 *
 * This routine removes a stack_data_t value from @a stack in a "last in,
 * first out" manner and stores the value in @a data.
 *
 * @note @a timeout must be set to K_NO_WAIT if called from ISR.
 *
 * @funcprops \isr_ok
 *
 * @param stack Address of the stack.
 * @param data Address of area to hold the value popped from the stack.
 * @param timeout Waiting period to obtain a value,
 *                or one of the special values K_NO_WAIT and
 *                K_FOREVER.
 *
 * @retval 0 Element popped from stack.
 * @retval -EBUSY Returned without waiting.
 * @retval -EAGAIN Waiting period timed out.
 */
__syscall int k_stack_pop(struct k_stack *stack, stack_data_t *data,
			  k_timeout_t timeout);

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
	stack_data_t __noinit                                  \
		_k_stack_buf_##name[stack_num_entries];        \
	STRUCT_SECTION_ITERABLE(k_stack, name) =               \
		Z_STACK_INITIALIZER(name, _k_stack_buf_##name, \
				    stack_num_entries)

/** @} */

/**
 * @cond INTERNAL_HIDDEN
 */

struct k_work;
struct k_work_q;
struct k_work_queue_config;
extern struct k_work_q k_sys_work_q;

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @defgroup mutex_apis Mutex APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * Mutex Structure
 * @ingroup mutex_apis
 */
struct k_mutex {
	/** Mutex wait queue */
	_wait_q_t wait_q;
	/** Mutex owner */
	struct k_thread *owner;

	/** Current lock count */
	uint32_t lock_count;

	/** Original thread priority */
	int owner_orig_prio;

	SYS_PORT_TRACING_TRACKING_FIELD(k_mutex)

#ifdef CONFIG_OBJ_CORE_MUTEX
	struct k_obj_core obj_core;
#endif
};

/**
 * @cond INTERNAL_HIDDEN
 */
#define Z_MUTEX_INITIALIZER(obj) \
	{ \
	.wait_q = Z_WAIT_Q_INIT(&obj.wait_q), \
	.owner = NULL, \
	.lock_count = 0, \
	.owner_orig_prio = K_LOWEST_APPLICATION_THREAD_PRIO, \
	}

/**
 * INTERNAL_HIDDEN @endcond
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
	STRUCT_SECTION_ITERABLE(k_mutex, name) = \
		Z_MUTEX_INITIALIZER(name)

/**
 * @brief Initialize a mutex.
 *
 * This routine initializes a mutex object, prior to its first use.
 *
 * Upon completion, the mutex is available and does not have an owner.
 *
 * @param mutex Address of the mutex.
 *
 * @retval 0 Mutex object created
 *
 */
__syscall int k_mutex_init(struct k_mutex *mutex);


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
 * Mutexes may not be locked in ISRs.
 *
 * @param mutex Address of the mutex.
 * @param timeout Waiting period to lock the mutex,
 *                or one of the special values K_NO_WAIT and
 *                K_FOREVER.
 *
 * @retval 0 Mutex locked.
 * @retval -EBUSY Returned without waiting.
 * @retval -EAGAIN Waiting period timed out.
 */
__syscall int k_mutex_lock(struct k_mutex *mutex, k_timeout_t timeout);

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
 * Mutexes may not be unlocked in ISRs, as mutexes must only be manipulated
 * in thread context due to ownership and priority inheritance semantics.
 *
 * @param mutex Address of the mutex.
 *
 * @retval 0 Mutex unlocked.
 * @retval -EPERM The current thread does not own the mutex
 * @retval -EINVAL The mutex is not locked
 *
 */
__syscall int k_mutex_unlock(struct k_mutex *mutex);

/**
 * @}
 */


struct k_condvar {
	_wait_q_t wait_q;

#ifdef CONFIG_OBJ_CORE_CONDVAR
	struct k_obj_core  obj_core;
#endif
};

#define Z_CONDVAR_INITIALIZER(obj)                                             \
	{                                                                      \
		.wait_q = Z_WAIT_Q_INIT(&obj.wait_q),                          \
	}

/**
 * @defgroup condvar_apis Condition Variables APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief Initialize a condition variable
 *
 * @param condvar pointer to a @p k_condvar structure
 * @retval 0 Condition variable created successfully
 */
__syscall int k_condvar_init(struct k_condvar *condvar);

/**
 * @brief Signals one thread that is pending on the condition variable
 *
 * @param condvar pointer to a @p k_condvar structure
 * @retval 0 On success
 */
__syscall int k_condvar_signal(struct k_condvar *condvar);

/**
 * @brief Unblock all threads that are pending on the condition
 * variable
 *
 * @param condvar pointer to a @p k_condvar structure
 * @return An integer with number of woken threads on success
 */
__syscall int k_condvar_broadcast(struct k_condvar *condvar);

/**
 * @brief Waits on the condition variable releasing the mutex lock
 *
 * Atomically releases the currently owned mutex, blocks the current thread
 * waiting on the condition variable specified by @a condvar,
 * and finally acquires the mutex again.
 *
 * The waiting thread unblocks only after another thread calls
 * k_condvar_signal, or k_condvar_broadcast with the same condition variable.
 *
 * @param condvar pointer to a @p k_condvar structure
 * @param mutex Address of the mutex.
 * @param timeout Waiting period for the condition variable
 *                or one of the special values K_NO_WAIT and K_FOREVER.
 * @retval 0 On success
 * @retval -EAGAIN Waiting period timed out.
 */
__syscall int k_condvar_wait(struct k_condvar *condvar, struct k_mutex *mutex,
			     k_timeout_t timeout);

/**
 * @brief Statically define and initialize a condition variable.
 *
 * The condition variable can be accessed outside the module where it is
 * defined using:
 *
 * @code extern struct k_condvar <name>; @endcode
 *
 * @param name Name of the condition variable.
 */
#define K_CONDVAR_DEFINE(name)                                                 \
	STRUCT_SECTION_ITERABLE(k_condvar, name) =                             \
		Z_CONDVAR_INITIALIZER(name)
/**
 * @}
 */

/**
 * @cond INTERNAL_HIDDEN
 */

struct k_sem {
	_wait_q_t wait_q;
	unsigned int count;
	unsigned int limit;

	Z_DECL_POLL_EVENT

	SYS_PORT_TRACING_TRACKING_FIELD(k_sem)

#ifdef CONFIG_OBJ_CORE_SEM
	struct k_obj_core  obj_core;
#endif
};

#define Z_SEM_INITIALIZER(obj, initial_count, count_limit) \
	{ \
	.wait_q = Z_WAIT_Q_INIT(&obj.wait_q), \
	.count = initial_count, \
	.limit = count_limit, \
	Z_POLL_EVENT_OBJ_INIT(obj) \
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
 * @brief Maximum limit value allowed for a semaphore.
 *
 * This is intended for use when a semaphore does not have
 * an explicit maximum limit, and instead is just used for
 * counting purposes.
 *
 */
#define K_SEM_MAX_LIMIT UINT_MAX

/**
 * @brief Initialize a semaphore.
 *
 * This routine initializes a semaphore object, prior to its first use.
 *
 * @param sem Address of the semaphore.
 * @param initial_count Initial semaphore count.
 * @param limit Maximum permitted semaphore count.
 *
 * @see K_SEM_MAX_LIMIT
 *
 * @retval 0 Semaphore created successfully
 * @retval -EINVAL Invalid values
 *
 */
__syscall int k_sem_init(struct k_sem *sem, unsigned int initial_count,
			  unsigned int limit);

/**
 * @brief Take a semaphore.
 *
 * This routine takes @a sem.
 *
 * @note @a timeout must be set to K_NO_WAIT if called from ISR.
 *
 * @funcprops \isr_ok
 *
 * @param sem Address of the semaphore.
 * @param timeout Waiting period to take the semaphore,
 *                or one of the special values K_NO_WAIT and K_FOREVER.
 *
 * @retval 0 Semaphore taken.
 * @retval -EBUSY Returned without waiting.
 * @retval -EAGAIN Waiting period timed out,
 *			or the semaphore was reset during the waiting period.
 */
__syscall int k_sem_take(struct k_sem *sem, k_timeout_t timeout);

/**
 * @brief Give a semaphore.
 *
 * This routine gives @a sem, unless the semaphore is already at its maximum
 * permitted count.
 *
 * @funcprops \isr_ok
 *
 * @param sem Address of the semaphore.
 */
__syscall void k_sem_give(struct k_sem *sem);

/**
 * @brief Resets a semaphore's count to zero.
 *
 * This routine sets the count of @a sem to zero.
 * Any outstanding semaphore takes will be aborted
 * with -EAGAIN.
 *
 * @param sem Address of the semaphore.
 */
__syscall void k_sem_reset(struct k_sem *sem);

/**
 * @brief Get a semaphore's count.
 *
 * This routine returns the current count of @a sem.
 *
 * @param sem Address of the semaphore.
 *
 * @return Current semaphore count.
 */
__syscall unsigned int k_sem_count_get(struct k_sem *sem);

/**
 * @internal
 */
static inline unsigned int z_impl_k_sem_count_get(struct k_sem *sem)
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
	STRUCT_SECTION_ITERABLE(k_sem, name) = \
		Z_SEM_INITIALIZER(name, initial_count, count_limit); \
	BUILD_ASSERT(((count_limit) != 0) && \
		     ((initial_count) <= (count_limit)) && \
			 ((count_limit) <= K_SEM_MAX_LIMIT));

/** @} */

/**
 * @cond INTERNAL_HIDDEN
 */

struct k_work_delayable;
struct k_work_sync;

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @defgroup workqueue_apis Work Queue APIs
 * @ingroup kernel_apis
 * @{
 */

/** @brief The signature for a work item handler function.
 *
 * The function will be invoked by the thread animating a work queue.
 *
 * @param work the work item that provided the handler.
 */
typedef void (*k_work_handler_t)(struct k_work *work);

/** @brief Initialize a (non-delayable) work structure.
 *
 * This must be invoked before submitting a work structure for the first time.
 * It need not be invoked again on the same work structure.  It can be
 * re-invoked to change the associated handler, but this must be done when the
 * work item is idle.
 *
 * @funcprops \isr_ok
 *
 * @param work the work structure to be initialized.
 *
 * @param handler the handler to be invoked by the work item.
 */
void k_work_init(struct k_work *work,
		  k_work_handler_t handler);

/** @brief Busy state flags from the work item.
 *
 * A zero return value indicates the work item appears to be idle.
 *
 * @note This is a live snapshot of state, which may change before the result
 * is checked.  Use locks where appropriate.
 *
 * @funcprops \isr_ok
 *
 * @param work pointer to the work item.
 *
 * @return a mask of flags K_WORK_DELAYED, K_WORK_QUEUED,
 * K_WORK_RUNNING, K_WORK_CANCELING, and K_WORK_FLUSHING.
 */
int k_work_busy_get(const struct k_work *work);

/** @brief Test whether a work item is currently pending.
 *
 * Wrapper to determine whether a work item is in a non-idle dstate.
 *
 * @note This is a live snapshot of state, which may change before the result
 * is checked.  Use locks where appropriate.
 *
 * @funcprops \isr_ok
 *
 * @param work pointer to the work item.
 *
 * @return true if and only if k_work_busy_get() returns a non-zero value.
 */
static inline bool k_work_is_pending(const struct k_work *work);

/** @brief Submit a work item to a queue.
 *
 * @param queue pointer to the work queue on which the item should run.  If
 * NULL the queue from the most recent submission will be used.
 *
 * @funcprops \isr_ok
 *
 * @param work pointer to the work item.
 *
 * @retval 0 if work was already submitted to a queue
 * @retval 1 if work was not submitted and has been queued to @p queue
 * @retval 2 if work was running and has been queued to the queue that was
 * running it
 * @retval -EBUSY
 * * if work submission was rejected because the work item is cancelling; or
 * * @p queue is draining; or
 * * @p queue is plugged.
 * @retval -EINVAL if @p queue is null and the work item has never been run.
 * @retval -ENODEV if @p queue has not been started.
 */
int k_work_submit_to_queue(struct k_work_q *queue,
			   struct k_work *work);

/** @brief Submit a work item to the system queue.
 *
 * @funcprops \isr_ok
 *
 * @param work pointer to the work item.
 *
 * @return as with k_work_submit_to_queue().
 */
int k_work_submit(struct k_work *work);

/** @brief Wait for last-submitted instance to complete.
 *
 * Resubmissions may occur while waiting, including chained submissions (from
 * within the handler).
 *
 * @note Be careful of caller and work queue thread relative priority.  If
 * this function sleeps it will not return until the work queue thread
 * completes the tasks that allow this thread to resume.
 *
 * @note Behavior is undefined if this function is invoked on @p work from a
 * work queue running @p work.
 *
 * @param work pointer to the work item.
 *
 * @param sync pointer to an opaque item containing state related to the
 * pending cancellation.  The object must persist until the call returns, and
 * be accessible from both the caller thread and the work queue thread.  The
 * object must not be used for any other flush or cancel operation until this
 * one completes.  On architectures with CONFIG_KERNEL_COHERENCE the object
 * must be allocated in coherent memory.
 *
 * @retval true if call had to wait for completion
 * @retval false if work was already idle
 */
bool k_work_flush(struct k_work *work,
		  struct k_work_sync *sync);

/** @brief Cancel a work item.
 *
 * This attempts to prevent a pending (non-delayable) work item from being
 * processed by removing it from the work queue.  If the item is being
 * processed, the work item will continue to be processed, but resubmissions
 * are rejected until cancellation completes.
 *
 * If this returns zero cancellation is complete, otherwise something
 * (probably a work queue thread) is still referencing the item.
 *
 * See also k_work_cancel_sync().
 *
 * @funcprops \isr_ok
 *
 * @param work pointer to the work item.
 *
 * @return the k_work_busy_get() status indicating the state of the item after all
 * cancellation steps performed by this call are completed.
 */
int k_work_cancel(struct k_work *work);

/** @brief Cancel a work item and wait for it to complete.
 *
 * Same as k_work_cancel() but does not return until cancellation is complete.
 * This can be invoked by a thread after k_work_cancel() to synchronize with a
 * previous cancellation.
 *
 * On return the work structure will be idle unless something submits it after
 * the cancellation was complete.
 *
 * @note Be careful of caller and work queue thread relative priority.  If
 * this function sleeps it will not return until the work queue thread
 * completes the tasks that allow this thread to resume.
 *
 * @note Behavior is undefined if this function is invoked on @p work from a
 * work queue running @p work.
 *
 * @param work pointer to the work item.
 *
 * @param sync pointer to an opaque item containing state related to the
 * pending cancellation.  The object must persist until the call returns, and
 * be accessible from both the caller thread and the work queue thread.  The
 * object must not be used for any other flush or cancel operation until this
 * one completes.  On architectures with CONFIG_KERNEL_COHERENCE the object
 * must be allocated in coherent memory.
 *
 * @retval true if work was pending (call had to wait for cancellation of a
 * running handler to complete, or scheduled or submitted operations were
 * cancelled);
 * @retval false otherwise
 */
bool k_work_cancel_sync(struct k_work *work, struct k_work_sync *sync);

/** @brief Initialize a work queue structure.
 *
 * This must be invoked before starting a work queue structure for the first time.
 * It need not be invoked again on the same work queue structure.
 *
 * @funcprops \isr_ok
 *
 * @param queue the queue structure to be initialized.
 */
void k_work_queue_init(struct k_work_q *queue);

/** @brief Initialize a work queue.
 *
 * This configures the work queue thread and starts it running.  The function
 * should not be re-invoked on a queue.
 *
 * @param queue pointer to the queue structure. It must be initialized
 *        in zeroed/bss memory or with @ref k_work_queue_init before
 *        use.
 *
 * @param stack pointer to the work thread stack area.
 *
 * @param stack_size size of the the work thread stack area, in bytes.
 *
 * @param prio initial thread priority
 *
 * @param cfg optional additional configuration parameters.  Pass @c
 * NULL if not required, to use the defaults documented in
 * k_work_queue_config.
 */
void k_work_queue_start(struct k_work_q *queue,
			k_thread_stack_t *stack, size_t stack_size,
			int prio, const struct k_work_queue_config *cfg);

/** @brief Access the thread that animates a work queue.
 *
 * This is necessary to grant a work queue thread access to things the work
 * items it will process are expected to use.
 *
 * @param queue pointer to the queue structure.
 *
 * @return the thread associated with the work queue.
 */
static inline k_tid_t k_work_queue_thread_get(struct k_work_q *queue);

/** @brief Wait until the work queue has drained, optionally plugging it.
 *
 * This blocks submission to the work queue except when coming from queue
 * thread, and blocks the caller until no more work items are available in the
 * queue.
 *
 * If @p plug is true then submission will continue to be blocked after the
 * drain operation completes until k_work_queue_unplug() is invoked.
 *
 * Note that work items that are delayed are not yet associated with their
 * work queue.  They must be cancelled externally if a goal is to ensure the
 * work queue remains empty.  The @p plug feature can be used to prevent
 * delayed items from being submitted after the drain completes.
 *
 * @param queue pointer to the queue structure.
 *
 * @param plug if true the work queue will continue to block new submissions
 * after all items have drained.
 *
 * @retval 1 if call had to wait for the drain to complete
 * @retval 0 if call did not have to wait
 * @retval negative if wait was interrupted or failed
 */
int k_work_queue_drain(struct k_work_q *queue, bool plug);

/** @brief Release a work queue to accept new submissions.
 *
 * This releases the block on new submissions placed when k_work_queue_drain()
 * is invoked with the @p plug option enabled.  If this is invoked before the
 * drain completes new items may be submitted as soon as the drain completes.
 *
 * @funcprops \isr_ok
 *
 * @param queue pointer to the queue structure.
 *
 * @retval 0 if successfully unplugged
 * @retval -EALREADY if the work queue was not plugged.
 */
int k_work_queue_unplug(struct k_work_q *queue);

/** @brief Initialize a delayable work structure.
 *
 * This must be invoked before scheduling a delayable work structure for the
 * first time.  It need not be invoked again on the same work structure.  It
 * can be re-invoked to change the associated handler, but this must be done
 * when the work item is idle.
 *
 * @funcprops \isr_ok
 *
 * @param dwork the delayable work structure to be initialized.
 *
 * @param handler the handler to be invoked by the work item.
 */
void k_work_init_delayable(struct k_work_delayable *dwork,
			   k_work_handler_t handler);

/**
 * @brief Get the parent delayable work structure from a work pointer.
 *
 * This function is necessary when a @c k_work_handler_t function is passed to
 * k_work_schedule_for_queue() and the handler needs to access data from the
 * container of the containing `k_work_delayable`.
 *
 * @param work Address passed to the work handler
 *
 * @return Address of the containing @c k_work_delayable structure.
 */
static inline struct k_work_delayable *
k_work_delayable_from_work(struct k_work *work);

/** @brief Busy state flags from the delayable work item.
 *
 * @funcprops \isr_ok
 *
 * @note This is a live snapshot of state, which may change before the result
 * can be inspected.  Use locks where appropriate.
 *
 * @param dwork pointer to the delayable work item.
 *
 * @return a mask of flags K_WORK_DELAYED, K_WORK_QUEUED, K_WORK_RUNNING,
 * K_WORK_CANCELING, and K_WORK_FLUSHING.  A zero return value indicates the
 * work item appears to be idle.
 */
int k_work_delayable_busy_get(const struct k_work_delayable *dwork);

/** @brief Test whether a delayed work item is currently pending.
 *
 * Wrapper to determine whether a delayed work item is in a non-idle state.
 *
 * @note This is a live snapshot of state, which may change before the result
 * can be inspected.  Use locks where appropriate.
 *
 * @funcprops \isr_ok
 *
 * @param dwork pointer to the delayable work item.
 *
 * @return true if and only if k_work_delayable_busy_get() returns a non-zero
 * value.
 */
static inline bool k_work_delayable_is_pending(
	const struct k_work_delayable *dwork);

/** @brief Get the absolute tick count at which a scheduled delayable work
 * will be submitted.
 *
 * @note This is a live snapshot of state, which may change before the result
 * can be inspected.  Use locks where appropriate.
 *
 * @funcprops \isr_ok
 *
 * @param dwork pointer to the delayable work item.
 *
 * @return the tick count when the timer that will schedule the work item will
 * expire, or the current tick count if the work is not scheduled.
 */
static inline k_ticks_t k_work_delayable_expires_get(
	const struct k_work_delayable *dwork);

/** @brief Get the number of ticks until a scheduled delayable work will be
 * submitted.
 *
 * @note This is a live snapshot of state, which may change before the result
 * can be inspected.  Use locks where appropriate.
 *
 * @funcprops \isr_ok
 *
 * @param dwork pointer to the delayable work item.
 *
 * @return the number of ticks until the timer that will schedule the work
 * item will expire, or zero if the item is not scheduled.
 */
static inline k_ticks_t k_work_delayable_remaining_get(
	const struct k_work_delayable *dwork);

/** @brief Submit an idle work item to a queue after a delay.
 *
 * Unlike k_work_reschedule_for_queue() this is a no-op if the work item is
 * already scheduled or submitted, even if @p delay is @c K_NO_WAIT.
 *
 * @funcprops \isr_ok
 *
 * @param queue the queue on which the work item should be submitted after the
 * delay.
 *
 * @param dwork pointer to the delayable work item.
 *
 * @param delay the time to wait before submitting the work item.  If @c
 * K_NO_WAIT and the work is not pending this is equivalent to
 * k_work_submit_to_queue().
 *
 * @retval 0 if work was already scheduled or submitted.
 * @retval 1 if work has been scheduled.
 * @retval -EBUSY if @p delay is @c K_NO_WAIT and
 *         k_work_submit_to_queue() fails with this code.
 * @retval -EINVAL if @p delay is @c K_NO_WAIT and
 *         k_work_submit_to_queue() fails with this code.
 * @retval -ENODEV if @p delay is @c K_NO_WAIT and
 *         k_work_submit_to_queue() fails with this code.
 */
int k_work_schedule_for_queue(struct k_work_q *queue,
			       struct k_work_delayable *dwork,
			       k_timeout_t delay);

/** @brief Submit an idle work item to the system work queue after a
 * delay.
 *
 * This is a thin wrapper around k_work_schedule_for_queue(), with all the API
 * characteristics of that function.
 *
 * @param dwork pointer to the delayable work item.
 *
 * @param delay the time to wait before submitting the work item.  If @c
 * K_NO_WAIT this is equivalent to k_work_submit_to_queue().
 *
 * @return as with k_work_schedule_for_queue().
 */
int k_work_schedule(struct k_work_delayable *dwork,
				   k_timeout_t delay);

/** @brief Reschedule a work item to a queue after a delay.
 *
 * Unlike k_work_schedule_for_queue() this function can change the deadline of
 * a scheduled work item, and will schedule a work item that is in any state
 * (e.g. is idle, submitted, or running).  This function does not affect
 * ("unsubmit") a work item that has been submitted to a queue.
 *
 * @funcprops \isr_ok
 *
 * @param queue the queue on which the work item should be submitted after the
 * delay.
 *
 * @param dwork pointer to the delayable work item.
 *
 * @param delay the time to wait before submitting the work item.  If @c
 * K_NO_WAIT this is equivalent to k_work_submit_to_queue() after canceling
 * any previous scheduled submission.
 *
 * @note If delay is @c K_NO_WAIT ("no delay") the return values are as with
 * k_work_submit_to_queue().
 *
 * @retval 0 if delay is @c K_NO_WAIT and work was already on a queue
 * @retval 1 if
 * * delay is @c K_NO_WAIT and work was not submitted but has now been queued
 *   to @p queue; or
 * * delay not @c K_NO_WAIT and work has been scheduled
 * @retval 2 if delay is @c K_NO_WAIT and work was running and has been queued
 * to the queue that was running it
 * @retval -EBUSY if @p delay is @c K_NO_WAIT and
 *         k_work_submit_to_queue() fails with this code.
 * @retval -EINVAL if @p delay is @c K_NO_WAIT and
 *         k_work_submit_to_queue() fails with this code.
 * @retval -ENODEV if @p delay is @c K_NO_WAIT and
 *         k_work_submit_to_queue() fails with this code.
 */
int k_work_reschedule_for_queue(struct k_work_q *queue,
				 struct k_work_delayable *dwork,
				 k_timeout_t delay);

/** @brief Reschedule a work item to the system work queue after a
 * delay.
 *
 * This is a thin wrapper around k_work_reschedule_for_queue(), with all the
 * API characteristics of that function.
 *
 * @param dwork pointer to the delayable work item.
 *
 * @param delay the time to wait before submitting the work item.
 *
 * @return as with k_work_reschedule_for_queue().
 */
int k_work_reschedule(struct k_work_delayable *dwork,
				     k_timeout_t delay);

/** @brief Flush delayable work.
 *
 * If the work is scheduled, it is immediately submitted.  Then the caller
 * blocks until the work completes, as with k_work_flush().
 *
 * @note Be careful of caller and work queue thread relative priority.  If
 * this function sleeps it will not return until the work queue thread
 * completes the tasks that allow this thread to resume.
 *
 * @note Behavior is undefined if this function is invoked on @p dwork from a
 * work queue running @p dwork.
 *
 * @param dwork pointer to the delayable work item.
 *
 * @param sync pointer to an opaque item containing state related to the
 * pending cancellation.  The object must persist until the call returns, and
 * be accessible from both the caller thread and the work queue thread.  The
 * object must not be used for any other flush or cancel operation until this
 * one completes.  On architectures with CONFIG_KERNEL_COHERENCE the object
 * must be allocated in coherent memory.
 *
 * @retval true if call had to wait for completion
 * @retval false if work was already idle
 */
bool k_work_flush_delayable(struct k_work_delayable *dwork,
			    struct k_work_sync *sync);

/** @brief Cancel delayable work.
 *
 * Similar to k_work_cancel() but for delayable work.  If the work is
 * scheduled or submitted it is canceled.  This function does not wait for the
 * cancellation to complete.
 *
 * @note The work may still be running when this returns.  Use
 * k_work_flush_delayable() or k_work_cancel_delayable_sync() to ensure it is
 * not running.
 *
 * @note Canceling delayable work does not prevent rescheduling it.  It does
 * prevent submitting it until the cancellation completes.
 *
 * @funcprops \isr_ok
 *
 * @param dwork pointer to the delayable work item.
 *
 * @return the k_work_delayable_busy_get() status indicating the state of the
 * item after all cancellation steps performed by this call are completed.
 */
int k_work_cancel_delayable(struct k_work_delayable *dwork);

/** @brief Cancel delayable work and wait.
 *
 * Like k_work_cancel_delayable() but waits until the work becomes idle.
 *
 * @note Canceling delayable work does not prevent rescheduling it.  It does
 * prevent submitting it until the cancellation completes.
 *
 * @note Be careful of caller and work queue thread relative priority.  If
 * this function sleeps it will not return until the work queue thread
 * completes the tasks that allow this thread to resume.
 *
 * @note Behavior is undefined if this function is invoked on @p dwork from a
 * work queue running @p dwork.
 *
 * @param dwork pointer to the delayable work item.
 *
 * @param sync pointer to an opaque item containing state related to the
 * pending cancellation.  The object must persist until the call returns, and
 * be accessible from both the caller thread and the work queue thread.  The
 * object must not be used for any other flush or cancel operation until this
 * one completes.  On architectures with CONFIG_KERNEL_COHERENCE the object
 * must be allocated in coherent memory.
 *
 * @retval true if work was not idle (call had to wait for cancellation of a
 * running handler to complete, or scheduled or submitted operations were
 * cancelled);
 * @retval false otherwise
 */
bool k_work_cancel_delayable_sync(struct k_work_delayable *dwork,
				  struct k_work_sync *sync);

enum {
/**
 * @cond INTERNAL_HIDDEN
 */

	/* The atomic API is used for all work and queue flags fields to
	 * enforce sequential consistency in SMP environments.
	 */

	/* Bits that represent the work item states.  At least nine of the
	 * combinations are distinct valid stable states.
	 */
	K_WORK_RUNNING_BIT = 0,
	K_WORK_CANCELING_BIT = 1,
	K_WORK_QUEUED_BIT = 2,
	K_WORK_DELAYED_BIT = 3,
	K_WORK_FLUSHING_BIT = 4,

	K_WORK_MASK = BIT(K_WORK_DELAYED_BIT) | BIT(K_WORK_QUEUED_BIT)
		| BIT(K_WORK_RUNNING_BIT) | BIT(K_WORK_CANCELING_BIT) | BIT(K_WORK_FLUSHING_BIT),

	/* Static work flags */
	K_WORK_DELAYABLE_BIT = 8,
	K_WORK_DELAYABLE = BIT(K_WORK_DELAYABLE_BIT),

	/* Dynamic work queue flags */
	K_WORK_QUEUE_STARTED_BIT = 0,
	K_WORK_QUEUE_STARTED = BIT(K_WORK_QUEUE_STARTED_BIT),
	K_WORK_QUEUE_BUSY_BIT = 1,
	K_WORK_QUEUE_BUSY = BIT(K_WORK_QUEUE_BUSY_BIT),
	K_WORK_QUEUE_DRAIN_BIT = 2,
	K_WORK_QUEUE_DRAIN = BIT(K_WORK_QUEUE_DRAIN_BIT),
	K_WORK_QUEUE_PLUGGED_BIT = 3,
	K_WORK_QUEUE_PLUGGED = BIT(K_WORK_QUEUE_PLUGGED_BIT),

	/* Static work queue flags */
	K_WORK_QUEUE_NO_YIELD_BIT = 8,
	K_WORK_QUEUE_NO_YIELD = BIT(K_WORK_QUEUE_NO_YIELD_BIT),

/**
 * INTERNAL_HIDDEN @endcond
 */
	/* Transient work flags */

	/** @brief Flag indicating a work item that is running under a work
	 * queue thread.
	 *
	 * Accessed via k_work_busy_get().  May co-occur with other flags.
	 */
	K_WORK_RUNNING = BIT(K_WORK_RUNNING_BIT),

	/** @brief Flag indicating a work item that is being canceled.
	 *
	 * Accessed via k_work_busy_get().  May co-occur with other flags.
	 */
	K_WORK_CANCELING = BIT(K_WORK_CANCELING_BIT),

	/** @brief Flag indicating a work item that has been submitted to a
	 * queue but has not started running.
	 *
	 * Accessed via k_work_busy_get().  May co-occur with other flags.
	 */
	K_WORK_QUEUED = BIT(K_WORK_QUEUED_BIT),

	/** @brief Flag indicating a delayed work item that is scheduled for
	 * submission to a queue.
	 *
	 * Accessed via k_work_busy_get().  May co-occur with other flags.
	 */
	K_WORK_DELAYED = BIT(K_WORK_DELAYED_BIT),

	/** @brief Flag indicating a synced work item that is being flushed.
	 *
	 * Accessed via k_work_busy_get().  May co-occur with other flags.
	 */
	K_WORK_FLUSHING = BIT(K_WORK_FLUSHING_BIT),
};

/** @brief A structure used to submit work. */
struct k_work {
	/* All fields are protected by the work module spinlock.  No fields
	 * are to be accessed except through kernel API.
	 */

	/* Node to link into k_work_q pending list. */
	sys_snode_t node;

	/* The function to be invoked by the work queue thread. */
	k_work_handler_t handler;

	/* The queue on which the work item was last submitted. */
	struct k_work_q *queue;

	/* State of the work item.
	 *
	 * The item can be DELAYED, QUEUED, and RUNNING simultaneously.
	 *
	 * It can be RUNNING and CANCELING simultaneously.
	 */
	uint32_t flags;
};

#define Z_WORK_INITIALIZER(work_handler) { \
	.handler = work_handler, \
}

/** @brief A structure used to submit work after a delay. */
struct k_work_delayable {
	/* The work item. */
	struct k_work work;

	/* Timeout used to submit work after a delay. */
	struct _timeout timeout;

	/* The queue to which the work should be submitted. */
	struct k_work_q *queue;
};

#define Z_WORK_DELAYABLE_INITIALIZER(work_handler) { \
	.work = { \
		.handler = work_handler, \
		.flags = K_WORK_DELAYABLE, \
	}, \
}

/**
 * @brief Initialize a statically-defined delayable work item.
 *
 * This macro can be used to initialize a statically-defined delayable
 * work item, prior to its first use. For example,
 *
 * @code static K_WORK_DELAYABLE_DEFINE(<dwork>, <work_handler>); @endcode
 *
 * Note that if the runtime dependencies support initialization with
 * k_work_init_delayable() using that will eliminate the initialized
 * object in ROM that is produced by this macro and copied in at
 * system startup.
 *
 * @param work Symbol name for delayable work item object
 * @param work_handler Function to invoke each time work item is processed.
 */
#define K_WORK_DELAYABLE_DEFINE(work, work_handler) \
	struct k_work_delayable work \
	  = Z_WORK_DELAYABLE_INITIALIZER(work_handler)

/**
 * @cond INTERNAL_HIDDEN
 */

/* Record used to wait for work to flush.
 *
 * The work item is inserted into the queue that will process (or is
 * processing) the item, and will be processed as soon as the item
 * completes.  When the flusher is processed the semaphore will be
 * signaled, releasing the thread waiting for the flush.
 */
struct z_work_flusher {
	struct k_work work;
	struct k_sem sem;
};

/* Record used to wait for work to complete a cancellation.
 *
 * The work item is inserted into a global queue of pending cancels.
 * When a cancelling work item goes idle any matching waiters are
 * removed from pending_cancels and are woken.
 */
struct z_work_canceller {
	sys_snode_t node;
	struct k_work *work;
	struct k_sem sem;
};

/**
 * INTERNAL_HIDDEN @endcond
 */

/** @brief A structure holding internal state for a pending synchronous
 * operation on a work item or queue.
 *
 * Instances of this type are provided by the caller for invocation of
 * k_work_flush(), k_work_cancel_sync() and sibling flush and cancel APIs.  A
 * referenced object must persist until the call returns, and be accessible
 * from both the caller thread and the work queue thread.
 *
 * @note If CONFIG_KERNEL_COHERENCE is enabled the object must be allocated in
 * coherent memory; see arch_mem_coherent().  The stack on these architectures
 * is generally not coherent.  be stack-allocated.  Violations are detected by
 * runtime assertion.
 */
struct k_work_sync {
	union {
		struct z_work_flusher flusher;
		struct z_work_canceller canceller;
	};
};

/** @brief A structure holding optional configuration items for a work
 * queue.
 *
 * This structure, and values it references, are not retained by
 * k_work_queue_start().
 */
struct k_work_queue_config {
	/** The name to be given to the work queue thread.
	 *
	 * If left null the thread will not have a name.
	 */
	const char *name;

	/** Control whether the work queue thread should yield between
	 * items.
	 *
	 * Yielding between items helps guarantee the work queue
	 * thread does not starve other threads, including cooperative
	 * ones released by a work item.  This is the default behavior.
	 *
	 * Set this to @c true to prevent the work queue thread from
	 * yielding between items.  This may be appropriate when a
	 * sequence of items should complete without yielding
	 * control.
	 */
	bool no_yield;

	/** Control whether the work queue thread should be marked as
	 * essential thread.
	 */
	bool essential;
};

/** @brief A structure used to hold work until it can be processed. */
struct k_work_q {
	/* The thread that animates the work. */
	struct k_thread thread;

	/* All the following fields must be accessed only while the
	 * work module spinlock is held.
	 */

	/* List of k_work items to be worked. */
	sys_slist_t pending;

	/* Wait queue for idle work thread. */
	_wait_q_t notifyq;

	/* Wait queue for threads waiting for the queue to drain. */
	_wait_q_t drainq;

	/* Flags describing queue state. */
	uint32_t flags;
};

/* Provide the implementation for inline functions declared above */

static inline bool k_work_is_pending(const struct k_work *work)
{
	return k_work_busy_get(work) != 0;
}

static inline struct k_work_delayable *
k_work_delayable_from_work(struct k_work *work)
{
	return CONTAINER_OF(work, struct k_work_delayable, work);
}

static inline bool k_work_delayable_is_pending(
	const struct k_work_delayable *dwork)
{
	return k_work_delayable_busy_get(dwork) != 0;
}

static inline k_ticks_t k_work_delayable_expires_get(
	const struct k_work_delayable *dwork)
{
	return z_timeout_expires(&dwork->timeout);
}

static inline k_ticks_t k_work_delayable_remaining_get(
	const struct k_work_delayable *dwork)
{
	return z_timeout_remaining(&dwork->timeout);
}

static inline k_tid_t k_work_queue_thread_get(struct k_work_q *queue)
{
	return &queue->thread;
}

/** @} */

struct k_work_user;

/**
 * @addtogroup workqueue_apis
 * @{
 */

/**
 * @typedef k_work_user_handler_t
 * @brief Work item handler function type for user work queues.
 *
 * A work item's handler function is executed by a user workqueue's thread
 * when the work item is processed by the workqueue.
 *
 * @param work Address of the work item.
 */
typedef void (*k_work_user_handler_t)(struct k_work_user *work);

/**
 * @cond INTERNAL_HIDDEN
 */

struct k_work_user_q {
	struct k_queue queue;
	struct k_thread thread;
};

enum {
	K_WORK_USER_STATE_PENDING,	/* Work item pending state */
};

struct k_work_user {
	void *_reserved;		/* Used by k_queue implementation. */
	k_work_user_handler_t handler;
	atomic_t flags;
};

/**
 * INTERNAL_HIDDEN @endcond
 */

#if defined(__cplusplus) && ((__cplusplus - 0) < 202002L)
#define Z_WORK_USER_INITIALIZER(work_handler) { NULL, work_handler, 0 }
#else
#define Z_WORK_USER_INITIALIZER(work_handler) \
	{ \
	._reserved = NULL, \
	.handler = work_handler, \
	.flags = 0 \
	}
#endif

/**
 * @brief Initialize a statically-defined user work item.
 *
 * This macro can be used to initialize a statically-defined user work
 * item, prior to its first use. For example,
 *
 * @code static K_WORK_USER_DEFINE(<work>, <work_handler>); @endcode
 *
 * @param work Symbol name for work item object
 * @param work_handler Function to invoke each time work item is processed.
 */
#define K_WORK_USER_DEFINE(work, work_handler) \
	struct k_work_user work = Z_WORK_USER_INITIALIZER(work_handler)

/**
 * @brief Initialize a userspace work item.
 *
 * This routine initializes a user workqueue work item, prior to its
 * first use.
 *
 * @param work Address of work item.
 * @param handler Function to invoke each time work item is processed.
 */
static inline void k_work_user_init(struct k_work_user *work,
				    k_work_user_handler_t handler)
{
	*work = (struct k_work_user)Z_WORK_USER_INITIALIZER(handler);
}

/**
 * @brief Check if a userspace work item is pending.
 *
 * This routine indicates if user work item @a work is pending in a workqueue's
 * queue.
 *
 * @note Checking if the work is pending gives no guarantee that the
 *       work will still be pending when this information is used. It is up to
 *       the caller to make sure that this information is used in a safe manner.
 *
 * @funcprops \isr_ok
 *
 * @param work Address of work item.
 *
 * @return true if work item is pending, or false if it is not pending.
 */
static inline bool k_work_user_is_pending(struct k_work_user *work)
{
	return atomic_test_bit(&work->flags, K_WORK_USER_STATE_PENDING);
}

/**
 * @brief Submit a work item to a user mode workqueue
 *
 * Submits a work item to a workqueue that runs in user mode. A temporary
 * memory allocation is made from the caller's resource pool which is freed
 * once the worker thread consumes the k_work item. The workqueue
 * thread must have memory access to the k_work item being submitted. The caller
 * must have permission granted on the work_q parameter's queue object.
 *
 * @funcprops \isr_ok
 *
 * @param work_q Address of workqueue.
 * @param work Address of work item.
 *
 * @retval -EBUSY if the work item was already in some workqueue
 * @retval -ENOMEM if no memory for thread resource pool allocation
 * @retval 0 Success
 */
static inline int k_work_user_submit_to_queue(struct k_work_user_q *work_q,
					      struct k_work_user *work)
{
	int ret = -EBUSY;

	if (!atomic_test_and_set_bit(&work->flags,
				     K_WORK_USER_STATE_PENDING)) {
		ret = k_queue_alloc_append(&work_q->queue, work);

		/* Couldn't insert into the queue. Clear the pending bit
		 * so the work item can be submitted again
		 */
		if (ret != 0) {
			atomic_clear_bit(&work->flags,
					 K_WORK_USER_STATE_PENDING);
		}
	}

	return ret;
}

/**
 * @brief Start a workqueue in user mode
 *
 * This works identically to k_work_queue_start() except it is callable from
 * user mode, and the worker thread created will run in user mode.  The caller
 * must have permissions granted on both the work_q parameter's thread and
 * queue objects, and the same restrictions on priority apply as
 * k_thread_create().
 *
 * @param work_q Address of workqueue.
 * @param stack Pointer to work queue thread's stack space, as defined by
 *		K_THREAD_STACK_DEFINE()
 * @param stack_size Size of the work queue thread's stack (in bytes), which
 *		should either be the same constant passed to
 *		K_THREAD_STACK_DEFINE() or the value of K_THREAD_STACK_SIZEOF().
 * @param prio Priority of the work queue's thread.
 * @param name optional thread name.  If not null a copy is made into the
 *		thread's name buffer.
 */
void k_work_user_queue_start(struct k_work_user_q *work_q,
				    k_thread_stack_t *stack,
				    size_t stack_size, int prio,
				    const char *name);

/**
 * @brief Access the user mode thread that animates a work queue.
 *
 * This is necessary to grant a user mode work queue thread access to things
 * the work items it will process are expected to use.
 *
 * @param work_q pointer to the user mode queue structure.
 *
 * @return the user mode thread associated with the work queue.
 */
static inline k_tid_t k_work_user_queue_thread_get(struct k_work_user_q *work_q)
{
	return &work_q->thread;
}

/** @} */

/**
 * @cond INTERNAL_HIDDEN
 */

struct k_work_poll {
	struct k_work work;
	struct k_work_q *workq;
	struct z_poller poller;
	struct k_poll_event *events;
	int num_events;
	k_work_handler_t real_handler;
	struct _timeout timeout;
	int poll_result;
};

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @addtogroup workqueue_apis
 * @{
 */

/**
 * @brief Initialize a statically-defined work item.
 *
 * This macro can be used to initialize a statically-defined workqueue work
 * item, prior to its first use. For example,
 *
 * @code static K_WORK_DEFINE(<work>, <work_handler>); @endcode
 *
 * @param work Symbol name for work item object
 * @param work_handler Function to invoke each time work item is processed.
 */
#define K_WORK_DEFINE(work, work_handler) \
	struct k_work work = Z_WORK_INITIALIZER(work_handler)

/**
 * @brief Initialize a triggered work item.
 *
 * This routine initializes a workqueue triggered work item, prior to
 * its first use.
 *
 * @param work Address of triggered work item.
 * @param handler Function to invoke each time work item is processed.
 */
void k_work_poll_init(struct k_work_poll *work,
			     k_work_handler_t handler);

/**
 * @brief Submit a triggered work item.
 *
 * This routine schedules work item @a work to be processed by workqueue
 * @a work_q when one of the given @a events is signaled. The routine
 * initiates internal poller for the work item and then returns to the caller.
 * Only when one of the watched events happen the work item is actually
 * submitted to the workqueue and becomes pending.
 *
 * Submitting a previously submitted triggered work item that is still
 * waiting for the event cancels the existing submission and reschedules it
 * the using the new event list. Note that this behavior is inherently subject
 * to race conditions with the pre-existing triggered work item and work queue,
 * so care must be taken to synchronize such resubmissions externally.
 *
 * @funcprops \isr_ok
 *
 * @warning
 * Provided array of events as well as a triggered work item must be placed
 * in persistent memory (valid until work handler execution or work
 * cancellation) and cannot be modified after submission.
 *
 * @param work_q Address of workqueue.
 * @param work Address of delayed work item.
 * @param events An array of events which trigger the work.
 * @param num_events The number of events in the array.
 * @param timeout Timeout after which the work will be scheduled
 *		  for execution even if not triggered.
 *
 *
 * @retval 0 Work item started watching for events.
 * @retval -EINVAL Work item is being processed or has completed its work.
 * @retval -EADDRINUSE Work item is pending on a different workqueue.
 */
int k_work_poll_submit_to_queue(struct k_work_q *work_q,
				       struct k_work_poll *work,
				       struct k_poll_event *events,
				       int num_events,
				       k_timeout_t timeout);

/**
 * @brief Submit a triggered work item to the system workqueue.
 *
 * This routine schedules work item @a work to be processed by system
 * workqueue when one of the given @a events is signaled. The routine
 * initiates internal poller for the work item and then returns to the caller.
 * Only when one of the watched events happen the work item is actually
 * submitted to the workqueue and becomes pending.
 *
 * Submitting a previously submitted triggered work item that is still
 * waiting for the event cancels the existing submission and reschedules it
 * the using the new event list. Note that this behavior is inherently subject
 * to race conditions with the pre-existing triggered work item and work queue,
 * so care must be taken to synchronize such resubmissions externally.
 *
 * @funcprops \isr_ok
 *
 * @warning
 * Provided array of events as well as a triggered work item must not be
 * modified until the item has been processed by the workqueue.
 *
 * @param work Address of delayed work item.
 * @param events An array of events which trigger the work.
 * @param num_events The number of events in the array.
 * @param timeout Timeout after which the work will be scheduled
 *		  for execution even if not triggered.
 *
 * @retval 0 Work item started watching for events.
 * @retval -EINVAL Work item is being processed or has completed its work.
 * @retval -EADDRINUSE Work item is pending on a different workqueue.
 */
int k_work_poll_submit(struct k_work_poll *work,
				     struct k_poll_event *events,
				     int num_events,
				     k_timeout_t timeout);

/**
 * @brief Cancel a triggered work item.
 *
 * This routine cancels the submission of triggered work item @a work.
 * A triggered work item can only be canceled if no event triggered work
 * submission.
 *
 * @funcprops \isr_ok
 *
 * @param work Address of delayed work item.
 *
 * @retval 0 Work item canceled.
 * @retval -EINVAL Work item is being processed or has completed its work.
 */
int k_work_poll_cancel(struct k_work_poll *work);

/** @} */

/**
 * @defgroup msgq_apis Message Queue APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief Message Queue Structure
 */
struct k_msgq {
	/** Message queue wait queue */
	_wait_q_t wait_q;
	/** Lock */
	struct k_spinlock lock;
	/** Message size */
	size_t msg_size;
	/** Maximal number of messages */
	uint32_t max_msgs;
	/** Start of message buffer */
	char *buffer_start;
	/** End of message buffer */
	char *buffer_end;
	/** Read pointer */
	char *read_ptr;
	/** Write pointer */
	char *write_ptr;
	/** Number of used messages */
	uint32_t used_msgs;

	Z_DECL_POLL_EVENT

	/** Message queue */
	uint8_t flags;

	SYS_PORT_TRACING_TRACKING_FIELD(k_msgq)

#ifdef CONFIG_OBJ_CORE_MSGQ
	struct k_obj_core  obj_core;
#endif
};
/**
 * @cond INTERNAL_HIDDEN
 */


#define Z_MSGQ_INITIALIZER(obj, q_buffer, q_msg_size, q_max_msgs) \
	{ \
	.wait_q = Z_WAIT_Q_INIT(&obj.wait_q), \
	.msg_size = q_msg_size, \
	.max_msgs = q_max_msgs, \
	.buffer_start = q_buffer, \
	.buffer_end = q_buffer + (q_max_msgs * q_msg_size), \
	.read_ptr = q_buffer, \
	.write_ptr = q_buffer, \
	.used_msgs = 0, \
	Z_POLL_EVENT_OBJ_INIT(obj) \
	}

/**
 * INTERNAL_HIDDEN @endcond
 */


#define K_MSGQ_FLAG_ALLOC	BIT(0)

/**
 * @brief Message Queue Attributes
 */
struct k_msgq_attrs {
	/** Message Size */
	size_t msg_size;
	/** Maximal number of messages */
	uint32_t max_msgs;
	/** Used messages */
	uint32_t used_msgs;
};


/**
 * @brief Statically define and initialize a message queue.
 *
 * The message queue's ring buffer contains space for @a q_max_msgs messages,
 * each of which is @a q_msg_size bytes long. Alignment of the message queue's
 * ring buffer is not necessary, setting @a q_align to 1 is sufficient.
 *
 * The message queue can be accessed outside the module where it is defined
 * using:
 *
 * @code extern struct k_msgq <name>; @endcode
 *
 * @param q_name Name of the message queue.
 * @param q_msg_size Message size (in bytes).
 * @param q_max_msgs Maximum number of messages that can be queued.
 * @param q_align Alignment of the message queue's ring buffer (power of 2).
 *
 */
#define K_MSGQ_DEFINE(q_name, q_msg_size, q_max_msgs, q_align)		\
	static char __noinit __aligned(q_align)				\
		_k_fifo_buf_##q_name[(q_max_msgs) * (q_msg_size)];	\
	STRUCT_SECTION_ITERABLE(k_msgq, q_name) =			\
	       Z_MSGQ_INITIALIZER(q_name, _k_fifo_buf_##q_name,	\
				  (q_msg_size), (q_max_msgs))

/**
 * @brief Initialize a message queue.
 *
 * This routine initializes a message queue object, prior to its first use.
 *
 * The message queue's ring buffer must contain space for @a max_msgs messages,
 * each of which is @a msg_size bytes long. Alignment of the message queue's
 * ring buffer is not necessary.
 *
 * @param msgq Address of the message queue.
 * @param buffer Pointer to ring buffer that holds queued messages.
 * @param msg_size Message size (in bytes).
 * @param max_msgs Maximum number of messages that can be queued.
 */
void k_msgq_init(struct k_msgq *msgq, char *buffer, size_t msg_size,
		 uint32_t max_msgs);

/**
 * @brief Initialize a message queue.
 *
 * This routine initializes a message queue object, prior to its first use,
 * allocating its internal ring buffer from the calling thread's resource
 * pool.
 *
 * Memory allocated for the ring buffer can be released by calling
 * k_msgq_cleanup(), or if userspace is enabled and the msgq object loses
 * all of its references.
 *
 * @param msgq Address of the message queue.
 * @param msg_size Message size (in bytes).
 * @param max_msgs Maximum number of messages that can be queued.
 *
 * @return 0 on success, -ENOMEM if there was insufficient memory in the
 *	thread's resource pool, or -EINVAL if the size parameters cause
 *	an integer overflow.
 */
__syscall int k_msgq_alloc_init(struct k_msgq *msgq, size_t msg_size,
				uint32_t max_msgs);

/**
 * @brief Release allocated buffer for a queue
 *
 * Releases memory allocated for the ring buffer.
 *
 * @param msgq message queue to cleanup
 *
 * @retval 0 on success
 * @retval -EBUSY Queue not empty
 */
int k_msgq_cleanup(struct k_msgq *msgq);

/**
 * @brief Send a message to a message queue.
 *
 * This routine sends a message to message queue @a q.
 *
 * @note The message content is copied from @a data into @a msgq and the @a data
 * pointer is not retained, so the message content will not be modified
 * by this function.
 *
 * @funcprops \isr_ok
 *
 * @param msgq Address of the message queue.
 * @param data Pointer to the message.
 * @param timeout Waiting period to add the message, or one of the special
 *                values K_NO_WAIT and K_FOREVER.
 *
 * @retval 0 Message sent.
 * @retval -ENOMSG Returned without waiting or queue purged.
 * @retval -EAGAIN Waiting period timed out.
 */
__syscall int k_msgq_put(struct k_msgq *msgq, const void *data, k_timeout_t timeout);

/**
 * @brief Receive a message from a message queue.
 *
 * This routine receives a message from message queue @a q in a "first in,
 * first out" manner.
 *
 * @note @a timeout must be set to K_NO_WAIT if called from ISR.
 *
 * @funcprops \isr_ok
 *
 * @param msgq Address of the message queue.
 * @param data Address of area to hold the received message.
 * @param timeout Waiting period to receive the message,
 *                or one of the special values K_NO_WAIT and
 *                K_FOREVER.
 *
 * @retval 0 Message received.
 * @retval -ENOMSG Returned without waiting.
 * @retval -EAGAIN Waiting period timed out.
 */
__syscall int k_msgq_get(struct k_msgq *msgq, void *data, k_timeout_t timeout);

/**
 * @brief Peek/read a message from a message queue.
 *
 * This routine reads a message from message queue @a q in a "first in,
 * first out" manner and leaves the message in the queue.
 *
 * @funcprops \isr_ok
 *
 * @param msgq Address of the message queue.
 * @param data Address of area to hold the message read from the queue.
 *
 * @retval 0 Message read.
 * @retval -ENOMSG Returned when the queue has no message.
 */
__syscall int k_msgq_peek(struct k_msgq *msgq, void *data);

/**
 * @brief Peek/read a message from a message queue at the specified index
 *
 * This routine reads a message from message queue at the specified index
 * and leaves the message in the queue.
 * k_msgq_peek_at(msgq, data, 0) is equivalent to k_msgq_peek(msgq, data)
 *
 * @funcprops \isr_ok
 *
 * @param msgq Address of the message queue.
 * @param data Address of area to hold the message read from the queue.
 * @param idx Message queue index at which to peek
 *
 * @retval 0 Message read.
 * @retval -ENOMSG Returned when the queue has no message at index.
 */
__syscall int k_msgq_peek_at(struct k_msgq *msgq, void *data, uint32_t idx);

/**
 * @brief Purge a message queue.
 *
 * This routine discards all unreceived messages in a message queue's ring
 * buffer. Any threads that are blocked waiting to send a message to the
 * message queue are unblocked and see an -ENOMSG error code.
 *
 * @param msgq Address of the message queue.
 */
__syscall void k_msgq_purge(struct k_msgq *msgq);

/**
 * @brief Get the amount of free space in a message queue.
 *
 * This routine returns the number of unused entries in a message queue's
 * ring buffer.
 *
 * @param msgq Address of the message queue.
 *
 * @return Number of unused ring buffer entries.
 */
__syscall uint32_t k_msgq_num_free_get(struct k_msgq *msgq);

/**
 * @brief Get basic attributes of a message queue.
 *
 * This routine fetches basic attributes of message queue into attr argument.
 *
 * @param msgq Address of the message queue.
 * @param attrs pointer to message queue attribute structure.
 */
__syscall void  k_msgq_get_attrs(struct k_msgq *msgq,
				 struct k_msgq_attrs *attrs);


static inline uint32_t z_impl_k_msgq_num_free_get(struct k_msgq *msgq)
{
	return msgq->max_msgs - msgq->used_msgs;
}

/**
 * @brief Get the number of messages in a message queue.
 *
 * This routine returns the number of messages in a message queue's ring buffer.
 *
 * @param msgq Address of the message queue.
 *
 * @return Number of messages.
 */
__syscall uint32_t k_msgq_num_used_get(struct k_msgq *msgq);

static inline uint32_t z_impl_k_msgq_num_used_get(struct k_msgq *msgq)
{
	return msgq->used_msgs;
}

/** @} */

/**
 * @defgroup mailbox_apis Mailbox APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief Mailbox Message Structure
 *
 */
struct k_mbox_msg {
	/** size of message (in bytes) */
	size_t size;
	/** application-defined information value */
	uint32_t info;
	/** sender's message data buffer */
	void *tx_data;
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
 * @brief Mailbox Structure
 *
 */
struct k_mbox {
	/** Transmit messages queue */
	_wait_q_t tx_msg_queue;
	/** Receive message queue */
	_wait_q_t rx_msg_queue;
	struct k_spinlock lock;

	SYS_PORT_TRACING_TRACKING_FIELD(k_mbox)

#ifdef CONFIG_OBJ_CORE_MAILBOX
	struct k_obj_core  obj_core;
#endif
};
/**
 * @cond INTERNAL_HIDDEN
 */

#define Z_MBOX_INITIALIZER(obj) \
	{ \
	.tx_msg_queue = Z_WAIT_Q_INIT(&obj.tx_msg_queue), \
	.rx_msg_queue = Z_WAIT_Q_INIT(&obj.rx_msg_queue), \
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
	STRUCT_SECTION_ITERABLE(k_mbox, name) = \
		Z_MBOX_INITIALIZER(name) \

/**
 * @brief Initialize a mailbox.
 *
 * This routine initializes a mailbox object, prior to its first use.
 *
 * @param mbox Address of the mailbox.
 */
void k_mbox_init(struct k_mbox *mbox);

/**
 * @brief Send a mailbox message in a synchronous manner.
 *
 * This routine sends a message to @a mbox and waits for a receiver to both
 * receive and process it. The message data may be in a buffer or non-existent
 * (i.e. an empty message).
 *
 * @param mbox Address of the mailbox.
 * @param tx_msg Address of the transmit message descriptor.
 * @param timeout Waiting period for the message to be received,
 *                or one of the special values K_NO_WAIT
 *                and K_FOREVER. Once the message has been received,
 *                this routine waits as long as necessary for the message
 *                to be completely processed.
 *
 * @retval 0 Message sent.
 * @retval -ENOMSG Returned without waiting.
 * @retval -EAGAIN Waiting period timed out.
 */
int k_mbox_put(struct k_mbox *mbox, struct k_mbox_msg *tx_msg,
		      k_timeout_t timeout);

/**
 * @brief Send a mailbox message in an asynchronous manner.
 *
 * This routine sends a message to @a mbox without waiting for a receiver
 * to process it. The message data may be in a buffer or non-existent
 * (i.e. an empty message). Optionally, the semaphore @a sem will be given
 * when the message has been both received and completely processed by
 * the receiver.
 *
 * @param mbox Address of the mailbox.
 * @param tx_msg Address of the transmit message descriptor.
 * @param sem Address of a semaphore, or NULL if none is needed.
 */
void k_mbox_async_put(struct k_mbox *mbox, struct k_mbox_msg *tx_msg,
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
 * @param timeout Waiting period for a message to be received,
 *                or one of the special values K_NO_WAIT and K_FOREVER.
 *
 * @retval 0 Message received.
 * @retval -ENOMSG Returned without waiting.
 * @retval -EAGAIN Waiting period timed out.
 */
int k_mbox_get(struct k_mbox *mbox, struct k_mbox_msg *rx_msg,
		      void *buffer, k_timeout_t timeout);

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
 */
void k_mbox_data_get(struct k_mbox_msg *rx_msg, void *buffer);

/** @} */

/**
 * @defgroup pipe_apis Pipe APIs
 * @ingroup kernel_apis
 * @{
 */

/** Pipe Structure */
struct k_pipe {
	unsigned char *buffer;          /**< Pipe buffer: may be NULL */
	size_t         size;            /**< Buffer size */
	size_t         bytes_used;      /**< # bytes used in buffer */
	size_t         read_index;      /**< Where in buffer to read from */
	size_t         write_index;     /**< Where in buffer to write */
	struct k_spinlock lock;		/**< Synchronization lock */

	struct {
		_wait_q_t      readers; /**< Reader wait queue */
		_wait_q_t      writers; /**< Writer wait queue */
	} wait_q;			/** Wait queue */

	Z_DECL_POLL_EVENT

	uint8_t	       flags;		/**< Flags */

	SYS_PORT_TRACING_TRACKING_FIELD(k_pipe)

#ifdef CONFIG_OBJ_CORE_PIPE
	struct k_obj_core  obj_core;
#endif
};

/**
 * @cond INTERNAL_HIDDEN
 */
#define K_PIPE_FLAG_ALLOC	BIT(0)	/** Buffer was allocated */

#define Z_PIPE_INITIALIZER(obj, pipe_buffer, pipe_buffer_size)     \
	{                                                           \
	.buffer = pipe_buffer,                                      \
	.size = pipe_buffer_size,                                   \
	.bytes_used = 0,                                            \
	.read_index = 0,                                            \
	.write_index = 0,                                           \
	.lock = {},                                                 \
	.wait_q = {                                                 \
		.readers = Z_WAIT_Q_INIT(&obj.wait_q.readers),       \
		.writers = Z_WAIT_Q_INIT(&obj.wait_q.writers)        \
	},                                                          \
	Z_POLL_EVENT_OBJ_INIT(obj)                                   \
	.flags = 0,                                                 \
	}

/**
 * INTERNAL_HIDDEN @endcond
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
 *
 */
#define K_PIPE_DEFINE(name, pipe_buffer_size, pipe_align)		\
	static unsigned char __noinit __aligned(pipe_align)		\
		_k_pipe_buf_##name[pipe_buffer_size];			\
	STRUCT_SECTION_ITERABLE(k_pipe, name) =				\
		Z_PIPE_INITIALIZER(name, _k_pipe_buf_##name, pipe_buffer_size)

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
 */
void k_pipe_init(struct k_pipe *pipe, unsigned char *buffer, size_t size);

/**
 * @brief Release a pipe's allocated buffer
 *
 * If a pipe object was given a dynamically allocated buffer via
 * k_pipe_alloc_init(), this will free it. This function does nothing
 * if the buffer wasn't dynamically allocated.
 *
 * @param pipe Address of the pipe.
 * @retval 0 on success
 * @retval -EAGAIN nothing to cleanup
 */
int k_pipe_cleanup(struct k_pipe *pipe);

/**
 * @brief Initialize a pipe and allocate a buffer for it
 *
 * Storage for the buffer region will be allocated from the calling thread's
 * resource pool. This memory will be released if k_pipe_cleanup() is called,
 * or userspace is enabled and the pipe object loses all references to it.
 *
 * This function should only be called on uninitialized pipe objects.
 *
 * @param pipe Address of the pipe.
 * @param size Size of the pipe's ring buffer (in bytes), or zero if no ring
 *             buffer is used.
 * @retval 0 on success
 * @retval -ENOMEM if memory couldn't be allocated
 */
__syscall int k_pipe_alloc_init(struct k_pipe *pipe, size_t size);

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
 * @param timeout Waiting period to wait for the data to be written,
 *                or one of the special values K_NO_WAIT and K_FOREVER.
 *
 * @retval 0 At least @a min_xfer bytes of data were written.
 * @retval -EIO Returned without waiting; zero data bytes were written.
 * @retval -EAGAIN Waiting period timed out; between zero and @a min_xfer
 *                 minus one data bytes were written.
 */
__syscall int k_pipe_put(struct k_pipe *pipe, const void *data,
			 size_t bytes_to_write, size_t *bytes_written,
			 size_t min_xfer, k_timeout_t timeout);

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
 * @param timeout Waiting period to wait for the data to be read,
 *                or one of the special values K_NO_WAIT and K_FOREVER.
 *
 * @retval 0 At least @a min_xfer bytes of data were read.
 * @retval -EINVAL invalid parameters supplied
 * @retval -EIO Returned without waiting; zero data bytes were read.
 * @retval -EAGAIN Waiting period timed out; between zero and @a min_xfer
 *                 minus one data bytes were read.
 */
__syscall int k_pipe_get(struct k_pipe *pipe, void *data,
			 size_t bytes_to_read, size_t *bytes_read,
			 size_t min_xfer, k_timeout_t timeout);

/**
 * @brief Query the number of bytes that may be read from @a pipe.
 *
 * @param pipe Address of the pipe.
 *
 * @retval a number n such that 0 <= n <= @ref k_pipe.size; the
 *         result is zero for unbuffered pipes.
 */
__syscall size_t k_pipe_read_avail(struct k_pipe *pipe);

/**
 * @brief Query the number of bytes that may be written to @a pipe
 *
 * @param pipe Address of the pipe.
 *
 * @retval a number n such that 0 <= n <= @ref k_pipe.size; the
 *         result is zero for unbuffered pipes.
 */
__syscall size_t k_pipe_write_avail(struct k_pipe *pipe);

/**
 * @brief Flush the pipe of write data
 *
 * This routine flushes the pipe. Flushing the pipe is equivalent to reading
 * both all the data in the pipe's buffer and all the data waiting to go into
 * that pipe into a large temporary buffer and discarding the buffer. Any
 * writers that were previously pended become unpended.
 *
 * @param pipe Address of the pipe.
 */
__syscall void k_pipe_flush(struct k_pipe *pipe);

/**
 * @brief Flush the pipe's internal buffer
 *
 * This routine flushes the pipe's internal buffer. This is equivalent to
 * reading up to N bytes from the pipe (where N is the size of the pipe's
 * buffer) into a temporary buffer and then discarding that buffer. If there
 * were writers previously pending, then some may unpend as they try to fill
 * up the pipe's emptied buffer.
 *
 * @param pipe Address of the pipe.
 */
__syscall void k_pipe_buffer_flush(struct k_pipe *pipe);

/** @} */

/**
 * @cond INTERNAL_HIDDEN
 */

struct k_mem_slab_info {
	uint32_t num_blocks;
	size_t   block_size;
	uint32_t num_used;
#ifdef CONFIG_MEM_SLAB_TRACE_MAX_UTILIZATION
	uint32_t max_used;
#endif
};

struct k_mem_slab {
	_wait_q_t wait_q;
	struct k_spinlock lock;
	char *buffer;
	char *free_list;
	struct k_mem_slab_info info;

	SYS_PORT_TRACING_TRACKING_FIELD(k_mem_slab)

#ifdef CONFIG_OBJ_CORE_MEM_SLAB
	struct k_obj_core  obj_core;
#endif
};

#define Z_MEM_SLAB_INITIALIZER(_slab, _slab_buffer, _slab_block_size, \
			       _slab_num_blocks)                      \
	{                                                             \
	.wait_q = Z_WAIT_Q_INIT(&(_slab).wait_q),                     \
	.lock = {},                                                   \
	.buffer = _slab_buffer,                                       \
	.free_list = NULL,                                            \
	.info = {_slab_num_blocks, _slab_block_size, 0}               \
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
 * @brief Statically define and initialize a memory slab in a public (non-static) scope.
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
 * @note This macro cannot be used together with a static keyword.
 *       If such a use-case is desired, use @ref K_MEM_SLAB_DEFINE_STATIC
 *       instead.
 *
 * @param name Name of the memory slab.
 * @param slab_block_size Size of each memory block (in bytes).
 * @param slab_num_blocks Number memory blocks.
 * @param slab_align Alignment of the memory slab's buffer (power of 2).
 */
#define K_MEM_SLAB_DEFINE(name, slab_block_size, slab_num_blocks, slab_align) \
	char __noinit_named(k_mem_slab_buf_##name) \
	   __aligned(WB_UP(slab_align)) \
	   _k_mem_slab_buf_##name[(slab_num_blocks) * WB_UP(slab_block_size)]; \
	STRUCT_SECTION_ITERABLE(k_mem_slab, name) = \
		Z_MEM_SLAB_INITIALIZER(name, _k_mem_slab_buf_##name, \
					WB_UP(slab_block_size), slab_num_blocks)

/**
 * @brief Statically define and initialize a memory slab in a private (static) scope.
 *
 * The memory slab's buffer contains @a slab_num_blocks memory blocks
 * that are @a slab_block_size bytes long. The buffer is aligned to a
 * @a slab_align -byte boundary. To ensure that each memory block is similarly
 * aligned to this boundary, @a slab_block_size must also be a multiple of
 * @a slab_align.
 *
 * @param name Name of the memory slab.
 * @param slab_block_size Size of each memory block (in bytes).
 * @param slab_num_blocks Number memory blocks.
 * @param slab_align Alignment of the memory slab's buffer (power of 2).
 */
#define K_MEM_SLAB_DEFINE_STATIC(name, slab_block_size, slab_num_blocks, slab_align) \
	static char __noinit_named(k_mem_slab_buf_##name) \
	   __aligned(WB_UP(slab_align)) \
	   _k_mem_slab_buf_##name[(slab_num_blocks) * WB_UP(slab_block_size)]; \
	static STRUCT_SECTION_ITERABLE(k_mem_slab, name) = \
		Z_MEM_SLAB_INITIALIZER(name, _k_mem_slab_buf_##name, \
					WB_UP(slab_block_size), slab_num_blocks)

/**
 * @brief Initialize a memory slab.
 *
 * Initializes a memory slab, prior to its first use.
 *
 * The memory slab's buffer contains @a slab_num_blocks memory blocks
 * that are @a slab_block_size bytes long. The buffer must be aligned to an
 * N-byte boundary matching a word boundary, where N is a power of 2
 * (i.e. 4 on 32-bit systems, 8, 16, ...).
 * To ensure that each memory block is similarly aligned to this boundary,
 * @a slab_block_size must also be a multiple of N.
 *
 * @param slab Address of the memory slab.
 * @param buffer Pointer to buffer used for the memory blocks.
 * @param block_size Size of each memory block (in bytes).
 * @param num_blocks Number of memory blocks.
 *
 * @retval 0 on success
 * @retval -EINVAL invalid data supplied
 *
 */
int k_mem_slab_init(struct k_mem_slab *slab, void *buffer,
			   size_t block_size, uint32_t num_blocks);

/**
 * @brief Allocate memory from a memory slab.
 *
 * This routine allocates a memory block from a memory slab.
 *
 * @note @a timeout must be set to K_NO_WAIT if called from ISR.
 * @note When CONFIG_MULTITHREADING=n any @a timeout is treated as K_NO_WAIT.
 *
 * @funcprops \isr_ok
 *
 * @param slab Address of the memory slab.
 * @param mem Pointer to block address area.
 * @param timeout Waiting period to wait for operation to complete.
 *        Use K_NO_WAIT to return without waiting,
 *        or K_FOREVER to wait as long as necessary.
 *
 * @retval 0 Memory allocated. The block address area pointed at by @a mem
 *         is set to the starting address of the memory block.
 * @retval -ENOMEM Returned without waiting.
 * @retval -EAGAIN Waiting period timed out.
 * @retval -EINVAL Invalid data supplied
 */
int k_mem_slab_alloc(struct k_mem_slab *slab, void **mem,
			    k_timeout_t timeout);

/**
 * @brief Free memory allocated from a memory slab.
 *
 * This routine releases a previously allocated memory block back to its
 * associated memory slab.
 *
 * @param slab Address of the memory slab.
 * @param mem Pointer to the memory block (as returned by k_mem_slab_alloc()).
 */
void k_mem_slab_free(struct k_mem_slab *slab, void *mem);

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
	return slab->info.num_used;
}

/**
 * @brief Get the number of maximum used blocks so far in a memory slab.
 *
 * This routine gets the maximum number of memory blocks that were
 * allocated in @a slab.
 *
 * @param slab Address of the memory slab.
 *
 * @return Maximum number of allocated memory blocks.
 */
static inline uint32_t k_mem_slab_max_used_get(struct k_mem_slab *slab)
{
#ifdef CONFIG_MEM_SLAB_TRACE_MAX_UTILIZATION
	return slab->info.max_used;
#else
	ARG_UNUSED(slab);
	return 0;
#endif
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
	return slab->info.num_blocks - slab->info.num_used;
}

/**
 * @brief Get the memory stats for a memory slab
 *
 * This routine gets the runtime memory usage stats for the slab @a slab.
 *
 * @param slab Address of the memory slab
 * @param stats Pointer to memory into which to copy memory usage statistics
 *
 * @retval 0 Success
 * @retval -EINVAL Any parameter points to NULL
 */

int k_mem_slab_runtime_stats_get(struct k_mem_slab *slab, struct sys_memory_stats *stats);

/**
 * @brief Reset the maximum memory usage for a slab
 *
 * This routine resets the maximum memory usage for the slab @a slab to its
 * current usage.
 *
 * @param slab Address of the memory slab
 *
 * @retval 0 Success
 * @retval -EINVAL Memory slab is NULL
 */
int k_mem_slab_runtime_stats_reset_max(struct k_mem_slab *slab);

/** @} */

/**
 * @addtogroup heap_apis
 * @{
 */

/* kernel synchronized heap struct */

struct k_heap {
	struct sys_heap heap;
	_wait_q_t wait_q;
	struct k_spinlock lock;
};

/**
 * @brief Initialize a k_heap
 *
 * This constructs a synchronized k_heap object over a memory region
 * specified by the user.  Note that while any alignment and size can
 * be passed as valid parameters, internal alignment restrictions
 * inside the inner sys_heap mean that not all bytes may be usable as
 * allocated memory.
 *
 * @param h Heap struct to initialize
 * @param mem Pointer to memory.
 * @param bytes Size of memory region, in bytes
 */
void k_heap_init(struct k_heap *h, void *mem,
		size_t bytes) __attribute_nonnull(1);

/** @brief Allocate aligned memory from a k_heap
 *
 * Behaves in all ways like k_heap_alloc(), except that the returned
 * memory (if available) will have a starting address in memory which
 * is a multiple of the specified power-of-two alignment value in
 * bytes.  The resulting memory can be returned to the heap using
 * k_heap_free().
 *
 * @note @a timeout must be set to K_NO_WAIT if called from ISR.
 * @note When CONFIG_MULTITHREADING=n any @a timeout is treated as K_NO_WAIT.
 *
 * @funcprops \isr_ok
 *
 * @param h Heap from which to allocate
 * @param align Alignment in bytes, must be a power of two
 * @param bytes Number of bytes requested
 * @param timeout How long to wait, or K_NO_WAIT
 * @return Pointer to memory the caller can now use
 */
void *k_heap_aligned_alloc(struct k_heap *h, size_t align, size_t bytes,
			k_timeout_t timeout) __attribute_nonnull(1);

/**
 * @brief Allocate memory from a k_heap
 *
 * Allocates and returns a memory buffer from the memory region owned
 * by the heap.  If no memory is available immediately, the call will
 * block for the specified timeout (constructed via the standard
 * timeout API, or K_NO_WAIT or K_FOREVER) waiting for memory to be
 * freed.  If the allocation cannot be performed by the expiration of
 * the timeout, NULL will be returned.
 * Allocated memory is aligned on a multiple of pointer sizes.
 *
 * @note @a timeout must be set to K_NO_WAIT if called from ISR.
 * @note When CONFIG_MULTITHREADING=n any @a timeout is treated as K_NO_WAIT.
 *
 * @funcprops \isr_ok
 *
 * @param h Heap from which to allocate
 * @param bytes Desired size of block to allocate
 * @param timeout How long to wait, or K_NO_WAIT
 * @return A pointer to valid heap memory, or NULL
 */
void *k_heap_alloc(struct k_heap *h, size_t bytes,
		k_timeout_t timeout) __attribute_nonnull(1);

/**
 * @brief Free memory allocated by k_heap_alloc()
 *
 * Returns the specified memory block, which must have been returned
 * from k_heap_alloc(), to the heap for use by other callers.  Passing
 * a NULL block is legal, and has no effect.
 *
 * @param h Heap to which to return the memory
 * @param mem A valid memory block, or NULL
 */
void k_heap_free(struct k_heap *h, void *mem) __attribute_nonnull(1);

/* Hand-calculated minimum heap sizes needed to return a successful
 * 1-byte allocation.  See details in lib/os/heap.[ch]
 */
#define Z_HEAP_MIN_SIZE (sizeof(void *) > 4 ? 56 : 44)

/**
 * @brief Define a static k_heap in the specified linker section
 *
 * This macro defines and initializes a static memory region and
 * k_heap of the requested size in the specified linker section.
 * After kernel start, &name can be used as if k_heap_init() had
 * been called.
 *
 * Note that this macro enforces a minimum size on the memory region
 * to accommodate metadata requirements.  Very small heaps will be
 * padded to fit.
 *
 * @param name Symbol name for the struct k_heap object
 * @param bytes Size of memory region, in bytes
 * @param in_section __attribute__((section(name))
 */
#define Z_HEAP_DEFINE_IN_SECT(name, bytes, in_section)		\
	char in_section						\
	     __aligned(8) /* CHUNK_UNIT */			\
	     kheap_##name[MAX(bytes, Z_HEAP_MIN_SIZE)];		\
	STRUCT_SECTION_ITERABLE(k_heap, name) = {		\
		.heap = {					\
			.init_mem = kheap_##name,		\
			.init_bytes = MAX(bytes, Z_HEAP_MIN_SIZE), \
		 },						\
	}

/**
 * @brief Define a static k_heap
 *
 * This macro defines and initializes a static memory region and
 * k_heap of the requested size.  After kernel start, &name can be
 * used as if k_heap_init() had been called.
 *
 * Note that this macro enforces a minimum size on the memory region
 * to accommodate metadata requirements.  Very small heaps will be
 * padded to fit.
 *
 * @param name Symbol name for the struct k_heap object
 * @param bytes Size of memory region, in bytes
 */
#define K_HEAP_DEFINE(name, bytes)				\
	Z_HEAP_DEFINE_IN_SECT(name, bytes,			\
			      __noinit_named(kheap_buf_##name))

/**
 * @brief Define a static k_heap in uncached memory
 *
 * This macro defines and initializes a static memory region and
 * k_heap of the requested size in uncached memory.  After kernel
 * start, &name can be used as if k_heap_init() had been called.
 *
 * Note that this macro enforces a minimum size on the memory region
 * to accommodate metadata requirements.  Very small heaps will be
 * padded to fit.
 *
 * @param name Symbol name for the struct k_heap object
 * @param bytes Size of memory region, in bytes
 */
#define K_HEAP_DEFINE_NOCACHE(name, bytes)			\
	Z_HEAP_DEFINE_IN_SECT(name, bytes, __nocache)

/**
 * @}
 */

/**
 * @defgroup heap_apis Heap APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief Allocate memory from the heap with a specified alignment.
 *
 * This routine provides semantics similar to aligned_alloc(); memory is
 * allocated from the heap with a specified alignment. However, one minor
 * difference is that k_aligned_alloc() accepts any non-zero @p size,
 * whereas aligned_alloc() only accepts a @p size that is an integral
 * multiple of @p align.
 *
 * Above, aligned_alloc() refers to:
 * C11 standard (ISO/IEC 9899:2011): 7.22.3.1
 * The aligned_alloc function (p: 347-348)
 *
 * @param align Alignment of memory requested (in bytes).
 * @param size Amount of memory requested (in bytes).
 *
 * @return Address of the allocated memory if successful; otherwise NULL.
 */
void *k_aligned_alloc(size_t align, size_t size);

/**
 * @brief Allocate memory from the heap.
 *
 * This routine provides traditional malloc() semantics. Memory is
 * allocated from the heap memory pool.
 * Allocated memory is aligned on a multiple of pointer sizes.
 *
 * @param size Amount of memory requested (in bytes).
 *
 * @return Address of the allocated memory if successful; otherwise NULL.
 */
void *k_malloc(size_t size);

/**
 * @brief Free memory allocated from heap.
 *
 * This routine provides traditional free() semantics. The memory being
 * returned must have been allocated from the heap memory pool.
 *
 * If @a ptr is NULL, no operation is performed.
 *
 * @param ptr Pointer to previously allocated memory.
 */
void k_free(void *ptr);

/**
 * @brief Allocate memory from heap, array style
 *
 * This routine provides traditional calloc() semantics. Memory is
 * allocated from the heap memory pool and zeroed.
 *
 * @param nmemb Number of elements in the requested array
 * @param size Size of each array element (in bytes).
 *
 * @return Address of the allocated memory if successful; otherwise NULL.
 */
void *k_calloc(size_t nmemb, size_t size);

/** @} */

/* polling API - PRIVATE */

#ifdef CONFIG_POLL
#define _INIT_OBJ_POLL_EVENT(obj) do { (obj)->poll_event = NULL; } while (false)
#else
#define _INIT_OBJ_POLL_EVENT(obj) do { } while (false)
#endif

/* private - types bit positions */
enum _poll_types_bits {
	/* can be used to ignore an event */
	_POLL_TYPE_IGNORE,

	/* to be signaled by k_poll_signal_raise() */
	_POLL_TYPE_SIGNAL,

	/* semaphore availability */
	_POLL_TYPE_SEM_AVAILABLE,

	/* queue/FIFO/LIFO data availability */
	_POLL_TYPE_DATA_AVAILABLE,

	/* msgq data availability */
	_POLL_TYPE_MSGQ_DATA_AVAILABLE,

	/* pipe data availability */
	_POLL_TYPE_PIPE_DATA_AVAILABLE,

	_POLL_NUM_TYPES
};

#define Z_POLL_TYPE_BIT(type) (1U << ((type) - 1U))

/* private - states bit positions */
enum _poll_states_bits {
	/* default state when creating event */
	_POLL_STATE_NOT_READY,

	/* signaled by k_poll_signal_raise() */
	_POLL_STATE_SIGNALED,

	/* semaphore is available */
	_POLL_STATE_SEM_AVAILABLE,

	/* data is available to read on queue/FIFO/LIFO */
	_POLL_STATE_DATA_AVAILABLE,

	/* queue/FIFO/LIFO wait was cancelled */
	_POLL_STATE_CANCELLED,

	/* data is available to read on a message queue */
	_POLL_STATE_MSGQ_DATA_AVAILABLE,

	/* data is available to read from a pipe */
	_POLL_STATE_PIPE_DATA_AVAILABLE,

	_POLL_NUM_STATES
};

#define Z_POLL_STATE_BIT(state) (1U << ((state) - 1U))

#define _POLL_EVENT_NUM_UNUSED_BITS \
	(32 - (0 \
	       + 8 /* tag */ \
	       + _POLL_NUM_TYPES \
	       + _POLL_NUM_STATES \
	       + 1 /* modes */ \
	      ))

/* end of polling API - PRIVATE */


/**
 * @defgroup poll_apis Async polling APIs
 * @ingroup kernel_apis
 * @{
 */

/* Public polling API */

/* public - values for k_poll_event.type bitfield */
#define K_POLL_TYPE_IGNORE 0
#define K_POLL_TYPE_SIGNAL Z_POLL_TYPE_BIT(_POLL_TYPE_SIGNAL)
#define K_POLL_TYPE_SEM_AVAILABLE Z_POLL_TYPE_BIT(_POLL_TYPE_SEM_AVAILABLE)
#define K_POLL_TYPE_DATA_AVAILABLE Z_POLL_TYPE_BIT(_POLL_TYPE_DATA_AVAILABLE)
#define K_POLL_TYPE_FIFO_DATA_AVAILABLE K_POLL_TYPE_DATA_AVAILABLE
#define K_POLL_TYPE_MSGQ_DATA_AVAILABLE Z_POLL_TYPE_BIT(_POLL_TYPE_MSGQ_DATA_AVAILABLE)
#define K_POLL_TYPE_PIPE_DATA_AVAILABLE Z_POLL_TYPE_BIT(_POLL_TYPE_PIPE_DATA_AVAILABLE)

/* public - polling modes */
enum k_poll_modes {
	/* polling thread does not take ownership of objects when available */
	K_POLL_MODE_NOTIFY_ONLY = 0,

	K_POLL_NUM_MODES
};

/* public - values for k_poll_event.state bitfield */
#define K_POLL_STATE_NOT_READY 0
#define K_POLL_STATE_SIGNALED Z_POLL_STATE_BIT(_POLL_STATE_SIGNALED)
#define K_POLL_STATE_SEM_AVAILABLE Z_POLL_STATE_BIT(_POLL_STATE_SEM_AVAILABLE)
#define K_POLL_STATE_DATA_AVAILABLE Z_POLL_STATE_BIT(_POLL_STATE_DATA_AVAILABLE)
#define K_POLL_STATE_FIFO_DATA_AVAILABLE K_POLL_STATE_DATA_AVAILABLE
#define K_POLL_STATE_MSGQ_DATA_AVAILABLE Z_POLL_STATE_BIT(_POLL_STATE_MSGQ_DATA_AVAILABLE)
#define K_POLL_STATE_PIPE_DATA_AVAILABLE Z_POLL_STATE_BIT(_POLL_STATE_PIPE_DATA_AVAILABLE)
#define K_POLL_STATE_CANCELLED Z_POLL_STATE_BIT(_POLL_STATE_CANCELLED)

/* public - poll signal object */
struct k_poll_signal {
	/** PRIVATE - DO NOT TOUCH */
	sys_dlist_t poll_events;

	/**
	 * 1 if the event has been signaled, 0 otherwise. Stays set to 1 until
	 * user resets it to 0.
	 */
	unsigned int signaled;

	/** custom result value passed to k_poll_signal_raise() if needed */
	int result;
};

#define K_POLL_SIGNAL_INITIALIZER(obj) \
	{ \
	.poll_events = SYS_DLIST_STATIC_INIT(&obj.poll_events), \
	.signaled = 0, \
	.result = 0, \
	}
/**
 * @brief Poll Event
 *
 */
struct k_poll_event {
	/** PRIVATE - DO NOT TOUCH */
	sys_dnode_t _node;

	/** PRIVATE - DO NOT TOUCH */
	struct z_poller *poller;

	/** optional user-specified tag, opaque, untouched by the API */
	uint32_t tag:8;

	/** bitfield of event types (bitwise-ORed K_POLL_TYPE_xxx values) */
	uint32_t type:_POLL_NUM_TYPES;

	/** bitfield of event states (bitwise-ORed K_POLL_STATE_xxx values) */
	uint32_t state:_POLL_NUM_STATES;

	/** mode of operation, from enum k_poll_modes */
	uint32_t mode:1;

	/** unused bits in 32-bit word */
	uint32_t unused:_POLL_EVENT_NUM_UNUSED_BITS;

	/** per-type data */
	union {
		void *obj;
		struct k_poll_signal *signal;
		struct k_sem *sem;
		struct k_fifo *fifo;
		struct k_queue *queue;
		struct k_msgq *msgq;
#ifdef CONFIG_PIPES
		struct k_pipe *pipe;
#endif
	};
};

#define K_POLL_EVENT_INITIALIZER(_event_type, _event_mode, _event_obj) \
	{ \
	.poller = NULL, \
	.type = _event_type, \
	.state = K_POLL_STATE_NOT_READY, \
	.mode = _event_mode, \
	.unused = 0, \
	{ \
		.obj = _event_obj, \
	}, \
	}

#define K_POLL_EVENT_STATIC_INITIALIZER(_event_type, _event_mode, _event_obj, \
					event_tag) \
	{ \
	.tag = event_tag, \
	.type = _event_type, \
	.state = K_POLL_STATE_NOT_READY, \
	.mode = _event_mode, \
	.unused = 0, \
	{ \
		.obj = _event_obj, \
	}, \
	}

/**
 * @brief Initialize one struct k_poll_event instance
 *
 * After this routine is called on a poll event, the event it ready to be
 * placed in an event array to be passed to k_poll().
 *
 * @param event The event to initialize.
 * @param type A bitfield of the types of event, from the K_POLL_TYPE_xxx
 *             values. Only values that apply to the same object being polled
 *             can be used together. Choosing K_POLL_TYPE_IGNORE disables the
 *             event.
 * @param mode Future. Use K_POLL_MODE_NOTIFY_ONLY.
 * @param obj Kernel object or poll signal.
 */

void k_poll_event_init(struct k_poll_event *event, uint32_t type,
			      int mode, void *obj);

/**
 * @brief Wait for one or many of multiple poll events to occur
 *
 * This routine allows a thread to wait concurrently for one or many of
 * multiple poll events to have occurred. Such events can be a kernel object
 * being available, like a semaphore, or a poll signal event.
 *
 * When an event notifies that a kernel object is available, the kernel object
 * is not "given" to the thread calling k_poll(): it merely signals the fact
 * that the object was available when the k_poll() call was in effect. Also,
 * all threads trying to acquire an object the regular way, i.e. by pending on
 * the object, have precedence over the thread polling on the object. This
 * means that the polling thread will never get the poll event on an object
 * until the object becomes available and its pend queue is empty. For this
 * reason, the k_poll() call is more effective when the objects being polled
 * only have one thread, the polling thread, trying to acquire them.
 *
 * When k_poll() returns 0, the caller should loop on all the events that were
 * passed to k_poll() and check the state field for the values that were
 * expected and take the associated actions.
 *
 * Before being reused for another call to k_poll(), the user has to reset the
 * state field to K_POLL_STATE_NOT_READY.
 *
 * When called from user mode, a temporary memory allocation is required from
 * the caller's resource pool.
 *
 * @param events An array of events to be polled for.
 * @param num_events The number of events in the array.
 * @param timeout Waiting period for an event to be ready,
 *                or one of the special values K_NO_WAIT and K_FOREVER.
 *
 * @retval 0 One or more events are ready.
 * @retval -EAGAIN Waiting period timed out.
 * @retval -EINTR Polling has been interrupted, e.g. with
 *         k_queue_cancel_wait(). All output events are still set and valid,
 *         cancelled event(s) will be set to K_POLL_STATE_CANCELLED. In other
 *         words, -EINTR status means that at least one of output events is
 *         K_POLL_STATE_CANCELLED.
 * @retval -ENOMEM Thread resource pool insufficient memory (user mode only)
 * @retval -EINVAL Bad parameters (user mode only)
 */

__syscall int k_poll(struct k_poll_event *events, int num_events,
		     k_timeout_t timeout);

/**
 * @brief Initialize a poll signal object.
 *
 * Ready a poll signal object to be signaled via k_poll_signal_raise().
 *
 * @param sig A poll signal.
 */

__syscall void k_poll_signal_init(struct k_poll_signal *sig);

/*
 * @brief Reset a poll signal object's state to unsignaled.
 *
 * @param sig A poll signal object
 */
__syscall void k_poll_signal_reset(struct k_poll_signal *sig);

/**
 * @brief Fetch the signaled state and result value of a poll signal
 *
 * @param sig A poll signal object
 * @param signaled An integer buffer which will be written nonzero if the
 *		   object was signaled
 * @param result An integer destination buffer which will be written with the
 *		   result value if the object was signaled, or an undefined
 *		   value if it was not.
 */
__syscall void k_poll_signal_check(struct k_poll_signal *sig,
				   unsigned int *signaled, int *result);

/**
 * @brief Signal a poll signal object.
 *
 * This routine makes ready a poll signal, which is basically a poll event of
 * type K_POLL_TYPE_SIGNAL. If a thread was polling on that event, it will be
 * made ready to run. A @a result value can be specified.
 *
 * The poll signal contains a 'signaled' field that, when set by
 * k_poll_signal_raise(), stays set until the user sets it back to 0 with
 * k_poll_signal_reset(). It thus has to be reset by the user before being
 * passed again to k_poll() or k_poll() will consider it being signaled, and
 * will return immediately.
 *
 * @note The result is stored and the 'signaled' field is set even if
 * this function returns an error indicating that an expiring poll was
 * not notified.  The next k_poll() will detect the missed raise.
 *
 * @param sig A poll signal.
 * @param result The value to store in the result field of the signal.
 *
 * @retval 0 The signal was delivered successfully.
 * @retval -EAGAIN The polling thread's timeout is in the process of expiring.
 */

__syscall int k_poll_signal_raise(struct k_poll_signal *sig, int result);

/** @} */

/**
 * @defgroup cpu_idle_apis CPU Idling APIs
 * @ingroup kernel_apis
 * @{
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
 * @note In some architectures, before returning, the function unmasks interrupts
 * unconditionally.
 */
static inline void k_cpu_idle(void)
{
	arch_cpu_idle();
}

/**
 * @brief Make the CPU idle in an atomic fashion.
 *
 * Similar to k_cpu_idle(), but must be called with interrupts locked.
 *
 * Enabling interrupts and entering a low-power mode will be atomic,
 * i.e. there will be no period of time where interrupts are enabled before
 * the processor enters a low-power mode.
 *
 * After waking up from the low-power mode, the interrupt lockout state will
 * be restored as if by irq_unlock(key).
 *
 * @param key Interrupt locking key obtained from irq_lock().
 */
static inline void k_cpu_atomic_idle(unsigned int key)
{
	arch_cpu_atomic_idle(key);
}

/**
 * @}
 */

/**
 * @cond INTERNAL_HIDDEN
 * @internal
 */
#ifdef ARCH_EXCEPT
/* This architecture has direct support for triggering a CPU exception */
#define z_except_reason(reason)	ARCH_EXCEPT(reason)
#else

#if !defined(CONFIG_ASSERT_NO_FILE_INFO)
#define __EXCEPT_LOC() __ASSERT_PRINT("@ %s:%d\n", __FILE__, __LINE__)
#else
#define __EXCEPT_LOC()
#endif

/* NOTE: This is the implementation for arches that do not implement
 * ARCH_EXCEPT() to generate a real CPU exception.
 *
 * We won't have a real exception frame to determine the PC value when
 * the oops occurred, so print file and line number before we jump into
 * the fatal error handler.
 */
#define z_except_reason(reason) do { \
		__EXCEPT_LOC();              \
		z_fatal_error(reason, NULL); \
	} while (false)

#endif /* _ARCH__EXCEPT */
/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @brief Fatally terminate a thread
 *
 * This should be called when a thread has encountered an unrecoverable
 * runtime condition and needs to terminate. What this ultimately
 * means is determined by the _fatal_error_handler() implementation, which
 * will be called will reason code K_ERR_KERNEL_OOPS.
 *
 * If this is called from ISR context, the default system fatal error handler
 * will treat it as an unrecoverable system error, just like k_panic().
 */
#define k_oops()	z_except_reason(K_ERR_KERNEL_OOPS)

/**
 * @brief Fatally terminate the system
 *
 * This should be called when the Zephyr kernel has encountered an
 * unrecoverable runtime condition and needs to terminate. What this ultimately
 * means is determined by the _fatal_error_handler() implementation, which
 * will be called will reason code K_ERR_KERNEL_PANIC.
 */
#define k_panic()	z_except_reason(K_ERR_KERNEL_PANIC)

/**
 * @cond INTERNAL_HIDDEN
 */

/*
 * private APIs that are utilized by one or more public APIs
 */

/**
 * @internal
 */
void z_timer_expiration_handler(struct _timeout *timeout);
/**
 * INTERNAL_HIDDEN @endcond
 */

#ifdef CONFIG_PRINTK
/**
 * @brief Emit a character buffer to the console device
 *
 * @param c String of characters to print
 * @param n The length of the string
 *
 */
__syscall void k_str_out(char *c, size_t n);
#endif

/**
 * @defgroup float_apis Floating Point APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief Disable preservation of floating point context information.
 *
 * This routine informs the kernel that the specified thread
 * will no longer be using the floating point registers.
 *
 * @warning
 * Some architectures apply restrictions on how the disabling of floating
 * point preservation may be requested, see arch_float_disable.
 *
 * @warning
 * This routine should only be used to disable floating point support for
 * a thread that currently has such support enabled.
 *
 * @param thread ID of thread.
 *
 * @retval 0        On success.
 * @retval -ENOTSUP If the floating point disabling is not implemented.
 *         -EINVAL  If the floating point disabling could not be performed.
 */
__syscall int k_float_disable(struct k_thread *thread);

/**
 * @brief Enable preservation of floating point context information.
 *
 * This routine informs the kernel that the specified thread
 * will use the floating point registers.

 * Invoking this routine initializes the thread's floating point context info
 * to that of an FPU that has been reset. The next time the thread is scheduled
 * by z_swap() it will either inherit an FPU that is guaranteed to be in a
 * "sane" state (if the most recent user of the FPU was cooperatively swapped
 * out) or the thread's own floating point context will be loaded (if the most
 * recent user of the FPU was preempted, or if this thread is the first user
 * of the FPU). Thereafter, the kernel will protect the thread's FP context
 * so that it is not altered during a preemptive context switch.
 *
 * The @a options parameter indicates which floating point register sets will
 * be used by the specified thread.
 *
 * For x86 options:
 *
 * - K_FP_REGS  indicates x87 FPU and MMX registers only
 * - K_SSE_REGS indicates SSE registers (and also x87 FPU and MMX registers)
 *
 * @warning
 * Some architectures apply restrictions on how the enabling of floating
 * point preservation may be requested, see arch_float_enable.
 *
 * @warning
 * This routine should only be used to enable floating point support for
 * a thread that currently has such support enabled.
 *
 * @param thread  ID of thread.
 * @param options architecture dependent options
 *
 * @retval 0        On success.
 * @retval -ENOTSUP If the floating point enabling is not implemented.
 *         -EINVAL  If the floating point enabling could not be performed.
 */
__syscall int k_float_enable(struct k_thread *thread, unsigned int options);

/**
 * @}
 */

/**
 * @brief Get the runtime statistics of a thread
 *
 * @param thread ID of thread.
 * @param stats Pointer to struct to copy statistics into.
 * @return -EINVAL if null pointers, otherwise 0
 */
int k_thread_runtime_stats_get(k_tid_t thread,
			       k_thread_runtime_stats_t *stats);

/**
 * @brief Get the runtime statistics of all threads
 *
 * @param stats Pointer to struct to copy statistics into.
 * @return -EINVAL if null pointers, otherwise 0
 */
int k_thread_runtime_stats_all_get(k_thread_runtime_stats_t *stats);

/**
 * @brief Enable gathering of runtime statistics for specified thread
 *
 * This routine enables the gathering of runtime statistics for the specified
 * thread.
 *
 * @param thread ID of thread
 * @return -EINVAL if invalid thread ID, otherwise 0
 */
int k_thread_runtime_stats_enable(k_tid_t thread);

/**
 * @brief Disable gathering of runtime statistics for specified thread
 *
 * This routine disables the gathering of runtime statistics for the specified
 * thread.
 *
 * @param thread ID of thread
 * @return -EINVAL if invalid thread ID, otherwise 0
 */
int k_thread_runtime_stats_disable(k_tid_t thread);

/**
 * @brief Enable gathering of system runtime statistics
 *
 * This routine enables the gathering of system runtime statistics. Note that
 * it does not affect the gathering of similar statistics for individual
 * threads.
 */
void k_sys_runtime_stats_enable(void);

/**
 * @brief Disable gathering of system runtime statistics
 *
 * This routine disables the gathering of system runtime statistics. Note that
 * it does not affect the gathering of similar statistics for individual
 * threads.
 */
void k_sys_runtime_stats_disable(void);

#ifdef __cplusplus
}
#endif

#include <zephyr/tracing/tracing.h>
#include <syscalls/kernel.h>

#endif /* !_ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_KERNEL_H_ */
