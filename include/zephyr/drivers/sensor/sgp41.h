/*
 * Copyright (c) 2026 Dotcom IoT LLP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API of the Sensirion SGP41 VOC/NOx sensor driver.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_SGP41_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_SGP41_H_

#include <zephyr/drivers/sensor.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SGP41 specific sensor channels.
 *
 * - SENSOR_CHAN_GAS_RES       : raw VOC signal SRAW_VOC (unitless ticks)
 * - SGP41_SENSOR_CHAN_NOX_RAW : raw NOx signal SRAW_NOX (unitless ticks)
 *
 * Raw signals are proportional to the logarithm of the sensing element
 * resistance. Use the Sensirion Gas Index Algorithm to convert them into
 * VOC / NOx index values.
 */
enum sensor_channel_sgp41 {
	/** Raw NOx signal SRAW_NOX, in ticks. Not valid during conditioning. */
	SGP41_SENSOR_CHAN_NOX_RAW = SENSOR_CHAN_PRIV_START,
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_SGP41_H_ */
