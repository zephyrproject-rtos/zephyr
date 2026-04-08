/*
 * Copyright (c) 2026 Analog Devices.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree bindings for the Analog Devices ADXL355 accelerometer
 *
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_ADXL355_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_ADXL355_H_

/**
 * @defgroup adxl355 ADXL355 DT Options
 * @ingroup sensor_interface
 * @{
 */

/**
 * @defgroup adxl355_odr Output Rate options
 * @{
 */
/** Output data rate: 4000 Hz */
#define ADXL355_DT_ODR_4000   0
/** Output data rate: 2000 Hz */
#define ADXL355_DT_ODR_2000   1
/** Output data rate: 1000 Hz */
#define ADXL355_DT_ODR_1000   2
/** Output data rate: 500 Hz */
#define ADXL355_DT_ODR_500    3
/** Output data rate: 250 Hz */
#define ADXL355_DT_ODR_250    4
/** Output data rate: 125 Hz */
#define ADXL355_DT_ODR_125    5
/** Output data rate: 62.5 Hz */
#define ADXL355_DT_ODR_62_5   6
/** Output data rate: 31.25 Hz */
#define ADXL355_DT_ODR_31_25  7
/** Output data rate: 15.625 Hz */
#define ADXL355_DT_ODR_15_625 8
/** Output data rate: 7.813 Hz */
#define ADXL355_DT_ODR_7_813  9
/** Output data rate: 3.906 Hz */
#define ADXL355_DT_ODR_3_906  10
/** @} */

/**
 * @defgroup adxl355_range Select range options
 * @{
 */
/** Select ±2 g measurement range */
#define ADXL355_DT_RANGE_2G 1
/** Select ±4 g measurement range */
#define ADXL355_DT_RANGE_4G 2
/** Select ±8 g measurement range */
#define ADXL355_DT_RANGE_8G 3
/** @} */

/**
 * @defgroup adxl355_hpf_corner High Pass Filter corner options
 * @{
 */
/** High-pass filter disabled */
#define ADXL355_DT_HPF_OFF       0
/** High-pass filter corner at 24.7e-4 times ODR */
#define ADXL355_DT_HPF_24_7e_4   1
/** High-pass filter corner at 6.208e-4 times ODR */
#define ADXL355_DT_HPF_6_208e_4  2
/** High-pass filter corner at 1.5545e-4 times ODR */
#define ADXL355_DT_HPF_1_5545e_4 3
/** High-pass filter corner at 0.3862e-4 times ODR */
#define ADXL355_DT_HPF_0_3862e_4 4
/** High-pass filter corner at 0.0954e-4 times ODR */
#define ADXL355_DT_HPF_0_0954e_4 5
/** High-pass filter corner at 0.0238e-4 times ODR */
#define ADXL355_DT_HPF_0_0238e_4 6
/** @} */

/**
 * @defgroup adxl355_ext_clk External Clock options
 * @{
 */
/** External clock disabled */
#define ADXL355_DT_EXTCLK_DISABLED 0
/** External clock enabled */
#define ADXL355_DT_EXTCLK_ENABLED  1
/** @} */

/**
 * @defgroup adxl355_ext_sync External Sync options
 * @{
 */
/** Internal synchronization */
#define ADXL355_DT_INTERNAL_SYNC                    0
/** External synchronization without interpolation */
#define ADXL355_DT_EXTERNAL_SYNC_NO_INTERPOLATION   1
/** External synchronization with interpolation */
#define ADXL355_DT_EXTERNAL_SYNC_WITH_INTERPOLATION 2
/** @} */

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_ADXL355_H_ */
