/*
 * Copyright (c) 2024 Bootlin
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_LSM9DS1_MAG_LSM9DS1_MAG_H_
#define ZEPHYR_DRIVERS_SENSOR_LSM9DS1_MAG_LSM9DS1_MAG_H_

#include "lsm9ds1_reg.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <zephyr/drivers/spi.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <zephyr/drivers/i2c.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */

struct lsm9ds1_mag_config {
	stmdev_ctx_t ctx;
	union {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
		const struct i2c_dt_spec i2c;
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
		const struct spi_dt_spec spi;
#endif
	} stmemsc_cfg;
	uint8_t mag_range;
	uint8_t mag_odr;
};

struct lsm9ds1_mag_data {
	int16_t mag[3];
	uint32_t mag_gain;
	uint8_t old_om;
	uint8_t powered_down;
};

#define MAX_NORMAL_ODR 80 /* 80 Hz : output data rate > 80 must be set differently */

/* Value to write in the register of the sensor to power it down */
#define LSM9DS1_MAG_POWER_DOWN_VALUE 2

#endif /* ZEPHYR_DRIVERS_SENSOR_LSM9DS1_MAG_LSM9DS1_MAG_H_ */
