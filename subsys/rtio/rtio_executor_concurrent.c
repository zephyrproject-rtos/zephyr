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
		if (exc->task_status[task_id & exc->task_mask] & CONEX_TASK_SUSPENDED) {
			LOG_DBG("resuming suspended task %d", task_id);
			exc->task_status[task_id & exc->task_mask] &= ~CONEX_TASK_SUSPENDED;
			rtio_executor_submit(&exc->task_cur[task_id & exc->task_mask]);
		}
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
	struct rtio_mpsc_node *node = rtio_mpsc_pop(&r->sq);

	if (node == NULL) {
		return 0;
	}

	struct rtio_iodev_sqe *iodev_sqe = CONTAINER_OF(node, struct rtio_iodev_sqe, q);

	/* Some validation on the sqe to ensure no programming errors were
	 * made so assumptions in ok and err are valid.
	 */
	iodev_sqe->r = r;

	rtio_executor_submit(iodev_sqe);

	return 0;
}

/**
 * @brief Callback from an iodev describing success
 */
void rtio_concurrent_ok(struct rtio_iodev_sqe *iodev_sqe, int result)
{
	struct rtio_iodev_sqe *curr = iodev_sqe, *next;
	struct rtio *r = curr->r;
	void *userdata;
	uint32_t sqe_flags, cqe_flags;

	do {
		userdata = curr->sqe.userdata;
		sqe_flags = curr->sqe.flags;
		cqe_flags = rtio_cqe_compute_flags(iodev_sqe);

		next = rtio_iodev_sqe_next(curr);
		k_mem_slab_free(r->sqe_pool, (void **)&iodev_sqe);
		rtio_cqe_submit(r, result, userdata, cqe_flags);
		curr = next;
	} while (sqe_flags & RTIO_SQE_TRANSACTION);

	/* Curr should now be the last sqe in the transaction if that is what completed */
	if (curr->sqe.flags & RTIO_SQE_CHAINED) {
		rtio_iodev_submit(curr);
	}
}

/**
 * @brief Callback from an iodev describing error
 */
void rtio_concurrent_err(struct rtio_iodev_sqe *iodev_sqe, int result)
{
	struct rtio *r = iodev_sqe->r;
	struct rtio_iodev_sqe *curr = iodev_sqe, *next;
	void *userdata = curr->sqe.userdata;
	uint32_t sqe_flags = iodev_sqe->sqe.flags;
	uint32_t cqe_flags = rtio_cqe_compute_flags(curr);

	while (sqe_flags & (RTIO_SQE_CHAINED | RTIO_SQE_TRANSACTION)) {
		userdata = curr->sqe.userdata;
		sqe_flags = curr->sqe.flags;
		cqe_flags = rtio_cqe_compute_flags(curr);

		next = rtio_iodev_sqe_next(curr);
		k_mem_slab_free(r->sqe_pool, (void **)&curr);
		rtio_cqe_submit(r, result, userdata, cqe_flags);
		curr = next;
		result = -ECANCELED;
	}

	k_mem_slab_free(r->sqe_pool, (void **)&curr);
	rtio_cqe_submit(r, result, userdata, cqe_flags);
}
