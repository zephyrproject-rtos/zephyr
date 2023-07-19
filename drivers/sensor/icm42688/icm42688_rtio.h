/*
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM42688_RTIO_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM42688_RTIO_H_

int icm42688_submit(const struct device *sensor, struct rtio_iodev_sqe *iodev_sqe);

#endif /* ZEPHYR_DRIVERS_SENSOR_ICM42688_RTIO_H_ */
