/*
 * Copyright (c) 2023 STMicroelectronics
 * Copyright (c) 2023 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ST_LPS2xDF_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ST_LPS2xDF_H_

/* Data rate */
#define LPS2xDF_DT_ODR_POWER_DOWN		0
#define LPS2xDF_DT_ODR_1HZ			1
#define LPS2xDF_DT_ODR_4HZ			2
#define LPS2xDF_DT_ODR_10HZ			3
#define LPS2xDF_DT_ODR_25HZ			4
#define LPS2xDF_DT_ODR_50HZ			5
#define LPS2xDF_DT_ODR_75HZ			6
#define LPS2xDF_DT_ODR_100HZ			7
#define LPS2xDF_DT_ODR_200HZ			8

/* Low Pass filter */
#define LPS2xDF_DT_LP_FILTER_OFF		0
#define LPS2xDF_DT_LP_FILTER_ODR_4		1
#define LPS2xDF_DT_LP_FILTER_ODR_9		3

/* Average (number of samples) filter */
#define LPS2xDF_DT_AVG_4_SAMPLES		0
#define LPS2xDF_DT_AVG_8_SAMPLES		1
#define LPS2xDF_DT_AVG_16_SAMPLES		2
#define LPS2xDF_DT_AVG_32_SAMPLES		3
#define LPS2xDF_DT_AVG_64_SAMPLES		4
#define LPS2xDF_DT_AVG_128_SAMPLES		5
#define LPS2xDF_DT_AVG_256_SAMPLES		6
#define LPS2xDF_DT_AVG_512_SAMPLES		7

/* Full Scale Pressure Mode */
#define LPS28DFW_DT_FS_MODE_1_1260		0
#define LPS28DFW_DT_FS_MODE_2_4060		1

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ST_LPS22DF_H_ */
