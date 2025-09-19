/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/rtio/rtio.h>
#include <zephyr/kernel.h>

#include "rtio_sched.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rtio_executor, CONFIG_RTIO_LOG_LEVEL);

/**
 * @brief Callback which completes an RTIO_AWAIT_OP handled by the executor
 *
 * The callback is triggered when the rtio_sqe tied to the RTIO_AWAIT_OP
 * is signaled by the user.
 *
 * @param iodev_sqe Submission to complete
 * @param userdata Additional data passed along
 */
static void rtio_executor_sqe_signaled(struct rtio_iodev_sqe *iodev_sqe, void *userdata)
{
	ARG_UNUSED(userdata);

	rtio_iodev_sqe_ok(iodev_sqe, 0);
}

/**
 * @brief Executor handled submissions
 */
static void rtio_executor_op(struct rtio_iodev_sqe *iodev_sqe, int last_result)
{
	const struct rtio_sqe *sqe = &iodev_sqe->sqe;

	switch (sqe->op) {
	case RTIO_OP_CALLBACK:
		sqe->callback.callback(iodev_sqe->r, sqe, last_result, sqe->callback.arg0);
		rtio_iodev_sqe_ok(iodev_sqe, 0);
		break;
	case RTIO_OP_DELAY:
		rtio_sched_alarm(iodev_sqe, sqe->delay.timeout);
		break;
	case RTIO_OP_AWAIT:
		rtio_iodev_sqe_await_signal(iodev_sqe, rtio_executor_sqe_signaled, NULL);
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
static inline void rtio_iodev_submit(struct rtio_iodev_sqe *iodev_sqe, int last_result)
{
	if (FIELD_GET(RTIO_SQE_CANCELED, iodev_sqe->sqe.flags)) {
		rtio_iodev_sqe_err(iodev_sqe, -ECANCELED);
		return;
	}

	/* No iodev means its an executor specific operation */
	if (iodev_sqe->sqe.iodev == NULL) {
		rtio_executor_op(iodev_sqe, last_result);
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
			bool multishot = iodev_sqe->sqe.flags & RTIO_SQE_MULTISHOT;

			__ASSERT((transaction ^ chained ^ multishot) &&
				 !(transaction && chained && multishot),
				 "Cannot have more than one of these flags"
				 " enabled: transaction, chained or multishot");
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

		rtio_iodev_submit(iodev_sqe, 0);

		node = mpsc_pop(&r->sq);
	}
}

/**
 * @brief Handle common logic when :c:macro:`RTIO_SQE_MULTISHOT` is set
 *
 * @param[in] iodev_sqe IODEV SQE that's being marked as finished.
 * @param[in] result The result of the latest request iteration
 * @param[in] is_ok Whether or not the SQE's result was successful
 */
static inline void rtio_executor_handle_multishot(struct rtio_iodev_sqe *iodev_sqe,
						  int result, bool is_ok)
{
	struct rtio *r = iodev_sqe->r;
	const bool is_canceled = FIELD_GET(RTIO_SQE_CANCELED, iodev_sqe->sqe.flags) == 1;
	const bool uses_mempool = FIELD_GET(RTIO_SQE_MEMPOOL_BUFFER, iodev_sqe->sqe.flags) == 1;
	const bool requires_response = FIELD_GET(RTIO_SQE_NO_RESPONSE, iodev_sqe->sqe.flags) == 0;
	uint32_t cqe_flags = rtio_cqe_compute_flags(iodev_sqe);
	void *userdata = iodev_sqe->sqe.userdata;

	if (iodev_sqe->sqe.op == RTIO_OP_RX && uses_mempool) {
		/* Reset the buffer info so the next request can get a new one */
		iodev_sqe->sqe.rx.buf = NULL;
		iodev_sqe->sqe.rx.buf_len = 0;
	}

	/** We're releasing reasources when erroring as an error handling scheme of multi-shot
	 * submissions by requiring to stop re-submitting if something goes wrong. Let the
	 * application decide what's best for handling the corresponding error: whether
	 * re-submitting, rebooting or anything else.
	 */
	if (is_canceled || !is_ok) {
		LOG_DBG("Releasing memory @%p size=%u", (void *)iodev_sqe->sqe.rx.buf,
			iodev_sqe->sqe.rx.buf_len);
		rtio_release_buffer(r, iodev_sqe->sqe.rx.buf, iodev_sqe->sqe.rx.buf_len);
		rtio_sqe_pool_free(r->sqe_pool, iodev_sqe);
	} else {
		/* Request was not canceled, put the SQE back in the queue */
		mpsc_push(&r->sq, &iodev_sqe->q);
		rtio_executor_submit(r);
	}

	if (requires_response) {
		rtio_cqe_submit(r, result, userdata, cqe_flags);
	}
}

/**
 * @brief Handle common logic one-shot items
 *
 * @param[in] iodev_sqe IODEV SQE that's being marked as finished.
 * @param[in] result The result of the latest request iteration
 * @param[in] is_ok Whether or not the SQE's result was successful
 */
static inline void rtio_executor_handle_oneshot(struct rtio_iodev_sqe *iodev_sqe,
						int last_result, bool is_ok)
{
	const bool is_canceled = FIELD_GET(RTIO_SQE_CANCELED, iodev_sqe->sqe.flags) == 1;
	struct rtio_iodev_sqe *curr = iodev_sqe;
	struct rtio *r = iodev_sqe->r;
	uint32_t sqe_flags;
	int result = last_result;

	/** Single-shot items may be linked as transactions or be chained together.
	 * Untangle the set of SQEs and act accordingly on each one.
	 */
	do {
		void *userdata = curr->sqe.userdata;
		uint32_t cqe_flags = rtio_cqe_compute_flags(iodev_sqe);
		struct rtio_iodev_sqe *next = rtio_iodev_sqe_next(curr);

		sqe_flags = curr->sqe.flags;

		if (!is_canceled && FIELD_GET(RTIO_SQE_NO_RESPONSE, sqe_flags) == 0) {
			/* Generate a result back to the client if need be.*/
			rtio_cqe_submit(r, result, userdata, cqe_flags);
		}

		rtio_sqe_pool_free(r->sqe_pool, curr);
		curr = next;

		if (!is_ok) {
			/* This is an error path, so cancel any chained SQEs */
			result = -ECANCELED;
		}
	} while (FIELD_GET(RTIO_SQE_TRANSACTION, sqe_flags) == 1);

	/* curr should now be the last sqe in the transaction if that is what completed */
	if (FIELD_GET(RTIO_SQE_CHAINED, sqe_flags) == 1) {
		rtio_iodev_submit(curr, last_result);
	}
}

static inline void rtio_executor_done(struct rtio_iodev_sqe *iodev_sqe, int result, bool is_ok)
{
	const bool is_multishot = FIELD_GET(RTIO_SQE_MULTISHOT, iodev_sqe->sqe.flags) == 1;

	if (is_multishot) {
		rtio_executor_handle_multishot(iodev_sqe, result, is_ok);
	} else {
		rtio_executor_handle_oneshot(iodev_sqe, result, is_ok);
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
