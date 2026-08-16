/*
 * Copyright (c) 2026 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of Excelitas TPiS 1S 1385 sensor
 * @ingroup tpi1s1385_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_TPI1S1385_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_TPI1S1385_H_

/**
 * @brief Excelitas CaliPile TPiS 1S 1385 infrared thermopile
 * @defgroup tpi1s1385_interface Excelitas TPiS 1S 1385
 * @ingroup sensor_interface_ext_excelitas
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>

/**
 * @brief Custom sensor channels for the TPiS 1S 1385.
 */
enum sensor_channel_tpi1s1385 {
	/** Temperature of the object in the field of view, in degrees Celsius.
	 *
	 * The sensor measures this temperature without contact, so it is
	 * neither the die temperature of a component nor the temperature of
	 * the surrounding air.
	 *
	 * @c sensor_value.val1 is the integer part of the temperature.
	 * @c sensor_value.val2 is the fractional part (in millionths of a
	 * degree).
	 */
	SENSOR_CHAN_TPI1S1385_OBJECT_TEMP = SENSOR_CHAN_PRIV_START,
	/** Presence counter, as an unsigned eight bit value.
	 *
	 * The device computes it from the two low pass filtered object
	 * signals selected by the @c src-select property. The value grows
	 * with the infrared contrast between a body and the background.
	 *
	 * @c sensor_value.val1 is the counter.
	 * @c sensor_value.val2 is unused (always 0).
	 */
	SENSOR_CHAN_TPI1S1385_PRESENCE,
	/** Motion counter, as an unsigned eight bit value.
	 *
	 * The device computes it from the object signal differentiated over
	 * the window selected by the @c cycle-time property. The value grows
	 * with how fast the infrared contrast changes.
	 *
	 * @c sensor_value.val1 is the counter.
	 * @c sensor_value.val2 is unused (always 0).
	 */
	SENSOR_CHAN_TPI1S1385_MOTION,
};

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_TPI1S1385_H_ */
