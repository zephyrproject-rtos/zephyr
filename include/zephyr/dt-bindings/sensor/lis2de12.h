/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ST_LIS2DE12_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ST_LIS2DE12_H_

/* Accel range */
#define	LIS2DE12_DT_FS_2G			0
#define	LIS2DE12_DT_FS_4G			1
#define	LIS2DE12_DT_FS_8G			2
#define	LIS2DE12_DT_FS_16G			3

/* Accel rates */
#define LIS2DE12_DT_ODR_OFF			0x00 /* Power-Down */
#define LIS2DE12_DT_ODR_AT_1Hz			0x01 /* 1Hz (normal) */
#define LIS2DE12_DT_ODR_AT_10Hz			0x02 /* 10Hz (normal) */
#define LIS2DE12_DT_ODR_AT_25Hz			0x03 /* 25Hz (normal) */
#define LIS2DE12_DT_ODR_AT_50Hz			0x04 /* 50Hz (normal) */
#define LIS2DE12_DT_ODR_AT_100Hz		0x05 /* 100Hz (normal) */
#define LIS2DE12_DT_ODR_AT_200Hz		0x06 /* 200Hz (normal) */
#define LIS2DE12_DT_ODR_AT_400Hz		0x07 /* 400Hz (normal) */
#define LIS2DE12_DT_ODR_AT_1kHz620		0x08 /* 1KHz620 (normal) */
#define LIS2DE12_DT_ODR_AT_5kHz376		0x09 /* 5KHz376 (normal) */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ST_LIS2DE12_H_ */
