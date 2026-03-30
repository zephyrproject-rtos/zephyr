/*
 * Copyright (c) 2025 Analog Devices Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of MAX30210 sensor
 * @ingroup max30210_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_MAX30210_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_MAX30210_H_

/**
 * @brief Analog Devices MAX30210 High-Accuracy Temperature Sensor
 * @defgroup max30210_interface MAX30210
 * @ingroup sensor_interface_ext
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>

/**
 * @brief Custom sensor attributes for MAX30210.
 */
enum sensor_attribute_max30210 {
	/**
	 * Interrupt polarity (reserved for future use).
	 */
	SENSOR_ATTR_MAX30210_INTERRUPT_POLARITY = SENSOR_ATTR_PRIV_START,
	/**
	 * Enable continuous conversion mode.
	 *
	 * - @c sensor_value.val1 and @c sensor_value.val2 are ignored.
	 */
	SENSOR_ATTR_MAX30210_CONTINUOUS_CONVERSION_MODE,
	/**
	 * Temperature sample-to-sample increment threshold (°C) for fast-change detection.
	 *
	 * - @c sensor_value.val1 is the integer part of the threshold (°C).
	 * - @c sensor_value.val2 is the fractional part (in millionths of a degree).
	 */
	SENSOR_ATTR_MAX30210_TEMP_INC_FAST_THRESH,
	/**
	 * Temperature sample-to-sample decrement threshold (°C) for fast-change detection.
	 *
	 * - @c sensor_value.val1 is the integer part of the threshold (°C).
	 * - @c sensor_value.val2 is the fractional part (in millionths of a degree).
	 *   Must be a multiple of 0.005 °C.
	 */
	SENSOR_ATTR_MAX30210_TEMP_DEC_FAST_THRESH,
	/**
	 * Rate-of-change filter coefficient (0-7).
	 *
	 * - @c sensor_value.val1 is the filter value.
	 * - @c sensor_value.val2 is unused (always 0).
	 */
	SENSOR_ATTR_MAX30210_RATE_CHG_FILTER,
	/**
	 * High-temperature alarm trip count (1-4).
	 *
	 * - @c sensor_value.val1 is the number of consecutive samples above threshold.
	 * - @c sensor_value.val2 is unused (always 0).
	 */
	SENSOR_ATTR_MAX30210_HI_TRIP_COUNT,
	/**
	 * Low-temperature alarm trip count (1-4).
	 *
	 * - @c sensor_value.val1 is the number of consecutive samples below threshold.
	 * - @c sensor_value.val2 is unused (always 0).
	 */
	SENSOR_ATTR_MAX30210_LO_TRIP_COUNT,
	/**
	 * Reset high-temperature trip counter.
	 *
	 * - @c sensor_value.val1 is 0 to leave counter unchanged, 1 to reset.
	 * - @c sensor_value.val2 is unused (always 0).
	 */
	SENSOR_ATTR_MAX30210_HI_TRIP_COUNT_RESET,
	/**
	 * Reset low-temperature trip counter.
	 *
	 * - @c sensor_value.val1 is 0 to leave counter unchanged, 1 to reset.
	 * - @c sensor_value.val2 is unused (always 0).
	 */
	SENSOR_ATTR_MAX30210_LO_TRIP_COUNT_RESET,
	/**
	 * High-temperature alarm non-consecutive mode.
	 *
	 * - @c sensor_value.val1 is 0 for consecutive, 1 for non-consecutive.
	 * - @c sensor_value.val2 is unused (always 0).
	 */
	SENSOR_ATTR_MAX30210_HI_NON_CONSECUTIVE_MODE,
	/**
	 * Low-temperature alarm non-consecutive mode.
	 *
	 * - @c sensor_value.val1 is 0 for consecutive, 1 for non-consecutive.
	 * - @c sensor_value.val2 is unused (always 0).
	 */
	SENSOR_ATTR_MAX30210_LO_NON_CONSECUTIVE_MODE,
	/**
	 * Alert mode (comparator vs interrupt).
	 *
	 * - @c sensor_value.val1 is 0 for comparator mode, 1 for interrupt mode.
	 * - @c sensor_value.val2 is unused (always 0).
	 */
	SENSOR_ATTR_MAX30210_ALERT_MODE,
	/**
	 * Perform software reset.
	 *
	 * - @c sensor_value.val1 and @c sensor_value.val2 are ignored.
	 */
	SENSOR_ATTR_MAX30210_SOFTWARE_RESET,
	/**
	 * Trigger single temperature conversion (reserved for future use).
	 */
	SENSOR_ATTR_MAX30210_TEMP_CONVERT,
	/**
	 * Enable auto mode (reserved for future use).
	 */
	SENSOR_ATTR_MAX30210_AUTO_MODE
};

/**
 * @brief Values for @ref SENSOR_ATTR_SAMPLING_FREQUENCY (Hz)
 *
 * When setting the sampling frequency, @c sensor_value.val1 is the integer part (Hz)
 * and @c sensor_value.val2 is the fractional part in millionths. For fractional rates
 * (e.g. 0.0625 Hz), use val1=0 and val2=62500.
 * @{
 */
enum sensor_sampling_rate_max30210 {
	SENSOR_SAMPLING_RATE_MAX30210_0_015625 = 0, /**< 0.015625 Hz */
	SENSOR_SAMPLING_RATE_MAX30210_0_03125,      /**< 0.03125 Hz */
	SENSOR_SAMPLING_RATE_MAX30210_0_0625,       /**< 0.0625 Hz */
	SENSOR_SAMPLING_RATE_MAX30210_0_125,        /**< 0.125 Hz */
	SENSOR_SAMPLING_RATE_MAX30210_0_25,         /**< 0.25 Hz */
	SENSOR_SAMPLING_RATE_MAX30210_0_5,          /**< 0.5 Hz */
	SENSOR_SAMPLING_RATE_MAX30210_1,            /**< 1 Hz */
	SENSOR_SAMPLING_RATE_MAX30210_2,            /**< 2 Hz */
	SENSOR_SAMPLING_RATE_MAX30210_4,            /**< 4 Hz */
	SENSOR_SAMPLING_RATE_MAX30210_8,            /**< 8 Hz */
};
/** @} */

/**
 * @brief Custom sensor trigger types for MAX30210.
 */
enum sensor_trigger_type_max30210 {
	/** Trigger fires when temperature increases beyond the fast threshold. */
	SENSOR_TRIG_TEMP_INC_FAST = SENSOR_TRIG_PRIV_START,
	/** Trigger fires when temperature decreases beyond the fast threshold. */
	SENSOR_TRIG_TEMP_DEC_FAST,
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_MAX30210_H_ */
