/*
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 * Copyright (c) 2026 Swarovski Optik AG & Co KG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM45605_STREAM_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM45605_STREAM_H_

int icm45605_stream_init(const struct device *dev);

void icm45605_stream_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);

#endif /* ZEPHYR_DRIVERS_SENSOR_ICM45605_STREAM_H_ */
