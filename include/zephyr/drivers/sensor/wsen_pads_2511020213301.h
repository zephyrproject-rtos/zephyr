/*
 * Copyright (c) 2025 Würth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended public API for WSEN-PADS-2511020213301 Sensor
 * @ingroup wsen_pads_2511020213301_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_WSEN_PADS_2511020213301_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_WSEN_PADS_2511020213301_H_

/**
 * @brief Würth Elektronik WSEN-PADS-2511020213301 absolute pressure sensor
 * @defgroup wsen_pads_2511020213301_interface WSEN-PADS-2511020213301
 * @ingroup sensor_interface_ext
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>

/** @brief Extended sensor attributes for the WSEN-PADS-2511020213301. */
enum sensor_attribute_wsen_pads_2511020213301 {
	/** Reference point used for differential pressure measurements. */
	SENSOR_ATTR_WSEN_PADS_2511020213301_REFERENCE_POINT = SENSOR_ATTR_PRIV_START,
};

#ifdef CONFIG_WSEN_PADS_2511020213301_PRESSURE_THRESHOLD
/** @brief Pressure-threshold triggers for the WSEN-PADS-2511020213301. */
enum sensor_trigger_wsen_pads_2511020213301 {
	/** Pressure rose above the upper threshold. */
	SENSOR_TRIG_WSEN_PADS_2511020213301_THRESHOLD_UPPER = SENSOR_TRIG_PRIV_START,
	/** Pressure fell below the lower threshold. */
	SENSOR_TRIG_WSEN_PADS_2511020213301_THRESHOLD_LOWER,
};
#endif /* CONFIG_WSEN_PADS_2511020213301_PRESSURE_THRESHOLD */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_WSEN_PADS_2511020213301_H_ */
