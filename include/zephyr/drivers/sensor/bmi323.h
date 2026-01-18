/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_BMI323_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_BMI323_H_

#include <zephyr/drivers/sensor.h>

enum sensor_attribute_bmi323 {
	/**
	 * Configure the -3dB cut-off frequency.
	 * sensor_value.val1 = 0 selects ODR/2.
	 * sensor_value.val1 = 1 selects ODR/4.
	 */
	SENSOR_ATTR_BANDWIDTH = SENSOR_ATTR_PRIV_START,
	/**
	 * Configure number of samples to be averaged.
	 * sensor_value.val1 is the number of samples to be averaged.
	 */
	SENSOR_ATTR_AVERAGE_NUM
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_BMI323_H_ */
