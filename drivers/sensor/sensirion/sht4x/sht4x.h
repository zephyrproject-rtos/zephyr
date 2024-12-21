/*
 * Copyright (c) 2021 Leonard Pollak
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_SHT4X_SHT4X_H_
#define ZEPHYR_DRIVERS_SENSOR_SHT4X_SHT4X_H_

#include <zephyr/device.h>

#define SHT4X_CMD_READ_SERIAL	0x89
#define SHT4X_CMD_RESET		0x94

#define SHT4X_RESET_WAIT_MS	1

#define SHT4X_HEATER_POWER_IDX_MAX	3
#define SHT4X_HEATER_DURATION_IDX_MAX	2

/*
 * CRC parameters were taken from the
 * "Checksum Calculation" section of the datasheet.
 */
#define SHT4X_CRC_POLY		0x31
#define SHT4X_CRC_INIT		0xFF

struct sht4x_config {
	struct i2c_dt_spec bus;
	uint8_t repeatability;
};

struct sht4x_data {
	uint16_t t_sample;
	uint16_t rh_sample;
	uint8_t heater_power;
	uint8_t heater_duration;
};

static const uint8_t measure_cmd[3] = {
	 0xE0, 0xF6, 0xFD
};

static const uint16_t measure_wait_us[3] = {
	1600, 4500, 8300
};

/*
 * heater specifics
 *
 * power:
 * High power heater pulse -> ~200 mW  @3.3V
 * Medium power heater pulse -> ~110 mW  @3.3V
 * Low power heater pulse -> ~20 mW  @3.3V
 *
 * duration:
 * Long heater pulse -> 1.1s
 * Short heater pulse -> 0.11s
 */

static const int8_t heater_cmd[SHT4X_HEATER_POWER_IDX_MAX][SHT4X_HEATER_DURATION_IDX_MAX] = {
	{ 0x39, 0x32 },
	{ 0x2F, 0x24 },
	{ 0x1E, 0x15 }
};

static const uint32_t heater_wait_ms[SHT4X_HEATER_DURATION_IDX_MAX] = {
	1000, 100
};

#endif /* ZEPHYR_DRIVERS_SENSOR_SHT4X_SHT4X_H_ */
