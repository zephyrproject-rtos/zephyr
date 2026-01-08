/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_MCUX_EQDC_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_MCUX_EQDC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>

/**
 * @brief EQDC specific sensor attributes
 */
enum sensor_attribute_mcux_eqdc {
	/** Counts per revolution - uint32_t */
	SENSOR_ATTR_EQDC_COUNTS_PER_REVOLUTION = SENSOR_ATTR_PRIV_START,

	/** Clock frequency of the EQDC module after prescaled - uint32_t */
	SENSOR_ATTR_EQDC_PRESCALED_FREQUENCY,

};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_MCUX_EQDC_H_ */
