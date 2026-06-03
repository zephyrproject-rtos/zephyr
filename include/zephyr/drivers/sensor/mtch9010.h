/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended public API for the Microchip MTCH9010 capacitive sensor.
 * @ingroup mtch9010_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_MTCH9010_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_MTCH9010_H_

/**
 * @brief Microchip MTCH9010 capacitive proximity/level sensor
 * @defgroup mtch9010_interface MTCH9010
 * @ingroup sensor_interface_ext
 * @{
 */

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

/** @cond INTERNAL_HIDDEN */
#define MTCH9010_MAX_RESULT 65535
#define MTCH9010_MIN_RESULT 0
/** @endcond */

/** @brief Custom sensor channels for the MTCH9010. */
enum sensor_channel_mtch9010 {
	/** Current state of the OUT line. */
	SENSOR_CHAN_MTCH9010_OUT_STATE = SENSOR_CHAN_PRIV_START,
	/** Whether the OUT line would be asserted based on the previous result. */
	SENSOR_CHAN_MTCH9010_SW_OUT_STATE,
	/** Reference value set for the sensor. */
	SENSOR_CHAN_MTCH9010_REFERENCE_VALUE,
	/** Threshold value set for the sensor. */
	SENSOR_CHAN_MTCH9010_THRESHOLD_VALUE,
	/** Last measured result. */
	SENSOR_CHAN_MTCH9010_MEAS_RESULT,
	/** Delta between the last measurement and the reference. */
	SENSOR_CHAN_MTCH9010_MEAS_DELTA,
	/** True if the heartbeat is in an error state. */
	SENSOR_CHAN_MTCH9010_HEARTBEAT_ERROR_STATE
};

/**
 * @}
 */

#endif
