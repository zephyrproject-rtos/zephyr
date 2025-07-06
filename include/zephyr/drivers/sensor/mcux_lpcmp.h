/*
 * Copyright (c) 2020 Vestas Wind Systems A/S
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Data structure for the NXP MCUX low-power analog comparator (LPCMP)
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_MCUX_LPCMP_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_MCUX_LPCMP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>

/**
 * @brief lpcmp channels.
 */
enum sensor_channel_mcux_lpcmp {
	/** LPCMP output. */
	SENSOR_CHAN_MCUX_LPCMP_OUTPUT = SENSOR_CHAN_PRIV_START,
};

/**
 * @brief lpcmp trigger types.
 */
enum sensor_trigger_type_mcux_lpcmp {
	/** LPCMP output rising event trigger. */
	SENSOR_TRIG_MCUX_LPCMP_OUTPUT_RISING = SENSOR_TRIG_PRIV_START,
	/** LPCMP output falling event trigger. */
	SENSOR_TRIG_MCUX_LPCMP_OUTPUT_FALLING,
};

/**
 * @brief lpcmp attribute types.
 */
enum sensor_attribute_mcux_lpcmp {
	/** LPCMP positive input mux. */
	SENSOR_ATTR_MCUX_LPCMP_POSITIVE_MUX_INPUT = SENSOR_ATTR_COMMON_COUNT,
	/** LPCMP negative input mux. */
	SENSOR_ATTR_MCUX_LPCMP_NEGATIVE_MUX_INPUT,

	/**
	 * LPCMP internal DAC enable.
	 *  0b: disable
	 *  1b: enable
	 */
	SENSOR_ATTR_MCUX_LPCMP_DAC_ENABLE,
	/**
	 * LPCMP internal DAC high power mode disabled.
	 *  0b: disable
	 *  1b: enable
	 */
	SENSOR_ATTR_MCUX_LPCMP_DAC_HIGH_POWER_MODE_ENABLE,
	/** LPCMP internal DAC voltage reference source. */
	SENSOR_ATTR_MCUX_LPCMP_DAC_REFERENCE_VOLTAGE_SOURCE,
	/** LPCMP internal DAC output voltage value. */
	SENSOR_ATTR_MCUX_LPCMP_DAC_OUTPUT_VOLTAGE,

	/** LPCMP internal filter sample enable. */
	SENSOR_ATTR_MCUX_LPCMP_SAMPLE_ENABLE,
	/** LPCMP internal filter sample count. */
	SENSOR_ATTR_MCUX_LPCMP_FILTER_COUNT,
	/** LPCMP internal filter sample period. */
	SENSOR_ATTR_MCUX_LPCMP_FILTER_PERIOD,

	/** LPCMP window signal invert. */
	SENSOR_ATTR_MCUX_LPCMP_COUTA_WINDOW_ENABLE,
	/** LPCMP window signal invert. */
	SENSOR_ATTR_MCUX_LPCMP_COUTA_WINDOW_SIGNAL_INVERT_ENABLE,
	/**
	 * LPCMP COUTA signal value when a window is closed:
	 *  00b: latched
	 *  01b: set to low
	 *  11b: set to high
	 */
	SENSOR_ATTR_MCUX_LPCMP_COUTA_SIGNAL,
	/**
	 * LPCMP COUT event to close an active window:
	 *  xx0b: COUT event cannot close an active window
	 *  001b: COUT rising edge event close an active window
	 *  011b: COUT falling edge event close an active window
	 *  1x1b: COUT both edges event close an active window
	 */
	SENSOR_ATTR_MCUX_LPCMP_COUT_EVENT_TO_CLOSE_WINDOW
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_MCUX_LPCMP_H_ */
