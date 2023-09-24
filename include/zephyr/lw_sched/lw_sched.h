/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright(c) 2023 Intel Corporation
 */

#ifndef ZEPHYR_INCLUDE_LW_SCHED_LW_SCHED_H_
#define ZEPHYR_INCLUDE_LW_SCHED_LW_SCHED_H_

#include <zephyr/kernel.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Lightweight Scheduler
 * @defgroup subsys_lw_sched Lightweight Scheduler
 * @ingroup os_services
 * @{
 */

struct lw_scheduler;
struct lw_task;

/** @brief Lightweight task operation handlers */
struct lw_task_ops {
	int  (*execute)(void *arg);   /**< Execute handler */
	void (*abort)(void *arg);     /**< Optional abort handler */
};

/** @brief Arguments to lightweight task operation handlers */
struct lw_task_args {
	void *execute;                /**< Args for execute handler */
	void *abort;                  /**< Args for abort handler */
};

#define LW_TASK_PAUSED  0             /**< Task is in a paused state */
#define LW_TASK_EXECUTE 1             /**< Task is executing */
#define LW_TASK_ABORT   2             /**< Task is aborted */

/** @brief Lightweight task structure */
struct lw_task {
	sys_dnode_t node;               /**< Node in list of tasks */
	sys_dnode_t reorder;            /**< Node in reorder list */
	struct lw_scheduler *sched;     /**< Associated scheduler */
	const struct lw_task_ops *ops;  /**< Task operations */
	struct lw_task_args *args;      /**< Task operation arguments */
	uint32_t state;                 /**< Task state */
	uint32_t order;                 /**< Task order */
	uint32_t new_order;             /**< New task order */
	uint32_t delay;                 /**< Number of intervals to delay */
};

/** @brief Lightweight task scheduler */
struct lw_scheduler {
	struct k_thread thread;  /**< Thread from which to execute tasks */
	struct k_timer timer;    /**< Timer driving the periodicity */
	struct k_sem    sem;     /**< Activation semaphore */
	struct k_spinlock lock;  /**< Scheduler's spinlock */
	struct lw_task *current; /**< Current lw_task being executed */
	sys_dlist_t list;        /**< List of tasks */
	sys_dlist_t reorder;     /**< List of tasks to be reordered */
	k_timeout_t period;      /**< Scheduler's period */
};

/**
 * @brief Initialize the specified lightweight (LW) scheduler
 *
 * This function initializes the lightweight scheduler specified by @a sched
 * but does not start it. This scheduler is essentially a thread that loops
 * through a list of handlers (aka tasks) at regular intervals defined by
 * @a period.
 *
 * @param sched Pointer to the LW scheduler to initialize
 * @param stack Pointer to the stack used by the LW scheduler's thread
 * @param stack_size Size of the stack used by the LW scheduler
 * @param priority Priority of the LW scheduler's thread
 * @param options Options to the LW scheduler's thread
 * @param period LW scheduler periodicity
 *
 * @return Pointer to the initialized LW scheduler
 */
struct lw_scheduler *lw_scheduler_init(struct lw_scheduler *sched,
				       k_thread_stack_t *stack,
				       size_t stack_size,
				       int priority, uint32_t options,
				       k_timeout_t  period);

/**
 * @brief Start the specified lightweight (LW) scheduler
 *
 * This function starts the specified LW scheduler so that it begins processing
 * its associated tasks after some after the specified delay. It should only be
 * called once per LW scheduler instance.
 *
 * @param sched Pointer to the LW scheduler to start
 * @param delay Initial duration until the associated LW tasks are processed
 */
void lw_scheduler_start(struct lw_scheduler *sched, k_timeout_t delay);

/**
 * @brief Abort the specified lightweight (LW) scheduler
 *
 * This routine aborts the specified LW scheduler. It terminates its scheduling
 * thread and then proceeds to call the abort handlers associated with its LW
 * tasks (if any exist). This routine should not be called from any LW task
 * belonging to the LW scheduler being terminated.
 *
 * @param sched Pointer to the LW scheduler to abort
 */
void lw_scheduler_abort(struct lw_scheduler *sched);

/**
 * @brief Initialize a lightweight (LW) task
 *
 * This function initializes the specified LW task and assigns it to the
 * specified LW scheduler.
 *
 * @param task Pointer to the LW task to initialize
 * @param ops Pointer to the LW task's set of handlers
 * @param args Pointer to the arguments passed to the LW task's handlers
 * @param sched Pointer to the LW scheduler to which to assign the LW task
 * @param order Identifies the relative order in which the LW task executes
 *              within the context of the LW scheduler
 *
 * @return Pointer to the initialized LW task on success; NULL on error
 */
struct lw_task *lw_task_init(struct lw_task *task,
			     struct lw_task_ops *ops,
			     struct lw_task_args *args,
			     struct lw_scheduler *sched,
			     uint32_t order);

/**
 * @brief Start the specified lightweight (LW) task
 *
 * This function starts the specified LW task. It can also be used to resume
 * a paused LW task. However, it can not be used to start an aborted LW task.
 *
 * @param task Pointer to LW task to start
 *
 * @return 0 on success; -EINVAL if LW task was aborted
 */
int lw_task_start(struct lw_task *task);

/**
 * @brief Abort the specified lightweight (LW) task
 *
 * This routine aborts the specified LW task. If the targeted LW task is the
 * current LW task, then the abort will be delayed until the LW task returns
 * to the LW scheduler.
 *
 * Note that if the LW task had an abort handler when it was initialized, that
 * handler will not be invoked as a result of calling this routine. The abort
 * handler is only invoked when the entire LW scheduler is aborted. Thus, any
 * resource cleanup must be handled by the caller or elsewhere.
 *
 * @param task Pointer to the LW task to abort
 */
void lw_task_abort(struct lw_task *task);

/**
 * @brief Pause the specified lightweight (LW) task
 *
 * This routine pauses the specified LW task. If the targeted LW task is the
 * current LW task, then the abort will be delayed until the LW task returns
 * to the LW scheduler.
 *
 * @param task Pointer to the LW task to pause
 */
void lw_task_pause(struct lw_task *task);

/**
 * @brief Delay the specified lightweight (LW) task
 *
 * This routine causes the specified LW task to delay for a specified number of
 * intervals, where an interval is the periodicity of the LW scheduler. Setting
 * @a num_intervals will cancel the delay. The delay only applies while the
 * LW task is in the execute state.
 *
 * @param task Pointer to the LW task to delay
 * @param num_intervals The number of LW scheduler intervals to delay
 */
void lw_task_delay(struct lw_task *task, uint32_t num_intervals);

/**
 * @brief Identify the currently executing lightweight (LW) task
 *
 * This function returns a pointer to the LW task that the specified LW
 * scheduler is currently processing.
 *
 * @param sched Pointer to the LW scheduler
 *
 * @return Pointer to current LW task; NULL if idle
 */
struct lw_task *lw_task_current_get(struct lw_scheduler *sched);

/**
 * @brief Reorder the specified lightweight (LW) task
 *
 * This routine informs the LW scheduler that it must reorder its list of LW
 * tasks to change the placement of the specified LW task according to its
 * new ordering value. The new order will take effect on the next pass through
 * this list (to ensure that each LW task is executed at most once per pass).
 *
 * @param task Pointer to the LW task
 * @param new_order New ordering value for the LW task
 */
void lw_task_reorder(struct lw_task *task, uint32_t new_order);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_LW_SCHED_LW_SCHED_H_ */
