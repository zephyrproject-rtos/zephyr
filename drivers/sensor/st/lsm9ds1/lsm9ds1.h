/*
 * Copyright (c) 2024 Bootlin
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_LSM9DS1_LSM9DS1_H_
#define ZEPHYR_DRIVERS_SENSOR_LSM9DS1_LSM9DS1_H_

#include "lsm9ds1_reg.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <zephyr/drivers/spi.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <zephyr/drivers/i2c.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */

#define GAIN_UNIT_XL (61LL)
#define GAIN_UNIT_G  (8750LL)

#define TEMP_OFFSET      25 /* raw output of zero indicate 25°C */
#define TEMP_SENSITIVITY 16 /* 16 LSB / °C */

#define GYRO_ODR_MASK 0x7

struct lsm9ds1_config {
	stmdev_ctx_t ctx;
	union {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
		const struct i2c_dt_spec i2c;
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
		const struct spi_dt_spec spi;
#endif
	} stmemsc_cfg;
	uint8_t accel_range;
	uint8_t gyro_range;
	uint8_t imu_odr;
};

struct lsm9ds1_data {
	int16_t acc[3];
	uint32_t acc_gain;
	int16_t gyro[3];
	uint32_t gyro_gain;

	uint16_t accel_odr;
	uint16_t gyro_odr;
#if defined(CONFIG_LSM9DS1_ENABLE_TEMP)
	int16_t temp_sample;
#endif
};

#endif /* ZEPHYR_DRIVERS_SENSOR_LSM9DS1_LSM9DS1_H_ */
