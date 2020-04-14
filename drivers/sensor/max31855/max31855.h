/*
 * Copyright (c) 2020 Christian Hirsch
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_MAX31855_MAX31855_H_
#define ZEPHYR_DRIVERS_SENSOR_MAX31855_MAX31855_H_

#include <zephyr/types.h>
#include <device.h>

struct max31855_data {
	struct device *spi;
	struct spi_config spi_cfg;

	/* Value */
	s32_t data;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_MAX31855_MAX31855_H_ */
