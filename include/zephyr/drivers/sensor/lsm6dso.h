/*
 * Copyright (c) 2022 tecinvent.ch
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended public API for the ST LSM6DSO
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_LSM6DSO_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_LSM6DSO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <drivers/sensor.h>

enum sensor_channel_lsm6dso {
	/** Tilt channel **/
	SENSOR_CHAN_TILT = SENSOR_CHAN_PRIV_START,
	/** Activity / Inactivity **/
	SENSOR_CHAN_ACTIVITY_INACTIVITY
};

enum sensor_attribute_lsm6dso {
	SENSOR_ATTR_TILT_ON_INT_1 = SENSOR_ATTR_PRIV_START,
	SENSOR_ATTR_ONLY_TILT_ON_INT_1,
	SENSOR_ATTR_TILT_ON_INT_2,
	SENSOR_ATTR_ACTIVITY_INACTIVITY_WAKE_THS,
	SENSOR_ATTR_ACTIVITY_INACTIVITY_WAKE_DUR,
	SENSOR_ATTR_ACTIVITY_INACTIVITY_SLEEP_DUR,
	SENSOR_ATTR_ACTIVITY_INACTIVITY_SLEEP_FUNC,
	SENSOR_ATTR_ACTIVITY_INACTIVITY_ENABLE_BASIC_INT,
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_LSM6DSO_ */