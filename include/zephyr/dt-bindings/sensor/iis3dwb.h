/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_IIS3DWB_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_IIS3DWB_H_

#include <zephyr/dt-bindings/dt-util.h>

/* Data rate */
#define IIS3DWB_DT_ODR_OFF      0
#define IIS3DWB_DT_ODR_26k7Hz   5  /* available in LP and HP mode */

/* Accelerometer Full-scale */
#define IIS3DWB_DT_FS_2G  0 /* 2g (0.061 mg/LSB)  */
#define IIS3DWB_DT_FS_16G 1 /* 16g (0.488 mg/LSB) */
#define IIS3DWB_DT_FS_4G  2 /* 4g (0.122 mg/LSB)  */
#define IIS3DWB_DT_FS_8G  3 /* 8g (0.244 mg/LSB)  */

/* filter settings */
#define IIS3DWB_DT_SLOPE_ODR_DIV_4  0x10
#define IIS3DWB_DT_HP_REF_MODE      0x37
#define IIS3DWB_DT_HP_ODR_DIV_10    0x11
#define IIS3DWB_DT_HP_ODR_DIV_20    0x12
#define IIS3DWB_DT_HP_ODR_DIV_45    0x13
#define IIS3DWB_DT_HP_ODR_DIV_100   0x14
#define IIS3DWB_DT_HP_ODR_DIV_200   0x15
#define IIS3DWB_DT_HP_ODR_DIV_400   0x16
#define IIS3DWB_DT_HP_ODR_DIV_800   0x17
#define IIS3DWB_DT_LP_6k3Hz         0x00
#define IIS3DWB_DT_LP_ODR_DIV_4     0x80
#define IIS3DWB_DT_LP_ODR_DIV_10    0x81
#define IIS3DWB_DT_LP_ODR_DIV_20    0x82
#define IIS3DWB_DT_LP_ODR_DIV_45    0x83
#define IIS3DWB_DT_LP_ODR_DIV_100   0x84
#define IIS3DWB_DT_LP_ODR_DIV_200   0x85
#define IIS3DWB_DT_LP_ODR_DIV_400   0x86
#define IIS3DWB_DT_LP_ODR_DIV_800   0x87

/* Accelerometer FIFO batching data rate */
#define IIS3DWB_DT_XL_NOT_BATCHED        0x0
#define IIS3DWB_DT_XL_BATCHED_AT_26k7Hz  0xA

/* Temperature FIFO batching data rate */
#define IIS3DWB_DT_TEMP_NOT_BATCHED      0
#define IIS3DWB_DT_TEMP_BATCHED_AT_104Hz 3

/* Accelerometer FIFO timestamp ratio */
#define IIS3DWB_DT_TS_NOT_BATCHED 0x0
#define IIS3DWB_DT_DEC_TS_1       0x1
#define IIS3DWB_DT_DEC_TS_8       0x2
#define IIS3DWB_DT_DEC_TS_32      0x3

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_IIS3DWB_H_ */
