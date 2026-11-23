/*
 * Copyright (c) 2026 Carl Zeiss Meditec AG
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended public API for the ams OSRAM TSL2522 ambient light sensor
 * @ingroup tsl2522_interface
 *
 * @par Channels
 * SENSOR_CHAN_LIGHT and SENSOR_CHAN_AMBIENT_LIGHT return the illuminance in lux.
 * SENSOR_CHAN_IR returns a normalized count rate in counts per ms at 1x gain (not a
 * photometric unit). Both take the glass attenuation from the devicetree into account.
 * If the sensor was saturated, sensor_channel_get() still fills the value but returns
 * -EOVERFLOW.
 *
 * @par Attributes
 * Attributes are written with sensor_attr_set() and read with sensor_attr_get(). Any of
 * SENSOR_CHAN_ALL, SENSOR_CHAN_AMBIENT_LIGHT, SENSOR_CHAN_LIGHT and SENSOR_CHAN_IR is
 * accepted as channel, the setting always applies to the whole device (both channels).
 * @c val2 is unused.
 *
 * @li SENSOR_ATTR_GAIN: @c val1 is an enumerator of @ref sensor_gain_tsl2522, not the
 *     numeric gain factor.
 * @li SENSOR_ATTR_MEASUREMENT_TIME_STEPS: @c val1 is the number of 1.388889 us steps
 *     (1 ... 2048).
 * @li SENSOR_ATTR_NUMBER_OF_SAMPLES: @c val1 is the number of samples per conversion
 *     (1 ... 2048).
 *
 * @note Changing an attribute restarts the measurement. sensor_sample_fetch() returns
 *       -EAGAIN until the first conversion with the new settings has finished.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_TSL2522_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_TSL2522_H_

#include <zephyr/drivers/sensor.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ams OSRAM ambient light sensor
 * @defgroup tsl2522_interface TSL2522
 * @version 0.1.0
 * @ingroup sensor_interface_ext_ams
 * @{
 */

/**
 * @brief Custom sensor attribute for TSL2522
 */
enum sensor_attribute_tsl2522 {
	/** ALS measurement time step. val1 contains the time in steps of 1.388889μs
	 * modulator clock.
	 */
	SENSOR_ATTR_MEASUREMENT_TIME_STEPS = SENSOR_ATTR_PRIV_START + 1,
	/** Number of samples in a conversion. val1 contains a value between 1 and 2048
	 */
	SENSOR_ATTR_NUMBER_OF_SAMPLES,
};

/**
 * @brief Modulator gain settings used in both channels.
 *
 * Discrete gain values. Pass the enumerator as @c val1 of @c SENSOR_ATTR_GAIN
 * (e.g. @c TSL2522_GAIN_MOD_16X), not the numeric gain factor.
 */
enum sensor_gain_tsl2522 {
	TSL2522_GAIN_MOD_HALF = 0U, /**< Gain x 0.5 */
	TSL2522_GAIN_MOD_1X,        /**< Gain x 1.0 */
	TSL2522_GAIN_MOD_2X,        /**< Gain x 2.0 */
	TSL2522_GAIN_MOD_4X,        /**< Gain x 4.0 */
	TSL2522_GAIN_MOD_8X,        /**< Gain x 8.0 */
	TSL2522_GAIN_MOD_16X,       /**< Gain x 16.0 */
	TSL2522_GAIN_MOD_32X,       /**< Gain x 32.0 */
	TSL2522_GAIN_MOD_64X,       /**< Gain x 64.0 */
	TSL2522_GAIN_MOD_128X,      /**< Gain x 128.0 */
	TSL2522_GAIN_MOD_256X,      /**< Gain x 256.0 */
	TSL2522_GAIN_MOD_512X,      /**< Gain x 512.0 */
	TSL2522_GAIN_MOD_1024X,     /**< Gain x 1024.0 */
	TSL2522_GAIN_MOD_2048X,     /**< Gain x 2048.0 */
	TSL2522_GAIN_MOD_4096X      /**< Gain x 4096.0 */
};

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_TSL2522_H_ */
