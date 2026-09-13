/*
 * Copyright (c) 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_VL53L5CX_RTIO_H_
#define ZEPHYR_DRIVERS_SENSOR_VL53L5CX_RTIO_H_

#include <zephyr/device.h>
#include <zephyr/rtio/rtio.h>

void vl53l5cx_submit(const struct device *sensor, struct rtio_iodev_sqe *iodev_sqe);

#endif /* ZEPHYR_DRIVERS_SENSOR_VL53L5CX_RTIO_H_ */
