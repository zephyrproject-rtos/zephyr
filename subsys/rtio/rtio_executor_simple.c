/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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
	struct rtio_simple_executor *exc = (struct rtio_simple_executor *)r->executor;

	/* Task is already running */
	if (exc->task.sqe != NULL) {
		return 0;
	}

	struct rtio_sqe *sqe = rtio_spsc_consume(r->sq);

	exc->task.sqe = sqe;
	exc->task.r = r;

	if (sqe != NULL) {
		rtio_iodev_submit(&exc->task);
	}

	return 0;
}

/**
 * @brief Callback from an iodev describing success
 */
void rtio_simple_ok(struct rtio_iodev_sqe *iodev_sqe, int result)
{
	struct rtio *r = iodev_sqe->r;
	const struct rtio_sqe *sqe = iodev_sqe->sqe;

#ifdef CONFIG_ASSERT
	struct rtio_simple_executor *exc =
		(struct rtio_simple_executor *)r->executor;

	__ASSERT_NO_MSG(iodev_sqe == &exc->task);
#endif

	void *userdata = sqe->userdata;

	rtio_spsc_release(r->sq);
	iodev_sqe->sqe = NULL;
	rtio_cqe_submit(r, result, userdata);
	rtio_simple_submit(r);
}

/**
 * @brief Callback from an iodev describing error
 */
void rtio_simple_err(struct rtio_iodev_sqe *iodev_sqe, int result)
{
	const struct rtio_sqe *nsqe;
	struct rtio *r = iodev_sqe->r;
		const struct rtio_sqe *sqe = iodev_sqe->sqe;
	void *userdata = sqe->userdata;
	bool chained = sqe->flags & RTIO_SQE_CHAINED;

#ifdef CONFIG_ASSERT
	struct rtio_simple_executor *exc =
		(struct rtio_simple_executor *)r->executor;

	__ASSERT_NO_MSG(iodev_sqe == &exc->task);
#endif


	rtio_spsc_release(r->sq);
	iodev_sqe->sqe = NULL;
	rtio_cqe_submit(r, result, sqe->userdata);

	if (chained) {

		nsqe = rtio_spsc_consume(r->sq);
		while (nsqe != NULL && nsqe->flags & RTIO_SQE_CHAINED) {
			userdata = nsqe->userdata;
			rtio_spsc_release(r->sq);
			rtio_cqe_submit(r, -ECANCELED, userdata);
			nsqe = rtio_spsc_consume(r->sq);
		}

		if (nsqe != NULL) {

			iodev_sqe->sqe = nsqe;
			rtio_iodev_submit(iodev_sqe);
		}

	} else {
		/* Now we can submit the next in the queue if we aren't done */
		rtio_simple_submit(r);
	}
}
