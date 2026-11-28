/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

/* Single consumer queues: submit under rc_sync_lock, consume only on rc_rx */

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/rc/rc.h>
#include <zephyr/rtio/rtio.h>

#include "rc_internal.h"

RTIO_DEFINE(rc_sync_rtio, CONFIG_RC_RECEIVE_SYNC_MAX, CONFIG_RC_RECEIVE_SYNC_MAX);
static K_MUTEX_DEFINE(rc_sync_lock);

struct rc_sync_wait {
	struct k_sem done;
	int result;
};

void rc_sync_dispatch(void)
{
	struct rtio_cqe *cqe;

	while ((cqe = rtio_cqe_consume(&rc_sync_rtio)) != NULL) {
		struct rc_sync_wait *wait = cqe->userdata;

		wait->result = cqe->result;
		rtio_cqe_release(&rc_sync_rtio, cqe);
		k_sem_give(&wait->done);
	}
}

int rc_receive_sync(const struct device *dev, struct rc_event *evt, k_timeout_t timeout)
{
	struct rc_iodev_config config = {.dev = dev};
	/* On the stack: every path below waits for the read */
	const struct rtio_iodev iodev = {
		.api = &rc_event_iodev_api,
		.data = &config,
	};
	struct rc_sync_wait wait;
	rtio_sqe_handle_t handle;
	struct rtio_sqe *sqe;

	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	/* Completions are dispatched by rc_rx */
	if (rc_rx_is_current()) {
		return -EDEADLK;
	}

	k_sem_init(&wait.done, 0, 1);

	(void)k_mutex_lock(&rc_sync_lock, K_FOREVER);
	sqe = rtio_sqe_acquire(&rc_sync_rtio);
	if (sqe == NULL) {
		(void)k_mutex_unlock(&rc_sync_lock);
		return -ENOMEM;
	}
	rtio_sqe_prep_read(sqe, &iodev, RTIO_PRIO_NORM, (uint8_t *)evt, sizeof(*evt), &wait);
	handle = rtio_sqe_handle(&rc_sync_rtio, sqe);
	(void)rtio_submit(&rc_sync_rtio, 0);
	(void)k_mutex_unlock(&rc_sync_lock);

	if (k_sem_take(&wait.done, timeout) != 0) {
		/* The read completes once: -ECANCELED or an event */
		(void)rtio_sqe_cancel(&rc_sync_rtio, handle);
		(void)k_sem_take(&wait.done, K_FOREVER);
	}

	return (wait.result == -ECANCELED) ? -EAGAIN : wait.result;
}
