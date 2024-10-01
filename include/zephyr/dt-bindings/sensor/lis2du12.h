/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ST_LIS2DU12_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ST_LIS2DU12_H_

/* Accel range */
#define	LIS2DU12_DT_FS_2G			0
#define	LIS2DU12_DT_FS_4G			1
#define	LIS2DU12_DT_FS_8G			2
#define	LIS2DU12_DT_FS_16G			3

/* Accel rates */
#define LIS2DU12_DT_ODR_OFF			0x00 /* Power-Down */
#define LIS2DU12_DT_ODR_AT_1Hz6_ULP		0x01 /* 1Hz6 (ultra low power) */
#define LIS2DU12_DT_ODR_AT_3Hz_ULP		0x02 /* 3Hz (ultra low power) */
#define LIS2DU12_DT_ODR_AT_6Hz_ULP		0x03 /* 6Hz (ultra low power) */
#define LIS2DU12_DT_ODR_AT_6Hz			0x04 /* 6Hz (normal) */
#define LIS2DU12_DT_ODR_AT_12Hz			0x05 /* 12Hz5 (normal) */
#define LIS2DU12_DT_ODR_AT_25Hz			0x06 /* 25Hz (normal) */
#define LIS2DU12_DT_ODR_AT_50Hz			0x07 /* 50Hz (normal) */
#define LIS2DU12_DT_ODR_AT_100Hz		0x08 /* 100Hz (normal) */
#define LIS2DU12_DT_ODR_AT_200Hz		0x09 /* 200Hz (normal) */
#define LIS2DU12_DT_ODR_AT_400Hz		0x0a /* 400Hz (normal) */
#define LIS2DU12_DT_ODR_AT_800Hz		0x0b /* 800Hz (normal) */
#define LIS2DU12_DT_ODR_TRIG_PIN		0x0e /* Single-shot high latency by INT2 */
#define LIS2DU12_DT_ODR_TRIG_SW			0x0f /* Single-shot high latency by IF */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ST_LIS2DU12_H_ */
