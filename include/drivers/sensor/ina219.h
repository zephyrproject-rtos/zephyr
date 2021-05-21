/*
 * Copyright (c) 2021 Leonard Pollak
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended public API for the Texas Instruments INA219
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_INA219_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_INA219_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <drivers/sensor.h>

enum sensor_channel_ina219 {
	/** Shunt Voltage in V **/
	SENSOR_CHAN_INA219_V_SHUNT = SENSOR_CHAN_PRIV_START,
	/** Bus Voltage in V **/
	SENSOR_CHAN_INA219_BUS_VOLTAGE,
	/** Power **/
	SENSOR_CHAN_INA219_BUS_POWER,
	/** Current **/
	SENSOR_CHAN_INA219_BUS_CURRENT,
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_INA219_ */
