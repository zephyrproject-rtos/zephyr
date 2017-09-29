/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _I2C_PRIV_H_
#define _I2C_PRIV_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <i2c.h>
#include <dt-bindings/i2c/i2c.h>

#ifndef SYS_LOG_LEVEL
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_I2C_LEVEL
#endif
#include <logging/sys_log.h>

static inline u32_t _i2c_map_dt_bitrate(u32_t bitrate)
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

	SYS_LOG_ERR("Invalid I2C bit rate value");

	return 0;
}

#ifdef __cplusplus
}
#endif

#endif /* _I2C_PRIV_H_ */
