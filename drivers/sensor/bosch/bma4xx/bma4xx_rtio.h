/*
 * Copyright (c) 2023 Google LLC
 * Copyright (c) 2024 Cienet
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BMA4XX_RTIO_H_
#define ZEPHYR_DRIVERS_SENSOR_BMA4XX_RTIO_H_

#include <zephyr/device.h>
#include <zephyr/rtio/rtio.h>

void bma4xx_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);

void bma4xx_submit_stream(const struct device *sensor, struct rtio_iodev_sqe *iodev_sqe);

void bma4xx_fifo_event(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_SENSOR_BMA4XX_RTIO_H_ */
