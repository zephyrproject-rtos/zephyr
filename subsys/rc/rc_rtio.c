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

static uint8_t rc_sqe_valid_op(const struct rtio_iodev_sqe *iodev_sqe)
{
	return (IS_ENABLED(CONFIG_RC_TX) && (iodev_sqe->sqe.iodev->api == &rc_tx_iodev_api))
		       ? RTIO_OP_TX
		       : RTIO_OP_RX;
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
		} else if (iodev_sqe->sqe.op != rc_sqe_valid_op(iodev_sqe)) {
			err = -EINVAL;
		}
		rtio_iodev_sqe_err(iodev_sqe, err);
		iodev_sqe = next;
	}

	rc_sync_dispatch();
	if (IS_ENABLED(CONFIG_RC_TX)) {
		rc_tx_dispatch();
	}
}

static K_WORK_DEFINE(rc_done_work, rc_done_work_handler);

void rc_sqe_done(struct rtio_iodev_sqe *iodev_sqe)
{
	k_spinlock_key_t key = k_spin_lock(&rc_done_lock);

	rc_sqe_set_next(iodev_sqe, rc_done_head);
	rc_done_head = iodev_sqe;
	k_spin_unlock(&rc_done_lock, key);

	(void)rc_rx_submit(&rc_done_work);
}

/* Caller holds the list lock */
static void rc_list_append(struct rtio_iodev_sqe **head, struct rtio_iodev_sqe **tail,
			   struct rtio_iodev_sqe *iodev_sqe)
{
	rc_sqe_set_next(iodev_sqe, NULL);
	if (*tail == NULL) {
		*head = iodev_sqe;
	} else {
		rc_sqe_set_next(*tail, iodev_sqe);
	}
	*tail = iodev_sqe;
}

static bool rc_list_remove(struct rtio_iodev_sqe **head, struct rtio_iodev_sqe **tail,
			   struct rtio_iodev_sqe *iodev_sqe)
{
	struct rtio_iodev_sqe *prev = NULL;

	for (struct rtio_iodev_sqe *curr = *head; curr != NULL;
	     prev = curr, curr = rc_sqe_next(curr)) {
		if (curr != iodev_sqe) {
			continue;
		}

		if (prev == NULL) {
			*head = rc_sqe_next(curr);
		} else {
			rc_sqe_set_next(prev, rc_sqe_next(curr));
		}
		if (*tail == curr) {
			*tail = prev;
		}

		return true;
	}

	return false;
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
	key = k_spin_lock(&data->evt_lock);
	rc_list_append(&data->evt_head, &data->evt_tail, iodev_sqe);
	k_spin_unlock(&data->evt_lock, key);
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
		removed = rc_list_remove(&data->evt_head, &data->evt_tail, iodev_sqe);
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

	rc_callback_call(dev, evt);
}

static void rc_raw_iodev_submit(struct rtio_iodev_sqe *iodev_sqe)
{
	struct rc_raw_iodev_data *raw = iodev_sqe->sqe.iodev->data;
	k_spinlock_key_t key;

	if (!device_is_ready(raw->config.dev) || (iodev_sqe->sqe.op != RTIO_OP_RX)) {
		rc_sqe_done(iodev_sqe);
		return;
	}

	key = k_spin_lock(&raw->lock);
	rc_list_append(&raw->head, &raw->tail, iodev_sqe);
	k_spin_unlock(&raw->lock, key);
}

static void rc_raw_iodev_cancel(const struct rtio_iodev *iodev, struct rtio *r,
				rtio_sqe_handle_t handle)
{
	struct rc_raw_iodev_data *raw = iodev->data;
	struct rtio_iodev_sqe *iodev_sqe;
	k_spinlock_key_t key;
	bool removed = false;

	key = k_spin_lock(&raw->lock);
	iodev_sqe = rtio_iodev_sqe_from_handle(r, handle);
	if (iodev_sqe != NULL) {
		removed = rc_list_remove(&raw->head, &raw->tail, iodev_sqe);
	}
	raw->active = false;
	raw->missed = false;
	k_spin_unlock(&raw->lock, key);

	if (removed) {
		rc_sqe_done(iodev_sqe);
	}
}

const struct rtio_iodev_api rc_raw_iodev_api = {
	.submit = rc_raw_iodev_submit,
	.cancel = rc_raw_iodev_cancel,
};

bool rc_raw_has_readers(void)
{
	int count;

	STRUCT_SECTION_COUNT(rc_raw_iodev_data, &count);

	return count > 0;
}

static void rc_raw_complete(struct rtio_iodev_sqe *iodev_sqe, bool missed, const uint32_t *words,
			    size_t count)
{
	size_t total = count + (missed ? 1U : 0U);
	uint32_t buf_len;
	uint32_t *out;
	size_t cap;
	size_t n = 0;
	uint8_t *buf;

	if (rtio_sqe_rx_buf(iodev_sqe, sizeof(uint32_t), total * sizeof(uint32_t), &buf,
			    &buf_len) != 0) {
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}

	out = (uint32_t *)buf;
	cap = MIN(buf_len / sizeof(uint32_t), total);
	if (missed) {
		out[n++] = RC_RAW_OVERFLOW;
	}
	for (size_t i = 0; n < cap; i++) {
		out[n++] = words[i];
	}
	if (cap < total) {
		out[cap - 1U] = RC_RAW_OVERFLOW;
	}

	rtio_iodev_sqe_ok(iodev_sqe, (int)cap);
}

void rc_raw_report(const struct device *dev, const uint32_t *words, size_t count)
{
	STRUCT_SECTION_FOREACH(rc_raw_iodev_data, raw) {
		struct rtio_iodev_sqe *iodev_sqe;
		k_spinlock_key_t key;
		bool missed;

		if (raw->config.dev != dev) {
			continue;
		}

		/* Detach first: a multishot completion re-submits to this list */
		key = k_spin_lock(&raw->lock);
		iodev_sqe = raw->head;
		raw->head = NULL;
		raw->tail = NULL;
		missed = raw->missed;
		raw->missed = (iodev_sqe == NULL) && raw->active;
		if (iodev_sqe != NULL) {
			raw->active = true;
		}
		k_spin_unlock(&raw->lock, key);

		while (iodev_sqe != NULL) {
			struct rtio_iodev_sqe *next = rc_sqe_next(iodev_sqe);

			rc_sqe_set_next(iodev_sqe, NULL);
			if (rtio_iodev_sqe_is_canceled(iodev_sqe)) {
				rtio_iodev_sqe_err(iodev_sqe, -ECANCELED);
			} else {
				rc_raw_complete(iodev_sqe, missed, words, count);
			}
			iodev_sqe = next;
		}
	}
}
