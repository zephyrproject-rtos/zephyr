/*
 * Copyright (c) 2024, Vitrolife A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_VEAA_X_3_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_VEAA_X_3_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>

enum sensor_attribute_veaa_x_3 {
	/* Set pressure setpoint value in kPa. */
	SENSOR_ATTR_VEAA_X_3_SETPOINT = SENSOR_ATTR_PRIV_START,
	/* Supported pressure range in kPa. val1 is minimum and val2 is maximum */
	SENSOR_ATTR_VEAA_X_3_RANGE,
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_VEAA_X_3_H_ */
