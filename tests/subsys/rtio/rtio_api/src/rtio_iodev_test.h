/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/rtio/rtio_mpsc.h>
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
	atomic_ptr_t iodev_sqe;
};

static void rtio_iodev_timer_fn(struct k_timer *tm)
{
	struct rtio_iodev_test_data *data = CONTAINER_OF(tm, struct rtio_iodev_test_data, timer);
	struct rtio_iodev_sqe *iodev_sqe = atomic_ptr_get(&data->iodev_sqe);
	struct rtio_mpsc_node *next =
		rtio_mpsc_pop((struct rtio_mpsc *)&iodev_sqe->sqe->iodev->iodev_sq);

	if (next != NULL) {
		struct rtio_iodev_sqe *next_sqe = CONTAINER_OF(next, struct rtio_iodev_sqe, q);

		atomic_ptr_set(&data->iodev_sqe, next_sqe);
		TC_PRINT("starting timer again from queued iodev_sqe %p!\n", next);
		k_timer_start(&data->timer, K_MSEC(10), K_NO_WAIT);
	} else {
		atomic_ptr_set(&data->iodev_sqe, NULL);
	}

	/* Complete the request with Ok and a result */
	TC_PRINT("sqe ok callback\n");

	rtio_iodev_sqe_ok(iodev_sqe, 0);
}

static void rtio_iodev_test_submit(struct rtio_iodev_sqe *iodev_sqe)
{
	struct rtio_iodev *iodev = (struct rtio_iodev *)iodev_sqe->sqe->iodev;
	struct rtio_iodev_test_data *data = iodev->data;

	/*
	 * If a task is already going queue up the next request in the mpsc.
	 */
	if (!atomic_ptr_cas(&data->iodev_sqe, NULL, iodev_sqe)) {
		TC_PRINT("adding queued sqe\n");
		rtio_mpsc_push(&iodev->iodev_sq, &iodev_sqe->q);
	}

	/*
	 * Simulate an async hardware request with a one shot timer
	 *
	 * In reality the time to complete might have some significant variance
	 * but this is proof enough of a working API flow.
	 */
	TC_PRINT("starting one shot\n");
	k_timer_start(&data->timer, K_MSEC(10), K_NO_WAIT);
}

const struct rtio_iodev_api rtio_iodev_test_api = {
	.submit = rtio_iodev_test_submit,
};

void rtio_iodev_test_init(struct rtio_iodev *test)
{
	struct rtio_iodev_test_data *data = test->data;

	rtio_mpsc_init(&test->iodev_sq);
	atomic_ptr_set(&data->iodev_sqe, NULL);
	k_timer_init(&data->timer, rtio_iodev_timer_fn, NULL);
}

#define RTIO_IODEV_TEST_DEFINE(name)                                                               \
	static struct rtio_iodev_test_data _iodev_data_##name;                                     \
	RTIO_IODEV_DEFINE(name, &rtio_iodev_test_api, &_iodev_data_##name)



#endif /* RTIO_IODEV_TEST_H_ */
