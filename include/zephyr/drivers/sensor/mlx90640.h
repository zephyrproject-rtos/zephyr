/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of MLX90640
 * @ingroup mlx90640_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_MLX90640_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_MLX90640_H_

/**
 * @brief Melexis MLX90640 32x24 IR array
 * @defgroup mlx90640_interface MLX90640
 * @ingroup sensor_interface_ext_melexis
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>

/** Number of temperature pixels in one frame (32 columns x 24 rows). */
#define MLX90640_PIXELS      768
#define MLX90640_PIXEL_COLS  32
#define MLX90640_PIXEL_ROWS  24

/**
 * @brief Custom sensor attributes for MLX90640
 */
enum mlx90640_sensor_attribute {
	/**
	 * Object emissivity.
	 *
	 * - sensor_value.val1 + sensor_value.val2 * 10^-6 is the unit-less
	 *   emissivity (for example 0 + 950000 = 0.95).
	 */
	MLX90640_SENSOR_ATTR_EMISSIVITY = SENSOR_ATTR_PRIV_START,
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_MLX90640_H_ */
