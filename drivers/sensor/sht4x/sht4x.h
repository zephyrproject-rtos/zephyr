/*
 * Copyright (c) 2021 Leonard Pollak
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_SHT4X_SHT4X_H_
#define ZEPHYR_DRIVERS_SENSOR_SHT4X_SHT4X_H_

#include <device.h>
#include <kernel.h>
#include <drivers/gpio.h>

#define SHT4X_CMD_READ_SERIAL	0x89
#define SHT4X_CMD_RESET		0x94

#define SHT4X_RESET_WAIT_MS	1

#if CONFIG_SHT4X_HEATER_ENABLE

	#if CONFIG_SHT4X_HEATER_POWER_HIGH
		#define SHT4X_HEATER_POWER_HIGH_IDX	0
	#elif CONFIG_SHT4X_HEATER_POWER_MED
		#define SHT4X_HEATER_POWER_MED_IDX	1
	#elif CONFIG_SHT4X_HEATER_POWER_LOW
		#define SHT4X_HEATER_POWER_LOW_IDX	2
	#endif

	#if CONFIG_SHT4X_HEATER_LONG
		#define SHT4X_HEATER_DURATION_LONG_IDX	0
	#elif CONFIG_SHT4X_HEATER_SHORT
		#define SHT4X_HEATER_DURATION_SHORT_IDX	1
	#endif
#endif

#if CONFIG_SHT4X_REPEATABILITY_LOW
	#define SHT4X_REPEATABILITY_IDX	0
#elif CONFIG_SHT4X_REPEATABILITY_MEDIUM
	#define SHT4X_REPEATABILITY_IDX	1
#elif CONFIG_SHT4X_REPEATABILITY_HIGH
	#define SHT4X_REPEATABILITY_IDX	2
#endif

struct sht4x_config {
	const struct device *bus;
	uint8_t i2c_addr;
	uint8_t repeatability_idx;
};

struct sht4x_data {
	uint16_t t_sample;
	uint16_t rh_sample;
};

static const uint8_t measure_cmd[3] = {
	0xFD, 0xF6, 0xE0
};

static const uint8_t measure_wait_ms[3] = {
	2, 5, 9
};

#endif /* ZEPHYR_DRIVERS_SENSOR_SHT4X_SHT4X_H_ */
