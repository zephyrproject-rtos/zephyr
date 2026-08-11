/*
 * SPDX-FileCopyrightText: 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of NXP MCUX EQDC sensor
 * @ingroup sensor_eqdc_mcux
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_EQDC_MCUX_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_EQDC_MCUX_H_

/**
 * @brief NXP MCUX EQDC sensor
 * @defgroup sensor_eqdc_mcux EQDC MCUX
 * @ingroup sensor_interface_ext
 * @{
 */

#include <zephyr/drivers/sensor.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Extended sensor attributes for NXP MCUX EQDC
 */
enum sensor_attribute_eqdc_mcux {
	/** Counts per revolution - uint32_t */
	SENSOR_ATTR_EQDC_COUNTS_PER_REVOLUTION = SENSOR_ATTR_PRIV_START,

	/** Clock frequency of the EQDC module after prescaled - uint32_t */
	SENSOR_ATTR_EQDC_PRESCALED_FREQUENCY,
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_EQDC_MCUX_H_ */
