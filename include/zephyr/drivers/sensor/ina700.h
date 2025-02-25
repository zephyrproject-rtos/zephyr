/*
 * Original license from ina23x driver:
 *  Copyright 2021 The Chromium OS Authors
 *  Copyright 2021 Grinn
 *
 * Copyright 2024, Remie Lowik
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended public API for INA700 current / power sensor
 *
 * This header file exposes an attribute an helper function to allow the
 * runtime configuration of ROM IDs for 1-Wire Sensors.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_INA700_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_INA700_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>

enum sensor_channel_ina700 {
	/** Energy in joules **/
	SENSOR_CHAN_ENERGY = SENSOR_CHAN_PRIV_START,
	/** Charge in coulombs **/
	SENSOR_CHAN_CHARGE,
};

enum sensor_attribute_ina700 {
	SENSOR_ATTR_CURRENT_OVER_LIMIT_THRESHOLD = SENSOR_ATTR_PRIV_START,
	SENSOR_ATTR_CURRENT_UNDER_LIMIT_THRESHOLD,
	SENSOR_ATTR_BUS_OVERVOLTAGE_THRESHOLD,
	SENSOR_ATTR_BUS_UNDERVOLTAGE_THRESHOLD,
	SENSOR_ATTR_TEMPERATURE_OVER_LIMIT_THRESHOLD,
	SENSOR_ATTR_POWER_OVER_LIMIT_THRESHOLD,
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_INA700_H_ */
