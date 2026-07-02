/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the ST IIS2ICLX inclinometer.
 * @ingroup iis2iclx_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ST_IIS2ICLX_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ST_IIS2ICLX_H_

/**
 * @defgroup iis2iclx_interface IIS2ICLX
 * @ingroup sensor_interface_ext_st
 * @brief STMicroelectronics IIS2ICLX 2-axis inclinometer
 * @{
 */

/**
 * @name Accelerometer full-scale range
 *
 * Values for the `range` devicetree property.
 * @{
 */
#define IIS2ICLX_DT_FS_500mG			0 /**< ±0.5 g */
#define IIS2ICLX_DT_FS_3G			1 /**< ±3 g */
#define IIS2ICLX_DT_FS_1G			2 /**< ±1 g */
#define IIS2ICLX_DT_FS_2G			3 /**< ±2 g */
/** @} */

/**
 * @name Accelerometer output data rate
 *
 * Values for the `odr` devicetree property.
 * @{
 */
#define IIS2ICLX_DT_ODR_OFF			0x0 /**< Power-down */
#define IIS2ICLX_DT_ODR_12Hz5			0x1 /**< 12.5 Hz */
#define IIS2ICLX_DT_ODR_26H			0x2 /**< 26 Hz */
#define IIS2ICLX_DT_ODR_52Hz			0x3 /**< 52 Hz */
#define IIS2ICLX_DT_ODR_104Hz			0x4 /**< 104 Hz */
#define IIS2ICLX_DT_ODR_208Hz			0x5 /**< 208 Hz */
#define IIS2ICLX_DT_ODR_416Hz			0x6 /**< 416 Hz */
#define IIS2ICLX_DT_ODR_833Hz			0x7 /**< 833 Hz */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ST_IIS2ICLX_H_ */
