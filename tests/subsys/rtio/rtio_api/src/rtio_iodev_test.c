/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/rtio/rtio.h>
#include <zephyr/kernel.h>
#include "rtio_iodev_test.h"

static void rtio_iodev_timer_fn(struct k_timer *tm)
{
	struct rtio_iodev_test *iodev = CONTAINER_OF(tm, struct rtio_iodev_test, timer);

	struct rtio *r = iodev->r;
	const struct rtio_sqe *sqe = iodev->sqe;

	iodev->r = NULL;
	iodev->sqe = NULL;

	/* Complete the request with Ok and a result */
	printk("sqe ok callback\n");
	rtio_sqe_ok(r, sqe, 0);
}

static void rtio_iodev_test_submit(const struct rtio_sqe *sqe, struct rtio *r)
{
	struct rtio_iodev_test *iodev = (struct rtio_iodev_test *)sqe->iodev;

	/**
	 * TODO this isn't quite right, probably should be equivalent to a
	 * pend instead of a fail here. This submission chain on this iodev
	 * needs to wait until the iodev is available again, which should be
	 * checked after each sqe using the iodev completes. A smart
	 * executor then should have, much like a thread scheduler, a pend
	 * list that it checks against on each completion.
	 */
	if (k_timer_remaining_get(&iodev->timer) != 0) {
		printk("would block, timer not free!\n");
		rtio_sqe_err(r, sqe, -EWOULDBLOCK);
		return;
	}

	iodev->sqe = sqe;
	iodev->r = r;

	/**
	 * Simulate an async hardware request with a one shot timer
	 *
	 * In reality the time to complete might have some significant variance
	 * but this is proof enough of a working API flow.
	 *
	 * TODO enable setting this the time here in some way
	 */
	printk("starting one shot\n");
	k_timer_start(&iodev->timer, K_MSEC(10), K_NO_WAIT);
}

const struct rtio_iodev_api rtio_iodev_test_api = {
	.submit = rtio_iodev_test_submit,
};

void rtio_iodev_test_init(struct rtio_iodev_test *test)
{
	test->iodev.api = &rtio_iodev_test_api;
	k_timer_init(&test->timer, rtio_iodev_timer_fn, NULL);
}
