/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/kernel.h>

#ifndef RTIO_IODEV_TEST_H_
#define RTIO_IODEV_TEST_H_

struct rtio_iodev_test_data {
		/**
	 * k_timer for an asynchronous task
	 */
	struct k_timer timer;

	/**
	 * Currently executing sqe
	 */
	const struct rtio_sqe *sqe;

	/**
	 * Currently executing rtio context
	 */
	struct rtio *r;
};


static void rtio_iodev_timer_fn(struct k_timer *tm)
{
	struct rtio_iodev_test_data *data = CONTAINER_OF(tm, struct rtio_iodev_test_data, timer);

	struct rtio *r = data->r;
	const struct rtio_sqe *sqe = data->sqe;

	data->r = NULL;
	data->sqe = NULL;

	/* Complete the request with Ok and a result */
	TC_PRINT("sqe ok callback\n");
	rtio_sqe_ok(r, sqe, 0);
}

static void rtio_iodev_test_submit(const struct rtio_sqe *sqe, struct rtio *r)
{
	struct rtio_iodev_test_data *data = sqe->iodev->data;

	/**
	 * This isn't quite right, probably should be equivalent to a
	 * pend instead of a fail here. In reality if the device is busy
	 * this should be enqueued to the iodev_sq and started as soon
	 * as the device is no longer busy (scheduled for the future).
	 */
	if (k_timer_remaining_get(&data->timer) != 0) {
		TC_PRINT("would block, timer not free!\n");
		rtio_sqe_err(r, sqe, -EWOULDBLOCK);
		return;
	}

	data->sqe = sqe;
	data->r = r;

	/**
	 * Simulate an async hardware request with a one shot timer
	 *
	 * In reality the time to complete might have some significant variance
	 * but this is proof enough of a working API flow.
	 */
	TC_PRINT("starting one shot\n");
	k_timer_start(&data->timer, K_MSEC(10), K_NO_WAIT);
}

static const struct rtio_iodev_api rtio_iodev_test_api = {
	.submit = rtio_iodev_test_submit,
};

const struct rtio_iodev_api *the_api = &rtio_iodev_test_api;

static inline void rtio_iodev_test_init(const struct rtio_iodev *test)
{
	struct rtio_iodev_test_data *data = test->data;

	k_timer_init(&data->timer, rtio_iodev_timer_fn, NULL);
}

#define RTIO_IODEV_TEST_DEFINE(name, qsize)                                                        \
	static struct rtio_iodev_test_data _iodev_data_##name;                                     \
	RTIO_IODEV_DEFINE(name, &rtio_iodev_test_api, qsize, &_iodev_data_##name)

#endif /* RTIO_IODEV_TEST_H_ */
