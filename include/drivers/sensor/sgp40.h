/*
 * Copyright (c) 2021, Leonard Pollak
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended public API for Sensirion's SGP40 gas sensor
 *
 * This exposes an API to the on-chip heater which is specific to
 * the application/environment and cannot be expressed within the
 * sensor driver abstraction.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_SGP40_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_SGP40_H_

#ifdef __cplusplus
extern "C" {
#endif

enum sensor_attribute_sgp40 {
	SENSOR_ATTR_SGP40_TEMPERATURE = SENSOR_ATTR_PRIV_START,
	SENSOR_ATTR_SGP40_HUMIDITY
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_SGP40_H_ */
