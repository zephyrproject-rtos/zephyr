/*
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM45686_STREAM_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM45686_STREAM_H_

int icm45686_stream_init(const struct device *dev);

void icm45686_stream_submit(const struct device *dev,
			    struct rtio_iodev_sqe *iodev_sqe);

#endif /* ZEPHYR_DRIVERS_SENSOR_ICM45686_STREAM_H_ */
