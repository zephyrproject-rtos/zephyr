/*
 *  Copyright (c) 2024 Instituto Superior de Engenharia do Porto (ISEP)
 *  SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Constant Bandwidth Server (CBS) implementation
 *
 * This module provides the implementation of the Constant
 * Bandwidth Server (CBS), an extension of the EDF scheduler
 * which isolates thread execution time unpredictabilities
 * from the rest of the system, thus reducing interference
 * in other thread's execution.
 */
#include <zephyr/sched_server/cbs.h>
#include <sched_server/cbs_internal.h>
#include <ksched.h>
#include <string.h>

#ifdef CONFIG_CBS_LOG
#include <sched_server/cbs_log.h>
#endif /* CONFIG_CBS_LOG */

#ifdef CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER
#define cbs_get_now() k_cycle_get_64()
#else
#define cbs_get_now() k_cycle_get_32()
#endif

static void cbs_replenish_due_to_condition(struct k_cbs *cbs, cbs_cycle_t cycle)
{
	/*
	 * if the budget replenishing condition is
	 * met when a new job comes to the server,
	 * this function is executed.
	 */
	cbs->abs_deadline = cycle + cbs->period;
	cbs->budget.current = cbs->budget.max;

#if defined(CONFIG_CBS_LOG) && defined(CONFIG_CBS_LOG_BUDGET_CONDITION)
	cbs_log(CBS_BUDGET_CONDITION_MET, cbs);
#endif
}

static inline void cbs_replenish_due_to_run_out(struct k_cbs *cbs, cbs_cycle_t cycle)
{
	/*
	 * if job is still running when the budget
	 * runs out, this function is executed.
	 */
	cbs->start_cycle = cycle;
	cbs->abs_deadline += cbs->period;
	cbs->budget.current = cbs->budget.max;

#if defined(CONFIG_CBS_LOG) && defined(CONFIG_CBS_LOG_BUDGET_RAN_OUT)
	cbs_log(CBS_BUDGET_RAN_OUT, cbs);
#endif
}

static inline void cbs_budget_update_consumption(struct k_cbs *cbs)
{
	if (!cbs->start_cycle) {
		return;
	}
	cbs_cycle_t now = cbs_get_now();
	cbs_cycle_t budget_used = now - cbs->start_cycle;

	if (budget_used < cbs->budget.current) {
		/*
		 * This is the expected case when this function
		 * is called: we have used less budget than what
		 * we have available. A simple subtracion will
		 * take care of updating what's left.
		 */
		cbs->budget.current -= budget_used;
		return;
	}
	/*
	 * If we end up here, the job has finished before
	 * the timer expired but also spent more than
	 * the allowed budget. We therefore need to
	 * compensate that on replenishing.

	 * It should be noted that the chances of falling
	 * into these edge cases are higher on low-resolution
	 * timers. The default 1ms resolution of Zephyr is
	 * relatively low and might result in many budget
	 * tracking inaccuracies depending on the types of
	 * jobs executed. It is therefore recommended to
	 * have a 100us resolution or lower instead.
	 */
	cbs_cycle_t excess;

	if (budget_used > cbs->budget.max) {
		/*
		 * Very edge case where the job surpassed
		 * the max allowed value for the budget
		 * itself one or more times in a row.
		 * It is more likely to happen when the
		 * tick resolution is too coarse (e.g. 1ms)
		 * and the budget is too small (e.g. 400us)
		 */
		for (; budget_used > cbs->budget.max; budget_used -= cbs->budget.max) {
			cbs_replenish_due_to_run_out(cbs, cbs->start_cycle + cbs->budget.max);
		}
		excess = budget_used;
	} else {
		excess = budget_used - cbs->budget.current;
	}
	K_SPINLOCK(&_sched_spinlock) {
		cbs_replenish_due_to_run_out(cbs, (now - excess));
		k_thread_deadline_set(cbs->thread, (int)(cbs->abs_deadline - cbs->start_cycle));
	}
	z_impl_k_reschedule();
	cbs->budget.current -= excess;
}

