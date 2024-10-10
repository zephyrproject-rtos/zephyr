/*
 * Copyright (c) 2023 Kurtis Dinelle
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended public API for Memsic MMC56X3 magnetometer and temperature sensor
 *
 * This exposes attributes for the MMC56X3 which can be used for
 * setting the continuous mode and bandwidth selection bits.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_TSL2591_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_TSL2591_H_

#include <zephyr/drivers/sensor.h>

#ifdef __cplusplus
extern "C" {
#endif

enum sensor_attribute_mmc56x3 {
	/* Bandwidth selection bit 0.
	 *
	 * Adjust length of decimation filter. Controls duration of
	 * each measurement. Affects ODR; see datasheet for details.
	 */
	SENSOR_ATTR_BANDWIDTH_SELECTION_BITS_0 = SENSOR_ATTR_PRIV_START + 1,

	/* Bandwidth selection bit 1.
	 *
	 * Adjust length of decimation filter. Controls duration of
	 * each measurement. Affects ODR; see datasheet for details.
	 */
	SENSOR_ATTR_BANDWIDTH_SELECTION_BITS_1,

	/* Automatic self reset.
	 *
	 * Enable automatic self-reset function.
	 * Affects ODR; see datasheet for details.
	 */
	SENSOR_ATTR_AUTOMATIC_SELF_RESET,
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_TSL2591_H_ */
