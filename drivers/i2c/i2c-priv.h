/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_I2C_I2C_PRIV_H_
#define ZEPHYR_DRIVERS_I2C_I2C_PRIV_H_

#include <zephyr/drivers/i2c.h>
#include <zephyr/dt-bindings/i2c/i2c.h>
#include <zephyr/logging/log.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline uint32_t i2c_map_dt_bitrate(uint32_t bitrate)
{
	switch (bitrate) {
	case I2C_BITRATE_STANDARD:
		return I2C_SPEED_STANDARD << I2C_SPEED_SHIFT;
	case I2C_BITRATE_FAST:
		return I2C_SPEED_FAST << I2C_SPEED_SHIFT;
	case I2C_BITRATE_FAST_PLUS:
		return I2C_SPEED_FAST_PLUS << I2C_SPEED_SHIFT;
	case I2C_BITRATE_HIGH:
		return I2C_SPEED_HIGH << I2C_SPEED_SHIFT;
	case I2C_BITRATE_ULTRA:
		return I2C_SPEED_ULTRA << I2C_SPEED_SHIFT;
	}

	LOG_ERR("Invalid I2C bit rate value");

	return 0;
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_I2C_I2C_PRIV_H_ */
