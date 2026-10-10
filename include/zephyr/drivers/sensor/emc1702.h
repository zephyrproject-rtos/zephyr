/*
 * Copyright (c) 2026  Microchip Technology Inc. and its subsidiaries
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_EMC1702_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_EMC1702_H_

#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/util_macro.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Channel bits returned in val1 by the three limit status attributes below.
 * The registers share one layout (datasheet Tables 5.19, 5.20 and 5.21).
 */
#define SENSOR_EMC1702_LIMIT_STATUS_DIE     BIT(0)
#define SENSOR_EMC1702_LIMIT_STATUS_AMBIENT BIT(1)
#define SENSOR_EMC1702_LIMIT_STATUS_VOLTAGE BIT(6)
#define SENSOR_EMC1702_LIMIT_STATUS_CURRENT BIT(7)

enum sensor_channel_emc1702 {
	/* External diode fault channel */
	SENSOR_CHAN_EMC1702_FAULT = SENSOR_CHAN_PRIV_START,
	/* High/Low/Critical limit status channel */
	SENSOR_CHAN_EMC1702_LIMIT_STATUS,
	/*
	 * Hysteresis shared by the critical limits of both temperature channels.
	 * Use with SENSOR_ATTR_HYSTERESIS, in whole degrees Celsius (0..127).
	 */
	SENSOR_CHAN_EMC1702_TEMP_HYSTERESIS,
};

enum sensor_attribute_emc1702 {
	/* Read the High Limit Status register (channels at/above their high limit). */
	SENSOR_ATTR_EMC1702_HIGH_LIMIT_STATUS = SENSOR_ATTR_PRIV_START,
	/* Read the Low Limit Status register (channels at/below their low limit). */
	SENSOR_ATTR_EMC1702_LOW_LIMIT_STATUS,
	/* Read the Critical Limit Status register (channels at/above their Tcrit limit. */
	SENSOR_ATTR_EMC1702_CRIT_LIMIT_STATUS,
	/*
	 * Get/set the critical (THERM) limit of SENSOR_CHAN_DIE_TEMP,
	 * SENSOR_CHAN_AMBIENT_TEMP, SENSOR_CHAN_CURRENT or SENSOR_CHAN_VOLTAGE.
	 * The temperature limits hold whole degrees Celsius only.
	 */
	SENSOR_ATTR_EMC1702_CRITICAL_LIMIT,
};

enum sensor_trigger_emc1702 {
	/* External diode fault trigger */
	SENSOR_TRIG_EMC1702_FAULT = SENSOR_TRIG_PRIV_START,
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_EMC1702_H_ */