static void cbs_budget_timer_expired_callback(struct k_timer *timer)
{
	/*
	 * This function is called by the timer when
	 * it expires, which means the budget has been
	 * entirely used and therefore needs replenishing.
	 */
	cbs_cycle_t now = cbs_get_now();
	struct k_cbs *cbs = CONTAINER_OF(timer, struct k_cbs, timer);

	K_SPINLOCK(&_sched_spinlock) {
		cbs_replenish_due_to_run_out(cbs, now);
		k_thread_deadline_set(cbs->thread, (int)(cbs->abs_deadline - now));
		k_timer_start(timer, K_CYC((uint32_t)cbs->budget.current), K_NO_WAIT);
	}
	z_impl_k_reschedule();
}

static void cbs_budget_timer_stop_callback(struct k_timer *timer)
{
	/*
	 * This function is called by the timer when it is stopped
	 * before expiring, which means the thread has finished
	 * executing the job or was preempted. There is still
	 * budget left, which is why we need to update its value.
	 */
	struct k_cbs *cbs = CONTAINER_OF(timer, struct k_cbs, timer);

	cbs_budget_update_consumption(cbs);
	cbs->start_cycle = 0;
}

static inline void cbs_check_current_bandwidth(struct k_cbs *cbs)
{
	cbs_cycle_t arrival = cbs_get_now();
	cbs_cycle_t deadline = cbs->abs_deadline;
	/*
	 * This function is a way to see, when a new job is
	 * pushed to the server, if the current server bandwidth
	 * (budget / deadline) is lower than the configured
	 * bandwidth. If that is true, the job can be served
	 * with the current budget and deadline. If not, the
	 * deadline needs to be postponed.
	 *
	 * So basically, if the condition below is met,
	 * we need to recalculate things. We only check
	 * it when a new job comes:
	 *
	 * Cs >= (ds - rjob) * U
	 *
	 * 'Cs' is the budget left
	 * 'ds' is the absolute deadline,
	 * 'rjob' is the arrival instant of the job
	 * 'U' is the server bandwidth (max budget / period)
	 *
	 * Note that if arrival >= deadline, condition is instantly
	 * met. Otherwise, we need to check more stuff. We exit
	 * without doing anything if the condition is not met.
	 */
	if (deadline > arrival) {
		cbs_cycle_t budget = (cbs->budget.current << cbs->left_shift);

		if (budget < (deadline - arrival) * cbs->bandwidth) {
			return;
		}
	}
	K_SPINLOCK(&_sched_spinlock) {
		cbs_replenish_due_to_condition(cbs, arrival);
		k_thread_deadline_set(cbs->thread, (int)cbs->period);
	}
	z_impl_k_reschedule();
}

static inline void cbs_budget_timer_start(struct k_cbs *cbs)
{
	if (cbs->start_cycle > 0) {
		return;
	}
	cbs->start_cycle = cbs_get_now();
	k_timer_start(&(cbs->timer), K_CYC((uint32_t)cbs->budget.current), K_NO_WAIT);
}

static inline void cbs_budget_timer_stop(struct k_cbs *cbs)
{
	k_timer_stop(&(cbs->timer));
}

static inline void cbs_find_highest_shift_for(struct k_cbs *cbs)
{
	/*
	 * This for loop finds the highest left-shift value
	 * we can apply to the budget before overflowing it.
	 *
	 * The actual left shift logical is done because the
	 * CBS condition checks an equation that, in paper,
	 * uses decimal numbers within it. However, all math
	 * is made here keeps the unsigned integer types of the
	 * variables involved for performance reasons. So
	 * the shift is applied in both sides of the equation
	 * in an attempt to preserve the value resolution.
	 *
	 * more details in cbs_check_current_bandwidth().
	 */
	for (unsigned int right_shift = 1; right_shift < 32; right_shift++) {
		if ((INT_MAX >> right_shift) < cbs->budget.max) {
			cbs->left_shift = right_shift - 1;
			cbs->bandwidth = (cbs->budget.max << cbs->left_shift) / cbs->period;
			break;
		}
	}
}

