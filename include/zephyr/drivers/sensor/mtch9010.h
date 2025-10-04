/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_MTCH9010_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_MTCH9010_H_

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#define MTCH9010_MAX_RESULT 65535
#define MTCH9010_MIN_RESULT 0

enum sensor_channel_mtch9010 {
	/* Polls the state of the OUT line */
	SENSOR_CHAN_MTCH9010_OUT_STATE = SENSOR_CHAN_PRIV_START,
	/* Calculates if the OUT line would be asserted based on previous result */
	SENSOR_CHAN_MTCH9010_SW_OUT_STATE,
	/* Returns the reference value set for the sensor */
	SENSOR_CHAN_MTCH9010_REFERENCE_VALUE,
	/* Returns the threshold value set for the sensor */
	SENSOR_CHAN_MTCH9010_THRESHOLD_VALUE,
	/* Returns the last measured result */
	SENSOR_CHAN_MTCH9010_MEAS_RESULT,
	/* Returns the last measured result */
	SENSOR_CHAN_MTCH9010_MEAS_DELTA,
	/* Returns true if the heartbeat is an error state */
	SENSOR_CHAN_MTCH9010_HEARTBEAT_ERROR_STATE
};

#endif
