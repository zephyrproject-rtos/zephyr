/*
 * Copyright (c) 2023 Andreas Kilian
 * Copyright (c) 2024 Jeff Welder (Ellenby Technologies, Inc.)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_VEML7700_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_VEML7700_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Number of enumerators in enum veml7700_als_gain.
 */
#define VEML7700_ALS_GAIN_ELEM_COUNT 4
/**
 * @brief Number of enumerators in enum veml7700_als_it.
 */
#define VEML7700_ALS_IT_ELEM_COUNT 6

/**
 * @brief Bit mask to check for the low threshold interrupt flag.
 *
 * See <tt>SENSOR_ATTR_VEML7700_INT_MODE</tt>
 * and <tt>SENSOR_CHAN_VEML7700_INTERRUPT</tt>
 */
#define VEML7700_ALS_INT_LOW_MASK BIT(15)
/**
 * @brief Bit mask to check for the high threshold interrupt flag.
 *
 * See <tt>SENSOR_ATTR_VEML7700_INT_MODE</tt>
 * and <tt>SENSOR_CHAN_VEML7700_INTERRUPT</tt>
 */
#define VEML7700_ALS_INT_HIGH_MASK BIT(14)

/**
 * @brief VEML7700 gain options for ambient light measurements.
 */
enum veml7700_als_gain {
	VEML7700_ALS_GAIN_1   = 0x00, /* 0b00 */
	VEML7700_ALS_GAIN_2   = 0x01, /* 0b01 */
	VEML7700_ALS_GAIN_1_8 = 0x02, /* 0b10 */
	VEML7700_ALS_GAIN_1_4 = 0x03, /* 0b11 */
};

/**
 * @brief VEML7700 integration time options for ambient light measurements.
 */
enum veml7700_als_it {
	VEML7700_ALS_IT_25,
	VEML7700_ALS_IT_50,
	VEML7700_ALS_IT_100,
	VEML7700_ALS_IT_200,
	VEML7700_ALS_IT_400,
	VEML7700_ALS_IT_800
};

/**
 * @brief VEML7700 ALS interrupt persistence protect number options.
 */
enum veml7700_int_mode {
	VEML7700_INT_DISABLED = 0xFF,
	VEML7700_ALS_PERS_1   = 0x00, /* 0b00 */
	VEML7700_ALS_PERS_2   = 0x01, /* 0b01 */
	VEML7700_ALS_PERS_4   = 0x02, /* 0b10 */
	VEML7700_ALS_PERS_8   = 0x03, /* 0b11 */
};

/**
 * @brief VEML7700 specific sensor attributes.
 *
 * For high and low threshold window settings (ALS_WH and ALS_WL)
 * use the generic attributes <tt>SENSOR_ATTR_UPPER_THRESH</tt> and
 * <tt>SENSOR_ATTR_LOWER_THRESH</tt> with 16-bit unsigned integer
 * values. Both threshold settings are in lux and converted by the
 * driver to a value compatible with the sensor. This conversion
 * depends on the current gain and integration time settings. So a
 * change in gain or integration time usually requires an update of
 * threshold window settings. To get the correct threshold values
 * into the sensor update the thresholds -after- a change of gain
 * or integration time.
 *
 * All attributes must be set for the <tt>SENSOR_CHAN_LIGHT</tt> channel.
 */
enum sensor_attribute_veml7700 {
	/**
	 * @brief Gain setting for ALS measurements (ALS_GAIN).
	 *
	 * Use enum veml7700_als_gain for attribute values.
	 */
	SENSOR_ATTR_VEML7700_GAIN = SENSOR_ATTR_PRIV_START,
	/**
	 * @brief Integration time setting for ALS measurements (ALS_IT).
	 *
	 * Use enum veml7700_als_it for attribute values.
	 */
	SENSOR_ATTR_VEML7700_ITIME,
	/**
	 * @brief Enable or disable use of ALS interrupt
	 * (ALS_INT_EN and ALS_PERS).
	 *
	 * Please mind that the VEML7700 does not have an interrupt pin.
	 * That's why this driver does not implement any asynchronous
	 * notification mechanism. Such notifications would require to
	 * periodically query the sensor for it's interrupt state and then
	 * trigger an event based on that state. It's up to the user to
	 * query the interrupt state manually using
	 * <tt>sensor_channel_fetch_chan()</tt> on the
	 * <tt>SENSOR_CHAN_VEML7700_INTERRUPT</tt> channel or using
	 * <tt>sensor_channel_fetch()</tt>.
	 *
	 * Use enum veml7700_int_mode for attribute values.
	 */
	SENSOR_ATTR_VEML7700_INT_MODE,
};

/**
 * @brief VEML7700 specific sensor channels.
 */
enum sensor_channel_veml7700 {
	/**
	 * @brief Channel for raw ALS sensor values.
	 *
	 * This channel represents the raw measurement counts provided by the
	 * sensors ALS register. It is useful for estimating the high/low
	 * threshold window attributes for the sensors interrupt handling.
	 *
	 * You cannot fetch this channel explicitly. Instead, this channel's
	 * value is fetched implicitly using <tt>SENSOR_CHAN_LIGHT</tt>.
	 * Trying to call <tt>sensor_channel_fetch_chan()</tt> with this
	 * enumerator as an argument will result in a <tt>-ENOTSUP</tt>.
	 */
	SENSOR_CHAN_VEML7700_RAW_COUNTS = SENSOR_CHAN_PRIV_START,

	/**
	 * @brief Channel for white light sensor values.
	 *
	 * This channel is the White Channel count output of the sensor.
	 * The white channel can be used to correct for light sources with
	 * strong infrared content in the 750-800nm spectrum.
	 */
	SENSOR_CHAN_VEML7700_WHITE_RAW_COUNTS,

	/**
	 * @brief This channel is used to query the ALS interrupt state (ALS_INT).
	 *
	 * In order for this channel to provide any meaningful data you need
	 * to enable the ALS interrupt mode using the
	 * <tt>SENSOR_ATTR_VEML7700_INT_MODE</tt> custom sensor attribute.
	 *
	 * It's important to note that the sensor resets it's interrupt state
	 * after retrieving it.
	 */
	SENSOR_CHAN_VEML7700_INTERRUPT,
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_VEML7700_H_ */
