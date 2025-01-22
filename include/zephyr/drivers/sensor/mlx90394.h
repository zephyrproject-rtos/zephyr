/*
 * Copyright (c) 2024 Florian Weber <Florian.Weber@live.de>
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended public API for Melexis mlx90394 Sensor
 *
 * Some capabilities and operational requirements for this sensor
 * cannot be expressed within the sensor driver abstraction.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_MLX90394_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_MLX90394_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>

enum mlx90394_sensor_attribute {
	MLX90394_SENSOR_ATTR_MAGN_LOW_NOISE = SENSOR_ATTR_PRIV_START,
	MLX90394_SENSOR_ATTR_MAGN_FILTER_XY,
	MLX90394_SENSOR_ATTR_MAGN_FILTER_Z,
	MLX90394_SENSOR_ATTR_MAGN_OSR,
	MLX90394_SENSOR_ATTR_TEMP_FILTER,
	MLX90394_SENSOR_ATTR_TEMP_OSR
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_MLX90394_H_ */
