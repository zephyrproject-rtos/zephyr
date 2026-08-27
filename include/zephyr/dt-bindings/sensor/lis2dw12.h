/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the ST LIS2DW12 accelerometer.
 * @ingroup lis2dw12_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_LIS2DW12_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_LIS2DW12_H_

/**
 * @defgroup lis2dw12_interface LIS2DW12
 * @ingroup sensor_interface_ext_st
 * @brief STMicroelectronics LIS2DW12 3-axis accelerometer
 * @{
 */

/**
 * @name Power modes
 *
 * Values for the `power-mode` devicetree property.
 * @{
 */
#define LIS2DW12_DT_LP_M1			0 /**< Low-power mode 1 (12-bit) */
#define LIS2DW12_DT_LP_M2			1 /**< Low-power mode 2 (14-bit) */
#define LIS2DW12_DT_LP_M3			2 /**< Low-power mode 3 (14-bit) */
#define LIS2DW12_DT_LP_M4			3 /**< Low-power mode 4 (14-bit) */
#define LIS2DW12_DT_HP_MODE			4 /**< High-performance mode (14-bit) */
/** @} */

/**
 * @name Filter bandwidth options
 *
 * Values for the `bw-filt` devicetree property.
 * @{
 */
#define LIS2DW12_DT_FILTER_BW_ODR_DIV_2		0 /**< ODR / 2 */
#define LIS2DW12_DT_FILTER_BW_ODR_DIV_4		1 /**< ODR / 4 */
#define LIS2DW12_DT_FILTER_BW_ODR_DIV_10	2 /**< ODR / 10 */
#define LIS2DW12_DT_FILTER_BW_ODR_DIV_20	3 /**< ODR / 20 */
/** @} */

/**
 * @name Tap detection mode
 *
 * Values for the `tap-mode` devicetree property.
 * @{
 */
#define LIS2DW12_DT_SINGLE_TAP			0 /**< Single-tap only */
#define LIS2DW12_DT_SINGLE_DOUBLE_TAP		1 /**< Single and double tap */
/** @} */

/**
 * @name Free-fall threshold options
 *
 * Values for the `ff-threshold` devicetree property.
 * @{
 */
#define LIS2DW12_DT_FF_THRESHOLD_156_mg		0 /**< 156 mg */
#define LIS2DW12_DT_FF_THRESHOLD_219_mg		1 /**< 219 mg */
#define LIS2DW12_DT_FF_THRESHOLD_250_mg		2 /**< 250 mg */
#define LIS2DW12_DT_FF_THRESHOLD_312_mg		3 /**< 312 mg */
#define LIS2DW12_DT_FF_THRESHOLD_344_mg		4 /**< 344 mg */
#define LIS2DW12_DT_FF_THRESHOLD_406_mg		5 /**< 406 mg */
#define LIS2DW12_DT_FF_THRESHOLD_469_mg		6 /**< 469 mg */
#define LIS2DW12_DT_FF_THRESHOLD_500_mg		7 /**< 500 mg */
/** @} */

/**
 * @name Wake-up duration options
 *
 * Values for the `wakeup-duration` devicetree property.
 * @{
 */
#define LIS2DW12_DT_WAKEUP_1_ODR		0 /**< 1 ODR period */
#define LIS2DW12_DT_WAKEUP_2_ODR		1 /**< 2 ODR periods */
#define LIS2DW12_DT_WAKEUP_3_ODR		2 /**< 3 ODR periods */
#define LIS2DW12_DT_WAKEUP_4_ODR		3 /**< 4 ODR periods */
/** @} */

/**
 * @name Sleep duration options
 *
 * Values for the `sleep-duration` devicetree property.
 * @{
 */
#define LIS2DW12_DT_SLEEP_0_ODR		    0  /**< 0 ODR periods */
#define LIS2DW12_DT_SLEEP_1_ODR		    1  /**< 1 ODR period */
#define LIS2DW12_DT_SLEEP_2_ODR		    2  /**< 2 ODR periods */
#define LIS2DW12_DT_SLEEP_3_ODR		    3  /**< 3 ODR periods */
#define LIS2DW12_DT_SLEEP_4_ODR		    4  /**< 4 ODR periods */
#define LIS2DW12_DT_SLEEP_5_ODR		    5  /**< 5 ODR periods */
#define LIS2DW12_DT_SLEEP_6_ODR		    6  /**< 6 ODR periods */
#define LIS2DW12_DT_SLEEP_7_ODR		    7  /**< 7 ODR periods */
#define LIS2DW12_DT_SLEEP_8_ODR		    8  /**< 8 ODR periods */
#define LIS2DW12_DT_SLEEP_9_ODR		    9  /**< 9 ODR periods */
#define LIS2DW12_DT_SLEEP_10_ODR		10 /**< 10 ODR periods */
#define LIS2DW12_DT_SLEEP_11_ODR		11 /**< 11 ODR periods */
#define LIS2DW12_DT_SLEEP_12_ODR		12 /**< 12 ODR periods */
#define LIS2DW12_DT_SLEEP_13_ODR		13 /**< 13 ODR periods */
#define LIS2DW12_DT_SLEEP_14_ODR		14 /**< 14 ODR periods */
#define LIS2DW12_DT_SLEEP_15_ODR		15 /**< 15 ODR periods */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_LIS2DW12_H_ */
