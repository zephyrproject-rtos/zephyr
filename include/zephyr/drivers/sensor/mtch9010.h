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

struct mtch9010_result_t {
	/* Received Reference Value */
	uint16_t measurement;
	/* Last Measurement Value */
	uint16_t prev_measurement;
	/* Received Delta Value */
	int16_t delta;
};

enum mtch9010_operating_mode {
	/* The MTCH9010 is in Capacitive Mode */
	MTCH9010_CAPACITIVE = 0,
	/* The MTCH9010 is in Conductive Mode */
	MTCH9010_CONDUCTIVE
};

enum mtch9010_output_format {
	/* MTCH9010 sends the delta value */
	MTCH9010_OUTPUT_FORMAT_DELTA = 0,
	/* MTCH9010 sends the current value */
	MTCH9010_OUTPUT_FORMAT_CURRENT,
	/* MTCH9010 sends both the delta and the current value */
	MTCH9010_OUTPUT_FORMAT_BOTH,
	/* MTCH9010 sends MPLAB Data Visualizer Packets */
	MTCH9010_OUTPUT_FORMAT_MPLAB_DATA_VISUALIZER
};

enum mtch9010_reference_value_init {
	/* MTCH9010 sets the current value as the reference value */
	MTCH9010_REFERENCE_CURRENT_VALUE = 0,
	/* MTCH9010 re-runs the measurement */
	MTCH9010_REFERENCE_RERUN_VALUE,
	/* MTCH9010 sets the reference to the value the user defines */
	MTCH9010_REFERENCE_CUSTOM_VALUE
};

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
