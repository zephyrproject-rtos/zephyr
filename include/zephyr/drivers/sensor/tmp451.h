/*
 * Copyright (c) 2026 Open Device Partnership and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended public API for TI's TMP451 temperature sensor
 * @ingroup tmp451_interface
 *
 * This exposes attributes for the TMP451 which can be used for
 * setting the conversion mode at runtime as well as additional convenience types.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_TMP451_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_TMP451_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief TI TMP451 temperature sensor
 * @defgroup tmp451_interface TMP451
 * @ingroup sensor_interface_ext
 * @{
 */

#include <zephyr/drivers/sensor.h>

/**
 * @brief Custom sensor attributes for TMP451
 */
enum sensor_attribute_tmp451 {
	/** Put the sensor into one-shot (shutdown + convert on demand) mode */
	SENSOR_ATTR_TMP451_ONE_SHOT_MODE = SENSOR_ATTR_PRIV_START,
	/** Put the sensor into continuous conversion mode (frequency set by user) */
	SENSOR_ATTR_TMP451_CONTINUOUS_MODE,
	/** Put the sensor into shutdown mode (no conversions) */
	SENSOR_ATTR_TMP451_SHUTDOWN_MODE,
};

/**
 * @brief Conversion rates in micro-Hz for use with SENSOR_ATTR_SAMPLING_FREQUENCY.
 *
 * These are the discrete conversion rates supported by TMP451.
 */
enum tmp451_conversion_rate {
	/** 0.0625 Hz (16 s period) */
	TMP451_RATE_0_0625_HZ = 62500,
	/** 0.125 Hz (8 s period) */
	TMP451_RATE_0_125_HZ = 125000,
	/** 0.25 Hz (4 s period) */
	TMP451_RATE_0_25_HZ = 250000,
	/** 0.5 Hz (2 s period) */
	TMP451_RATE_0_5_HZ = 500000,
	/** 1 Hz (1 s period) */
	TMP451_RATE_1_HZ = 1000000,
	/** 2 Hz (500 ms period) */
	TMP451_RATE_2_HZ = 2000000,
	/** 4 Hz (250 ms period) */
	TMP451_RATE_4_HZ = 4000000,
	/** 8 Hz (125 ms period) */
	TMP451_RATE_8_HZ = 8000000,
	/** 16 Hz (63 ms period) */
	TMP451_RATE_16_HZ = 16000000,
	/** 32 Hz (32 ms period) */
	TMP451_RATE_32_HZ = 32000000,
};

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_TMP451_H_ */
