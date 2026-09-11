/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of RAK12035
 * @ingroup rak12035_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_RAK12035_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_RAK12035_H_

/**
 * @brief RAKwireless RAK12035 capacitive soil moisture sensor
 * @defgroup rak12035_interface RAK12035
 * @ingroup sensor_interface_ext_rakwireless
 * @{
 *
 * @c SENSOR_CHAN_AMBIENT_TEMP is the probe temperature.
 *
 * @c SENSOR_CHAN_HUMIDITY is a soil moisture percentage relative to the
 * stored dry and wet calibration points. It is not air relative humidity
 * or a standardized volumetric water content.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>

/** Raw capacitive soil moisture reading (unscaled count, no physical unit). */
#define SENSOR_CHAN_RAK12035_CAPACITANCE_RAW ((enum sensor_channel)SENSOR_CHAN_PRIV_START)

/**
 * @brief Custom sensor attributes for RAK12035
 */
enum rak12035_sensor_attribute {
	/**
	 * Capacitance corresponding to dry soil (0%).
	 *
	 * - sensor_value.val1 is the unscaled capacitance count
	 */
	SENSOR_ATTR_RAK12035_CALIBRATION_DRY = SENSOR_ATTR_PRIV_START,
	/**
	 * Capacitance corresponding to saturated soil (100%).
	 *
	 * - sensor_value.val1 is the unscaled capacitance count
	 */
	SENSOR_ATTR_RAK12035_CALIBRATION_WET,
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_RAK12035_H_ */
