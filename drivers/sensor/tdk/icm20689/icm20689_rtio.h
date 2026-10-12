/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM20689_RTIO_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM20689_RTIO_H_

#include <zephyr/device.h>
#include <zephyr/rtio/rtio.h>

void icm20689_submit(const struct device *sensor, struct rtio_iodev_sqe *iodev_sqe);

#endif /* ZEPHYR_DRIVERS_SENSOR_ICM20689_RTIO_H_ */
