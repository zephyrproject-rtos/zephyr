/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ST_LPS33HW_LPS33HW_H_
#define ZEPHYR_DRIVERS_SENSOR_ST_LPS33HW_LPS33HW_H_

#include <stdint.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>

#define LPS33HW_REG_WHO_AM_I		0x0F
#define LPS33HW_WHO_AM_I_VALUE		0xB1

#define LPS33HW_REG_CTRL_REG1		0x10
#define LPS33HW_CTRL_REG1_ODR_MASK	GENMASK(6, 4)
#define LPS33HW_CTRL_REG1_ODR_SHIFT	4
#define LPS33HW_CTRL_REG1_BDU		BIT(1)

#define LPS33HW_REG_CTRL_REG2		0x11
#define LPS33HW_CTRL_REG2_IF_ADD_INC	BIT(4)
#define LPS33HW_CTRL_REG2_SWRESET	BIT(2)

#define LPS33HW_REG_PRESS_OUT_XL	0x28

#define LPS33HW_RESET_TIMEOUT_MS	100

struct lps33hw_config {
	struct i2c_dt_spec i2c;
	uint8_t odr;
};

struct lps33hw_data {
	int32_t pressure;
	int16_t temperature;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_ST_LPS33HW_LPS33HW_H_ */
