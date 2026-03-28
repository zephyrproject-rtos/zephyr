/*
 * Copyright (c) 2025 Croxel, Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BMI08X_BMI08X_GYRO_STREAM_H_
#define ZEPHYR_DRIVERS_SENSOR_BMI08X_BMI08X_GYRO_STREAM_H_

#include <zephyr/device.h>
#include <zephyr/rtio/rtio.h>

int bmi08x_gyro_stream_init(const struct device *dev);
void bmi08x_gyro_stream_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);

#endif /* ZEPHYR_DRIVERS_SENSOR_BMI08X_BMI08X_GYRO_STREAM_H_ */
