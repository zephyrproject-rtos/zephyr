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

#include "rtio_executor_common.h"

#define CONEX_TASK_COMPLETE  BIT(0)
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
	return task_id & exc->task_mask;
}

static inline uint16_t conex_task_id(struct rtio_concurrent_executor *exc,
				     const struct rtio_iodev_sqe *iodev_sqe)
{
	__ASSERT_NO_MSG(iodev_sqe <= &exc->task_cur[exc->task_mask] &&
			iodev_sqe >= &exc->task_cur[0]);
	return iodev_sqe - &exc->task_cur[0];
}

static void conex_sweep_task(struct rtio *r, struct rtio_concurrent_executor *exc)
{
	struct rtio_sqe *sqe = rtio_spsc_consume(r->sq);

	while (sqe != NULL && (sqe->flags & (RTIO_SQE_CHAINED | RTIO_SQE_TRANSACTION))) {
		rtio_spsc_release(r->sq);
		sqe = rtio_spsc_consume(r->sq);
	}

	rtio_spsc_release(r->sq);

	if (sqe == exc->last_sqe) {
		exc->last_sqe = NULL;
	}
}

/**
 * @brief Sweep like a GC of sorts old tasks that are completed in order
 *
 * Will only sweep tasks in the order they arrived in the submission queue.
 * Meaning there might be completed tasks that could be freed but are not yet
 * because something before it has not yet completed.
 */
static void conex_sweep(struct rtio *r, struct rtio_concurrent_executor *exc)
{
	/* In order sweep up */
	for (uint16_t task_id = exc->task_out; task_id < exc->task_in; task_id++) {
		if (exc->task_status[task_id & exc->task_mask] & CONEX_TASK_COMPLETE) {
			LOG_DBG("sweeping oldest task %d", task_id);
			conex_sweep_task(r, exc);
			exc->task_out++;
		} else {
			break;
		}
	}
}

/**
 * @brief Prepare tasks to run by iterating through the submission queue
 *
 * For each submission in the queue that begins a chain or transaction
 * start a task if possible. Concurrency is limited by the allocated concurrency
 * per executor instance.
 */
static void conex_prepare(struct rtio *r, struct rtio_concurrent_executor *exc)
{
	struct rtio_sqe *sqe, *last_sqe;

	/* If never submitted before peek at the first item
	 * otherwise start back up where the last submit call
	 * left off
	 */
	if (exc->last_sqe == NULL) {
		last_sqe = NULL;
		sqe = rtio_spsc_peek(r->sq);
	} else {
		last_sqe = exc->last_sqe;
		sqe = rtio_spsc_next(r->sq, last_sqe);
	}

	LOG_DBG("starting at sqe %p, last %p", sqe, exc->last_sqe);

	while (sqe != NULL && conex_task_free(exc)) {
		/* Get the next free task id */
		uint16_t task_idx = conex_task_next(exc);

		LOG_DBG("preparing task %d, sqe %p", task_idx, sqe);

		/* Setup task */
		exc->task_cur[task_idx].sqe = sqe;
		exc->task_cur[task_idx].r = r;
		exc->task_status[task_idx] = CONEX_TASK_SUSPENDED;

		/* Go to the next sqe not in the current chain or transaction */
		while (sqe->flags & (RTIO_SQE_CHAINED | RTIO_SQE_TRANSACTION)) {
			sqe = rtio_spsc_next(r->sq, sqe);
		}

		/* SQE is the end of the previous chain or transaction so skip it */
		last_sqe = sqe;
		sqe = rtio_spsc_next(r->sq, sqe);
	}

	/* Out of available tasks so remember where we left off to begin again once tasks free up */
	exc->last_sqe = last_sqe;
}

/**
 * @brief Resume tasks that are suspended
 *
 * All tasks begin as suspended tasks. This kicks them off to the submissions
 * associated iodev.
 */
static void conex_resume(struct rtio *r, struct rtio_concurrent_executor *exc)
{
	/* In order resume tasks */
	for (uint16_t task_id = exc->task_out; task_id < exc->task_in; task_id++) {
		uint16_t task_idx = task_id & exc->task_mask;

		if (FIELD_GET(CONEX_TASK_SUSPENDED, exc->task_status[task_idx]) == 0) {
			continue;
		}

		if (FIELD_GET(RTIO_SQE_CANCELED, exc->task_cur[task_idx].sqe->flags)) {
			LOG_DBG("Skipping canceled task %d", task_id);
			rtio_iodev_sqe_err(&exc->task_cur[task_idx], -ECANCELED);
			continue;
		}
		LOG_DBG("resuming suspended task %d", task_id);
		exc->task_status[task_idx] &= ~CONEX_TASK_SUSPENDED;
		rtio_executor_submit(&exc->task_cur[task_idx]);
	}
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

	struct rtio_concurrent_executor *exc = (struct rtio_concurrent_executor *)r->executor;
	k_spinlock_key_t key;

	key = k_spin_lock(&exc->lock);
	exc->is_locked = true;

	/* Prepare tasks to run, they start in a suspended state */
	conex_prepare(r, exc);

	/* Resume all suspended tasks */
	conex_resume(r, exc);

	exc->is_locked = false;
	k_spin_unlock(&exc->lock, key);

	return 0;
}

