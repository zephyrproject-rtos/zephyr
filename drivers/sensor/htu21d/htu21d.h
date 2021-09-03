/*
 * Copyright (c) 2021, Jonas Norling
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_HTU21D_HTU21D_H_
#define ZEPHYR_DRIVERS_SENSOR_HTU21D_HTU21D_H_

#include <device.h>

#define HTU21D_TRIGGER_T_NHM  0xF3
#define HTU21D_TRIGGER_RH_NHM 0xF5
#define HTU21D_SOFT_RESET     0xFE

struct htu21d_data {
	uint16_t t_sample;
	uint16_t rh_sample;
};

struct htu21d_config {
	struct i2c_dt_spec i2c;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_HTU21_HTU21D_H_ */
