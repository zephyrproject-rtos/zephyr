/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the ST LIS2DUX12 accelerometer.
 * @ingroup lis2dux12_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_LIS2DUX12_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_LIS2DUX12_H_

#include <zephyr/dt-bindings/dt-util.h>

/**
 * @defgroup lis2dux12_interface LIS2DUX12
 * @ingroup sensor_interface_ext_st
 * @brief STMicroelectronics LIS2DUX12 3-axis accelerometer
 * @{
 */

/**
 * @name Operating mode
 *
 * Values for the `power-mode` devicetree property.
 * @{
 */
#define LIS2DUX12_OPER_MODE_POWER_DOWN        0 /**< Power-down */
#define LIS2DUX12_OPER_MODE_LOW_POWER         1 /**< Low-power mode */
#define LIS2DUX12_OPER_MODE_HIGH_PERFORMANCE  2 /**< High-performance mode */
#define LIS2DUX12_OPER_MODE_SINGLE_SHOT       3 /**< Single-shot mode */
/** @} */

/**
 * @name Output data rate options
 *
 * Values for the `odr` devicetree property.
 *
 * ULP: ultra-low-power, LP: low-power, HP: high-performance mode.
 * @{
 */
#define LIS2DUX12_DT_ODR_OFF      0  /**< Power-down */
#define LIS2DUX12_DT_ODR_1Hz_ULP  1  /**< 1 Hz (ULP mode only) */
#define LIS2DUX12_DT_ODR_3Hz_ULP  2  /**< 3 Hz (ULP mode only) */
#define LIS2DUX12_DT_ODR_25Hz_ULP 3  /**< 25 Hz (ULP mode only) */
#define LIS2DUX12_DT_ODR_6Hz      4  /**< 6 Hz (LP or HP mode) */
#define LIS2DUX12_DT_ODR_12Hz5    5  /**< 12.5 Hz (LP or HP mode) */
#define LIS2DUX12_DT_ODR_25Hz     6  /**< 25 Hz (LP or HP mode) */
#define LIS2DUX12_DT_ODR_50Hz     7  /**< 50 Hz (LP or HP mode) */
#define LIS2DUX12_DT_ODR_100Hz    8  /**< 100 Hz (LP or HP mode) */
#define LIS2DUX12_DT_ODR_200Hz    9  /**< 200 Hz (LP or HP mode) */
#define LIS2DUX12_DT_ODR_400Hz    10 /**< 400 Hz (LP or HP mode) */
#define LIS2DUX12_DT_ODR_800Hz    11 /**< 800 Hz (LP or HP mode) */
#define LIS2DUX12_DT_ODR_END      12 /**< Sentinel, one past the last ODR */
/** @} */

/**
 * @name Accelerometer full-scale range
 *
 * Values for the `range` devicetree property.
 * @{
 */
#define LIS2DUX12_DT_FS_2G  0 /**< ±2 g (0.061 mg/LSB) */
#define LIS2DUX12_DT_FS_4G  1 /**< ±4 g (0.122 mg/LSB) */
#define LIS2DUX12_DT_FS_8G  2 /**< ±8 g (0.244 mg/LSB) */
#define LIS2DUX12_DT_FS_16G 3 /**< ±16 g (0.488 mg/LSB) */
/** @} */

/**
 * @name Accelerometer FIFO batching data rate
 *
 * Values for the `accel-fifo-batch-rate` devicetree property.
 * @{
 */
#define LIS2DUX12_DT_BDR_XL_ODR        0x0 /**< Batched at ODR */
#define LIS2DUX12_DT_BDR_XL_ODR_DIV_2  0x1 /**< Batched at ODR / 2 */
#define LIS2DUX12_DT_BDR_XL_ODR_DIV_4  0x2 /**< Batched at ODR / 4 */
#define LIS2DUX12_DT_BDR_XL_ODR_DIV_8  0x3 /**< Batched at ODR / 8 */
#define LIS2DUX12_DT_BDR_XL_ODR_DIV_16 0x4 /**< Batched at ODR / 16 */
#define LIS2DUX12_DT_BDR_XL_ODR_DIV_32 0x5 /**< Batched at ODR / 32 */
#define LIS2DUX12_DT_BDR_XL_ODR_DIV_64 0x6 /**< Batched at ODR / 64 */
#define LIS2DUX12_DT_BDR_XL_ODR_OFF    0x7 /**< Not batched */
/** @} */

/**
 * @name Accelerometer FIFO timestamp decimation
 *
 * Values for the `timestamp-fifo-batch-rate` devicetree property.
 * @{
 */
#define LIS2DUX12_DT_DEC_TS_OFF  0x0 /**< Timestamp not batched */
#define LIS2DUX12_DT_DEC_TS_1    0x1 /**< Decimation by 1 */
#define LIS2DUX12_DT_DEC_TS_8    0x2 /**< Decimation by 8 */
#define LIS2DUX12_DT_DEC_TS_32   0x3 /**< Decimation by 32 */
/** @} */

/**
 * @name FIFO record tags
 *
 * FIFO record tag values reported in the FIFO data stream.
 * @{
 */
#define LIS2DUXXX_FIFO_EMPTY         0x0  /**< FIFO empty */
#define LIS2DUXXX_XL_TEMP_TAG        0x2  /**< Accelerometer and temperature */
#define LIS2DUXXX_XL_ONLY_2X_TAG     0x3  /**< Accelerometer only, 2x compressed */
#define LIS2DUXXX_TIMESTAMP_TAG      0x4  /**< Timestamp */
#define LIS2DUXXX_STEP_COUNTER_TAG   0x12 /**< Step counter */
#define LIS2DUXXX_MLC_RESULT_TAG     0x1A /**< Machine-learning core result */
#define LIS2DUXXX_MLC_FILTER_TAG     0x1B /**< Machine-learning core filter */
#define LIS2DUXXX_MLC_FEATURE        0x1C /**< Machine-learning core feature */
#define LIS2DUXXX_FSM_RESULT_TAG     0x1D /**< Finite-state-machine result */
/** @} */

/**
 * @name FIFO modes
 *
 * FIFO operating modes used by the driver.
 * @{
 */
#define LIS2DUXXX_DT_BYPASS_MODE            0x0 /**< Bypass (FIFO disabled) */
#define LIS2DUXXX_DT_FIFO_MODE              0x1 /**< FIFO mode */
#define LIS2DUXXX_DT_STREAM_TO_FIFO_MODE    0x3 /**< Stream-to-FIFO mode */
#define LIS2DUXXX_DT_BYPASS_TO_STREAM_MODE  0x4 /**< Bypass-to-stream mode */
#define LIS2DUXXX_DT_STREAM_MODE            0x6 /**< Continuous stream mode */
#define LIS2DUXXX_DT_BYPASS_TO_FIFO_MODE    0x7 /**< Bypass-to-FIFO mode */
#define LIS2DUXXX_DT_FIFO_OFF               0x8 /**< FIFO off */
/** @} */

/**
 * @name Register addresses
 *
 * Register addresses used by the driver.
 * @{
 */
#define LIS2DUXXX_DT_FIFO_CTRL           0x15U /**< FIFO_CTRL register */
#define LIS2DUXXX_DT_STATUS              0x25U /**< STATUS register */
#define LIS2DUXXX_DT_FIFO_STATUS1        0x26U /**< FIFO_STATUS1 register */
#define LIS2DUXXX_DT_OUTX_L              0x28U /**< OUTX_L output register */
#define LIS2DUXXX_DT_FIFO_DATA_OUT_TAG   0x40U /**< FIFO_DATA_OUT_TAG register */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_LIS2DUX12_H_ */
