/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/rtio/rtio.h>
#include <zephyr/kernel.h>

#ifndef RTIO_IODEV_TEST_H_
#define RTIO_IODEV_TEST_H_

/*
 * @brief A simple asynchronous testable iodev
 */
struct rtio_iodev_test {
	/**
	 * io device struct as the first member, makes this an rtio_iodev
	 */
	struct rtio_iodev iodev;

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

void rtio_iodev_test_init(struct rtio_iodev_test *test);

#endif /* RTIO_IODEV_TEST_H_ */
