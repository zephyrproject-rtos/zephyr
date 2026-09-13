/*
 * Copyright (c) 2026 Filics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of ISM330DHCX sensor
 * @ingroup ism330dhcx_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_ISM330DHCX_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_ISM330DHCX_H_

/**
 * @defgroup ism330dhcx_interface ISM330DHCX
 * @ingroup sensor_interface_ext_st
 * @brief ST Microelectronics ISM330DHCX 6-axis IMU
 * @{
 */

#include <zephyr/drivers/sensor.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Custom sensor attributes for ISM330DHCX
 */
enum sensor_attribute_ism330dhcx {
	/**
	 * Real output data rate in Hz, the configured rate corrected by the
	 * factory oscillator trim in INTERNAL_FREQ_FINE. Read-only.
	 */
	SENSOR_ATTR_ISM330DHCX_REAL_ODR = SENSOR_ATTR_PRIV_START,
};

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_ISM330DHCX_H_ */
