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

/**
 * @file
 * @brief Header file for extended sensor API of BMP581 sensor
 * @ingroup bmp581_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_BMP581_USER_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_BMP581_USER_H_

/**
 * @brief Bosch BMP581 Barometric pressure sensor
 * @defgroup bmp581_interface BMP581
 * @ingroup sensor_interface_ext
 * @{
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/dt-bindings/sensor/bmp581.h>

/* Custom ATTR values */

/* This is used to enable IIR config,
 * keep in mind that disabling IIR back in runtime is not
 * supported yet
 */

/**
 * @brief IIR configuration for pressure and temperature channels
 *
 * sensor_value.val1 is the IIR filter coefficient for temperature.
 * sensor_value.val2 is the IIR filter coefficient for pressure.
 *
 * See @ref bmp581_iir_filter for the valid values.
 */
#define BMP5_ATTR_IIR_CONFIG (SENSOR_ATTR_PRIV_START + 1u)
/**
 * @brief Power mode
 *
 * The attribute value should be a @ref bmp5_powermode enum value.
 */
#define BMP5_ATTR_POWER_MODE (SENSOR_ATTR_PRIV_START + 2u)

/**
 * @brief BMP581 power modes
 */
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

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_BMP581_USER_H_ */
