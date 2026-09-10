/*
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/rtio/rtio.h>

#ifndef ZEPHYR_DRIVERS_SENSOR_PAA3905_STREAM_H_
#define ZEPHYR_DRIVERS_SENSOR_PAA3905_STREAM_H_

int paa3905_stream_init(const struct device *dev);

void paa3905_stream_submit(const struct device *dev,
			   struct rtio_iodev_sqe *iodev_sqe);

#endif /* ZEPHYR_DRIVERS_SENSOR_PAA3905_STREAM_H_ */
