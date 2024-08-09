/*
 * Copyright (c) 2024 Chaim Zax
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ADXL34X_CONVERT_H_
#define ZEPHYR_DRIVERS_SENSOR_ADXL34X_CONVERT_H_

#include <stdint.h>

#include <zephyr/drivers/sensor/adxl34x.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Conversion from raw register values to g (see table 1:
 *        "Specifications" from the datasheet).
 *
 * @var adxl34x_range_conv
 */
static const uint16_t adxl34x_range_conv[] = {
	[ADXL34X_RANGE_2G] = 39,
	[ADXL34X_RANGE_4G] = 78,
	[ADXL34X_RANGE_8G] = 156,
	[ADXL34X_RANGE_16G] = 312,
};

/**
 * @brief Conversion from register values to maximum g values
 *
 * @var adxl34x_max_g_conv
 */
static const uint8_t adxl34x_max_g_conv[] = {
	[ADXL34X_RANGE_2G] = 2,
	[ADXL34X_RANGE_4G] = 4,
	[ADXL34X_RANGE_8G] = 8,
	[ADXL34X_RANGE_16G] = 16,
};

/**
 * @brief The shift values can be calculated by taking the maximum value of a
 *        specific range, convert it to m/s^2, round it up to the nearest power
 *        of two, the power is the shift value.
 *
 * @var adxl34x_shift_conv
 */
static const uint8_t adxl34x_shift_conv[] = {
	[ADXL34X_RANGE_2G] = 5,  /**<  2g =>  19.6m/s^2 =>  32 => 2^5 */
	[ADXL34X_RANGE_4G] = 6,  /**<  4g =>  39.2m/s^2 =>  64 => 2^6 */
	[ADXL34X_RANGE_8G] = 7,  /**<  8g =>  78.4m/s^2 => 128 => 2^7 */
	[ADXL34X_RANGE_16G] = 8, /**< 16g => 156.9m/s^2 => 256 => 2^8 */
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_ADXL34X_CONVERT_H_ */
