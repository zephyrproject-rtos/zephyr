/*
 * Copyright (c) 2020 George Gkinis
 * Copyright (c) 2021 Jan Gnip
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of HX7xx sensor
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_HX7XX_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_HX7XX_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

/**
 * Enumeration fo the allowed gain for HX7xx
 */
enum hx7xx_gain {
	/** Gain 8x */
	HX7xx_GAIN_8X = 8,
	/** Gain 32x */
	HX7xx_GAIN_32X = 32,
	/** Gain 64x */
	HX7xx_GAIN_64X = 64,
	/** Gain 128x */
	HX7xx_GAIN_128X = 128,
};

/**
 * Enumeration of allowed sampling rates
 */
enum hx7xx_rate {
	/** Rate 10Hz */
	HX7xx_RATE_10HZ = 10,
	/** Rate 20Hz */
	HX7xx_RATE_20HZ = 20,
	/** Rate 40Hz */
	HX7xx_RATE_40HZ = 40,
	/** Rate 80Hz */
	HX7xx_RATE_80HZ = 80,
	/** Rate 320Hz */
	HX7xx_RATE_320HZ = 320,
};

/**
 * @brief Zero the HX7xx.
 *
 * @param dev Pointer to the HX7xx device structure
 * @param readings Number of readings to get average offset.
 *        5~10 readings should be enough, although more are allowed.
 *
 * @retval 0 If successful.
 * @retval -EINVAL If bad arguments are passed.
 *
 */
int avia_hx7xx_tare(const struct device *dev, uint8_t readings);

/**
 * @brief Calibrate the HX7xx.
 *
 * Given a target value of a known weight the slope gets calculated.
 * This is actually unit agnostic.
 * If the target weight is given in grams, lb, Kg or any other weight unit,
 * the slope will be calculated accordingly.
 * This function should be invoked before any readings are taken.
 *
 * @param dev Pointer to the HX7xx device structure
 * @param readings Number of readings to take for calibration.
 *        5~10 readings should be enough, although more are allowed.
 * @param calibration_weight Know weight in grams.
 *
 * @retval 0 If successful.
 * @retval -EINVAL If bad arguments are passed.
 */
int avia_hx7xx_calibrate(const struct device *dev, uint8_t readings, double calibration_weight);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_HX7XX_H_ */
