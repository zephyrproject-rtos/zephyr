/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

/* One transmit work queue for all devices; completions run on rc_rx */

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/rc.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/rc/rc.h>
#include <zephyr/rc/rc_protocol.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/mpsc_lockfree.h>

#include "rc_internal.h"

static K_THREAD_STACK_DEFINE(rc_tx_stack, CONFIG_RC_TX_WORKQ_STACK_SIZE);
static struct k_work_q rc_tx_wq;

static struct k_spinlock rc_tx_lock;
static struct rtio_iodev_sqe *rc_tx_head;
static struct rtio_iodev_sqe *rc_tx_tail;

static struct rtio_iodev_sqe *rc_tx_done_sqe;
static int rc_tx_done_result;

RTIO_DEFINE(rc_send_rtio, CONFIG_RC_SEND_SYNC_MAX, CONFIG_RC_SEND_SYNC_MAX);
static K_MUTEX_DEFINE(rc_send_lock);

struct rc_send_wait {
	struct k_sem done;
	int result;
	/* Set on timeout: stop at the next frame */
	atomic_t stop;
};

/* Used only by the transmit work queue */
static uint32_t rc_tx_words[CONFIG_RC_TX_BUFFER_WORDS];

static struct rtio_iodev_sqe *rc_tx_next(struct rtio_iodev_sqe *iodev_sqe)
{
	struct mpsc_node *node = mpsc_ptr_get(iodev_sqe->q.next);

	return (node == NULL) ? NULL : CONTAINER_OF(node, struct rtio_iodev_sqe, q);
}

static void rc_tx_set_next(struct rtio_iodev_sqe *iodev_sqe, struct rtio_iodev_sqe *next)
{
	mpsc_ptr_set(iodev_sqe->q.next, (next == NULL) ? NULL : &next->q);
}

static const struct device *rc_tx_dev(const struct rtio_iodev_sqe *iodev_sqe)
{
	const struct rc_iodev_config *config = iodev_sqe->sqe.iodev->data;

	return config->dev;
}

static bool rc_tx_capable(const struct device *dev)
{
	const struct rc_driver_api *api = DEVICE_API_GET(rc, dev);

	return ((rc_data(dev)->caps.roles & RC_ROLE_RAW_TX) != 0U) && (api->tx != NULL);
}

static size_t rc_tx_max_words(const struct device *dev)
{
	uint32_t max_words = rc_data(dev)->caps.max_tx_words;

	if ((max_words != 0U) && (max_words < ARRAY_SIZE(rc_tx_words))) {
		return max_words;
	}

	return ARRAY_SIZE(rc_tx_words);
}

static int rc_tx_check(const struct device *dev, const uint32_t *words, size_t count,
		       uint32_t *total_us)
{
	uint32_t total = 0U;

	if (count > rc_tx_max_words(dev)) {
		return -ENOBUFS;
	}

	if ((count == 0U) || ((count % 2U) == 0U)) {
		return -EINVAL;
	}

	for (size_t i = 0; i < count; i++) {
		uint32_t us = rc_raw_duration_us(words[i]);

		if (((words[i] & (RC_RAW_TIMEOUT | RC_RAW_OVERFLOW)) != 0U) ||
		    (rc_raw_is_pulse(words[i]) != ((i % 2U) == 0U)) || (us == 0U) ||
		    (us > RC_TX_MAX_DURATION_US - total)) {
			return -EINVAL;
		}
		total += us;
	}

	*total_us = total;

	return 0;
}

static const struct rc_protocol *rc_tx_encoder(uint16_t proto)
{
	STRUCT_SECTION_FOREACH(rc_protocol, p) {
		if ((p->encode != NULL) && (proto >= p->proto_first) && (proto <= p->proto_last)) {
			return p;
		}
	}

	return NULL;
}

