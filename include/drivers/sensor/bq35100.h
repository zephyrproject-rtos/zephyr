/*
 * Copyright (c) 2021 arithmetics.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended public API for the Texas Instruments BQ35100
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_BQ35100_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_BQ35100_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <drivers/sensor.h>

/**
 * @brief Extended sensor channels specific to this device.
 */
enum sensor_channel_bq35100 {
	/** Example channel 1 **/
	SENSOR_CHAN_BQ35100_EXAMPLE1 = SENSOR_CHAN_PRIV_START,
	/** Internal Temperature **/
	SENSOR_CHAN_BQ35100_GAUGE_INT_TEMP,
	/** Designed Capacity **/
	SENSON_CHAN_BQ35100_GAUGE_DES_CAP

};

/**
 * @brief Extended sensor attribute types specific to this device
 */
enum sensor_attribute_bq35100 {
	/** Example attribute 1 **/
	SENSOR_ATTR_BQ35100_EXAMPLE1 = SENSOR_CHAN_PRIV_START,
	/** Gauge Start **/
	SENSOR_ATTR_BQ35100_GAUGE_START,
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_BQ35100_ */
