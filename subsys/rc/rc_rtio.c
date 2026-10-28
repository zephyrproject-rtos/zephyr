/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

/* All rc completions run on rc_rx: one CQE producer per context */

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/rc.h>
#include <zephyr/kernel.h>
#include <zephyr/rc/rc.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/mpsc_lockfree.h>

#include "rc_internal.h"

static struct k_spinlock rc_done_lock;
static struct rtio_iodev_sqe *rc_done_head;

static struct rtio_iodev_sqe *rc_sqe_next(struct rtio_iodev_sqe *iodev_sqe)
{
	struct mpsc_node *node = mpsc_ptr_get(iodev_sqe->q.next);

	return (node == NULL) ? NULL : CONTAINER_OF(node, struct rtio_iodev_sqe, q);
}

static void rc_sqe_set_next(struct rtio_iodev_sqe *iodev_sqe, struct rtio_iodev_sqe *next)
{
	mpsc_ptr_set(iodev_sqe->q.next, (next == NULL) ? NULL : &next->q);
}

static const struct device *rc_sqe_dev(const struct rtio_iodev_sqe *iodev_sqe)
{
	const struct rc_iodev_config *config = iodev_sqe->sqe.iodev->data;

	return config->dev;
}

static void rc_done_work_handler(struct k_work *work)
{
	struct rtio_iodev_sqe *iodev_sqe;
	k_spinlock_key_t key;

	ARG_UNUSED(work);

	key = k_spin_lock(&rc_done_lock);
	iodev_sqe = rc_done_head;
	rc_done_head = NULL;
	k_spin_unlock(&rc_done_lock, key);

	while (iodev_sqe != NULL) {
		struct rtio_iodev_sqe *next = rc_sqe_next(iodev_sqe);
		int err = -ECANCELED;

		rc_sqe_set_next(iodev_sqe, NULL);
		if (!device_is_ready(rc_sqe_dev(iodev_sqe))) {
			err = -ENODEV;
		} else if (iodev_sqe->sqe.op != RTIO_OP_RX) {
			err = -EINVAL;
		}
		rtio_iodev_sqe_err(iodev_sqe, err);
		iodev_sqe = next;
	}

	rc_sync_dispatch();
}

static K_WORK_DEFINE(rc_done_work, rc_done_work_handler);

static void rc_sqe_done(struct rtio_iodev_sqe *iodev_sqe)
{
	k_spinlock_key_t key = k_spin_lock(&rc_done_lock);

	rc_sqe_set_next(iodev_sqe, rc_done_head);
	rc_done_head = iodev_sqe;
	k_spin_unlock(&rc_done_lock, key);

	(void)rc_rx_submit(&rc_done_work);
}

static void rc_event_iodev_submit(struct rtio_iodev_sqe *iodev_sqe)
{
	const struct device *dev = rc_sqe_dev(iodev_sqe);
	struct rc_dev_data *data;
	k_spinlock_key_t key;

	if (!device_is_ready(dev) || (iodev_sqe->sqe.op != RTIO_OP_RX)) {
		rc_sqe_done(iodev_sqe);
		return;
	}

	data = rc_data(dev);
	rc_sqe_set_next(iodev_sqe, NULL);

	key = k_spin_lock(&data->evt_lock);
	if (data->evt_tail == NULL) {
		data->evt_head = iodev_sqe;
	} else {
		rc_sqe_set_next(data->evt_tail, iodev_sqe);
	}
	data->evt_tail = iodev_sqe;
	k_spin_unlock(&data->evt_lock, key);
}

/* Caller holds evt_lock */
static bool rc_evt_remove(struct rc_dev_data *data, struct rtio_iodev_sqe *iodev_sqe)
{
	struct rtio_iodev_sqe *prev = NULL;

	for (struct rtio_iodev_sqe *curr = data->evt_head; curr != NULL;
	     prev = curr, curr = rc_sqe_next(curr)) {
		if (curr != iodev_sqe) {
			continue;
		}

		if (prev == NULL) {
			data->evt_head = rc_sqe_next(curr);
		} else {
			rc_sqe_set_next(prev, rc_sqe_next(curr));
		}
		if (data->evt_tail == curr) {
			data->evt_tail = prev;
		}

		return true;
	}

	return false;
}

static void rc_event_iodev_cancel(const struct rtio_iodev *iodev, struct rtio *r,
				  rtio_sqe_handle_t handle)
{
	const struct rc_iodev_config *config = iodev->data;
	struct rtio_iodev_sqe *iodev_sqe;
	struct rc_dev_data *data;
	k_spinlock_key_t key;
	bool removed = false;

	if (!device_is_ready(config->dev)) {
		return;
	}

	data = rc_data(config->dev);
	key = k_spin_lock(&data->evt_lock);
	iodev_sqe = rtio_iodev_sqe_from_handle(r, handle);
	if (iodev_sqe != NULL) {
		removed = rc_evt_remove(data, iodev_sqe);
	}
	k_spin_unlock(&data->evt_lock, key);

	/* Not pending: already completed by its owner */
	if (removed) {
		rc_sqe_done(iodev_sqe);
	}
}

const struct rtio_iodev_api rc_event_iodev_api = {
	.submit = rc_event_iodev_submit,
	.cancel = rc_event_iodev_cancel,
};

void rc_event_report(const struct device *dev, const struct rc_event *evt)
{
	struct rc_dev_data *data = rc_data(dev);
	struct rtio_iodev_sqe *iodev_sqe;
	k_spinlock_key_t key;

	/* Detach first: a multishot completion re-submits to this list */
	key = k_spin_lock(&data->evt_lock);
	iodev_sqe = data->evt_head;
	data->evt_head = NULL;
	data->evt_tail = NULL;
	k_spin_unlock(&data->evt_lock, key);

	while (iodev_sqe != NULL) {
		struct rtio_iodev_sqe *next = rc_sqe_next(iodev_sqe);
		uint32_t buf_len;
		uint8_t *buf;

		rc_sqe_set_next(iodev_sqe, NULL);

		if (rtio_iodev_sqe_is_canceled(iodev_sqe)) {
			rtio_iodev_sqe_err(iodev_sqe, -ECANCELED);
		} else if (rtio_sqe_rx_buf(iodev_sqe, sizeof(*evt), sizeof(*evt), &buf, &buf_len) !=
			   0) {
			rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		} else {
			memcpy(buf, evt, sizeof(*evt));
			rtio_iodev_sqe_ok(iodev_sqe, 0);
		}

		iodev_sqe = next;
	}

	rc_sync_dispatch();
}
