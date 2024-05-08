/*
 * Copyright (c) 2023 Andreas Karner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_ADXL345_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_ADXL345_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>

/**
 * @brief Custom trigger values for the adxl345 accelerometer
 */
enum sensor_trigger_type_adxl345 {
	/**
	 * Threshold in m/s2 to identify an active movement
	 */
	SENSOR_ATTR_ACTIVE_THRESH,

	/**
	 * Threshold in m/s2 to identify an inactive movement
	 */
	SENSOR_ATTR_INACTIVE_THRESH,

	/**
	 * Threshold in seconds to identify an inactive movement
	 */
	SENSOR_ATTR_INACTIVE_TIME,

	/**
	 *  Various accelerator sensors have the attribute to link active and inactive movements
	 */
	SENSOR_ATTR_LINK_MOVEMENT,
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_ADXL345_H_ */
