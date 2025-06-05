/*
 * Copyright (c) 2025 Croxel Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/rtio/rtio.h>

#include "rtio_sched.h"

static void rtio_sched_alarm_expired(struct k_timeout_record *record)
{
	struct rtio_sqe *sqe = CONTAINER_OF(record, struct rtio_sqe, delay.record);
	struct rtio_iodev_sqe *iodev_sqe = CONTAINER_OF(sqe, struct rtio_iodev_sqe, sqe);

	rtio_iodev_sqe_ok(iodev_sqe, 0);
}

void rtio_sched_alarm(struct rtio_iodev_sqe *iodev_sqe, k_timeout_t timeout)
{
	struct rtio_sqe *sqe = &iodev_sqe->sqe;

	k_timeout_record_init(&sqe->delay.record);
	k_timeout_record_add(&sqe->delay.record, rtio_sched_alarm_expired, timeout);
}
