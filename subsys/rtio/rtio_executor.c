/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/rtio/rtio.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rtio_executor, CONFIG_RTIO_LOG_LEVEL);

/**
 * @brief Submit to an iodev a submission to work on
 *
 * Should be called by the executor when it wishes to submit work
 * to an iodev.
 *
 * @param iodev_sqe Submission to work on
 */
static inline void rtio_iodev_submit(struct rtio_iodev_sqe *iodev_sqe)
{
	iodev_sqe->sqe.iodev->api->submit(iodev_sqe);
}



/**
 * @brief Executor handled submissions
 */
static void rtio_executor_op(struct rtio_iodev_sqe *iodev_sqe)
{
	const struct rtio_sqe *sqe = &iodev_sqe->sqe;

	switch (sqe->op) {
	case RTIO_OP_CALLBACK:
		sqe->callback(iodev_sqe->r, sqe, sqe->arg0);
		rtio_iodev_sqe_ok(iodev_sqe, 0);
		break;
	default:
		rtio_iodev_sqe_err(iodev_sqe, -EINVAL);
	}
}

/**
 * @brief Submit operations in the queue to iodevs
 *
 * @param r RTIO context
 *
 * @retval 0 Always succeeds
 */
void rtio_executor_submit(struct rtio *r)
{
	struct rtio_mpsc_node *node = rtio_mpsc_pop(&r->sq);

	while (node != NULL) {
		struct rtio_iodev_sqe *iodev_sqe = CONTAINER_OF(node, struct rtio_iodev_sqe, q);

		iodev_sqe->r = r;

		if (iodev_sqe->sqe.iodev == NULL) {
			rtio_executor_op(iodev_sqe);
		} else {
			struct rtio_iodev_sqe *curr = iodev_sqe, *next;

			/* Link up transaction or queue list if needed */
			while (curr->sqe.flags & (RTIO_SQE_TRANSACTION | RTIO_SQE_CHAINED)) {
#ifdef CONFIG_ASSERT
				bool transaction = iodev_sqe->sqe.flags & RTIO_SQE_TRANSACTION;
				bool chained = iodev_sqe->sqe.flags & RTIO_SQE_CHAINED;

				__ASSERT(transaction != chained,
					 "Expected chained or transaction flag, not both");
#endif
				node = rtio_mpsc_pop(&iodev_sqe->r->sq);
				next = CONTAINER_OF(node, struct rtio_iodev_sqe, q);
				curr->next = next;
				curr = next;
				curr->r = r;

				__ASSERT(curr != NULL,
					 "Expected a valid sqe following transaction or chain flag");
			}

			curr->next = NULL;
			curr->r = r;

			rtio_iodev_submit(iodev_sqe);
		}

		node = rtio_mpsc_pop(&r->sq);
	}
}

/**
 * @brief Callback from an iodev describing success
 */
void rtio_executor_ok(struct rtio_iodev_sqe *iodev_sqe, int result)
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
		rtio_mpsc_push(&r->sq_free, &curr->q);
		r->sqe_pool_free++;
		rtio_cqe_submit(r, result, userdata, cqe_flags);
		curr = next;
	} while (sqe_flags & RTIO_SQE_TRANSACTION);

	/* Curr should now be the last sqe in the transaction if that is what completed */
	if (sqe_flags & RTIO_SQE_CHAINED) {
		rtio_iodev_submit(curr);
	}
}

/**
 * @brief Callback from an iodev describing error
 *
 * Some assumptions are made and should have been validated on rtio_submit
 * - a sqe marked as chained or transaction has a next sqe
 * - a sqe is marked either chained or transaction but not both
 */
void rtio_executor_err(struct rtio_iodev_sqe *iodev_sqe, int result)
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
		rtio_mpsc_push(&r->sq_free, &curr->q);
		r->sqe_pool_free++;
		rtio_cqe_submit(r, result, userdata, cqe_flags);
		curr = next;
		result = -ECANCELED;
	}

	rtio_mpsc_push(&r->sq_free, &curr->q);
	r->sqe_pool_free++;

	rtio_cqe_submit(r, result, userdata, cqe_flags);
}
