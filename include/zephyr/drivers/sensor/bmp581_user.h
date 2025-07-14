/*
 * Copyright (c) 2022 Badgerd Technologies B.V.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Driver is developed to be used with Zephyr. And it only supports i2c interface.
 *
 * Author: Talha Can Havadar <havadartalha@gmail.com>
 *
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_BMP581_USER_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_BMP581_USER_H_

#include <zephyr/drivers/sensor.h>
#include <zephyr/dt-bindings/sensor/bmp581.h>

/* Custom ATTR values */

/* This is used to enable IIR config,
 * keep in mind that disabling IIR back in runtime is not
 * supported yet
 */
#define BMP5_ATTR_IIR_CONFIG (SENSOR_ATTR_PRIV_START + 1u)
#define BMP5_ATTR_POWER_MODE (SENSOR_ATTR_PRIV_START + 2u)

enum bmp5_powermode {
	/*! Standby powermode */
	BMP5_POWERMODE_STANDBY,
	/*! Normal powermode */
	BMP5_POWERMODE_NORMAL,
	/*! Forced powermode */
	BMP5_POWERMODE_FORCED,
	/*! Continuous powermode */
	BMP5_POWERMODE_CONTINUOUS,
	/*! Deep standby powermode */
	BMP5_POWERMODE_DEEP_STANDBY
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_BMP581_USER_H_ */
