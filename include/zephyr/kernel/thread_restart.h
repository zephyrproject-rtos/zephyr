/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Restart a thread after a fatal error
 */

#ifndef ZEPHYR_INCLUDE_KERNEL_THREAD_RESTART_H_
#define ZEPHYR_INCLUDE_KERNEL_THREAD_RESTART_H_

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Thread restart on fatal error APIs
 * @defgroup thread_restart_apis Thread restart on fatal error APIs
 * @ingroup fatal_apis
 * @since 4.6
 * @version 0.1.0
 * @{
 */

struct k_thread_restart;

/**
 * @brief Callback run before a restarted thread is started
 *
 * Called by the system workqueue thread after the thread has been re-created with a
 * @c K_FOREVER start delay, named, assigned its resource pool and added to its memory domain,
 * and before it is started. It can complete the setup of the thread, for example pin it to a
 * CPU, change its priority or grant it access to kernel objects, or decide not to restart it.
 *
 * It must not start or abort the thread, and should return quickly: it blocks the system
 * workqueue.
 *
 * @param rst Thread restart descriptor; use CONTAINER_OF() to reach application data
 * @param thread The re-created thread, not started yet
 *
 * @return 0 to start the thread. Any other value vetoes the restart: the thread is aborted
 *         without being started, which wakes threads waiting in k_thread_join() for it. The
 *         restart still counts against the restart limit.
 */
typedef int (*k_thread_restart_cb_t)(struct k_thread_restart *rst, k_tid_t thread);

/**
 * @brief Thread restart descriptor
 *
 * Holds the parameters used to re-create a thread with k_thread_create() after it has been
 * aborted by the fatal error handling, and the restart policy.
 *
 * Define it with K_THREAD_RESTART_DEFINE() or initialize it with k_thread_restart_init().
 *
 * The descriptor must have static storage duration, as a global or a @c static local
 * variable, and must not be declared on a stack: the system workqueue uses it after the
 * faulting thread, and the function that scheduled the restart, are gone.
 */
struct k_thread_restart {
	/** Thread object to re-create */
	struct k_thread *thread;

	/** Thread stack */
	k_thread_stack_t *stack;

	/** Thread stack size in bytes */
	size_t stack_size;

	/** Thread entry point */
	k_thread_entry_t entry;

	/** First entry point parameter */
	void *p1;

	/** Second entry point parameter */
	void *p2;

	/** Third entry point parameter */
	void *p3;

	/** Thread priority */
	int prio;

	/** Thread options, see k_thread_create() */
	uint32_t options;

	/**
	 * Thread name, or NULL. Only used with @kconfig{CONFIG_THREAD_NAME}. The string is
	 * copied each time the thread is restarted, so it must stay valid as long as the
	 * descriptor is in use.
	 */
	const char *name;

	/** Callback run before the restarted thread is started, or NULL */
	k_thread_restart_cb_t cb;

	/**
	 * Maximum number of restarts, 0 for unlimited. A restart dropped by the system workqueue
	 * still counts.
	 */
	uint32_t max_restarts;

	/** Initial interval in milliseconds to poll for the aborted thread to exit */
	uint32_t poll_min_ms;

	/** Maximum interval in milliseconds to poll for the aborted thread to exit */
	uint32_t poll_max_ms;

	/** @cond INTERNAL_HIDDEN */
	/** @internal Resource pool of the thread, recorded by k_thread_restart_schedule() */
	struct k_heap *resource_pool;

	/**
	 * @internal Memory domain of the thread, recorded by k_thread_restart_schedule(). NULL
	 * without @kconfig{CONFIG_USERSPACE}.
	 */
	struct k_mem_domain *mem_domain;

	/** @internal Number of restarts scheduled, see k_thread_restart_count_get() */
	uint32_t restart_count;

	/** @internal Current poll interval in ticks, doubled up to @c poll_max_ms */
	k_ticks_t poll_ticks;

	/** @internal Restart scheduled and the thread not re-created yet */
	bool pending;

	/** @internal Restart work item on the system workqueue */
	struct k_work_delayable work;

	/** @endcond */
};

/** @cond INTERNAL_HIDDEN */
/**
 * @internal
 * @brief Restart work handler, referenced by K_THREAD_RESTART_DEFINE()
 *
 * @param work Work item of a thread restart descriptor
 */
void z_thread_restart_handler(struct k_work *work);
/** @endcond */

/**
 * @brief Statically define and initialize a thread restart descriptor
 *
 * The poll intervals are set to
 * @kconfig{CONFIG_THREAD_RESTART_ON_FATAL_ERROR_POLL_MIN_MS} and
 * @kconfig{CONFIG_THREAD_RESTART_ON_FATAL_ERROR_POLL_MAX_MS}; use
 * k_thread_restart_poll_set() to override them.
 *
 * @param _name Name of the descriptor variable
 * @param _thread Pointer to the thread object to re-create
 * @param _stack Thread stack
 * @param _stack_size Thread stack size in bytes
 * @param _entry Thread entry point
 * @param _p1 First entry point parameter
 * @param _p2 Second entry point parameter
 * @param _p3 Third entry point parameter
 * @param _prio Thread priority
 * @param _options Thread options
 * @param _thread_name Thread name, or NULL
 * @param _max_restarts Maximum number of restarts, 0 for unlimited
 * @param _cb Callback run before the restarted thread is started (k_thread_restart_cb_t), or
 *            NULL
 */
#define K_THREAD_RESTART_DEFINE(_name, _thread, _stack, _stack_size, _entry, _p1, _p2, _p3, _prio, \
				_options, _thread_name, _max_restarts, _cb)                        \
	struct k_thread_restart _name = {                                                          \
		.thread = (_thread),                                                               \
		.stack = (_stack),                                                                 \
		.stack_size = (_stack_size),                                                       \
		.entry = (_entry),                                                                 \
		.p1 = (_p1),                                                                       \
		.p2 = (_p2),                                                                       \
		.p3 = (_p3),                                                                       \
		.prio = (_prio),                                                                   \
		.options = (_options),                                                             \
		.name = (_thread_name),                                                            \
		.resource_pool = NULL,                                                             \
		.mem_domain = NULL,                                                                \
		.cb = (_cb),                                                                       \
		.max_restarts = (_max_restarts),                                                   \
		.poll_min_ms = CONFIG_THREAD_RESTART_ON_FATAL_ERROR_POLL_MIN_MS,                   \
		.poll_max_ms = CONFIG_THREAD_RESTART_ON_FATAL_ERROR_POLL_MAX_MS,                   \
		.restart_count = 0U,                                                               \
		.poll_ticks = 0,                                                                   \
		.pending = false,                                                                  \
		.work = Z_WORK_DELAYABLE_INITIALIZER(z_thread_restart_handler),                    \
	}