/**
 * @brief Callback from an iodev describing success
 */
void rtio_concurrent_ok(struct rtio_iodev_sqe *iodev_sqe, int result)
{
	struct rtio *r = iodev_sqe->r;
	const struct rtio_sqe *sqe = iodev_sqe->sqe;
	struct rtio_concurrent_executor *exc = (struct rtio_concurrent_executor *)r->executor;
	const struct rtio_sqe *next_sqe;
	k_spinlock_key_t key;

	/* Interrupt may occur in spsc_acquire, breaking the contract
	 * so spin around it effectively preventing another interrupt on
	 * this core, and another core trying to concurrently work in here.
	 */
	key = k_spin_lock(&exc->lock);

	LOG_DBG("completed sqe %p", sqe);

	/* Determine the task id by memory offset O(1) */
	uint16_t task_id = conex_task_id(exc, iodev_sqe);

	if (sqe->flags & RTIO_SQE_CHAINED) {
		next_sqe = rtio_spsc_next(r->sq, sqe);

		exc->task_cur[task_id].sqe = next_sqe;
		if (FIELD_GET(RTIO_SQE_CANCELED, sqe->flags)) {
			rtio_iodev_sqe_err(&exc->task_cur[task_id], -ECANCELED);
		} else {
			rtio_executor_submit(&exc->task_cur[task_id]);
		}
	} else {
		exc->task_status[task_id] |= CONEX_TASK_COMPLETE;
	}

	bool transaction;

	do {
		/* Capture the sqe information */
		void *userdata = sqe->userdata;
		uint32_t flags = rtio_cqe_compute_flags(iodev_sqe);

		transaction = sqe->flags & RTIO_SQE_TRANSACTION;

		/* Release the sqe */
		conex_sweep(r, exc);

		/* Submit the completion event */
		rtio_cqe_submit(r, result, userdata, flags);

		if (transaction) {
			/* sqe was a transaction, get the next one */
			sqe = rtio_spsc_next(r->sq, sqe);
			__ASSERT_NO_MSG(sqe != NULL);
		}
	} while (transaction);
	conex_prepare(r, exc);
	conex_resume(r, exc);

	k_spin_unlock(&exc->lock, key);
}

/**
 * @brief Callback from an iodev describing error
 */
void rtio_concurrent_err(struct rtio_iodev_sqe *iodev_sqe, int result)
{
	k_spinlock_key_t key;
	struct rtio *r = iodev_sqe->r;
	const struct rtio_sqe *sqe = iodev_sqe->sqe;
	struct rtio_concurrent_executor *exc = (struct rtio_concurrent_executor *)r->executor;
	void *userdata = sqe->userdata;
	uint32_t flags = rtio_cqe_compute_flags(iodev_sqe);
	bool chained = sqe->flags & RTIO_SQE_CHAINED;
	bool transaction = sqe->flags & RTIO_SQE_TRANSACTION;
	uint16_t task_id = conex_task_id(exc, iodev_sqe);

	/* Another interrupt (and sqe complete) may occur in spsc_acquire,
	 * breaking the contract so spin around it effectively preventing another
	 * interrupt on this core, and another core trying to concurrently work
	 * in here.
	 */
	if (!exc->is_locked) {
		key = k_spin_lock(&exc->lock);
	}

	if (!transaction) {
		rtio_cqe_submit(r, result, userdata, flags);
	}

	/* While the last sqe was marked as chained or transactional, do more work */
	while (chained | transaction) {
		sqe = rtio_spsc_next(r->sq, sqe);
		chained = sqe->flags & RTIO_SQE_CHAINED;
		transaction = sqe->flags & RTIO_SQE_TRANSACTION;
		userdata = sqe->userdata;

		if (!transaction) {
			rtio_cqe_submit(r, result, userdata, flags);
		} else {
			rtio_cqe_submit(r, -ECANCELED, userdata, flags);
		}
	}

	/* Determine the task id : O(1) */
	exc->task_status[task_id] |= CONEX_TASK_COMPLETE;

	conex_sweep(r, exc);
	conex_prepare(r, exc);
	conex_resume(r, exc);

	if (!exc->is_locked) {
		k_spin_unlock(&exc->lock, key);
	}
}
