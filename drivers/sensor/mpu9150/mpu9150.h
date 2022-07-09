/*
 * Copyright (c) 2022 Benjamin Björnsson <benjamin.bjornsson@gmail.com>.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_MPU9150_MPU9150_H_
#define ZEPHYR_DRIVERS_SENSOR_MPU9150_MPU9150_H_

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <stdbool.h>

#define MPU9150_REG_BYPASS_CFG		0x37
#define MPU9150_I2C_BYPASS_EN		BIT(1)

#define MPU9150_REG_PWR_MGMT1		0x6B
#define MPU9150_SLEEP_EN		BIT(6)

struct mpu9150_config {
	struct i2c_dt_spec i2c;
	bool ak8975_pass_through;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_MPU9150_MPU9150_H_ */
