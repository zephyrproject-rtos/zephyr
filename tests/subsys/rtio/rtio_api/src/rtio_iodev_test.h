/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/mpsc_lockfree.h>
#include <zephyr/kernel.h>

#ifndef RTIO_IODEV_TEST_H_
#define RTIO_IODEV_TEST_H_

struct rtio_iodev_test_data {
	/* k_timer for an asynchronous task */
	struct k_timer timer;

	/* Queue of requests */
	struct mpsc io_q;

	/* Currently executing transaction */
	struct rtio_iodev_sqe *txn_head;
	struct rtio_iodev_sqe *txn_curr;

	/* Count of submit calls */
	atomic_t submit_count;

	/* Lock around kicking off next timer */
	struct k_spinlock lock;

	/* Mocked result to receive by the IODEV */
	int result;
};

static void rtio_iodev_test_next(struct rtio_iodev_test_data *data, bool completion)
{
	/* The next section must be serialized to ensure single consumer semantics */
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	/* Already working on something, bail early */
	if (!completion && data->txn_head != NULL) {
		goto out;
	}

	struct mpsc_node *next = mpsc_pop(&data->io_q);

	/* Nothing left to do, cleanup */
	if (next == NULL) {
		data->txn_head = NULL;
		data->txn_curr = NULL;
		goto out;
	}

	struct rtio_iodev_sqe *next_sqe = CONTAINER_OF(next, struct rtio_iodev_sqe, q);

	data->txn_head = next_sqe;
	data->txn_curr = next_sqe;
	k_timer_start(&data->timer, K_MSEC(10), K_NO_WAIT);

out:
	k_spin_unlock(&data->lock, key);
}

static void rtio_iodev_test_complete(struct rtio_iodev_test_data *data, int status)
{
	if (status < 0) {
		rtio_iodev_sqe_err(data->txn_head, status);
		rtio_iodev_test_next(data, true);
		return;
	}

	data->txn_curr = rtio_txn_next(data->txn_curr);
	if (data->txn_curr) {
		k_timer_start(&data->timer, K_MSEC(10), K_NO_WAIT);
		return;
	}

	rtio_iodev_sqe_ok(data->txn_head, status);
	rtio_iodev_test_next(data, true);
}

static void rtio_iodev_await_signaled(struct rtio_iodev_sqe *iodev_sqe, void *userdata)
{
	struct rtio_iodev_test_data *data = userdata;

	rtio_iodev_test_complete(data, data->result);
}

static void rtio_iodev_timer_fn(struct k_timer *tm)
{
	struct rtio_iodev_test_data *data = CONTAINER_OF(tm, struct rtio_iodev_test_data, timer);
	struct rtio_iodev_sqe *iodev_sqe = data->txn_curr;
	uint8_t *buf;
	uint32_t buf_len;
	int rc;

	switch (iodev_sqe->sqe.op) {
	case RTIO_OP_NOP:
		rtio_iodev_test_complete(data, data->result);
		break;
	case RTIO_OP_RX:
		rc = rtio_sqe_rx_buf(iodev_sqe, 16, 16, &buf, &buf_len);
		if (rc != 0) {
			rtio_iodev_test_complete(data, rc);
			return;
		}
		/* For reads the test device copies from the given userdata */
		memcpy(buf, ((uint8_t *)iodev_sqe->sqe.userdata), 16);
		rtio_iodev_test_complete(data, data->result);
		break;
	case RTIO_OP_AWAIT:
		rtio_iodev_sqe_await_signal(iodev_sqe, rtio_iodev_await_signaled, data);
		break;
	default:
		rtio_iodev_test_complete(data, -ENOTSUP);
	}
}

static void rtio_iodev_test_submit(struct rtio_iodev_sqe *iodev_sqe)
{
	struct rtio_iodev *iodev = (struct rtio_iodev *)iodev_sqe->sqe.iodev;
	struct rtio_iodev_test_data *data = iodev->data;

	atomic_inc(&data->submit_count);

	/* The only safe operation is enqueuing */
	mpsc_push(&data->io_q, &iodev_sqe->q);

	rtio_iodev_test_next(data, false);
}

const struct rtio_iodev_api rtio_iodev_test_api = {
	.submit = rtio_iodev_test_submit,
};

void rtio_iodev_test_init(struct rtio_iodev *test)
{
	struct rtio_iodev_test_data *data = test->data;

	mpsc_init(&data->io_q);
	data->txn_head = NULL;
	data->txn_curr = NULL;
	k_timer_init(&data->timer, rtio_iodev_timer_fn, NULL);
	data->result = 0;
}

void rtio_iodev_test_set_result(struct rtio_iodev *test, int result)
{
	struct rtio_iodev_test_data *data = test->data;

	data->result = result;
}

#define RTIO_IODEV_TEST_DEFINE(name)                                                               \
	static struct rtio_iodev_test_data _iodev_data_##name;                                     \
	RTIO_IODEV_DEFINE(name, &rtio_iodev_test_api, &_iodev_data_##name)



#endif /* RTIO_IODEV_TEST_H_ */
