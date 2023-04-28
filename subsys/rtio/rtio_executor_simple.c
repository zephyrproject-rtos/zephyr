/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "rtio_executor_common.h"
#include <zephyr/rtio/rtio_executor_simple.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rtio_executor_simple, CONFIG_RTIO_LOG_LEVEL);


/**
 * @brief Submit submissions to simple executor
 *
 * The simple executor provides no concurrency instead
 * execution each submission chain one after the next.
 *
 * @param r RTIO context
 *
 * @retval 0 Always succeeds
 */
int rtio_simple_submit(struct rtio *r)
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
void rtio_simple_ok(struct rtio_iodev_sqe *iodev_sqe, int result)
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
	} else {
		rtio_simple_submit(r);
	}
}

/**
 * @brief Callback from an iodev describing error
 *
 * Some assumptions are made and should have been validated on rtio_submit
 * - a sqe marked as chained or transaction has a next sqe
 * - a sqe is marked either chained or transaction but not both
 */
void rtio_simple_err(struct rtio_iodev_sqe *iodev_sqe, int result)
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
