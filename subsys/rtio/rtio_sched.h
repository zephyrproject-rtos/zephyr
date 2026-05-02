/*
 * Copyright (c) 2025 Croxel Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#ifndef ZEPHYR_SUBSYS_RTIO_SCHED_H_
#define ZEPHYR_SUBSYS_RTIO_SCHED_H_

void rtio_sched_alarm(struct rtio_iodev_sqe *iodev_sqe, k_timeout_t timeout);

#endif /* ZEPHYR_SUBSYS_RTIO_SCHED_H_ */
