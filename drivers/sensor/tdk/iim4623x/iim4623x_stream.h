/*
 * Copyright (c) 2025 Sentry Technologies ApS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_TDK_IIM4623X_STREAM_H
#define ZEPHYR_DRIVERS_SENSOR_TDK_IIM4623X_STREAM_H

#include <zephyr/device.h>
#include <zephyr/rtio/rtio.h>

void iim4623x_stream_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);

void iim4623x_stream_event(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_SENSOR_TDK_IIM4623X_STREAM_H */
