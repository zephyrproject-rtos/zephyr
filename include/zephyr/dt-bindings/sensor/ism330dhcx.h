/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the ST ISM330DHCX IMU.
 * @ingroup ism330dhcx_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ST_ISM330DHCX_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ST_ISM330DHCX_H_

/**
 * @defgroup ism330dhcx_interface ISM330DHCX
 * @ingroup sensor_interface_ext_st
 * @brief STMicroelectronics ISM330DHCX 6-axis IMU
 * @{
 */

/**
 * @name Accelerometer and gyroscope output data rate
 *
 * Values for the `accel-odr` and `gyro-odr` devicetree properties.
 * @{
 */
#define ISM330DHCX_DT_ODR_OFF			0x0 /**< Power-down */
#define ISM330DHCX_DT_ODR_12Hz5			0x1 /**< 12.5 Hz */
#define ISM330DHCX_DT_ODR_26H			0x2 /**< 26 Hz */
#define ISM330DHCX_DT_ODR_52Hz			0x3 /**< 52 Hz */
#define ISM330DHCX_DT_ODR_104Hz			0x4 /**< 104 Hz */
#define ISM330DHCX_DT_ODR_208Hz			0x5 /**< 208 Hz */
#define ISM330DHCX_DT_ODR_416Hz			0x6 /**< 416 Hz */
#define ISM330DHCX_DT_ODR_833Hz			0x7 /**< 833 Hz */
#define ISM330DHCX_DT_ODR_1666Hz		0x8 /**< 1666 Hz */
#define ISM330DHCX_DT_ODR_3332Hz		0x9 /**< 3332 Hz */
#define ISM330DHCX_DT_ODR_6667Hz		0xa /**< 6667 Hz */
#define ISM330DHCX_DT_ODR_1Hz6			0xb /**< 1.6 Hz (low-power) */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ST_ISM330DHCX_H_ */
