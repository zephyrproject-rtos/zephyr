/*
 * Copyright (c) 2020 arithmetics.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of FDC2X1X sensor
 * @ingroup fdc2x1x_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_FDC2X1X_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_FDC2X1X_H_

/**
 * @brief Texas Instruments FDC2X1X capacitive sensor
 * @defgroup fdc2x1x_interface FDC2X1X
 * @ingroup sensor_interface_ext
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>

/**
 * Custom sensor channels for FDC2X1X
 */
enum sensor_channel_fdc2x1x {
	/** CH0 Capacitance, in picofarad **/
	SENSOR_CHAN_FDC2X1X_CAPACITANCE_CH0 = SENSOR_CHAN_PRIV_START,
	/** CH1 Capacitance, in picofarad **/
	SENSOR_CHAN_FDC2X1X_CAPACITANCE_CH1,
	/** CH2 Capacitance, in picofarad **/
	SENSOR_CHAN_FDC2X1X_CAPACITANCE_CH2,
	/** CH3 Capacitance, in picofarad **/
	SENSOR_CHAN_FDC2X1X_CAPACITANCE_CH3,

	/** CH0 Frequency, in MHz **/
	SENSOR_CHAN_FDC2X1X_FREQ_CH0,
	/** CH1 Frequency, in MHz **/
	SENSOR_CHAN_FDC2X1X_FREQ_CH1,
	/** CH2 Frequency, in MHz **/
	SENSOR_CHAN_FDC2X1X_FREQ_CH2,
	/** CH3 Frequency, in MHz **/
	SENSOR_CHAN_FDC2X1X_FREQ_CH3,
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_FDC2X1X_ */
