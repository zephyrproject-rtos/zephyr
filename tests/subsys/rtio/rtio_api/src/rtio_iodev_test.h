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
	/* k_timer for an asynchronous task */
	struct k_timer timer;

	/* Currently executing sqe */
	struct rtio_iodev_sqe *iodev_sqe;
	const struct rtio_sqe *sqe;

	/* Count of submit calls */
	atomic_t submit_count;

	/* Lock around kicking off next timer */
	struct k_spinlock lock;
};

static void rtio_iodev_test_next(struct rtio_iodev *iodev)
{
	struct rtio_iodev_test_data *data = iodev->data;

	/* The next section must be serialized to ensure single consumer semantics */
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (data->iodev_sqe != NULL) {
		goto out;
	}

	struct rtio_mpsc_node *next = rtio_mpsc_pop(&iodev->iodev_sq);

	if (next != NULL) {
		struct rtio_iodev_sqe *next_sqe = CONTAINER_OF(next, struct rtio_iodev_sqe, q);

		data->iodev_sqe = next_sqe;
		data->sqe = next_sqe->sqe;
		k_timer_start(&data->timer, K_MSEC(10), K_NO_WAIT);
	}

out:
	k_spin_unlock(&data->lock, key);
}

static void rtio_iodev_timer_fn(struct k_timer *tm)
{
	struct rtio_iodev_test_data *data = CONTAINER_OF(tm, struct rtio_iodev_test_data, timer);
	struct rtio_iodev_sqe *iodev_sqe = data->iodev_sqe;
	struct rtio_iodev *iodev = (struct rtio_iodev *)iodev_sqe->sqe->iodev;

	if (data->sqe->op == RTIO_OP_RX) {
		uint8_t *buf;
		uint32_t buf_len;

		int rc = rtio_sqe_rx_buf(iodev_sqe, 16, 16, &buf, &buf_len);

		if (rc != 0) {
			rtio_iodev_sqe_err(iodev_sqe, rc);
			return;
		}

		for (int i = 0; i < 16; ++i) {
			buf[i] = ((uint8_t *)iodev_sqe->sqe->userdata)[i];
		}
	}

	if (data->sqe->flags & RTIO_SQE_TRANSACTION) {
		data->sqe = rtio_spsc_next(data->iodev_sqe->r->sq, data->sqe);
		k_timer_start(&data->timer, K_MSEC(10), K_NO_WAIT);
		return;
	}

	data->iodev_sqe = NULL;
	data->sqe = NULL;
	rtio_iodev_sqe_ok(iodev_sqe, 0);
	rtio_iodev_test_next(iodev);
}

static void rtio_iodev_test_submit(struct rtio_iodev_sqe *iodev_sqe)
{
	struct rtio_iodev *iodev = (struct rtio_iodev *)iodev_sqe->sqe->iodev;
	struct rtio_iodev_test_data *data = iodev->data;

	atomic_inc(&data->submit_count);

	/* The only safe operation is enqueuing */
	rtio_mpsc_push(&iodev->iodev_sq, &iodev_sqe->q);

	rtio_iodev_test_next(iodev);
}

const struct rtio_iodev_api rtio_iodev_test_api = {
	.submit = rtio_iodev_test_submit,
};

void rtio_iodev_test_init(struct rtio_iodev *test)
{
	struct rtio_iodev_test_data *data = test->data;

	rtio_mpsc_init(&test->iodev_sq);
	data->iodev_sqe = NULL;
	k_timer_init(&data->timer, rtio_iodev_timer_fn, NULL);
}

#define RTIO_IODEV_TEST_DEFINE(name)                                                               \
	static struct rtio_iodev_test_data _iodev_data_##name;                                     \
	RTIO_IODEV_DEFINE(name, &rtio_iodev_test_api, &_iodev_data_##name)



#endif /* RTIO_IODEV_TEST_H_ */
