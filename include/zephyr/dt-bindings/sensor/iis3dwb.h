/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the ST IIS3DWB accelerometer.
 * @ingroup iis3dwb_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_IIS3DWB_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_IIS3DWB_H_

#include <zephyr/dt-bindings/dt-util.h>

/**
 * @defgroup iis3dwb_interface IIS3DWB
 * @ingroup sensor_interface_ext_st
 * @brief STMicroelectronics IIS3DWB 3-axis vibration accelerometer
 * @{
 */

/**
 * @name Output data rate options
 *
 * Values for the `odr` devicetree property.
 * @{
 */
#define IIS3DWB_DT_ODR_OFF      0 /**< Power-down */
#define IIS3DWB_DT_ODR_26k7Hz   5 /**< 26.7 kHz */
/** @} */

/**
 * @name Accelerometer full-scale range
 *
 * Values for the `range` devicetree property.
 * @{
 */
#define IIS3DWB_DT_FS_2G  0 /**< ±2 g (0.061 mg/LSB) */
#define IIS3DWB_DT_FS_16G 1 /**< ±16 g (0.488 mg/LSB) */
#define IIS3DWB_DT_FS_4G  2 /**< ±4 g (0.122 mg/LSB) */
#define IIS3DWB_DT_FS_8G  3 /**< ±8 g (0.244 mg/LSB) */
/** @} */

/**
 * @name Filter path options
 *
 * Values for the `filter` devicetree property.
 *
 * Select the digital filter applied to the accelerometer output: slope,
 * high-pass (HP) or low-pass (LP), with the corner expressed as a fraction of
 * the output data rate (ODR).
 * @{
 */
#define IIS3DWB_DT_SLOPE_ODR_DIV_4  0x10 /**< Slope filter, ODR / 4 */
#define IIS3DWB_DT_HP_REF_MODE      0x37 /**< High-pass reference mode */
#define IIS3DWB_DT_HP_ODR_DIV_10    0x11 /**< High-pass, ODR / 10 */
#define IIS3DWB_DT_HP_ODR_DIV_20    0x12 /**< High-pass, ODR / 20 */
#define IIS3DWB_DT_HP_ODR_DIV_45    0x13 /**< High-pass, ODR / 45 */
#define IIS3DWB_DT_HP_ODR_DIV_100   0x14 /**< High-pass, ODR / 100 */
#define IIS3DWB_DT_HP_ODR_DIV_200   0x15 /**< High-pass, ODR / 200 */
#define IIS3DWB_DT_HP_ODR_DIV_400   0x16 /**< High-pass, ODR / 400 */
#define IIS3DWB_DT_HP_ODR_DIV_800   0x17 /**< High-pass, ODR / 800 */
#define IIS3DWB_DT_LP_6k3Hz         0x00 /**< Low-pass, 6.3 kHz */
#define IIS3DWB_DT_LP_ODR_DIV_4     0x80 /**< Low-pass, ODR / 4 */
#define IIS3DWB_DT_LP_ODR_DIV_10    0x81 /**< Low-pass, ODR / 10 */
#define IIS3DWB_DT_LP_ODR_DIV_20    0x82 /**< Low-pass, ODR / 20 */
#define IIS3DWB_DT_LP_ODR_DIV_45    0x83 /**< Low-pass, ODR / 45 */
#define IIS3DWB_DT_LP_ODR_DIV_100   0x84 /**< Low-pass, ODR / 100 */
#define IIS3DWB_DT_LP_ODR_DIV_200   0x85 /**< Low-pass, ODR / 200 */
#define IIS3DWB_DT_LP_ODR_DIV_400   0x86 /**< Low-pass, ODR / 400 */
#define IIS3DWB_DT_LP_ODR_DIV_800   0x87 /**< Low-pass, ODR / 800 */
/** @} */

/**
 * @name Accelerometer FIFO batching rate
 *
 * Values for the `accel-fifo-batch-rate` devicetree property.
 * @{
 */
#define IIS3DWB_DT_XL_NOT_BATCHED        0x0 /**< Not batched */
#define IIS3DWB_DT_XL_BATCHED_AT_26k7Hz  0xA /**< 26.7 kHz */
/** @} */

/**
 * @name Temperature FIFO batching rate
 *
 * Values for the `temp-fifo-batch-rate` devicetree property.
 * @{
 */
#define IIS3DWB_DT_TEMP_NOT_BATCHED      0 /**< Not batched */
#define IIS3DWB_DT_TEMP_BATCHED_AT_104Hz 3 /**< 104 Hz */
/** @} */

/**
 * @name FIFO timestamp decimation
 *
 * Values for the `timestamp-fifo-batch-rate` devicetree property.
 * @{
 */
#define IIS3DWB_DT_TS_NOT_BATCHED 0x0 /**< Timestamp not batched */
#define IIS3DWB_DT_DEC_TS_1       0x1 /**< Decimation by 1 */
#define IIS3DWB_DT_DEC_TS_8       0x2 /**< Decimation by 8 */
#define IIS3DWB_DT_DEC_TS_32      0x3 /**< Decimation by 32 */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_IIS3DWB_H_ */