static int rc_tx_carrier(const struct device *dev, uint32_t hz)
{
	const struct rc_driver_api *api = DEVICE_API_GET(rc, dev);

	if ((api->set_carrier == NULL) || (rc_data(dev)->caps.carrier_max_hz == 0U)) {
		return 0;
	}

	return api->set_carrier(dev, hz, rc_data(dev)->tx_duty_pct);
}

static int rc_tx_raw(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct rc_driver_api *api = DEVICE_API_GET(rc, dev);
	const uint8_t *buf = iodev_sqe->sqe.tx.buf;
	size_t count = iodev_sqe->sqe.tx.buf_len / sizeof(uint32_t);
	uint32_t us;
	int ret;

	if ((iodev_sqe->sqe.tx.buf_len % sizeof(uint32_t)) != 0U) {
		return -EINVAL;
	}

	/* rc_tx_check() rejects more words than were copied */
	memcpy(rc_tx_words, buf, MIN(count, ARRAY_SIZE(rc_tx_words)) * sizeof(uint32_t));

	ret = rc_tx_check(dev, rc_tx_words, count, &us);
	if (ret != 0) {
		return ret;
	}

	/* A scancode transmission may have left its protocol carrier */
	ret = rc_tx_carrier(dev, rc_data(dev)->tx_carrier_hz);
	if (ret != 0) {
		return ret;
	}

	return api->tx(dev, rc_tx_words, count);
}

static bool rc_tx_stopped(struct rtio_iodev_sqe *iodev_sqe)
{
	if (rtio_iodev_sqe_is_canceled(iodev_sqe)) {
		return true;
	}

	if (iodev_sqe->r == &rc_send_rtio) {
		struct rc_send_wait *wait = iodev_sqe->sqe.userdata;

		return atomic_get(&wait->stop) != 0;
	}

	return false;
}

static int rc_tx_encode(const struct device *dev, const struct rc_protocol *p,
			const struct rc_tx_req *req, bool repeat, uint16_t flags, uint32_t *words,
			size_t max, size_t *count, uint32_t *us)
{
	int ret;

	if (repeat) {
		ret = p->encode_repeat(req->proto, req->scancode, flags, words, max);
	} else {
		ret = p->encode(req->proto, req->scancode, flags, words, max);
	}
	if (ret < 0) {
		return ret;
	}

	*count = (size_t)ret;
	if (*count > max) {
		return -EINVAL;
	}

	return rc_tx_check(dev, words, *count, us);
}

static uint32_t rc_tx_spacing_us(const struct rc_protocol *p, uint32_t us)
{
	return MAX((uint32_t)p->tx_period_ms * USEC_PER_MSEC, us + p->min_timeout_us);
}

