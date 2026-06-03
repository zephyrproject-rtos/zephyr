/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "loopback_iodev.h"

#include <zephyr/drivers/pulse_io.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/mpsc_lockfree.h>
#include <string.h>

#define PULSE_IO_LOOPBACK_DEPTH 8

struct pulse_io_loopback {
	struct rtio_iodev iodev;
	struct mpsc io_q;
	struct rtio_iodev_sqe *txn_head;
	struct rtio_iodev_sqe *txn_curr;
	struct k_timer timer;
	struct k_spinlock lock;
	struct pulse_symbol stash[PULSE_IO_LOOPBACK_DEPTH * 16];
	size_t stash_len;
};

static struct pulse_io_loopback pulse_io_loopback_inst;

static void loopback_next(struct pulse_io_loopback *lb, bool completion)
{
	k_spinlock_key_t key = k_spin_lock(&lb->lock);

	if (!completion && lb->txn_head != NULL) {
		goto out;
	}

	struct mpsc_node *next = mpsc_pop(&lb->io_q);

	if (next == NULL) {
		lb->txn_head = NULL;
		lb->txn_curr = NULL;
		goto out;
	}

	struct rtio_iodev_sqe *next_sqe = CONTAINER_OF(next, struct rtio_iodev_sqe, q);

	lb->txn_head = next_sqe;
	lb->txn_curr = next_sqe;
	k_timer_start(&lb->timer, K_MSEC(1), K_NO_WAIT);

out:
	k_spin_unlock(&lb->lock, key);
}

static void loopback_complete(struct pulse_io_loopback *lb, int status)
{
	if (status < 0) {
		rtio_iodev_sqe_err(lb->txn_head, status);
		loopback_next(lb, true);
		return;
	}

	lb->txn_curr = rtio_txn_next(lb->txn_curr);
	if (lb->txn_curr != NULL) {
		k_timer_start(&lb->timer, K_MSEC(1), K_NO_WAIT);
		return;
	}

	rtio_iodev_sqe_ok(lb->txn_head, status);
	loopback_next(lb, true);
}

static void loopback_timer_fn(struct k_timer *timer)
{
	struct pulse_io_loopback *lb = CONTAINER_OF(timer, struct pulse_io_loopback, timer);
	struct rtio_iodev_sqe *iodev_sqe = lb->txn_curr;
	struct rtio_sqe *sqe = &iodev_sqe->sqe;

	if (sqe->op == RTIO_OP_TX) {
		size_t nsyms = sqe->tx.buf_len / sizeof(struct pulse_symbol);

		if (nsyms > ARRAY_SIZE(lb->stash)) {
			loopback_complete(lb, -ENOMEM);
			return;
		}
		memcpy(lb->stash, sqe->tx.buf, nsyms * sizeof(struct pulse_symbol));
		lb->stash_len = nsyms;
		loopback_complete(lb, 0);
	} else if (sqe->op == RTIO_OP_RX) {
		uint8_t *buf = NULL;
		uint32_t buf_len = 0;
		size_t want = lb->stash_len * sizeof(struct pulse_symbol);
		int rc = rtio_sqe_rx_buf(iodev_sqe, want, want, &buf, &buf_len);

		if (rc != 0) {
			loopback_complete(lb, rc);
			return;
		}
		memcpy(buf, lb->stash, want);
		loopback_complete(lb, (int)want);
	} else {
		loopback_complete(lb, -ENOTSUP);
	}
}

static void loopback_submit(struct rtio_iodev_sqe *iodev_sqe)
{
	struct pulse_io_loopback *lb =
		CONTAINER_OF(iodev_sqe->sqe.iodev, struct pulse_io_loopback, iodev);

	mpsc_push(&lb->io_q, &iodev_sqe->q);
	loopback_next(lb, false);
}

static const struct rtio_iodev_api loopback_iodev_api = {
	.submit = loopback_submit,
};

void pulse_io_loopback_iodev_init(void)
{
	struct pulse_io_loopback *lb = &pulse_io_loopback_inst;

	lb->iodev.api = &loopback_iodev_api;
	mpsc_init(&lb->io_q);
	k_timer_init(&lb->timer, loopback_timer_fn, NULL);
}

struct rtio_iodev *pulse_io_loopback_iodev(void)
{
	return &pulse_io_loopback_inst.iodev;
}
