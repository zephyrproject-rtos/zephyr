/*
 * Copyright (c) 2024 Michal Piekos
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Custom channels, attributes, modes and profiles for VL53L0X ToF Sensor
 *
 * These channels and attributes provide additional sensor data and runtime
 * configuration not covered by the standard Zephyr sensor API.
 * Application must include vl53l0x.h file to gain access to these features.
 *
 * Example usage:
 * @code{c}
 * #include <zephyr/drivers/sensor/vl53l0x.h>
 *
 * if (sensor_channel_get(dev, SENSOR_CHAN_VL53L0X_RANGE_STATUS, &value)) {
 *	printk("Status: %d\n", value.val1);
 * }
 *
 * // Switch to continuous mode
 * struct sensor_value mode = { .val1 = VL53L0X_MODE_CONTINUOUS };
 * sensor_attr_set(dev, SENSOR_CHAN_DISTANCE,
 *                 SENSOR_ATTR_VL53L0X_MODE, &mode);
 *
 * // Apply high-accuracy profile
 * struct sensor_value profile = { .val1 = VL53L0X_PROFILE_HIGH_ACCURACY };
 * sensor_attr_set(dev, SENSOR_CHAN_DISTANCE,
 *                 SENSOR_ATTR_VL53L0X_PROFILE, &profile);
 * @endcode
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_VL53L0X_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_VL53L0X_H_

#include <zephyr/drivers/sensor.h>

/* VL53L0x specific channels */
enum sensor_channel_vl53l0x {
	SENSOR_CHAN_VL53L0X_RANGE_DMAX = SENSOR_CHAN_PRIV_START,
	SENSOR_CHAN_VL53L0X_SIGNAL_RATE_RTN_CPS,
	SENSOR_CHAN_VL53L0X_AMBIENT_RATE_RTN_CPS,
	SENSOR_CHAN_VL53L0X_EFFECTIVE_SPAD_RTN_COUNT,
	SENSOR_CHAN_VL53L0X_RANGE_STATUS,
};

/* VL53L0x meas status values */
#define VL53L0X_RANGE_STATUS_RANGE_VALID    (0)
#define VL53L0X_RANGE_STATUS_SIGMA_FAIL	    (1)
#define VL53L0X_RANGE_STATUS_SIGNAL_FAIL    (2)
#define VL53L0X_RANGE_STATUS_MIN_RANGE_FAIL (3)
#define VL53L0X_RANGE_STATUS_PHASE_FAIL	    (4)
#define VL53L0X_RANGE_STATUS_HARDWARE_FAIL  (5)
#define VL53L0X_RANGE_STATUS_NO_UPDATE      (255)

/* VL53L0x specific sensor attributes for use with sensor_attr_set/get */
enum sensor_attribute_vl53l0x {
	/** Device measurement mode (val1 = enum vl53l0x_mode).
	 *  Setting to continuous starts measurement automatically.
	 */
	SENSOR_ATTR_VL53L0X_MODE = SENSOR_ATTR_PRIV_START,

	/** Timing budget in microseconds (val1 = us, min ~20000).
	 *  Lower = faster but less accurate. Default 33000.
	 */
	SENSOR_ATTR_VL53L0X_TIMING_BUDGET,

	/** Inter-measurement period in ms (val1 = ms).
	 *  Only used in VL53L0X_MODE_CONTINUOUS_TIMED mode.
	 */
	SENSOR_ATTR_VL53L0X_INTER_MEASUREMENT_PERIOD,

	/** VCSEL pulse period for pre-range phase (val1 = period). */
	SENSOR_ATTR_VL53L0X_VCSEL_PRE_RANGE,

	/** VCSEL pulse period for final-range phase (val1 = period). */
	SENSOR_ATTR_VL53L0X_VCSEL_FINAL_RANGE,

	/** Signal rate final-range limit in FixPoint16.16 MCPS (val1 = value).
	 *  Example: 0.25 MCPS = (FixPoint1616_t)(0.25 * 65536) = 16384
	 */
	SENSOR_ATTR_VL53L0X_SIGNAL_RATE_LIMIT,

	/** Sigma final-range limit in FixPoint16.16 mm (val1 = value).
	 *  Example: 32 mm = (FixPoint1616_t)(32 * 65536) = 2097152
	 */
	SENSOR_ATTR_VL53L0X_SIGMA_LIMIT,

	/** Apply a preset measurement profile (val1 = enum vl53l0x_profile).
	 *  Sets timing budget, signal/sigma limits, and VCSEL periods.
	 */
	SENSOR_ATTR_VL53L0X_PROFILE,

	/** Offset calibration in micrometers (val1 = um, signed). */
	SENSOR_ATTR_VL53L0X_OFFSET_CAL,

	/** Crosstalk compensation rate in FixPoint16.16 MCPS (val1 = value).
	 *  Setting a non-zero value enables crosstalk compensation.
	 */
	SENSOR_ATTR_VL53L0X_XTALK_RATE,
};

/** VL53L0X measurement modes for use with SENSOR_ATTR_VL53L0X_MODE */
enum vl53l0x_mode {
	/** Single-shot: one measurement per sample_fetch (blocking, ~33 ms). */
	VL53L0X_MODE_SINGLE          = 0,

	/** Continuous back-to-back: non-stop measurements.
	 *  sample_fetch returns -EAGAIN if no new data ready.
	 */
	VL53L0X_MODE_CONTINUOUS      = 1,

	/** Continuous timed: measurements at inter_measurement_period interval.
	 *  sample_fetch returns -EAGAIN if no new data ready.
	 */
	VL53L0X_MODE_CONTINUOUS_TIMED = 3,
};

/** VL53L0X measurement profiles for use with SENSOR_ATTR_VL53L0X_PROFILE */
enum vl53l0x_profile {
	/** Default: 33 ms budget, signal 0.1 MCPS, sigma 60 mm.
	 *  Good balance of speed and accuracy. Range ~2 m.
	 */
	VL53L0X_PROFILE_DEFAULT       = 0,

	/** High speed: 20 ms budget, signal 0.25 MCPS, sigma 32 mm.
	 *  Fastest updates, good for short range (<1.2 m).
	 */
	VL53L0X_PROFILE_HIGH_SPEED    = 1,

	/** Long range: 33 ms budget, signal 0.1 MCPS, sigma 60 mm.
	 *  Maximizes detection distance (~2 m).
	 */
	VL53L0X_PROFILE_LONG_RANGE    = 2,

	/** High accuracy: 200 ms budget, signal 0.25 MCPS, sigma 32 mm.
	 *  Best accuracy, very slow updates.
	 */
	VL53L0X_PROFILE_HIGH_ACCURACY = 3,
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_VL53L0X_H_ */