/**
 * @brief Initialize a thread restart descriptor at runtime
 *
 * Runtime equivalent of K_THREAD_RESTART_DEFINE(). Takes the k_thread_create() parameters the
 * thread is re-created with, without the start delay, followed by its name and the restart
 * limit and the callback. Resets the restart count and sets the poll intervals to
 * @kconfig{CONFIG_THREAD_RESTART_ON_FATAL_ERROR_POLL_MIN_MS} and
 * @kconfig{CONFIG_THREAD_RESTART_ON_FATAL_ERROR_POLL_MAX_MS}. Must not be called while a
 * restart is pending or the restart work is running.
 *
 * Only @p thread and @p options are checked, by k_thread_restart_schedule(). The other
 * parameters are passed to k_thread_create() as they are when the thread is restarted. @p rst
 * must not be on a stack, see @ref k_thread_restart.
 *
 * @param rst Thread restart descriptor
 * @param thread Thread object to re-create
 * @param stack Thread stack
 * @param stack_size Thread stack size in bytes
 * @param entry Thread entry point
 * @param p1 First entry point parameter
 * @param p2 Second entry point parameter
 * @param p3 Third entry point parameter
 * @param prio Thread priority
 * @param options Thread options
 * @param name Thread name, or NULL. Only used with @kconfig{CONFIG_THREAD_NAME}. Must stay
 *             valid as long as the descriptor is in use, see @ref k_thread_restart.
 * @param max_restarts Maximum number of restarts, 0 for unlimited
 * @param cb Callback run before the restarted thread is started, or NULL
 */
void k_thread_restart_init(struct k_thread_restart *rst, struct k_thread *thread,
			   k_thread_stack_t *stack, size_t stack_size, k_thread_entry_t entry,
			   void *p1, void *p2, void *p3, int prio, uint32_t options,
			   const char *name, uint32_t max_restarts, k_thread_restart_cb_t cb);

