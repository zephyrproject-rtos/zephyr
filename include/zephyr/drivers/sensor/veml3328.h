/*
 * Copyright (c) 2026 Christopher Ruehl
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_VEML3328_H
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_VEML3328_H

/**
 * @file
 * @brief Header file for extended sensor API of VEML3328 sensor
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Number of enumerators in enum veml3328_dg_factor.
 */
#define VEML3328_DGFACTOR_ELEM_COUNT 3

/**
 * @brief Number of enumerators in enum veml3328_rgbcir_gain.
 */
#define VEML3328_GAIN_ELEM_COUNT 4

/**
 * @brief VEML3328 dgfactor options for RGBC and IR measurements.
 */
enum veml3328_dg_factor {
	/** Digital gain factor 1 (default). */
	VEML3328_DG_FACT_1   = 0x00,
	/** Digital gain factor 2. */
	VEML3328_DG_FACT_2   = 0x01,
	/** Digital gain factor 4. */
	VEML3328_DG_FACT_4   = 0x02,
};

/**
 * @brief VEML3328 gain options for color and IR measurements.
 */
enum veml3328_rgbcir_gain {
	/** Sensor Gain factor 1 (default). */
	VEML3328_RGBCIR_GAIN_1   = 0x00,
	/** Sensor Gain factor 2. */
	VEML3328_RGBCIR_GAIN_2   = 0x01,
	/** Sensor Gain factor 4. */
	VEML3328_RGBCIR_GAIN_4   = 0x02,
	/** Sensor Gain factor 1/2. */
	VEML3328_RGBCIR_GAIN_1_2 = 0x03,
};

/**
 * @brief VEML3328 integration time options for RGBC and IR measurements.
 */
enum veml3328_rgbcir_it {
	/** Sensor integration time 50ms (default). */
	VEML3328_RGBCIR_IT_50  = 0x00,
	/** Sensor integration time 100ms. */
	VEML3328_RGBCIR_IT_100 = 0x01,
	/** Sensor integration time 200ms. */
	VEML3328_RGBCIR_IT_200 = 0x02,
	/** Sensor integration time 400ms. */
	VEML3328_RGBCIR_IT_400 = 0x03,
};

/**
 * @brief VEML3328 specific sensor attributes.
 */
enum sensor_attribute_veml3328 {
	/**
	 * @brief DG Factor setting for RGBC and IR measurements.
	 *
	 * Use enum veml3328_dg_factor for attribute values.
	 */
	SENSOR_ATTR_VEML3328_DGFACT = SENSOR_ATTR_PRIV_START,
	/**
	 * @brief Gain setting for RGBC and IR measurements.
	 *
	 * Use enum veml3328_rgbcir_gain for attribute values.
	 */
	SENSOR_ATTR_VEML3328_GAIN,
	/**
	 * @brief Integration time setting for RGBC and IR measurements.
	 *
	 * Use enum veml3328_rgbcir_it for attribute values.
	 */
	SENSOR_ATTR_VEML3328_ITIME,
	/**
	 * @brief Enable or disable use of SD_ALS ONLY
	 * If enabled, only G,C and IR is powered up.
	 */
	SENSOR_ATTR_VEML3328_SD_ALS_MODE,
	/**
	 * @brief Enable or disable use low sensetifty
	 * Set the sensetifity to 1/3 of the default sensing
	 */
	SENSOR_ATTR_VEML3328_SENS_MODE,
	/**
	 * @brief Enable or disable use active force mode
	 * The active force mode supports a single messurement
	 * cycle fired by set the TRIG(BIT 2) of the Command Register
	 * to 1.
	 */
	SENSOR_ATTR_VEML3328_AF_MODE,
	/**
	 * @brief Trigger bit, used with active force mode enabled
	 * to start a single messurement cycle
	 */
	SENSOR_ATTR_VEML3328_TRIG,
};

/**
 * @brief VEML3328 specific sensor channels.
 * Sensor: R G B, Clear, Infrared and LUX.
 */
enum sensor_channel_veml3328 {
	/**
	 * @brief Channel for clear (~ 520nm) sensor values.
	 *
	 * This channel is the sensors Clear Channel raw count output.
	 */
	SENSOR_CHAN_VEML3328_CLEAR_RAW = SENSOR_CHAN_PRIV_START,

	/**
	 * @brief Channel for IR (~ 850nm) sensor values.
	 *
	 * This channel is the sensors Infra Red Channel raw count output.
	 */
	SENSOR_CHAN_VEML3328_IR_RAW,

	/**
	 * @brief Channel for RED (~ 643nm) sensor values.
	 *
	 * This channel is the sensors Red Channel raw count output.
	 * Note: Not available if sd_als_only flag is set!
	 */
	SENSOR_CHAN_VEML3328_RED_RAW,

	/**
	 * @brief Channel for GREEN (~ 520nm) sensor values.
	 *
	 * This channel is the sensors Green Channel raw count output.
	 */
	SENSOR_CHAN_VEML3328_GREEN_RAW,

	/**
	 * @brief Channel for BLUE (~ 460nm) sensor values.
	 *
	 * This channel is the Blue Channel count output of the sensor.
	 * Note: Not available if sd_als_only flag is set!
	 */
	SENSOR_CHAN_VEML3328_BLUE_RAW,

	/**
	 * @brief Channel Ambient Light Sensor values.
	 *
	 * This channel the ALS in LUX based on the
	 * Green Sensor, which match well with "Human Eye".
	 */
	SENSOR_CHAN_VEML3328_LUX_SENSING,

};
#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_VEML3328_H */
