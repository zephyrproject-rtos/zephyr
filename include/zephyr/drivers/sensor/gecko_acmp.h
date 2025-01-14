/*
 * Copyright 2025 Kickmaker
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended public API for the SiLabs Gecko analog comparator (ACMP)
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_GECKO_ACMP_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_GECKO_ACMP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>

/**
 * @brief acmp channels.
 */
enum sensor_channel_gecko_acmp {
	/** ACMP output. */
	SENSOR_CHAN_GECKO_ACMP_OUTPUT = SENSOR_CHAN_PRIV_START,
#if CONFIG_GECKO_ACMP_EDGE_COUNTER
	/** ACMP rising edge counter. */
	SENSOR_CHAN_GECKO_ACMP_RISING_EDGE_COUNTER,
	/** ACMP falling edge counter. */
	SENSOR_CHAN_GECKO_ACMP_FALLING_EDGE_COUNTER,
#endif /* CONFIG_GECKO_ACMP_EDGE_COUNTER */
};

#if CONFIG_GECKO_ACMP_TRIGGER
/**
 * @brief acmp trigger types.
 */
enum sensor_trigger_type_gecko_acmp {
	/** ACMP output rising event trigger. */
	SENSOR_TRIG_GECKO_ACMP_OUTPUT_RISING = SENSOR_TRIG_PRIV_START,
	/** ACMP output falling event trigger. */
	SENSOR_TRIG_GECKO_ACMP_OUTPUT_FALLING,
};
#endif /* CONFIG_GECKO_ACMP_TRIGGER */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_GECKO_ACMP_H_ */
