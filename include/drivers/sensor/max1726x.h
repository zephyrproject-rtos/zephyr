/*
 * Copyright (c) 2021 Wiifor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs for the MAX1726X driver.
 *
 * Following types require to be visible by user application and can't be
 * defined in max1726x driver header.
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_MAX1726X_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_MAX1726X_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <drivers/sensor.h>

/**
 * @brief MAX1726X attribute types.
 */
enum max1726x_sensor_attribute {
	SENSOR_ATTR_MAX1726X_HIBERNATE = SENSOR_CHAN_PRIV_START,
	SENSOR_ATTR_MAX1726X_SHUTDOWN,
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_MAX1726X_H_ */