/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/spinlock.h>
#include <zephyr/rtio/rtio_executor_concurrent.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rtio_executor_concurrent, CONFIG_RTIO_LOG_LEVEL);

#define CONEX_TASK_COMPLETE BIT(0)
#define CONEX_TASK_SUSPENDED BIT(1)


/**
 * @file
 * @brief Concurrent RTIO Executor
 *
 * The concurrent executor provides fixed amounts of concurrency
 * using minimal overhead but assumes a small number of concurrent tasks.
 *
 * Many of the task lookup and management functions in here are O(N) over N
 * tasks. This is fine when the task set is *small*. Task lookup could be
 * improved in the future with a binary search at the expense of code size.
 *
 * The assumption here is that perhaps only 8-16 concurrent tasks are likely
 * such that simple short for loops over task array are reasonably fast.
 *
 * A maximum of 65K submissions queue entries are possible.
 */

/**
 * check if there is a free task available
 */
static bool conex_task_free(struct rtio_concurrent_executor *exc)
{
	return (exc->task_in - exc->task_out) < (exc->task_mask + 1);
}

/**
 * get the next free available task index
 */
static uint16_t conex_task_next(struct rtio_concurrent_executor *exc)
{
	uint16_t task_id = exc->task_in;

	exc->task_in++;
	return task_id;
}

static uint16_t conex_task_id(struct rtio_concurrent_executor *exc,
	const struct rtio_sqe *sqe)
{
	uint16_t task_id = exc->task_out;

	for (; task_id < exc->task_in; task_id++) {
		if (exc->task_cur[task_id & exc->task_mask] == sqe) {
			break;
		}
	}
	return task_id;
}

static void conex_sweep_task(struct rtio *r, struct rtio_concurrent_executor *exc)
{
	struct rtio_sqe *sqe = rtio_spsc_consume(r->sq);

	while (sqe != NULL && sqe->flags & RTIO_SQE_CHAINED) {
		rtio_spsc_release(r->sq);
		sqe = rtio_spsc_consume(r->sq);
	}

	rtio_spsc_release(r->sq);
}

static void conex_sweep(struct rtio *r, struct rtio_concurrent_executor *exc)
{
	/* In order sweep up */
	for (uint16_t task_id = exc->task_out; task_id < exc->task_in; task_id++) {
		if (exc->task_status[task_id & exc->task_mask] & CONEX_TASK_COMPLETE) {
			LOG_INF("sweeping oldest task %d", task_id);
			conex_sweep_task(r, exc);
			exc->task_out++;
		} else {
			break;
		}
	}
}

static void conex_resume(struct rtio *r, struct rtio_concurrent_executor *exc)
{
	/* In order resume tasks */
	for (uint16_t task_id = exc->task_out; task_id < exc->task_in; task_id++) {
		if (exc->task_status[task_id & exc->task_mask] & CONEX_TASK_SUSPENDED) {
			LOG_INF("resuming suspended task %d", task_id);
			exc->task_status[task_id] &= ~CONEX_TASK_SUSPENDED;
			rtio_iodev_submit(exc->task_cur[task_id], r);
		}
	}
}

static void conex_sweep_resume(struct rtio *r, struct rtio_concurrent_executor *exc)
{
	conex_sweep(r, exc);
	conex_resume(r, exc);
}

/**
 * @brief Submit submissions to concurrent executor
 *
 * @param r RTIO context
 *
 * @retval 0 Always succeeds
 */
