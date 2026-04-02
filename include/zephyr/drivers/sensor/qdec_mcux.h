/*
 * Copyright (c) 2022, Prevas A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of QDEC MCUX sensor
 * @ingroup sensor_qdec_mcux
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_QDEC_MCUX_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_QDEC_MCUX_H_

/**
 * @brief NXP MCUX QDEC sensor
 * @defgroup sensor_qdec_mcux QDEC MCUX
 * @ingroup sensor_interface_ext
 * @{
 */

#include <zephyr/drivers/sensor.h>

/**
 * @brief Extended sensor attributes for QDEC MCUX
 */
enum sensor_attribute_qdec_mcux {
	/** Number of counts per revolution */
	SENSOR_ATTR_QDEC_MOD_VAL = SENSOR_ATTR_PRIV_START,
	/** Single phase counting */
	SENSOR_ATTR_QDEC_ENABLE_SINGLE_PHASE,
};

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_QDEC_MCUX_H_ */
