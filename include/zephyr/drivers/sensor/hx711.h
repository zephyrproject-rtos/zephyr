/*
 * Copyright (c) 2026 Zephyr Project Developers
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of HX711 sensor
 * @ingroup hx711_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_HX711_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_HX711_H_

/**
 * @defgroup hx711_interface HX711
 * @ingroup sensor_interface_ext
 * @brief Avia Semiconductor HX711 24-bit ADC Specialized for Load Cells / Strain Gauges
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>

/**
 * @brief Custom sensor channels for HX711
 */
enum sensor_channel_hx711 {
	/** Measured mass in grams */
	SENSOR_CHAN_HX711_MASS = SENSOR_CHAN_PRIV_START,
};

/**
 * @brief Custom sensor attributes for HX711
 */
enum sensor_attribute_hx711 {
	/**
	 * Voltage to mass conversion factor in kg/uV.
	 */
	SENSOR_ATTR_HX711_CONV_FACTOR = SENSOR_ATTR_PRIV_START,
	/**
	 * Voltage offset to the real zero of the sensor, used in mass conversion.
	 */
	SENSOR_ATTR_HX711_OFFSET = SENSOR_ATTR_PRIV_START + 1,
};

/**
 * @brief Gain options for the next read operation
 */
enum hx711_gain {
	/** Gain = 64 */
	HX711_GAIN_64,
	/** Gain = 128 */
	HX711_GAIN_128,
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_HX711_H_ */
