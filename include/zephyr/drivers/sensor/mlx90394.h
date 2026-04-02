/*
 * Copyright (c) 2024 Florian Weber <Florian.Weber@live.de>
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of MLX90394 sensor
 * @ingroup mlx90394_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_MLX90394_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_MLX90394_H_

/**
 * @brief Melexis MLX90394 magnetometer
 * @defgroup mlx90394_interface MLX90394
 * @ingroup sensor_interface_ext
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>

/**
 * @brief Custom sensor attributes for MLX90394
 */
enum mlx90394_sensor_attribute {
	/**
	 * Low noise mode. (SENSOR_CHAN_MAGN_XYZ channel only)
	 */
	MLX90394_SENSOR_ATTR_MAGN_LOW_NOISE = SENSOR_ATTR_PRIV_START,
	/**
	 * Digital filter setting for X and Y axes
	 *
	 * Controls the number of filter taps applied to the X and Y axes for noise reduction.
	 * Higher values provide better noise performance at cost of measurement time.
	 *
	 * - sensor_value.val1: Filter setting (0-7)
	 */
	MLX90394_SENSOR_ATTR_MAGN_FILTER_XY,
	/**
	 * Digital filter configuration for Z axis
	 *
	 * Similar to XY filter but configured independently since Z axis typically has different
	 * noise characteristics.
	 *
	 * - sensor_value.val1: Filter setting (0-7)
	 */
	MLX90394_SENSOR_ATTR_MAGN_FILTER_Z,
	/**
	 * Over-sampling ratio for magnetic measurements.
	 *
	 * - sensor_value.val1 == 0: Normal sampling rate.
	 * - sensor_value.val1 == 1: Double sampling rate.
	 */
	MLX90394_SENSOR_ATTR_MAGN_OSR,
	/**
	 * Digital filter configuration for temperature measurements
	 *
	 * Higher settings provide better temperature measurement stability.
	 *
	 * - sensor_value.val1: Filter setting (0-7)
	 */
	MLX90394_SENSOR_ATTR_TEMP_FILTER,
	/**
	 * Over-sampling ratio for temperature measurements.
	 *
	 * - sensor_value.val1 == 0: Normal sampling rate.
	 * - sensor_value.val1 == 1: Double sampling rate.
	 */
	MLX90394_SENSOR_ATTR_TEMP_OSR
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_MLX90394_H_ */
