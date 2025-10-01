/*
 * Copyright (c) 2025 Andreas Klinger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of VEML6031 sensor
 * @ingroup veml6031_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_VEML6031_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_VEML6031_H_

/**
 * @defgroup veml6031_interface VEML6031
 * @ingroup sensor_interface_ext
 * @brief Vishay VEML6031 High Accuracy Ambient Light Sensor
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief VEML6031 integration time options for ambient light measurements.
 *
 * Possible values for @ref SENSOR_ATTR_VEML6031_IT custom attribute.
 */
enum veml6031_it {
	VEML6031_IT_3_125, /**< 3.125 ms */
	VEML6031_IT_6_25,  /**< 6.25 ms */
	VEML6031_IT_12_5,  /**< 12.5 ms */
	VEML6031_IT_25,    /**< 25 ms */
	VEML6031_IT_50,    /**< 50 ms */
	VEML6031_IT_100,   /**< 100 ms */
	VEML6031_IT_200,   /**< 200 ms */
	VEML6031_IT_400,   /**< 400 ms */
	/** @cond INTERNAL_HIDDEN */
	VEML6031_IT_COUNT,
	/** @endcond */
};

/**
 * @brief VEML6031 size options for ambient light measurements.
 *
 * Possible values for @ref SENSOR_ATTR_VEML6031_DIV4 custom attribute.
 */
enum veml6031_div4 {
	VEML6031_SIZE_4_4 = 0x00, /**< 4/4 photodiode size */
	VEML6031_SIZE_1_4 = 0x01, /**< 1/4 photodiode size */
	/** @cond INTERNAL_HIDDEN */
	VEML6031_DIV4_COUNT = 2,
	/** @endcond */
};

/**
 * @brief VEML6031 gain options for ambient light measurements.
 */
enum veml6031_gain {
	VEML6031_GAIN_1 = 0x00,    /**< 1x gain */
	VEML6031_GAIN_2 = 0x01,    /**< 2x gain */
	VEML6031_GAIN_0_66 = 0x02, /**< 0.66x gain */
	VEML6031_GAIN_0_5 = 0x03,  /**< 0.5x gain */
	/** @cond INTERNAL_HIDDEN */
	VEML6031_GAIN_COUNT = 4,
	/** @endcond */
};

/**
 * @brief VEML6031 ALS interrupt persistence protect number options.
 *
 * Possible values for @ref SENSOR_ATTR_VEML6031_PERS custom attribute.
 */
enum veml6031_pers {
	VEML6031_PERS_1 = 0x00, /**< 1 measurement */
	VEML6031_PERS_2 = 0x01, /**< 2 measurements */
	VEML6031_PERS_4 = 0x02, /**< 4 measurements */
	VEML6031_PERS_8 = 0x03, /**< 8 measurements */
};

/**
 * @brief Custom sensor attributes for VEML6031 sensor.
 *
 * For high and low threshold window settings (ALS_WH_L, ALS_WH_H, ALS_WL_L and
 * ALS_WL_H) use the generic attributes @ref SENSOR_ATTR_UPPER_THRESH and
 * @ref SENSOR_ATTR_LOWER_THRESH with 16-bit unsigned integer values. Both
 * threshold settings are in lux and converted by the driver to a value
 * compatible with the sensor. This conversion depends on the current gain,
 * integration time and effective photodiode size settings. So a change in
 * gain, integration time or effective photodiode size usually requires an
 * update of threshold window settings. To get the correct threshold values
 * into the sensor update the thresholds -after- a change of gain or
 * integration time.
 *
 * All attributes must be set for the @ref SENSOR_CHAN_LIGHT channel.
 *
 * When the sensor goes into saturation @c -E2BIG is returned. This
 * happens when the maximum value <tt>0xFFFF</tt> is returned as raw ALS value.
 * In this case it's up to the user to reduce one or more of the following
 * attributes to come back into the optimal measurement range of the sensor:
 * - @ref SENSOR_ATTR_VEML6031_GAIN (gain)
 * - @ref SENSOR_ATTR_VEML6031_IT (integration time)
 * - @ref SENSOR_ATTR_VEML6031_DIV4 (effective photodiode size)
 */
enum sensor_attribute_veml6031 {
	/**
	 * @brief Integration time setting for ALS measurements (IT).
	 *
	 * Use enum veml6031_it for attribute values.
	 */
	SENSOR_ATTR_VEML6031_IT = SENSOR_ATTR_PRIV_START,
	/**
	 * @brief Effective photodiode size (DIV4)
	 *
	 * Use enum veml6031_div4 for attribute values.
	 */
	SENSOR_ATTR_VEML6031_DIV4,
	/**
	 * @brief Gain setting for ALS measurements (GAIN).
	 *
	 * Use enum veml6031_gain for attribute values.
	 */
	SENSOR_ATTR_VEML6031_GAIN,
	/**
	 * @brief ALS persistence protect number setting (PERS).
	 *
	 * Use enum veml6031_pers for attribute values.
	 */
	SENSOR_ATTR_VEML6031_PERS,
};

/**
 * @brief Custom sensor channels for VEML6031 sensor.
 */
enum sensor_channel_veml6031 {
	/**
	 * @brief Channel for raw ALS sensor values.
	 *
	 * This channel represents the raw measurement counts provided by the
	 * sensor ALS register. It is useful for estimating good values for
	 * integration time, effective photodiode size and gain attributes in
	 * fetch and get mode.
	 *
	 * For future implementations with triggers it can also be used to
	 * estimate the threshold window attributes for the sensor interrupt
	 * handling.
	 *
	 * It cannot be fetched directly. Instead, this channel's value is
	 * fetched implicitly using @ref SENSOR_CHAN_LIGHT. Trying to call
	 * @ref sensor_sample_fetch_chan with this enumerator as an
	 * argument will result in a @c -ENOTSUP.
	 */
	SENSOR_CHAN_VEML6031_ALS_RAW_COUNTS = SENSOR_CHAN_PRIV_START,

	/**
	 * @brief Channel for IR sensor values.
	 *
	 * This channel is the raw IR Channel count output of the sensor. About
	 * fetching the same as for
	 * @ref SENSOR_CHAN_VEML6031_ALS_RAW_COUNTS applies.
	 */
	SENSOR_CHAN_VEML6031_IR_RAW_COUNTS,
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_VEML6031_H_ */
