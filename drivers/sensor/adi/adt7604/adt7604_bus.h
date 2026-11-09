/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ADI_ADT7604_ADT7604_BUS_H_
#define ZEPHYR_DRIVERS_SENSOR_ADI_ADT7604_ADT7604_BUS_H_

#include <zephyr/drivers/sensor.h>

int adt7604_bus_read(const struct device *dev, uint16_t reg, uint8_t *buf, size_t len);
int adt7604_bus_write(const struct device *dev, uint16_t reg, const uint8_t *buf, size_t len);

#endif /* ZEPHYR_DRIVERS_SENSOR_ADI_ADT7604_ADT7604_BUS_H_ */
