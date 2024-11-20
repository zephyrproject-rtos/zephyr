/*
 * Copyright (c) 2023 Kurtis Dinelle
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended public API for AMS's TSL2591 ambient light sensor
 *
 * This exposes attributes for the TSL2591 which can be used for
 * setting the on-chip gain, integration time, and persist filter parameters.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_TSL2591_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_TSL2591_H_

#include <zephyr/drivers/sensor.h>

#ifdef __cplusplus
extern "C" {
#endif

enum sensor_gain_tsl2591 {
	TSL2591_SENSOR_GAIN_LOW,
	TSL2591_SENSOR_GAIN_MED,
	TSL2591_SENSOR_GAIN_HIGH,
	TSL2591_SENSOR_GAIN_MAX
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_TSL2591_H_ */
