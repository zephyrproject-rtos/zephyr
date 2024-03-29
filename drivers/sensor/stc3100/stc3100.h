/*
 * Copyright (c) 2024 Benedikt Schmidt <benediktibk@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_STC3100_H_
#define ZEPHYR_DRIVERS_SENSOR_STC3100_H_

#include <zephyr/drivers/i2c.h>

struct stc3100_config {
	struct i2c_dt_spec i2c;
	uint16_t sense_resistor;
	uint32_t nominal_capacity;
};

struct stc3100_data {
	/* cumulative charge in µAh */
	int32_t cumulative_charge;
	/* current in µA */
	int32_t current;
	/* temperature in 1e-6 °C */
	int32_t temperature;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_STC3100_H_ */
