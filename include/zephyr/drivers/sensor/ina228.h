/*
 * Copyright 2025 Nova Dynamics LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_INA23X_INA228_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_INA23X_INA228_H_

#include <zephyr/drivers/sensor.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief INA228 extra sensor channels.
 */
enum sensor_channel_ina228 {
	/** Accumulated energy, in Joules **/
	SENSOR_CHAN_INA228_ENERGY = SENSOR_CHAN_PRIV_START,

	/** Accumulated charge, in Coulombs **/
	SENSOR_CHAN_INA228_CHARGE,
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_INA23X_INA228_H_ */
