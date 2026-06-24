/*
 * Copyright (c) 2024
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of EQDC MCUX sensor
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_EQDC_MCUX_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_EQDC_MCUX_H_

#include <zephyr/drivers/sensor.h>

/**
 * @brief Extended sensor attributes for EQDC MCUX
 */
enum sensor_attribute_eqdc_mcux {
    /** Number of counts per revolution */
    SENSOR_ATTR_EQDC_MOD_VAL = SENSOR_ATTR_PRIV_START,
    /** Single phase counting */
    SENSOR_ATTR_EQDC_ENABLE_SINGLE_PHASE,
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_EQDC_MCUX_H_ */