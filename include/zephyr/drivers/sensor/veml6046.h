/*
 * Copyright (c) 2025 Andreas Klinger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor/veml60xx-common.h>

/**
 * @file
 * @brief Header file for extended sensor API of VEML6046 sensor.
 * @ingroup veml6046_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_VEML6046_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_VEML6046_H_

/**
 * @defgroup veml6046_interface VEML6046
 * @ingroup sensor_interface_ext
 * @brief Vishay VEML6046 RGBIR Sensor
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief VEML6046 size options for light measurements.
 *
 * Possible values for @ref SENSOR_ATTR_VEML6046_PDD custom attribute.
 */
enum veml6046_pdd {
	VEML6046_SIZE_2_2 = 0x00, /**< 2/2 photodiode size */
	VEML6046_SIZE_1_2 = 0x01, /**< 1/2 photodiode size */
	/** @cond INTERNAL_HIDDEN */
	VEML6046_PDD_COUNT = 2,
	/** @endcond */
};

/**
 * @brief VEML6046 specific sensor attributes.
 *
 * For high and low threshold window settings (G_THDH_L, G_THDH_H, G_THDL_L and
 * G_THDL_H) use the generic attributes @ref SENSOR_ATTR_UPPER_THRESH and
 * @ref SENSOR_ATTR_LOWER_THRESH with 16-bit unsigned integer values. Both
 * threshold settings are in lux and converted by the driver to a value
 * compatible with the sensor. This conversion depends on the current gain,
 * integration time and effective photodiode size settings. So a change in
 * gain, integration time or effective photodiode size usually requires an
 * update of threshold window settings. To get the correct threshold values
 * into the sensor update the thresholds -after- a change of gain or
 * integration time.
 *
 * When the sensor goes into saturation @c -E2BIG is returned. This happens
 * when the maximum value @c 0xFFFF is returned as raw ALS value.  In this case
 * it's up to the user to reduce one or more of the following attributes to
 * come back into the optimal measurement range of the sensor:
 *  @ref SENSOR_ATTR_VEML6046_GAIN (gain)
 *  @ref SENSOR_ATTR_VEML6046_IT (integration time)
 *  @ref SENSOR_ATTR_VEML6046_PDD (effective photodiode size)
 */
enum sensor_attribute_veml6046 {
	/**
	 * @brief Integration time setting for measurements (IT).
	 *
	 * Use enum veml6046_it for attribute values.
	 */
	SENSOR_ATTR_VEML6046_IT = SENSOR_ATTR_PRIV_START,
	/**
	 * @brief Effective photodiode size (PDD)
	 *
	 * Use enum veml6046_pdd for attribute values.
	 */
	SENSOR_ATTR_VEML6046_PDD,
	/**
	 * @brief Gain setting for measurements (GAIN).
	 *
	 * Use enum veml6046_gain for attribute values.
	 */
	SENSOR_ATTR_VEML6046_GAIN,
	/**
	 * @brief Persistence protect number setting (PERS).
	 *
	 * Use enum veml6046_pers for attribute values.
	 */
	SENSOR_ATTR_VEML6046_PERS,
};

/**
 * @brief VEML6046 specific sensor channels.
 */
enum sensor_channel_veml6046 {
	/**
	 * @brief Channel for raw red sensor values.
	 *
	 * This channel represents the raw measurement counts provided by the
	 * sensor register. It is useful for estimating good values for
	 * integration time, effective photodiode size and gain attributes in
	 * fetch and get mode.
	 *
	 * For future implementations with triggers it can also be used to
	 * estimate the threshold window attributes for the sensor interrupt
	 * handling.
	 *
	 * It cannot be fetched directly. Instead, this channel's value is
	 * fetched implicitly using @ref SENSOR_CHAN_RED.
	 * Trying to call sensor_channel_fetch_chan with this enumerator
	 * as an argument will result in a @c -ENOTSUP.
	 */
	SENSOR_CHAN_VEML6046_RED_RAW_COUNTS = SENSOR_CHAN_PRIV_START,

	/**
	 * @brief Channel for green sensor values.
	 *
	 * This channel is the raw green channel count output of the sensor.
	 * About fetching the same as for
	 * @ref SENSOR_CHAN_VEML6046_RED_RAW_COUNTS applies.
	 */
	SENSOR_CHAN_VEML6046_GREEN_RAW_COUNTS,

	/**
	 * @brief Channel for blue sensor values.
	 *
	 * This channel is the raw blue channel count output of the sensor.
	 * About fetching the same as for
	 * @ref SENSOR_CHAN_VEML6046_RED_RAW_COUNTS applies.
	 */
	SENSOR_CHAN_VEML6046_BLUE_RAW_COUNTS,

	/**
	 * @brief Channel for IR sensor values.
	 *
	 * This channel is the raw IR channel count output of the sensor. About
	 * fetching the same as for
	 * @ref SENSOR_CHAN_VEML6046_RED_RAW_COUNTS applies.
	 */
	SENSOR_CHAN_VEML6046_IR_RAW_COUNTS,
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_VEML6046_H_ */
