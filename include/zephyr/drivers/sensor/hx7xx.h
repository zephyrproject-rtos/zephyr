/*
 * Copyright (c) 2020 George Gkinis
 * Copyright (c) 2021 Jan Gnip
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_HX711_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_HX711_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

enum hx7xx_gain {
	HX7xx_GAIN_8X = 8,
	HX7xx_GAIN_32X = 32,
	HX7xx_GAIN_64X = 64,
	HX7xx_GAIN_128X = 128,
};

enum hx7xx_rate {
	HX7xx_RATE_10HZ = 10,
	HX7xx_RATE_20HZ = 20,
	HX7xx_RATE_40HZ = 40,
	HX7xx_RATE_80HZ = 80,
	HX7xx_RATE_320HZ = 320,
};

/**
 * @brief Zero the HX711.
 *
 * @param dev Pointer to the hx711 device structure
 * @param readings Number of readings to get average offset.
 *        5~10 readings should be enough, although more are allowed.
 *
 * @retval 0 If successful.
 * @retval -EINVAL If bad arguments are passed.
 *
 */
int avia_hx7xx_tare(const struct device *dev, uint8_t readings);

/**
 * @brief Callibrate the HX711.
 *
 * Given a target value of a known weight the slope gets calculated.
 * This is actually unit agnostic.
 * If the target weight is given in grams, lb, Kg or any other weight unit,
 * the slope will be calculated accordingly.
 * This function should be invoked before any readings are taken.
 *
 * @param dev Pointer to the hx711 device structure
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

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_HX711_H_ */
