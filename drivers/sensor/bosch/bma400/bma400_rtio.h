/*
 * Bosch BMA400 3-axis accelerometer driver
 * SPDX-FileCopyrightText: Copyright 2026 Luca Gessi lucagessi90@gmail.com
 * SPDX-FileCopyrightText: Copyright 2026 The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BMA400_RTIO_H_
#define ZEPHYR_DRIVERS_SENSOR_BMA400_RTIO_H_

#include <zephyr/device.h>
#include <zephyr/rtio/rtio.h>

void bma400_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);

void bma400_submit_stream(const struct device *sensor, struct rtio_iodev_sqe *iodev_sqe);

void bma400_stream_event(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_SENSOR_BMA400_RTIO_H_ */
