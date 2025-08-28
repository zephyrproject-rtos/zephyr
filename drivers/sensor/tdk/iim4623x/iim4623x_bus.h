/*
 * Copyright (c) 2025 Sentry Technologies ApS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_TDK_IIM4623X_BUS_H
#define ZEPHYR_DRIVERS_SENSOR_TDK_IIM4623X_BUS_H

#include <zephyr/device.h>

/* Synchronously write to the iim4623x */
int iim4623x_bus_write(const struct device *dev, const uint8_t *buf, size_t len);

/* Synchronously read from the iim4623x */
int iim4623x_bus_read(const struct device *dev, uint8_t *buf, size_t len);

/* Synchronously write to then read from the iim4623x */
int iim4623x_bus_write_then_read(const struct device *dev, const uint8_t *wbuf, size_t wlen,
				 uint8_t *rbuf, size_t rlen);

#endif /* ZEPHYR_DRIVERS_SENSOR_TDK_IIM4623X_BUS_H */
