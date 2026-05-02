/*
 * Copyright (c) 2023 SILA Embedded Solutions GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_INFINEON_TLE9104_TLE9104_DIAGNOSTICS_H_
#define ZEPHYR_DRIVERS_SENSOR_INFINEON_TLE9104_TLE9104_DIAGNOSTICS_H_

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/mfd/tle9104.h>

struct tle9104_diagnostics_data {
	struct gpio_tle9104_channel_diagnostics values[TLE9104_GPIO_COUNT];
};

struct tle9104_diagnostics_config {
	const struct device *parent;
	uint8_t channel;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_INFINEON_TLE9104_TLE9104_DIAGNOSTICS_H_ */