static int rc_tx_scancode(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct rc_driver_api *api = DEVICE_API_GET(rc, dev);
	struct rc_dev_data *data = rc_data(dev);
	size_t max = rc_tx_max_words(dev);
	const struct rc_protocol *p;
	struct rc_tx_req req;
	bool repeat_code;
	uint32_t frame_us;
	uint32_t repeat_us;
	uint64_t total_us;
	uint16_t flags;
	size_t count;
	int ret;

	if (iodev_sqe->sqe.tx.buf_len != sizeof(req)) {
		return -EINVAL;
	}

	memcpy(&req, iodev_sqe->sqe.tx.buf, sizeof(req));

	p = rc_tx_encoder(req.proto);
	if (p == NULL) {
		return -ENOTSUP;
	}

	if ((req.repeats > 0U) && (p->tx_period_ms == 0U)) {
		return -EINVAL;
	}

	if ((p->carrier_hz != 0U) && (data->caps.carrier_max_hz != 0U) &&
	    ((p->carrier_hz < data->caps.carrier_min_hz) ||
	     (p->carrier_hz > data->caps.carrier_max_hz))) {
		return -ENOTSUP;
	}

	/* Committed once the request is sent */
	flags = !data->tx_toggle ? RC_EVENT_FLAG_TOGGLE : 0U;
	repeat_code = (req.repeats > 0U) && (p->encode_repeat != NULL);

	if (repeat_code) {
		ret = rc_tx_encode(dev, p, &req, true, flags, rc_tx_words, max, &count, &repeat_us);
		if (ret != 0) {
			return ret;
		}
	}

	ret = rc_tx_encode(dev, p, &req, false, flags, rc_tx_words, max, &count, &frame_us);
	if (ret != 0) {
		return ret;
	}

	if (!repeat_code) {
		repeat_us = frame_us;
	}

	total_us = frame_us;
	if (req.repeats > 0U) {
		total_us = (uint64_t)rc_tx_spacing_us(p, frame_us) +
			   ((uint64_t)req.repeats - 1U) * rc_tx_spacing_us(p, repeat_us) +
			   repeat_us;
	}
	if (total_us > ((uint64_t)CONFIG_RC_TX_MAX_DURATION_MS * USEC_PER_MSEC)) {
		return -E2BIG;
	}

	ret = rc_tx_carrier(dev, (p->carrier_hz != 0U) ? p->carrier_hz : data->tx_carrier_hz);
	if (ret != 0) {
		return ret;
	}

	data->tx_toggle = !data->tx_toggle;

	for (uint32_t i = 0U;; i++) {
		k_timepoint_t next;

		if (rc_tx_stopped(iodev_sqe)) {
			return -ECANCELED;
		}

		next = sys_timepoint_calc(
			K_USEC(rc_tx_spacing_us(p, (i == 0U) ? frame_us : repeat_us)));
		ret = api->tx(dev, rc_tx_words, count);
		if ((ret != 0) || (i == req.repeats)) {
			return ret;
		}

		/* The buffer keeps the full frame when there is no repeat code */
		if ((i == 0U) && repeat_code) {
			ret = rc_tx_encode(dev, p, &req, true, flags, rc_tx_words, max, &count,
					   &repeat_us);
			if (ret != 0) {
				return ret;
			}
		}

		k_sleep(sys_timepoint_timeout(next));
	}
}

static int rc_tx_one(struct rtio_iodev_sqe *iodev_sqe)
{
	const struct device *dev = rc_tx_dev(iodev_sqe);

	if (rtio_iodev_sqe_is_canceled(iodev_sqe)) {
		return -ECANCELED;
	}

	if (!rc_tx_capable(dev)) {
		return -ENOTSUP;
	}

	if ((iodev_sqe->sqe.iodev_flags & RC_IODEV_TX_SCANCODE) != 0U) {
		return rc_tx_scancode(dev, iodev_sqe);
	}

	return rc_tx_raw(dev, iodev_sqe);
}

static void rc_send_dispatch(void)
{
	struct rtio_cqe *cqe;

	while ((cqe = rtio_cqe_consume(&rc_send_rtio)) != NULL) {
		struct rc_send_wait *wait = cqe->userdata;

		wait->result = cqe->result;
		rtio_cqe_release(&rc_send_rtio, cqe);
		k_sem_give(&wait->done);
	}
}

void rc_tx_dispatch(void)
{
	rc_send_dispatch();
}

static void rc_tx_done_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	if (rc_tx_done_result == 0) {
		rtio_iodev_sqe_ok(rc_tx_done_sqe, 0);
	} else {
		rtio_iodev_sqe_err(rc_tx_done_sqe, rc_tx_done_result);
	}

	rc_send_dispatch();
}

static K_WORK_DEFINE(rc_tx_done_work, rc_tx_done_handler);

static void rc_tx_work_handler(struct k_work *work);

static K_WORK_DEFINE(rc_tx_work, rc_tx_work_handler);

