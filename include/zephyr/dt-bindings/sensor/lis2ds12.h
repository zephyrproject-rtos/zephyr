/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ST_LIS2DS12_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ST_LIS2DS12_H_

/* power-modes */
#define LIS2DS12_DT_POWER_DOWN			0
#define LIS2DS12_DT_LOW_POWER			1
#define LIS2DS12_DT_HIGH_RESOLUTION		2
#define LIS2DS12_DT_HIGH_FREQUENCY		3

/* Data rate */
#define LIS2DS12_DT_ODR_OFF			0
#define LIS2DS12_DT_ODR_1Hz_LP			1  /* available in LP mode only */
#define LIS2DS12_DT_ODR_12Hz5			2  /* available in LP and HR mode */
#define LIS2DS12_DT_ODR_25Hz			3  /* available in LP and HR mode */
#define LIS2DS12_DT_ODR_50Hz			4  /* available in LP and HR mode */
#define LIS2DS12_DT_ODR_100Hz			5  /* available in LP and HR mode */
#define LIS2DS12_DT_ODR_200Hz			6  /* available in LP and HR mode */
#define LIS2DS12_DT_ODR_400Hz			7  /* available in LP and HR mode */
#define LIS2DS12_DT_ODR_800Hz			8  /* available in LP and HR mode */
#define LIS2DS12_DT_ODR_1600Hz			9  /* available in HF mode only */
#define LIS2DS12_DT_ODR_3200Hz_HF		10 /* available in HF mode only */
#define LIS2DS12_DT_ODR_6400Hz_HF		11 /* available in HF mode only */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ST_LIS2DS12_H_ */
