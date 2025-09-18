/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for IIS2DLPC Devicetree constants
 * @ingroup iis2dlpc_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ST_IIS2DLPC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ST_IIS2DLPC_H_

/**
 * @defgroup iis2dlpc_interface IIS2DLPC
 * @ingroup sensor_interface_ext
 * @brief IIS2DLPC 3-axis accelerometer
 * @{
 */

/**
 * @name Power modes
 * @{
 */
#define IIS2DLPC_DT_LP_M1			0 /**< Low Power Mode 1 */
#define IIS2DLPC_DT_LP_M2			1 /**< Low Power Mode 2 */
#define IIS2DLPC_DT_LP_M3			2 /**< Low Power Mode 3 */
#define IIS2DLPC_DT_LP_M4			3 /**< Low Power Mode 4 */
#define IIS2DLPC_DT_HP_MODE			4 /**< High Performance Mode */
/** @} */

/**
 * @name Filter bandwidth
 *
 * Low-pass cutoff selection as a fraction of ODR.
 * @{
 */
#define IIS2DLPC_DT_FILTER_BW_ODR_DIV_2		0 /**< ODR/2 */
#define IIS2DLPC_DT_FILTER_BW_ODR_DIV_4		1 /**< ODR/4 */
#define IIS2DLPC_DT_FILTER_BW_ODR_DIV_10	2 /**< ODR/10 */
#define IIS2DLPC_DT_FILTER_BW_ODR_DIV_20	3 /**< ODR/20 */
/** @} */

/**
 * @name Tap mode
 * @{
 */
#define IIS2DLPC_DT_SINGLE_TAP			0 /**< Single-tap only */
#define IIS2DLPC_DT_SINGLE_DOUBLE_TAP		1 /**< Single-tap and double-tap */
/** @} */

/**
 * @name Free-Fall threshold
 * @{
 */
#define IIS2DLPC_DT_FF_THRESHOLD_156_mg		0 /**< 156 mg */
#define IIS2DLPC_DT_FF_THRESHOLD_219_mg		1 /**< 219 mg */
#define IIS2DLPC_DT_FF_THRESHOLD_250_mg		2 /**< 250 mg */
#define IIS2DLPC_DT_FF_THRESHOLD_312_mg		3 /**< 312 mg */
#define IIS2DLPC_DT_FF_THRESHOLD_344_mg		4 /**< 344 mg */
#define IIS2DLPC_DT_FF_THRESHOLD_406_mg		5 /**< 406 mg */
#define IIS2DLPC_DT_FF_THRESHOLD_469_mg		6 /**< 469 mg */
#define IIS2DLPC_DT_FF_THRESHOLD_500_mg		7 /**< 500 mg */
/** @} */

/**
 * @name Wakeup duration
 * @{
 */
#define IIS2DLPC_DT_WAKEUP_1_ODR		0 /**< 1/ODR  */
#define IIS2DLPC_DT_WAKEUP_2_ODR		1 /**< 2/ODR */
#define IIS2DLPC_DT_WAKEUP_3_ODR		2 /**< 3/ODR */
#define IIS2DLPC_DT_WAKEUP_4_ODR		3 /**< 4/ODR */
/** @} */

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ST_IIS2DLPC_H_ */
