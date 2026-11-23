/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of MTCH9010 sensor
 * @ingroup mtch9010_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_MTCH9010_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_MTCH9010_H_

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

/**
 * @addtogroup mtch9010_interface
 * @{
 */

/** Maximum measurement result */
#define MTCH9010_MAX_RESULT 65535
/** Minimum measurement result */
#define MTCH9010_MIN_RESULT 0

/** @brief Custom sensor channels for MTCH9010 */
enum sensor_channel_mtch9010 {
	/** State of the OUT line */
	SENSOR_CHAN_MTCH9010_OUT_STATE = SENSOR_CHAN_PRIV_START,
	/** Whether the OUT line would be asserted for the last result */
	SENSOR_CHAN_MTCH9010_SW_OUT_STATE,
	/** Reference value set for the sensor */
	SENSOR_CHAN_MTCH9010_REFERENCE_VALUE,
	/** Threshold value set for the sensor */
	SENSOR_CHAN_MTCH9010_THRESHOLD_VALUE,
	/** Last measured result */
	SENSOR_CHAN_MTCH9010_MEAS_RESULT,
	/** Last measured delta from the reference value */
	SENSOR_CHAN_MTCH9010_MEAS_DELTA,
	/** Whether the heartbeat signals an error state */
	SENSOR_CHAN_MTCH9010_HEARTBEAT_ERROR_STATE
};

/** @} */

#endif
