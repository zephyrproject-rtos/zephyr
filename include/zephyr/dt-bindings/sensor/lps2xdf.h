/*
 * Copyright (c) 2023 STMicroelectronics
 * Copyright (c) 2023 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the ST LPS2xDF pressure sensors.
 * @ingroup lps2xdf_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ST_LPS2xDF_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ST_LPS2xDF_H_

/**
 * @defgroup lps2xdf_interface LPS2xDF
 * @ingroup sensor_interface_ext_st
 * @brief STMicroelectronics LPS2xDF pressure and temperature sensors
 * @{
 */

/**
 * @name Output data rate options
 *
 * Values for the `odr` devicetree property.
 * @{
 */
#define LPS2xDF_DT_ODR_POWER_DOWN		0 /**< Power-down */
#define LPS2xDF_DT_ODR_1HZ			1 /**< 1 Hz */
#define LPS2xDF_DT_ODR_4HZ			2 /**< 4 Hz */
#define LPS2xDF_DT_ODR_10HZ			3 /**< 10 Hz */
#define LPS2xDF_DT_ODR_25HZ			4 /**< 25 Hz */
#define LPS2xDF_DT_ODR_50HZ			5 /**< 50 Hz */
#define LPS2xDF_DT_ODR_75HZ			6 /**< 75 Hz */
#define LPS2xDF_DT_ODR_100HZ			7 /**< 100 Hz */
#define LPS2xDF_DT_ODR_200HZ			8 /**< 200 Hz */
/** @} */

/**
 * @name Low-pass filter options
 *
 * Values for the `lpf` devicetree property.
 * @{
 */
#define LPS2xDF_DT_LP_FILTER_OFF		0 /**< Filter disabled */
#define LPS2xDF_DT_LP_FILTER_ODR_4		1 /**< ODR / 4 */
#define LPS2xDF_DT_LP_FILTER_ODR_9		3 /**< ODR / 9 */
/** @} */

/**
 * @name Averaging (number of samples) options
 *
 * Values for the `avg` devicetree property.
 * @{
 */
#define LPS2xDF_DT_AVG_4_SAMPLES		0 /**< 4 samples */
#define LPS2xDF_DT_AVG_8_SAMPLES		1 /**< 8 samples */
#define LPS2xDF_DT_AVG_16_SAMPLES		2 /**< 16 samples */
#define LPS2xDF_DT_AVG_32_SAMPLES		3 /**< 32 samples */
#define LPS2xDF_DT_AVG_64_SAMPLES		4 /**< 64 samples */
#define LPS2xDF_DT_AVG_128_SAMPLES		5 /**< 128 samples */
#define LPS2xDF_DT_AVG_256_SAMPLES		6 /**< 256 samples */
#define LPS2xDF_DT_AVG_512_SAMPLES		7 /**< 512 samples */
/** @} */

/**
 * @name Full-scale pressure mode options
 *
 * Values for the `fs` devicetree property.
 * @{
 */
#define LPS28DFW_DT_FS_MODE_1_1260		0 /**< LPS28DFW mode 1 (1260 hPa) */
#define LPS28DFW_DT_FS_MODE_2_4060		1 /**< LPS28DFW mode 2 (4060 hPa) */
#define ILPS22QS_DT_FS_MODE_1_1260		0 /**< ILPS22QS mode 1 (1260 hPa) */
#define ILPS22QS_DT_FS_MODE_2_4060		1 /**< ILPS22QS mode 2 (4060 hPa) */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ST_LPS22DF_H_ */
