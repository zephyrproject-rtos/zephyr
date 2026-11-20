/*
 * Copyright (c) 2020 George Gkinis
 * Copyright (c) 2021 Jan Gnip
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of HX7xx sensor
 * @ingroup hx7xx_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_HX7XX_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_HX7XX_H_

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

/**
 * @defgroup hx7xx_interface HX7xx
 * @ingroup sensor_interface_ext_avia
 * @brief Avia Semiconductor HX710/HX711/HX712/HX717/HX720/HX71708 weight scale ADCs
 *
 * Driver for the HX7xx family that communicates with the chip by bit-banging
 * the DOUT and SCK lines over GPIO.
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Allowed gain values for HX7xx.
 */
enum hx7xx_gain {
	HX7xx_GAIN_8X = 8,     /**< Gain 8x */
	HX7xx_GAIN_32X = 32,   /**< Gain 32x */
	HX7xx_GAIN_64X = 64,   /**< Gain 64x */
	HX7xx_GAIN_128X = 128, /**< Gain 128x */
};

/**
 * @brief Allowed sampling rates for HX7xx.
 */
enum hx7xx_rate {
	HX7xx_RATE_10HZ = 10,   /**< Rate 10 Hz */
	HX7xx_RATE_20HZ = 20,   /**< Rate 20 Hz */
	HX7xx_RATE_40HZ = 40,   /**< Rate 40 Hz */
	HX7xx_RATE_80HZ = 80,   /**< Rate 80 Hz */
	HX7xx_RATE_320HZ = 320, /**< Rate 320 Hz */
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
 * @param calibration_weight Known weight in grams.
 *
 * @retval 0 If successful.
 * @retval -EINVAL If bad arguments are passed.
 */
int avia_hx7xx_calibrate(const struct device *dev, uint8_t readings, double calibration_weight);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_HX7XX_H_ */
