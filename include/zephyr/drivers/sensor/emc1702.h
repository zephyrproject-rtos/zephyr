/*
 * Copyright (c) 2026  Microchip Technology Inc. and its subsidiaries
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of EMC1702 sensor
 * @ingroup emc1702_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_EMC1702_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_EMC1702_H_

/**
 * @brief Microchip EMC1702 high-side current sense and temperature monitor
 * @defgroup emc1702_interface EMC1702
 * @ingroup sensor_interface_ext
 * @{
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/util_macro.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name Limit status channel bits
 *
 * Channel bits returned in val1 by the three limit status attributes. The
 * registers share one layout (datasheet Tables 5.19, 5.20 and 5.21).
 * @{
 */

/** Internal diode (die temperature) channel */
#define SENSOR_EMC1702_LIMIT_STATUS_DIE     BIT(0)
/** External diode (ambient temperature) channel */
#define SENSOR_EMC1702_LIMIT_STATUS_AMBIENT BIT(1)
/** VSOURCE (bus voltage) channel */
#define SENSOR_EMC1702_LIMIT_STATUS_VOLTAGE BIT(6)
/** VSENSE (current) channel */
#define SENSOR_EMC1702_LIMIT_STATUS_CURRENT BIT(7)

/** @} */

/**
 * @brief Custom sensor channels for EMC1702
 */
enum sensor_channel_emc1702 {
	/** External diode fault channel */
	SENSOR_CHAN_EMC1702_FAULT = SENSOR_CHAN_PRIV_START,
	/** High/Low/Critical limit status channel */
	SENSOR_CHAN_EMC1702_LIMIT_STATUS,
	/**
	 * Hysteresis shared by the critical limits of both temperature channels.
	 * Use with SENSOR_ATTR_HYSTERESIS, in whole degrees Celsius (0..127).
	 */
	SENSOR_CHAN_EMC1702_TEMP_HYSTERESIS,
};

/**
 * @brief Custom sensor attributes for EMC1702
 */
enum sensor_attribute_emc1702 {
	/** Read the High Limit Status register (channels at/above their high limit). */
	SENSOR_ATTR_EMC1702_HIGH_LIMIT_STATUS = SENSOR_ATTR_PRIV_START,
	/** Read the Low Limit Status register (channels at/below their low limit). */
	SENSOR_ATTR_EMC1702_LOW_LIMIT_STATUS,
	/** Read the Critical Limit Status register (channels at/above their Tcrit limit). */
	SENSOR_ATTR_EMC1702_CRIT_LIMIT_STATUS,
	/**
	 * Get/set the critical (THERM) limit of SENSOR_CHAN_DIE_TEMP,
	 * SENSOR_CHAN_AMBIENT_TEMP, SENSOR_CHAN_CURRENT or SENSOR_CHAN_VOLTAGE.
	 * The temperature limits hold whole degrees Celsius only.
	 */
	SENSOR_ATTR_EMC1702_CRITICAL_LIMIT,
};

/**
 * @brief Custom sensor triggers for EMC1702
 */
enum sensor_trigger_emc1702 {
	/** External diode fault trigger */
	SENSOR_TRIG_EMC1702_FAULT = SENSOR_TRIG_PRIV_START,
};

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_EMC1702_H_ */
