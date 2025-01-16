/*
 * Copyright (c) 2025 Andreas Klinger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_VEML6031_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_VEML6031_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ALS integration time settings in veml6031_it
 */
#define VEML6031_IT_COUNT 8

/**
 * @brief Effective photodiode size in enum veml6031_size.
 */
#define VEML6031_DIV4_COUNT 2

/**
 * @brief Gain selections in enum veml6031_gain.
 */
#define VEML6031_GAIN_COUNT 4

/**
 * @brief VEML6031 integration time options for ambient light measurements.
 */
enum veml6031_it {
	VEML6031_IT_3_125,
	VEML6031_IT_6_25,
	VEML6031_IT_12_5,
	VEML6031_IT_25,
	VEML6031_IT_50,
	VEML6031_IT_100,
	VEML6031_IT_200,
	VEML6031_IT_400,
};

/**
 * @brief VEML6031 size options for ambient light measurements.
 */
enum veml6031_div4 {
	VEML6031_SIZE_4_4 = 0x00, /* 0b0 */
	VEML6031_SIZE_1_4 = 0x01, /* 0b1 */
};

/**
 * @brief VEML6031 gain options for ambient light measurements.
 */
enum veml6031_gain {
	VEML6031_GAIN_1 = 0x00,    /* 0b00 */
	VEML6031_GAIN_2 = 0x01,    /* 0b01 */
	VEML6031_GAIN_0_66 = 0x02, /* 0b10 */
	VEML6031_GAIN_0_5 = 0x03,  /* 0b11 */
};

/**
 * @brief VEML6031 ALS interrupt persistence protect number options.
 */
enum veml6031_pers {
	VEML6031_PERS_1 = 0x00, /* 0b00 */
	VEML6031_PERS_2 = 0x01, /* 0b01 */
	VEML6031_PERS_4 = 0x02, /* 0b10 */
	VEML6031_PERS_8 = 0x03, /* 0b11 */
};

/**
 * @brief VEML6031 specific sensor attributes.
 *
 * For high and low threshold window settings (ALS_WH_L, ALS_WH_H, ALS_WL_L and
 * ALS_WL_H) use the generic attributes <tt>SENSOR_ATTR_UPPER_THRESH</tt> and
 * <tt>SENSOR_ATTR_LOWER_THRESH</tt> with 16-bit unsigned integer values. Both
 * threshold settings are in lux and converted by the driver to a value
 * compatible with the sensor. This conversion depends on the current gain,
 * integration time and effective photodiode size settings. So a change in
 * gain, integration time or effective photodiode size usually requires an
 * update of threshold window settings. To get the correct threshold values
 * into the sensor update the thresholds -after- a change of gain or
 * integration time.
 *
 * All attributes must be set for the <tt>SENSOR_CHAN_LIGHT</tt> channel.
 *
 * When the sensor goes into saturation <tt>-E2BIG</tt> is returned. This
 * happens when the maximum value <tt>0xFFFF</tt> is returned as raw ALS value.
 * In this case it's up to the user to reduce one or more of the following
 * attributes to come back into the optimal measurement range of the sensor:
 *  <tt>SENSOR_ATTR_VEML6031_GAIN</tt> (gain)
 *  <tt>SENSOR_ATTR_VEML6031_IT</tt> (integration time)
 *  <tt>SENSOR_ATTR_VEML6031_DIV4</tt> (effective photodiode size)
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
 * @brief VEML6031 specific sensor channels.
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
	 * fetched implicitly using <tt>SENSOR_CHAN_LIGHT</tt>. Trying to call
	 * <tt>sensor_channel_fetch_chan()</tt> with this enumerator as an
	 * argument will result in a <tt>-ENOTSUP</tt>.
	 */
	SENSOR_CHAN_VEML6031_ALS_RAW_COUNTS = SENSOR_CHAN_PRIV_START,

	/**
	 * @brief Channel for IR sensor values.
	 *
	 * This channel is the raw IR Channel count output of the sensor. About
	 * fetching the same as for
	 * <tt>SENSOR_CHAN_VEML6031_ALS_RAW_COUNTS</tt> applies.
	 */
	SENSOR_CHAN_VEML6031_IR_RAW_COUNTS,
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_VEML6031_H_ */