static void rc_tx_work_handler(struct k_work *work)
{
	struct rtio_iodev_sqe *iodev_sqe;
	struct k_work_sync sync;
	k_spinlock_key_t key;
	bool more;

	ARG_UNUSED(work);

	key = k_spin_lock(&rc_tx_lock);
	iodev_sqe = rc_tx_head;
	if (iodev_sqe != NULL) {
		rc_tx_head = rc_tx_next(iodev_sqe);
		if (rc_tx_head == NULL) {
			rc_tx_tail = NULL;
		}
		rc_tx_set_next(iodev_sqe, NULL);
	}
	k_spin_unlock(&rc_tx_lock, key);

	if (iodev_sqe == NULL) {
		return;
	}

	rc_tx_done_result = rc_tx_one(iodev_sqe);
	rc_tx_done_sqe = iodev_sqe;
	(void)rc_rx_submit(&rc_tx_done_work);
	(void)k_work_flush(&rc_tx_done_work, &sync);

	key = k_spin_lock(&rc_tx_lock);
	more = (rc_tx_head != NULL);
	k_spin_unlock(&rc_tx_lock, key);

	if (more) {
		(void)k_work_submit_to_queue(&rc_tx_wq, &rc_tx_work);
	}
}

static void rc_tx_iodev_submit(struct rtio_iodev_sqe *iodev_sqe)
{
	k_spinlock_key_t key;

	if (!device_is_ready(rc_tx_dev(iodev_sqe)) || (iodev_sqe->sqe.op != RTIO_OP_TX)) {
		rc_sqe_done(iodev_sqe);
		return;
	}

	rc_tx_set_next(iodev_sqe, NULL);
	key = k_spin_lock(&rc_tx_lock);
	if (rc_tx_tail == NULL) {
		rc_tx_head = iodev_sqe;
	} else {
		rc_tx_set_next(rc_tx_tail, iodev_sqe);
	}
	rc_tx_tail = iodev_sqe;
	k_spin_unlock(&rc_tx_lock, key);

	(void)k_work_submit_to_queue(&rc_tx_wq, &rc_tx_work);
}

/* Caller holds rc_tx_lock */
static bool rc_tx_unlink(struct rtio_iodev_sqe *iodev_sqe)
{
	struct rtio_iodev_sqe *prev = NULL;

	for (struct rtio_iodev_sqe *curr = rc_tx_head; curr != NULL;
	     prev = curr, curr = rc_tx_next(curr)) {
		if (curr != iodev_sqe) {
			continue;
		}
		if (prev == NULL) {
			rc_tx_head = rc_tx_next(curr);
		} else {
			rc_tx_set_next(prev, rc_tx_next(curr));
		}
		if (rc_tx_tail == curr) {
			rc_tx_tail = prev;
		}

		return true;
	}

	return false;
}

static void rc_tx_iodev_cancel(const struct rtio_iodev *iodev, struct rtio *r,
			       rtio_sqe_handle_t handle)
{
	struct rtio_iodev_sqe *iodev_sqe;
	k_spinlock_key_t key;
	bool removed = false;

	ARG_UNUSED(iodev);

	key = k_spin_lock(&rc_tx_lock);
	iodev_sqe = rtio_iodev_sqe_from_handle(r, handle);
	if (iodev_sqe != NULL) {
		removed = rc_tx_unlink(iodev_sqe);
	}
	k_spin_unlock(&rc_tx_lock, key);

	if (removed) {
		rc_sqe_done(iodev_sqe);
	}
}

const struct rtio_iodev_api rc_tx_iodev_api = {
	.submit = rc_tx_iodev_submit,
	.cancel = rc_tx_iodev_cancel,
};

int rc_set_tx_carrier(const struct device *dev, uint32_t hz, uint8_t duty_pct)
{
	struct rc_dev_data *data = rc_data(dev);

	if (!rc_tx_capable(dev)) {
		return -ENOTSUP;
	}

	if ((duty_pct == 0U) || (duty_pct > 99U) || (hz < data->caps.carrier_min_hz) ||
	    (hz > data->caps.carrier_max_hz)) {
		return -EINVAL;
	}

	data->tx_carrier_hz = hz;
	data->tx_duty_pct = duty_pct;

	return 0;
}

