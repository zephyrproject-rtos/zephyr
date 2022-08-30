/*
 * Copyright (c) 2021, Jimmy Johnson <catch22@fastmail.net>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended public API for TI's TMP108 temperature sensor
 *
 * This exposes attributes for the TMP108 which can be used for
 * setting the on-chip Temperature Mode and alert parameters.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_TMP108_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_TMP108_H_

#ifdef __cplusplus
extern "C" {
#endif

enum sensor_attribute_tmp_108 {
	/** Turn on power saving/one shot mode */
	SENSOR_ATTR_TMP108_ONE_SHOT_MODE = SENSOR_ATTR_PRIV_START,
	/** Shutdown the sensor */
	SENSOR_ATTR_TMP108_SHUTDOWN_MODE,
	/** Turn on continuous conversion */
	SENSOR_ATTR_TMP108_CONTINUOUS_CONVERSION_MODE,
	/** Set the alert pin polarity */
	SENSOR_ATTR_TMP108_ALERT_POLARITY
};

/** a mask for the over temp alert bit in the status word*/
#define OVER_TEMP_MASK 0x1000U

/** a mask for the under temp alert bit in the status word*/
#define UNDER_TEMP_MASK	0x0800U

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_TMP108_H_ */
