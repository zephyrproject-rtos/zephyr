/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/rtio/rtio.h>

/**
 * @brief Executor handled submissions
 */
static void rtio_executor_submit_self(struct rtio_iodev_sqe *iodev_sqe)
{
	const struct rtio_sqe *sqe = iodev_sqe->sqe;

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
 * @brief Common executor handling of submit
 *
 * Some submissions may have NULL for the iodev pointer which implies
 * an executor related operation such as a callback or flush. This
 * common codepath avoids duplicating efforts for dealing with this.
 */
static void rtio_executor_submit(struct rtio_iodev_sqe *iodev_sqe)
{
	if (iodev_sqe->sqe->iodev == NULL) {
		rtio_executor_submit_self(iodev_sqe);
	} else {
		rtio_iodev_submit(iodev_sqe);
	}
}
