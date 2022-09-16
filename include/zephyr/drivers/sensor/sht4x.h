/*
 * Copyright (c) 2021, Leonard Pollak
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended public API for Sensirion's SHT4X T/RH sensors
 *
 * This exposes an API to the on-chip heater which is specific to
 * the application/environment and cannot be expressed within the
 * sensor driver abstraction.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_SHT4X_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_SHT4X_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>

/* Maximum temperature above which the heater should not be used */
#define SHT4X_HEATER_MAX_TEMP 65

enum sensor_attribute_sht4x {
	/* Heater Power: 0, 1, 2 -> high, med, low */
	SENSOR_ATTR_SHT4X_HEATER_POWER = SENSOR_ATTR_PRIV_START,
	/* Heater Duration: 0, 1 -> long, short */
	SENSOR_ATTR_SHT4X_HEATER_DURATION
};

/**
 * @brief Fetches data using the on-chip heater.
 *
 * Measurement is always done with "high" repeatability.
 *
 * @param dev Pointer to the sensor device
 *
 * @return 0 if successful, negative errno code if failure.
 */
int sht4x_fetch_with_heater(const struct device *dev);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_SHT4X_H_ */
