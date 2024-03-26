/*
 * Copyright (c) 2024 Amarula Solutions B.V.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_SHT2X_H_
#define ZEPHYR_DRIVERS_SENSOR_SHT2X_H_

#include <zephyr/device.h>

#define SHT2X_CMD_RESET		0xFE
#define SHT2X_RESET_WAIT_MS	15

/*
 * CRC parameters were taken from the
 * "Checksum Calculation" section of the datasheet.
 */
#define SHT2X_CRC_POLY		0x31
#define SHT2X_CRC_INIT		0x00

struct sht2x_config {
	struct i2c_dt_spec bus;
	uint8_t repeatability;
};

struct sht2x_data {
	uint16_t t_sample;
	uint16_t rh_sample;
};

#define READ_TEMP	0
#define READ_HUMIDITY	1

static const uint8_t measure_cmd[2] = {
	 0xF3, 0xF5
};

static const uint16_t measure_wait_ms[2] = {
	85, 29
};

#endif /* ZEPHYR_DRIVERS_SENSOR_SHT2X_H_ */