/**
 * @brief Override the poll intervals of a thread restart descriptor
 *
 * After a restart is scheduled, the restart work item polls for the aborted thread to exit,
 * starting at @p min_ms and doubling the interval on each attempt up to @p max_ms.
 *
 * @param rst Thread restart descriptor
 * @param min_ms Initial poll interval in milliseconds
 * @param max_ms Maximum poll interval in milliseconds
 *
 * @retval 0 Poll intervals updated
 * @retval -EINVAL @p rst is NULL, @p min_ms is 0 or @p min_ms is greater than @p max_ms
 * @retval -EBUSY A restart is pending
 */
int k_thread_restart_poll_set(struct k_thread_restart *rst, uint32_t min_ms, uint32_t max_ms);

/**
 * @brief Schedule a restart of the faulting thread
 *
 * To be called from a custom k_sys_fatal_error_handler() on behalf of the faulting thread. It
 * schedules a delayable work item on the system workqueue which, once the thread has exited,
 * re-creates it with the parameters in @p rst. Polling for the exit starts at
 * @c rst->poll_min_ms and backs off up to @c rst->poll_max_ms.
 *
 * k_sys_fatal_error_handler() must return for the thread to be aborted and restarted. The
 * caller decides whether the fault is recoverable. A restart may be requested for a
 * @c K_ERR_KERNEL_PANIC only with @kconfig{CONFIG_ASSERT} disabled: otherwise z_fatal_error()
 * asserts when k_sys_fatal_error_handler() returns for it, outside @kconfig{CONFIG_TEST}. It
 * should not be requested for a fault taken in interrupt context, where recovery is not
 * supported. If the thread is never
 * aborted, the work item keeps polling.
 *
 * A thread may also call this function on itself outside the fatal error path; it is then
 * re-created once it has exited.
 *
 * This function records the resource pool and, with @kconfig{CONFIG_USERSPACE}, the memory
 * domain of the thread. The system workqueue thread re-creates the thread with a @c K_FOREVER
 * start delay, assigns it the recorded resource pool, adds it to the recorded memory domain
 * and then starts it, so neither is inherited from the workqueue thread. If the thread cannot
 * be added to the memory domain, it is aborted instead of started. @c rst->cb, if set, is then
 * called and can veto the restart, see @ref k_thread_restart_cb_t. Other changes made to the
 * thread after its creation, such as CPU affinity, priority or deadline, are not restored.
 * Resources held by the faulting thread, such as mutexes or heap allocations, are not
 * released. The recorded resource pool and memory domain must stay initialized until the
 * thread has been restarted: once the thread is aborted it no longer belongs to the memory
 * domain, so k_mem_domain_deinit() does not refuse to de-initialize it.
 *
 * If the restart work has to wait while polling for the thread to exit, the restart is dropped
 * if the system workqueue is draining, plugged or not started when the wait ends, although
 * this function returned 0. The thread is then not restarted: only a thread that is still
 * running can schedule a new restart, an aborted thread must be re-created by the application.
 *
 * @note Can be called with interrupts locked. It takes the thread restart, workqueue, timeout
 * and scheduler spinlocks, so it deadlocks if the fault happened while the current CPU held
 * one of them, e.g. inside k_thread_restart_poll_set() with an invalid descriptor.
 *
 * @param rst Thread restart descriptor of the faulting thread
 *
 * @retval 0 Restart scheduled
 * @retval -EINVAL @p rst or its thread is NULL, its thread is not the current thread, or the
 *         options contain @c K_INHERIT_PERMS, which would grant the workqueue thread's object
 *         permissions
 * @retval -ENOTSUP The thread is the system workqueue thread, is essential or is a user
 *         thread, or the options contain @c K_USER or @c K_ESSENTIAL
 * @retval -EALREADY A restart is already pending
 * @retval -ENOSPC The maximum number of restarts has been reached
 * @retval -EBUSY The system workqueue is draining or plugged
 * @retval -ENODEV The system workqueue has not been started
 */
int k_thread_restart_schedule(struct k_thread_restart *rst);

/**
 * @brief Get the number of restarts scheduled for a thread restart descriptor
 *
 * @param rst Thread restart descriptor
 *
 * @return Number of restarts scheduled since the descriptor was defined or initialized
 */
static inline uint32_t k_thread_restart_count_get(const struct k_thread_restart *rst)
{
	return rst->restart_count;
}

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_KERNEL_THREAD_RESTART_H_ */
