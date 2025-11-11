/*
 * Copyright 2025 Nova Dynamics LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_INA2XX_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_INA2XX_H_

#include <zephyr/drivers/sensor.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief INA2XX extra sensor channels.
 */
enum sensor_channel_ina2xx {
	/** Accumulated energy, in Joules **/
	SENSOR_CHAN_INA2XX_ENERGY = SENSOR_CHAN_PRIV_START,

	/** Accumulated charge, in Coulombs **/
	SENSOR_CHAN_INA2XX_CHARGE,
};

enum sensor_attribute_ina2xx {
	/** ADC configuration **/
	SENSOR_ATTR_ADC_CONFIGURATION = SENSOR_ATTR_PRIV_START,
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_INA2XX_H_ */
