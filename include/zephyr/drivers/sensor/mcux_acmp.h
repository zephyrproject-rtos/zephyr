/*
 * Copyright (c) 2020 Vestas Wind Systems A/S
 * Copyright 2022, 2024 NXP
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

#include <zephyr/drivers/sensor.h>

#if defined(FSL_FEATURE_ACMP_HAS_C1_INPSEL_BIT) && (FSL_FEATURE_ACMP_HAS_C1_INPSEL_BIT == 1U)
#define MCUX_ACMP_HAS_INPSEL 1
#else
#define MCUX_ACMP_HAS_INPSEL 0
#endif

#if defined(FSL_FEATURE_ACMP_HAS_C1_INNSEL_BIT) && (FSL_FEATURE_ACMP_HAS_C1_INNSEL_BIT == 1U)
#define MCUX_ACMP_HAS_INNSEL 1
#else
#define MCUX_ACMP_HAS_INNSEL 0
#endif

#if defined(FSL_FEATURE_ACMP_HAS_C0_OFFSET_BIT) && (FSL_FEATURE_ACMP_HAS_C0_OFFSET_BIT == 1U)
#define MCUX_ACMP_HAS_OFFSET 1
#else
#define MCUX_ACMP_HAS_OFFSET 0
#endif

#if defined(FSL_FEATURE_ACMP_HAS_C3_REG) && (FSL_FEATURE_ACMP_HAS_C3_REG != 0U)
#define MCUX_ACMP_HAS_DISCRETE_MODE 1
#else
#define MCUX_ACMP_HAS_DISCRETE_MODE 0
#endif

#if defined(FSL_FEATURE_ACMP_HAS_C0_HYSTCTR_BIT) && (FSL_FEATURE_ACMP_HAS_C0_HYSTCTR_BIT == 1U)
#define MCUX_ACMP_HAS_HYSTCTR 1
#else
#define MCUX_ACMP_HAS_HYSTCTR 0
#endif

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
#if MCUX_ACMP_HAS_DISCRETE_MODE
	/** Analog Comparator Positive Channel Discrete Mode Enable. */
	SENSOR_ATTR_MCUX_ACMP_POSITIVE_DISCRETE_MODE,
	/** Analog Comparator Negative Channel Discrete Mode Enable. */
	SENSOR_ATTR_MCUX_ACMP_NEGATIVE_DISCRETE_MODE,
	/** Analog Comparator discrete mode clock selection. */
	SENSOR_ATTR_MCUX_ACMP_DISCRETE_CLOCK,
	/** Analog Comparator resistor divider enable. */
	SENSOR_ATTR_MCUX_ACMP_DISCRETE_ENABLE_RESISTOR_DIVIDER,
	/** Analog Comparator discrete sample selection. */
	SENSOR_ATTR_MCUX_ACMP_DISCRETE_SAMPLE_TIME,
	/** Analog Comparator discrete phase1 sampling time selection. */
	SENSOR_ATTR_MCUX_ACMP_DISCRETE_PHASE1_TIME,
	/** Analog Comparator discrete phase2 sampling time selection. */
	SENSOR_ATTR_MCUX_ACMP_DISCRETE_PHASE2_TIME,
#endif
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_MCUX_ACMP_H_ */
