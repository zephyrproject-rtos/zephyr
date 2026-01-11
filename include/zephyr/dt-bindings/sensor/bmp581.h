/*
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for BMP581 sensor Devicetree constants
 * @ingroup bmp581_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_BOSCH_BMP581_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_BOSCH_BMP581_H_

/**
 * @addtogroup bmp581_interface
 * @{
 */

/**
 * @defgroup bmp581_power_modes Sensor power modes
 * @{
 */
#define BMP581_DT_MODE_NORMAL		1 /**< NORMAL mode */
#define BMP581_DT_MODE_FORCED		2 /**< FORCED mode */
#define BMP581_DT_MODE_CONTINUOUS	3 /**< CONTINUOUS mode */
/** @} */

/**
 * @defgroup bmp581_output_data_rate Output data rate options
 * @{
 */
#define BMP581_DT_ODR_240_HZ		0x00 /**< 240 Hz */
#define BMP581_DT_ODR_218_5_HZ		0x01 /**< 218.5 Hz */
#define BMP581_DT_ODR_199_1_HZ		0x02 /**< 199.1 Hz */
#define BMP581_DT_ODR_179_2_HZ		0x03 /**< 179.2 Hz */
#define BMP581_DT_ODR_160_HZ		0x04 /**< 160 Hz */
#define BMP581_DT_ODR_149_3_HZ		0x05 /**< 149.3 Hz */
#define BMP581_DT_ODR_140_HZ		0x06 /**< 140 Hz */
#define BMP581_DT_ODR_129_8_HZ		0x07 /**< 129.8 Hz */
#define BMP581_DT_ODR_120_HZ		0x08 /**< 120 Hz */
#define BMP581_DT_ODR_110_1_HZ		0x09 /**< 110.1 Hz */
#define BMP581_DT_ODR_100_2_HZ		0x0A /**< 100.2 Hz */
#define BMP581_DT_ODR_89_6_HZ		0x0B /**< 89.6 Hz */
#define BMP581_DT_ODR_80_HZ		0x0C /**< 80 Hz */
#define BMP581_DT_ODR_70_HZ		0x0D /**< 70 Hz */
#define BMP581_DT_ODR_60_HZ		0x0E /**< 60 Hz */
#define BMP581_DT_ODR_50_HZ		0x0F /**< 50 Hz */
#define BMP581_DT_ODR_45_HZ		0x10 /**< 45 Hz */
#define BMP581_DT_ODR_40_HZ		0x11 /**< 40 Hz */
#define BMP581_DT_ODR_35_HZ		0x12 /**< 35 Hz */
#define BMP581_DT_ODR_30_HZ		0x13 /**< 30 Hz */
#define BMP581_DT_ODR_25_HZ		0x14 /**< 25 Hz */
#define BMP581_DT_ODR_20_HZ		0x15 /**< 20 Hz */
#define BMP581_DT_ODR_15_HZ		0x16 /**< 15 Hz */
#define BMP581_DT_ODR_10_HZ		0x17 /**< 10 Hz */
#define BMP581_DT_ODR_5_HZ		0x18 /**< 5 Hz */
#define BMP581_DT_ODR_4_HZ		0x19 /**< 4 Hz */
#define BMP581_DT_ODR_3_HZ		0x1A /**< 3 Hz */
#define BMP581_DT_ODR_2_HZ		0x1B /**< 2 Hz */
#define BMP581_DT_ODR_1_HZ		0x1C /**< 1 Hz */
#define BMP581_DT_ODR_0_5_HZ		0x1D /**< 0.5 Hz */
#define BMP581_DT_ODR_0_250_HZ		0x1E /**< 0.250 Hz */
#define BMP581_DT_ODR_0_125_HZ		0x1F /**< 0.125 Hz */
/** @} */

/**
 * @defgroup bmp581_oversampling Oversampling options.
 *
 * Valid values for temperature and pressure sensor oversampling ratio.
 * @{
 */
#define BMP581_DT_OVERSAMPLING_1X	0x00 /**< x1 */
#define BMP581_DT_OVERSAMPLING_2X	0x01 /**< x2 */
#define BMP581_DT_OVERSAMPLING_4X	0x02 /**< x4 */
#define BMP581_DT_OVERSAMPLING_8X	0x03 /**< x8 */
#define BMP581_DT_OVERSAMPLING_16X	0x04 /**< x16 */
#define BMP581_DT_OVERSAMPLING_32X	0x05 /**< x32 */
#define BMP581_DT_OVERSAMPLING_64X	0x06 /**< x64 */
#define BMP581_DT_OVERSAMPLING_128X	0x07 /**< x128 */
/** @} */

/**
 * @defgroup bmp581_iir_filter IIR Filter options.
 *
 * Valid values for temperature and pressure IIR filter coefficient.
 *
 * @f[
 *   data_n = \frac{data_{n-1} \times filtercoefficient + data_{in}}{filtercoefficient + 1}
 * @f]
 *
 * where:
 * - @f$ data_n @f$ is the current filtered output
 * - @f$ data_{n-1} @f$ is the previous filtered output
 * - @f$ data_{in} @f$ is the current input sample
 * - @f$ filtercoefficient @f$ is one of the coefficient values below
 *
 * Higher coefficient values provide more filtering (smoother output) but increase response time.
 *
 * @{
 */
#define BMP581_DT_IIR_FILTER_BYPASS	0x00 /**< Bypass */
#define BMP581_DT_IIR_FILTER_COEFF_1	0x01 /**< 1 */
#define BMP581_DT_IIR_FILTER_COEFF_3	0x02 /**< 3 */
#define BMP581_DT_IIR_FILTER_COEFF_7	0x03 /**< 7 */
#define BMP581_DT_IIR_FILTER_COEFF_15	0x04 /**< 15 */
#define BMP581_DT_IIR_FILTER_COEFF_31	0x05 /**< 31 */
#define BMP581_DT_IIR_FILTER_COEFF_63	0x06 /**< 63 */
#define BMP581_DT_IIR_FILTER_COEFF_127	0x07 /**< 127 */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_BOSCH_BMP581_H_*/
