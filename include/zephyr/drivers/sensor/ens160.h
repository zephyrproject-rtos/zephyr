/*
 * Copyright (c) 2024 Gustavo Silva
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of ENS160 sensor
 * @ingroup ens160_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_ENS160_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_ENS160_H_

/**
 * @defgroup ens160_interface ENS160
 * @ingroup sensor_interface_ext
 * @brief ScioSense ENS160 digital metal-oxide multi-gas sensor
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>

/**
 * @brief Custom sensor channels for ENS160
 */
enum sensor_channel_ens160 {
	/** Air Quality Index */
	SENSOR_CHAN_ENS160_AQI = SENSOR_CHAN_PRIV_START,
};

/**
 * @brief Custom sensor attributes for ENS160
 */
enum sensor_attribute_ens160 {
	/**
	 * Temperature compensation.
	 * Used to set ambient temperature compensation for the sensor.
	 */
	SENSOR_ATTR_ENS160_TEMP = SENSOR_ATTR_PRIV_START,
	/**
	 * Relative humidity compensation.
	 * Used to set ambient relative humidity compensation for the sensor.
	 */
	SENSOR_ATTR_ENS160_RH,
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_ENS160_H_ */
