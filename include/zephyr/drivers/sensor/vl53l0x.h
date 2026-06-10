/*
 * Copyright (c) 2024 Michal Piekos
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Custom channels and values for VL53L0X ToF Sensor
 * @ingroup vl53l0x_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_VL53L0X_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_VL53L0X_H_

/**
 * @brief ST VL53L0X time-of-flight ranging sensor
 * @defgroup vl53l0x_interface VL53L0X
 * @ingroup sensor_interface_ext
 * @{
 */

#include <zephyr/drivers/sensor.h>

/**
 * @brief Custom sensor channels for the VL53L0X.
 *
 * These channels provide additional sensor data not covered by the standard
 * Zephyr sensor channels.
 *
 * Example usage:
 * @code{c}
 * #include <zephyr/drivers/sensor/vl53l0x.h>
 *
 * if (sensor_channel_get(dev, SENSOR_CHAN_VL53L0X_RANGE_STATUS, &value)) {
 *	printk("Status: %d\n", value.val1);
 * }
 * @endcode
 *
 * Each channel returns its value in a @c sensor_value structure. Unless noted otherwise,
 * @c sensor_value.val2 is unused (always 0).
 */
enum sensor_channel_vl53l0x {
	/**
	 * Maximum range the device can measure, in meters.
	 *
	 * - @c sensor_value.val1 is the integer part of the distance (meters).
	 * - @c sensor_value.val2 is the fractional part (in millionths of a meter).
	 */
	SENSOR_CHAN_VL53L0X_RANGE_DMAX = SENSOR_CHAN_PRIV_START,
	/**
	 * Return signal rate, in counts per second.
	 *
	 * - @c sensor_value.val1 is the signal rate (CPS).
	 */
	SENSOR_CHAN_VL53L0X_SIGNAL_RATE_RTN_CPS,
	/**
	 * Ambient signal rate, in counts per second.
	 *
	 * - @c sensor_value.val1 is the ambient rate (CPS).
	 */
	SENSOR_CHAN_VL53L0X_AMBIENT_RATE_RTN_CPS,
	/**
	 * Effective number of SPADs used for the return measurement.
	 *
	 * - @c sensor_value.val1 is the integer SPAD count.
	 */
	SENSOR_CHAN_VL53L0X_EFFECTIVE_SPAD_RTN_COUNT,
	/**
	 * Range status of the last measurement.
	 *
	 * - @c sensor_value.val1 is one of the @ref vl53l0x_range_status_values "range status
	 *   values".
	 */
	SENSOR_CHAN_VL53L0X_RANGE_STATUS,
};

/**
 * @name VL53L0X measurement status values
 * @anchor vl53l0x_range_status_values
 *
 * Reported in @c sensor_value.val1 by @ref SENSOR_CHAN_VL53L0X_RANGE_STATUS.
 * @{
 */
/** Range measurement is valid. */
#define VL53L0X_RANGE_STATUS_RANGE_VALID    (0)
/** Sigma (noise) check failed. */
#define VL53L0X_RANGE_STATUS_SIGMA_FAIL	    (1)
/** Signal level check failed. */
#define VL53L0X_RANGE_STATUS_SIGNAL_FAIL    (2)
/** Target is below the minimum measurable range. */
#define VL53L0X_RANGE_STATUS_MIN_RANGE_FAIL (3)
/** Phase check failed (target out of range). */
#define VL53L0X_RANGE_STATUS_PHASE_FAIL	    (4)
/** Hardware failure. */
#define VL53L0X_RANGE_STATUS_HARDWARE_FAIL  (5)
/** No new measurement available. */
#define VL53L0X_RANGE_STATUS_NO_UPDATE      (255)
/** @} */

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_VL53L0X_H_ */
