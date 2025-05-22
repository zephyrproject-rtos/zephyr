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
 * @brief Executor handled submissions
 */
static void rtio_executor_op(struct rtio_iodev_sqe *iodev_sqe)
{
	const struct rtio_sqe *sqe = &iodev_sqe->sqe;

	switch (sqe->op) {
	case RTIO_OP_CALLBACK:
		sqe->callback.callback(iodev_sqe->r, sqe, sqe->callback.arg0);
		rtio_iodev_sqe_ok(iodev_sqe, 0);
		break;
	default:
		rtio_iodev_sqe_err(iodev_sqe, -EINVAL);
	}
}

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
	if (FIELD_GET(RTIO_SQE_CANCELED, iodev_sqe->sqe.flags)) {
		rtio_iodev_sqe_err(iodev_sqe, -ECANCELED);
		return;
	}

	/* No iodev means its an executor specific operation */
	if (iodev_sqe->sqe.iodev == NULL) {
		rtio_executor_op(iodev_sqe);
		return;
	}

	iodev_sqe->sqe.iodev->api->submit(iodev_sqe);
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
	const uint16_t cancel_no_response = (RTIO_SQE_CANCELED | RTIO_SQE_NO_RESPONSE);
	struct mpsc_node *node = mpsc_pop(&r->sq);

	while (node != NULL) {
		struct rtio_iodev_sqe *iodev_sqe = CONTAINER_OF(node, struct rtio_iodev_sqe, q);

		/* If this submission was cancelled before submit, then generate no response */
		if (iodev_sqe->sqe.flags  & RTIO_SQE_CANCELED) {
			iodev_sqe->sqe.flags |= cancel_no_response;
		}
		iodev_sqe->r = r;

		struct rtio_iodev_sqe *curr = iodev_sqe, *next;

		/* Link up transaction or queue list if needed */
		while (curr->sqe.flags & (RTIO_SQE_TRANSACTION | RTIO_SQE_CHAINED)) {
#ifdef CONFIG_ASSERT
			bool transaction = iodev_sqe->sqe.flags & RTIO_SQE_TRANSACTION;
			bool chained = iodev_sqe->sqe.flags & RTIO_SQE_CHAINED;

			__ASSERT(transaction != chained,
				    "Expected chained or transaction flag, not both");
#endif
			node = mpsc_pop(&iodev_sqe->r->sq);

			__ASSERT(node != NULL,
				    "Expected a valid submission in the queue while in a transaction or chain");

			next = CONTAINER_OF(node, struct rtio_iodev_sqe, q);

			/* If the current submission was cancelled before submit,
			 * then cancel the next one and generate no response
			 */
			if (curr->sqe.flags  & RTIO_SQE_CANCELED) {
				next->sqe.flags |= cancel_no_response;
			}
			curr->next = next;
			curr = next;
			curr->r = r;

			__ASSERT(
				curr != NULL,
				"Expected a valid sqe following transaction or chain flag");
		}

		curr->next = NULL;
		curr->r = r;

		rtio_iodev_submit(iodev_sqe);

		node = mpsc_pop(&r->sq);
	}
}

/**
 * @brief Handle common logic when :c:macro:`RTIO_SQE_MULTISHOT` is set
 *
 * @param[in] r RTIO context
 * @param[in] curr Current IODev SQE that's being marked for finished.
 * @param[in] is_canceled Whether or not the SQE is canceled
 */
static inline void rtio_executor_handle_multishot(struct rtio *r, struct rtio_iodev_sqe *curr,
						  bool is_canceled)
{
	/* Reset the mempool if needed */
	if (curr->sqe.op == RTIO_OP_RX && FIELD_GET(RTIO_SQE_MEMPOOL_BUFFER, curr->sqe.flags)) {
		if (is_canceled) {
			/* Free the memory first since no CQE will be generated */
			LOG_DBG("Releasing memory @%p size=%u", (void *)curr->sqe.rx.buf,
				curr->sqe.rx.buf_len);
			rtio_release_buffer(r, curr->sqe.rx.buf, curr->sqe.rx.buf_len);
		}
		/* Reset the buffer info so the next request can get a new one */
		curr->sqe.rx.buf = NULL;
		curr->sqe.rx.buf_len = 0;
	}
	if (!is_canceled) {
		/* Request was not canceled, put the SQE back in the queue */
		mpsc_push(&r->sq, &curr->q);
		rtio_executor_submit(r);
	}
}

static inline void rtio_executor_done(struct rtio_iodev_sqe *iodev_sqe, int result, bool is_ok)
{
	const bool is_multishot = FIELD_GET(RTIO_SQE_MULTISHOT, iodev_sqe->sqe.flags) == 1;
	const bool is_canceled = FIELD_GET(RTIO_SQE_CANCELED, iodev_sqe->sqe.flags) == 1;
	struct rtio *r = iodev_sqe->r;
	struct rtio_iodev_sqe *curr = iodev_sqe, *next;
	void *userdata;
	uint32_t sqe_flags, cqe_flags;

	do {
		userdata = curr->sqe.userdata;
		sqe_flags = curr->sqe.flags;
		cqe_flags = rtio_cqe_compute_flags(iodev_sqe);

		next = rtio_iodev_sqe_next(curr);
		if (is_multishot) {
			rtio_executor_handle_multishot(r, curr, is_canceled);
		}
		if (!is_multishot || is_canceled) {
			/* SQE is no longer needed, release it */
			rtio_sqe_pool_free(r->sqe_pool, curr);
		}
		if (!is_canceled && FIELD_GET(RTIO_SQE_NO_RESPONSE, sqe_flags) == 0) {
			/* Request was not canceled, generate a CQE */
			rtio_cqe_submit(r, result, userdata, cqe_flags);
		}
		curr = next;
		if (!is_ok) {
			/* This is an error path, so cancel any chained SQEs */
			result = -ECANCELED;
		}
	} while (sqe_flags & RTIO_SQE_TRANSACTION);

	/* curr should now be the last sqe in the transaction if that is what completed */
	if (sqe_flags & RTIO_SQE_CHAINED) {
		rtio_iodev_submit(curr);
	}
}

/**
 * @brief Callback from an iodev describing success
 */
void rtio_executor_ok(struct rtio_iodev_sqe *iodev_sqe, int result)
{
	rtio_executor_done(iodev_sqe, result, true);
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
	rtio_executor_done(iodev_sqe, result, false);
}