void cbs_thread(void *server_name, void *cbs_struct, void *cbs_args)
{
	struct cbs_arg *args = (struct cbs_arg *)cbs_args;
	struct k_cbs *cbs = (struct k_cbs *)cbs_struct;
	struct cbs_job job;

	cbs->thread = k_current_get();
	cbs->thread->cbs = cbs;
	cbs->start_cycle = 0;
	cbs->period = CBS_TICKS_TO_CYC(args->period.ticks);
	cbs->budget.max = CBS_TICKS_TO_CYC(args->budget.ticks);
	cbs->budget.current = cbs->budget.max;
	cbs->left_shift = 0;
	cbs->abs_deadline = 0;
	cbs_find_highest_shift_for(cbs);
	k_timer_init(&(cbs->timer), cbs_budget_timer_expired_callback,
		     cbs_budget_timer_stop_callback);

#ifdef CONFIG_CBS_LOG
	strncpy(cbs->name, (char *)server_name, CONFIG_CBS_THREAD_MAX_NAME_LEN - 1);
#endif

	for (;;) {
		/*
		 * The CBS thread remains dormant while
		 * there are no jobs to execute. Once they
		 * arrive, timer starts, execution is made,
		 * timer ends, budget is updated and
		 * the cycle repeats.
		 */
		k_msgq_get(cbs->queue, &job, K_FOREVER);
		cbs_budget_timer_start(cbs);
		job.function(job.arg);
		cbs_budget_timer_stop(cbs);

#if defined(CONFIG_CBS_LOG) && defined(CONFIG_CBS_LOG_JOB_COMPLETE)
		cbs_log(CBS_COMPLETED_JOB, cbs);
#endif
		cbs->is_active = (cbs->queue->used_msgs > 0);
	}
}

void cbs_switched_in(struct k_cbs *cbs)
{
	/*
	 * this function is called at every context switch
	 * and just starts the budget timer if the incoming
	 * thread belongs to an active CBS. If it doesn't,
	 * there's nothing to do.
	 */
	if (cbs->is_active) {
		cbs_budget_timer_start(cbs);
	}
#if defined(CONFIG_CBS_LOG) && defined(CONFIG_CBS_LOG_SWITCHED_IN)
	cbs_log(CBS_SWITCH_TO, cbs);
#endif
}

void cbs_switched_out(struct k_cbs *cbs)
{
	/*
	 * this function is called at every context switch
	 * and just stops the budget timer if the outgoing
	 * thread belongs to an active CBS. If it doesn't,
	 * there's nothing to do.
	 */
	if (cbs->is_active) {
		cbs_budget_timer_stop(cbs);
	}
#if defined(CONFIG_CBS_LOG) && defined(CONFIG_CBS_LOG_SWITCHED_OUT)
	cbs_log(CBS_SWITCH_AWAY, cbs);
#endif
}

int k_cbs_push_job(struct k_cbs *cbs, cbs_callback_t job_function, void *job_arg,
		   k_timeout_t timeout)
{
	/*
	 * this function is the public API to insert a job
	 * within the CBS queue. If the CBS wasn't executing
	 * anything before, it will become active and check
	 * if the existing bandwidth (budget / deadline) is
	 * compatible with the configured (budget / period).
	 */
	if (!cbs) {
		return -ENOMSG;
	}

	struct cbs_job job = {job_function, job_arg};
	int result = k_msgq_put(cbs->queue, &job, timeout);

	if (result == 0) {
#if defined(CONFIG_CBS_LOG) && defined(CONFIG_CBS_LOG_JOB_PUSH)
		cbs_log(CBS_PUSH_JOB, cbs);
#endif
		if (!cbs->is_active) {
			cbs_check_current_bandwidth(cbs);
			cbs->is_active = true;
		}
	}

	return result;
}
