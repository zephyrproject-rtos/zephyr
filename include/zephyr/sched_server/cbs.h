/*
 *  Copyright (c) 2024 Instituto Superior de Engenharia do Porto (ISEP)
 *  SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Constant Bandwidth Server (CBS) public API
 */

#ifndef ZEPHYR_CBS
#define ZEPHYR_CBS

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_CBS

/**
 * @defgroup cbs_apis Constant Bandwidth Server (CBS) APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief CBS job format.
 *
 * A job pushed to a CBS is a regular function that must
 * have a void pointer as an argument (can be NULL if not
 * needed) and return nothing. jobs are pushed to the CBS by
 * invoking k_cbs_push_job().
 */
typedef void (*cbs_callback_t)(void *arg);

/**
 * @cond INTERNAL_HIDDEN
 */

#ifdef CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER
typedef uint64_t cbs_cycle_t;
#else
typedef uint32_t cbs_cycle_t;
#endif

struct cbs_job {
	cbs_callback_t function;
	void *arg;
};

struct cbs_budget {
	cbs_cycle_t current;
	cbs_cycle_t max;
};

struct cbs_arg {
	k_timeout_t budget;
	k_timeout_t period;
};

struct k_cbs {
	struct k_timer timer;
	struct k_msgq *queue;
	struct k_thread *thread;
	struct cbs_budget budget;
	cbs_cycle_t period;
	cbs_cycle_t abs_deadline;
	cbs_cycle_t start_cycle;
	cbs_cycle_t bandwidth;
	bool is_active;
	unsigned int left_shift;
#ifdef CONFIG_CBS_LOG
	char name[CONFIG_CBS_THREAD_MAX_NAME_LEN];
#endif
};

extern void cbs_thread(void *job_queue, void *cbs_struct, void *unused);

/** @endcond */

/**
 * @brief pushes a job to a CBS queue.
 *
 * This routine inserts a job (i.e. a regular C function) in a CBS queue,
 * which will eventually execute it when the CBS deadline becomes the
 * earliest of the taskset. Inserted jobs are always served in a FIFO
 * manner. The job queue can store up to @kconfig{CONFIG_CBS_QUEUE_LENGTH}
 * jobs at once.
 *
 * @param cbs Name of the CBS.
 * @param job_function Function of the job.
 * @param job_arg Argument to be passed to the job function.
 * @param timeout Waiting period to push the job, or one of the special
 * values K_NO_WAIT and K_FOREVER.
 *
 * @retval 0 the job was pushed to the CBS queue.
 * @retval -ENOMSG if CBS is not defined, returned without waiting or CBS queue purged.
 * @retval -EAGAIN if waiting period timed out.
 */
int k_cbs_push_job(struct k_cbs *cbs, cbs_callback_t job_function, void *job_arg,
		   k_timeout_t timeout);

/**
 * @brief Statically define and initialize a Constant Bandwidth Server (CBS).
 *
 * A CBS is an extension of the Earliest Deadline First (EDF) scheduler
 * that allows tasks to be executed virtually isolated from each other,
 * in a way that if a task executes for longer than expected it doesn’t
 * interfere on the execution of the others. In other words, the CBS
 * prevents that a task misbehavior causes other tasks to miss their
 * own deadlines.
 *
 * In a nutshell, the CBS is a work-conserving wrapper for the tasks
 * that automatically recalculates their deadlines when they exceed their
 * allowed execution time slice. This time slice is known as the CBS "budget".
 * The value used to recalculate the deadline is known as the CBS "period".
 *
 * When a task instance (i.e. job) runs within a CBS, it consumes the "budget".
 * When the "budget" runs out, the deadline is postponed by "period" time units
 * and the "budget" is replenished to its maximum capacity. when there are no jobs
 * left for a CBS to execute, it remains idle and takes no CPU time.
 *
 * Finally, whenever a new job is pushed to an idle server, the kernel verifies
 * if the current pair of (budget, deadline) are proportionally compatible with
 * the configured values. If not, the deadline is also recalculated here.
 * These two procedures ensure the CBS will never use the CPU more than
 * what was configured, and that it will not endanger other thread's deadlines.
 *
 * Once created, the CBS can be referenced through its name:
 *
 * @code extern const struct k_cbs <cbs_name>; @endcode
 *
 * @param cbs_name Name of the CBS.
 * @param cbs_budget Budget of the CBS thread, in system ticks.
 * Used for triggering deadline recalculations.
 * @param cbs_period Period of the CBS thread. in system ticks.
 * Used for recalculating the absolute deadline.
 * @param cbs_static_priority Static priority of the CBS thread.
 *
 * @note The CBS is meant to be used alongside the EDF policy, which in Zephyr
 * is effectively used as a "tie-breaker" when two threads of equal static priorities
 * are ready for execution. Therefore it is recommended that all user threads feature
 * the same preemptive static priority (e.g. 5) in order to ensure the scheduling
 * to work as expected, and that this same value is passed as @a cbs_static_priority.
 *
 * @note you should have @kconfig{CONFIG_CBS} enabled in your project to use the CBS.
 */
#define K_CBS_DEFINE(cbs_name, cbs_budget, cbs_period, cbs_static_priority)                        \
	K_MSGQ_DEFINE(queue_##cbs_name, sizeof(struct cbs_job), CONFIG_CBS_QUEUE_LENGTH, 1);       \
	static struct k_cbs cbs_name = {                                                           \
		.queue = &queue_##cbs_name,                                                        \
		.is_active = false,                                                                \
	};                                                                                         \
	static struct cbs_arg args_##cbs_name = {.budget = cbs_budget, .period = cbs_period};      \
	K_THREAD_DEFINE(thread_##cbs_name, CONFIG_CBS_THREAD_STACK_SIZE, cbs_thread,               \
			(void *)STRINGIFY(cbs_name), (void *)&cbs_name, (void *)&args_##cbs_name,  \
					  cbs_static_priority, 0, CONFIG_CBS_INITIAL_DELAY)
/** @} */ /* end of Constant Bandwidth Server (CBS) APIs */

#endif /* CONFIG_CBS */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_CBS */
