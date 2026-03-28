/*
 * Copyright (c) 2023 Kurtis Dinelle
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of MMC56X3 sensor
 * @ingroup mmc56x3_interface
 *
 * This exposes attributes for the MMC56X3 which can be used for
 * setting the continuous mode and bandwidth selection bits.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_MMC56X3_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_MMC56X3_H_

/**
 * @defgroup mmc56x3_interface MMC56X3
 * @ingroup sensor_interface_ext
 * @brief Memsic MMC56X3 3-axis magnetic and temperature sensor
 * @{
 */

#include <zephyr/drivers/sensor.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Custom sensor attributes for MMC56X3
 */
enum sensor_attribute_mmc56x3 {
	/**
	 * Bandwidth selection bit 0.
	 *
	 * Adjust length of decimation filter. Controls duration of
	 * each measurement. Affects ODR; see datasheet for details.
	 */
	SENSOR_ATTR_BANDWIDTH_SELECTION_BITS_0 = SENSOR_ATTR_PRIV_START + 1,

	/**
	 * Bandwidth selection bit 1.
	 *
	 * Adjust length of decimation filter. Controls duration of
	 * each measurement. Affects ODR; see datasheet for details.
	 */
	SENSOR_ATTR_BANDWIDTH_SELECTION_BITS_1,

	/**
	 * Automatic self reset.
	 *
	 * Enable automatic self-reset function.
	 * Affects ODR; see datasheet for details.
	 */
	SENSOR_ATTR_AUTOMATIC_SELF_RESET,
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_MMC56X3_H_ */
