/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended sensor API for the LDC1X1X
 * @ingroup ldc1x1x_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_LDC1X1X_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_LDC1X1X_H_

/**
 * @brief Texas Instruments LDC1X1X inductive sensor
 * @defgroup ldc1x1x_interface LDC1X1X
 * @ingroup sensor_interface_ext_ti
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>

/** LDC1X1X specific sensor channels */
enum sensor_channel_ldc1x1x {
	/** CH0 sensor frequency, in Hz */
	SENSOR_CHAN_LDC1X1X_FREQ_CH0 = SENSOR_CHAN_PRIV_START,
	/** CH1 sensor frequency, in Hz */
	SENSOR_CHAN_LDC1X1X_FREQ_CH1,
	/** CH2 sensor frequency, in Hz */
	SENSOR_CHAN_LDC1X1X_FREQ_CH2,
	/** CH3 sensor frequency, in Hz */
	SENSOR_CHAN_LDC1X1X_FREQ_CH3,
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_LDC1X1X_H_ */
