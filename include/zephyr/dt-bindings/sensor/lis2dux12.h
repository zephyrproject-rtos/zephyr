/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_LIS2DUX12_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_LIS2DUX12_H_

#include <zephyr/dt-bindings/dt-util.h>

/* Operating Mode */
#define LIS2DUX12_OPER_MODE_POWER_DOWN        0
#define LIS2DUX12_OPER_MODE_LOW_POWER         1
#define LIS2DUX12_OPER_MODE_HIGH_PERFORMANCE  2
#define LIS2DUX12_OPER_MODE_SINGLE_SHOT       3

/* Data rate */
#define LIS2DUX12_DT_ODR_OFF      0
#define LIS2DUX12_DT_ODR_1Hz_ULP  1  /* available in ultra-low power mode */
#define LIS2DUX12_DT_ODR_3Hz_ULP  2  /* available in ultra-low power mode */
#define LIS2DUX12_DT_ODR_25Hz_ULP 3  /* available in ultra-low power mode */
#define LIS2DUX12_DT_ODR_6Hz      4  /* available in LP and HP mode */
#define LIS2DUX12_DT_ODR_12Hz5    5  /* available in LP and HP mode */
#define LIS2DUX12_DT_ODR_25Hz     6  /* available in LP and HP mode */
#define LIS2DUX12_DT_ODR_50Hz     7  /* available in LP and HP mode */
#define LIS2DUX12_DT_ODR_100Hz    8  /* available in LP and HP mode */
#define LIS2DUX12_DT_ODR_200Hz    9  /* available in LP and HP mode */
#define LIS2DUX12_DT_ODR_400Hz    10 /* available in LP and HP mode */
#define LIS2DUX12_DT_ODR_800Hz    11 /* available in LP and HP mode */
#define LIS2DUX12_DT_ODR_END      12

/* Accelerometer Full-scale */
#define LIS2DUX12_DT_FS_2G  0 /* 2g (0.061 mg/LSB)  */
#define LIS2DUX12_DT_FS_4G  1 /* 4g (0.122 mg/LSB)  */
#define LIS2DUX12_DT_FS_8G  2 /* 8g (0.244 mg/LSB)  */
#define LIS2DUX12_DT_FS_16G 3 /* 16g (0.488 mg/LSB) */

/* Accelerometer FIFO batching data rate */
#define LIS2DUX12_DT_BDR_XL_ODR        0x0
#define LIS2DUX12_DT_BDR_XL_ODR_DIV_2  0x1
#define LIS2DUX12_DT_BDR_XL_ODR_DIV_4  0x2
#define LIS2DUX12_DT_BDR_XL_ODR_DIV_8  0x3
#define LIS2DUX12_DT_BDR_XL_ODR_DIV_16 0x4
#define LIS2DUX12_DT_BDR_XL_ODR_DIV_32 0x5
#define LIS2DUX12_DT_BDR_XL_ODR_DIV_64 0x6
#define LIS2DUX12_DT_BDR_XL_ODR_OFF    0x7

/* Accelerometer FIFO timestamp ratio */
#define LIS2DUX12_DT_DEC_TS_OFF  0x0
#define LIS2DUX12_DT_DEC_TS_1    0x1
#define LIS2DUX12_DT_DEC_TS_8    0x2
#define LIS2DUX12_DT_DEC_TS_32   0x3

/* Accelerometer FIFO tags (aligned with lis2dux12_fifo_sensor_tag_t) */
#define LIS2DUXXX_FIFO_EMPTY         0x0
#define LIS2DUXXX_XL_TEMP_TAG        0x2
#define LIS2DUXXX_XL_ONLY_2X_TAG     0x3
#define LIS2DUXXX_TIMESTAMP_TAG      0x4
#define LIS2DUXXX_STEP_COUNTER_TAG   0x12
#define LIS2DUXXX_MLC_RESULT_TAG     0x1A
#define LIS2DUXXX_MLC_FILTER_TAG     0x1B
#define LIS2DUXXX_MLC_FEATURE        0x1C
#define LIS2DUXXX_FSM_RESULT_TAG     0x1D

/* Accelerometer FIFO modes (aligned with lis2dux12_operation_t) */
#define LIS2DUXXX_DT_BYPASS_MODE            0x0
#define LIS2DUXXX_DT_FIFO_MODE              0x1
#define LIS2DUXXX_DT_STREAM_TO_FIFO_MODE    0x3
#define LIS2DUXXX_DT_BYPASS_TO_STREAM_MODE  0x4
#define LIS2DUXXX_DT_STREAM_MODE            0x6
#define LIS2DUXXX_DT_BYPASS_TO_FIFO_MODE    0x7
#define LIS2DUXXX_DT_FIFO_OFF               0x8

/* Accelerometer registers */
#define LIS2DUXXX_DT_FIFO_CTRL           0x15U
#define LIS2DUXXX_DT_STATUS              0x25U
#define LIS2DUXXX_DT_FIFO_STATUS1        0x26U
#define LIS2DUXXX_DT_OUTX_L              0x28U
#define LIS2DUXXX_DT_FIFO_DATA_OUT_TAG   0x40U

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_LIS2DUX12_H_ */