int rtio_concurrent_submit(struct rtio *r)
{

	LOG_INF("submit");

	struct rtio_concurrent_executor *exc =
		(struct rtio_concurrent_executor *)r->executor;
	struct rtio_sqe *sqe;
	struct rtio_sqe *last_sqe;
	k_spinlock_key_t key;

	key = k_spin_lock(&exc->lock);

	/* If never submitted before peek at the first item
	 * otherwise start back up where the last submit call
	 * left off
	 */
	if (exc->last_sqe == NULL) {
		sqe = rtio_spsc_peek(r->sq);
	} else {
		/* Pickup from last submit call */
		sqe = rtio_spsc_next(r->sq, exc->last_sqe);
	}

	last_sqe = sqe;
	while (sqe != NULL && conex_task_free(exc)) {
		LOG_INF("head SQE in chain %p", sqe);

		/* Get the next task id if one exists */
		uint16_t task_idx = conex_task_next(exc);

		LOG_INF("setting up task %d", task_idx);

		/* Setup task (yes this is it) */
		exc->task_cur[task_idx] = sqe;
		exc->task_status[task_idx] = CONEX_TASK_SUSPENDED;

		LOG_INF("submitted sqe %p", sqe);
		/* Go to the next sqe not in the current chain */
		while (sqe != NULL && (sqe->flags & RTIO_SQE_CHAINED)) {
			sqe = rtio_spsc_next(r->sq, sqe);
		}

		LOG_INF("tail SQE in chain %p", sqe);

		last_sqe = sqe;

		/* SQE is the end of the previous chain */
		sqe = rtio_spsc_next(r->sq, sqe);
	}

	/* Out of available pointers, wait til others complete, note the
	 * first pending submission queue. May be NULL if nothing is pending.
	 */
	exc->pending_sqe = sqe;

	/**
	 * Run through the queue until the last item
	 * and take not of it
	 */
	while (sqe != NULL) {
		last_sqe = sqe;
		sqe = rtio_spsc_next(r->sq, sqe);
	}

	/* Note the last sqe for the next submit call */
	exc->last_sqe = last_sqe;

	/* Resume all suspended tasks */
	conex_resume(r, exc);

	k_spin_unlock(&exc->lock, key);

	return 0;
}

/**
 * @brief Callback from an iodev describing success
 */
void rtio_concurrent_ok(struct rtio *r, const struct rtio_sqe *sqe, int result)
{
	struct rtio_sqe *next_sqe;
	k_spinlock_key_t key;
	struct rtio_concurrent_executor *exc = (struct rtio_concurrent_executor *)r->executor;

	/* Interrupt may occur in spsc_acquire, breaking the contract
	 * so spin around it effectively preventing another interrupt on
	 * this core, and another core trying to concurrently work in here.
	 *
	 * This can and should be broken up into a few sections with a try
	 * lock around the sweep and resume.
	 */
	key = k_spin_lock(&exc->lock);

	rtio_cqe_submit(r, result, sqe->userdata);

	/* Determine the task id : O(n) */
	uint16_t task_id = conex_task_id(exc, sqe);

	if (sqe->flags & RTIO_SQE_CHAINED) {
		next_sqe = rtio_spsc_next(r->sq, sqe);

		rtio_iodev_submit(next_sqe, r);

		exc->task_cur[task_id] = next_sqe;
	} else {
		exc->task_status[task_id]  |= CONEX_TASK_COMPLETE;
	}


	/* Sweep up unused SQEs and tasks, retry suspended tasks */
	/* TODO Use a try lock here and don't bother doing it if we are already
	 * doing it elsewhere
	 */
	conex_sweep_resume(r, exc);

	k_spin_unlock(&exc->lock, key);
}

/**
 * @brief Callback from an iodev describing error
 */
void rtio_concurrent_err(struct rtio *r, const struct rtio_sqe *sqe, int result)
{
	struct rtio_sqe *nsqe;
	k_spinlock_key_t key;
	struct rtio_concurrent_executor *exc = (struct rtio_concurrent_executor *)r->executor;

	/* Another interrupt (and sqe complete) may occur in spsc_acquire,
	 * breaking the contract so spin around it effectively preventing another
	 * interrupt on this core, and another core trying to concurrently work
	 * in here.
	 *
	 * This can and should be broken up into a few sections with a try
	 * lock around the sweep and resume.
	 */
	key = k_spin_lock(&exc->lock);

	rtio_cqe_submit(r, result, sqe->userdata);

	/* Determine the task id : O(n) */
	uint16_t task_id = conex_task_id(exc, sqe);


	/* Fail the remaining sqe's in the chain */
	if (sqe->flags & RTIO_SQE_CHAINED) {
		nsqe = rtio_spsc_next(r->sq, sqe);
		while (nsqe != NULL && nsqe->flags & RTIO_SQE_CHAINED) {
			rtio_cqe_submit(r, -ECANCELED, nsqe->userdata);
			nsqe = rtio_spsc_next(r->sq, nsqe);
		}
	}

	/* Task is complete (failed) */
	exc->task_status[task_id] |= CONEX_TASK_COMPLETE;

	conex_sweep_resume(r, exc);

	k_spin_unlock(&exc->lock, key);
}