static int rc_send_check(const struct device *dev)
{
	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	if (rc_rx_is_current() || (k_current_get() == k_work_queue_thread_get(&rc_tx_wq))) {
		return -EDEADLK;
	}

	if (!rc_tx_capable(dev)) {
		return -ENOTSUP;
	}

	return 0;
}

static int rc_send_wait(const struct device *dev, const uint8_t *buf, uint32_t len,
			uint32_t iodev_flags, k_timeout_t timeout)
{
	struct rc_iodev_config config = {.dev = dev};
	/* On the stack: every path below waits for the write */
	const struct rtio_iodev iodev = {
		.api = &rc_tx_iodev_api,
		.data = &config,
	};
	struct rc_send_wait wait;
	rtio_sqe_handle_t handle;
	struct rtio_sqe *sqe;

	k_sem_init(&wait.done, 0, 1);
	atomic_clear(&wait.stop);

	(void)k_mutex_lock(&rc_send_lock, K_FOREVER);
	sqe = rtio_sqe_acquire(&rc_send_rtio);
	if (sqe == NULL) {
		(void)k_mutex_unlock(&rc_send_lock);
		return -ENOMEM;
	}
	rtio_sqe_prep_write(sqe, &iodev, RTIO_PRIO_NORM, buf, len, &wait);
	sqe->iodev_flags = iodev_flags;
	handle = rtio_sqe_handle(&rc_send_rtio, sqe);
	(void)rtio_submit(&rc_send_rtio, 0);
	(void)k_mutex_unlock(&rc_send_lock);

	if (k_sem_take(&wait.done, timeout) != 0) {
		struct rtio_iodev_sqe *mine;
		k_spinlock_key_t key;
		bool unlinked = false;

		/* Unlink a write still queued; one being sent stops at the next frame */
		key = k_spin_lock(&rc_tx_lock);
		mine = rtio_iodev_sqe_from_handle(&rc_send_rtio, handle);
		if (mine != NULL) {
			unlinked = rc_tx_unlink(mine);
		}
		k_spin_unlock(&rc_tx_lock, key);

		if (unlinked) {
			rc_sqe_done(mine);
		} else {
			atomic_set(&wait.stop, 1);
		}
		(void)k_sem_take(&wait.done, K_FOREVER);
	}

	return (wait.result == -ECANCELED) ? -EAGAIN : wait.result;
}

int rc_send_raw_sync(const struct device *dev, const uint32_t *words, size_t count,
		     k_timeout_t timeout)
{
	int ret = rc_send_check(dev);
	uint32_t us;

	if (ret != 0) {
		return ret;
	}

	ret = rc_tx_check(dev, words, count, &us);
	if (ret != 0) {
		return ret;
	}

	return rc_send_wait(dev, (const uint8_t *)words, count * sizeof(uint32_t), 0U, timeout);
}

int rc_send_sync(const struct device *dev, const struct rc_tx_req *req, k_timeout_t timeout)
{
	int ret = rc_send_check(dev);

	if (ret != 0) {
		return ret;
	}

	if (rc_tx_encoder(req->proto) == NULL) {
		return -ENOTSUP;
	}

	return rc_send_wait(dev, (const uint8_t *)req, sizeof(*req), RC_IODEV_TX_SCANCODE, timeout);
}

static int rc_tx_wq_init(void)
{
	const struct k_work_queue_config cfg = {.name = "rc_tx"};

	k_work_queue_init(&rc_tx_wq);
	k_work_queue_start(&rc_tx_wq, rc_tx_stack, K_THREAD_STACK_SIZEOF(rc_tx_stack),
			   K_PRIO_PREEMPT(CONFIG_RC_TX_WORKQ_PRIORITY), &cfg);

	return 0;
}

SYS_INIT(rc_tx_wq_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
