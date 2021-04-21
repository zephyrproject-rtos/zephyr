/*
 * Copyright (c) 2020 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended public API for the NXP MCUX Analog Comparator (ACMP)
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_MCUX_ACMP_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_MCUX_ACMP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <drivers/sensor.h>

enum sensor_channel_mcux_acmp {
	/** Analog Comparator Output. */
	SENSOR_CHAN_MCUX_ACMP_OUTPUT = SENSOR_CHAN_PRIV_START,
};

enum sensor_trigger_type_mcux_acmp {
	/** Analog Comparator Output rising event trigger. */
	SENSOR_TRIG_MCUX_ACMP_OUTPUT_RISING = SENSOR_TRIG_PRIV_START,
	/** Analog Comparator Output falling event trigger. */
	SENSOR_TRIG_MCUX_ACMP_OUTPUT_FALLING,
};

enum sensor_attribute_mcux_acmp {
	/** Analog Comparator hard block offset. */
	SENSOR_ATTR_MCUX_ACMP_OFFSET_LEVEL = SENSOR_ATTR_COMMON_COUNT,
	/** Analog Comparator hysteresis level. */
	SENSOR_ATTR_MCUX_ACMP_HYSTERESIS_LEVEL,
	/**
	 * Analog Comparator Digital-to-Analog Converter voltage
	 * reference source.
	 */
	SENSOR_ATTR_MCUX_ACMP_DAC_VOLTAGE_REFERENCE,
	/** Analog Comparator Digital-to-Analog Converter value. */
	SENSOR_ATTR_MCUX_ACMP_DAC_VALUE,
	/** Analog Comparator positive port input. */
	SENSOR_ATTR_MCUX_ACMP_POSITIVE_PORT_INPUT,
	/** Analog Comparator positive mux input. */
	SENSOR_ATTR_MCUX_ACMP_POSITIVE_MUX_INPUT,
	/** Analog Comparator negative port input. */
	SENSOR_ATTR_MCUX_ACMP_NEGATIVE_PORT_INPUT,
	/** Analog Comparator negative mux input. */
	SENSOR_ATTR_MCUX_ACMP_NEGATIVE_MUX_INPUT,
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_MCUX_ACMP_H_ */
