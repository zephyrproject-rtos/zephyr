/*
 * Copyright (c) 2024 Chaim Zax
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ADXL34X_RTIO_H_
#define ZEPHYR_DRIVERS_SENSOR_ADXL34X_RTIO_H_

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

void adxl34x_submit(const struct device *dev, struct rtio_iodev_sqe *sqe);
int adxl34x_rtio_handle_motion_data(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_SENSOR_ADXL34X_RTIO_H_ */
