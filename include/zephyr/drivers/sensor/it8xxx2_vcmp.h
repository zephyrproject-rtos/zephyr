/*
 * Copyright (c) 2022 ITE Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of IT8XXX2 voltage comparator
 * @ingroup it8xxx2_vcmp_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_IT8XXX2_VCMP_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_IT8XXX2_VCMP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>

/**
 * @addtogroup it8xxx2_vcmp_interface
 * @{
 */

/** @brief Custom sensor attributes for the IT8XXX2 voltage comparator */
enum it8xxx2_vcmp_sensor_attribute {
	/** Lower threshold, in mV */
	SENSOR_ATTR_LOWER_VOLTAGE_THRESH = SENSOR_ATTR_PRIV_START,
	/** Upper threshold, in mV */
	SENSOR_ATTR_UPPER_VOLTAGE_THRESH,
};

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_IT8XXX2_VCMP_H_ */
