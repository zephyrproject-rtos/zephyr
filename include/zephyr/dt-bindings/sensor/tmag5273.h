/*
 * Copyright (c) 2023 deveritec GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the TI TMAG5273 3-axis Hall-effect sensor.
 * @ingroup tmag5273_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_TMAG5273_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_TMAG5273_H_

#include <zephyr/dt-bindings/dt-util.h>

/**
 * @addtogroup tmag5273_interface
 * @{
 */

/**
 * @name Operating mode
 *
 * Values for the `operation-mode` devicetree property.
 * @{
 */
#define TMAG5273_DT_OPER_MODE_CONTINUOUS 0 /**< Continuous conversion */
#define TMAG5273_DT_OPER_MODE_STANDBY    1 /**< Standby (conversion on request) */
/** @} */

/**
 * @name Magnetic axis selection
 *
 * Values for the `axis` devicetree property.
 * @{
 */
#define TMAG5273_DT_AXIS_NONE 0x0 /**< No axis */
#define TMAG5273_DT_AXIS_X    0x1 /**< X axis */
#define TMAG5273_DT_AXIS_Y    0x2 /**< Y axis */
#define TMAG5273_DT_AXIS_Z    0x4 /**< Z axis */
/** X and Y axes */
#define TMAG5273_DT_AXIS_XY   (TMAG5273_DT_AXIS_X | TMAG5273_DT_AXIS_Y)
/** X and Z axes */
#define TMAG5273_DT_AXIS_XZ   (TMAG5273_DT_AXIS_X | TMAG5273_DT_AXIS_Z)
/** Y and Z axes */
#define TMAG5273_DT_AXIS_YZ   (TMAG5273_DT_AXIS_Y | TMAG5273_DT_AXIS_Z)
/** All axes */
#define TMAG5273_DT_AXIS_XYZ  (TMAG5273_DT_AXIS_X | TMAG5273_DT_AXIS_Y | TMAG5273_DT_AXIS_Z)
#define TMAG5273_DT_AXIS_XYX  0x8 /**< X, Y then X axes */
#define TMAG5273_DT_AXIS_YXY  0x9 /**< Y, X then Y axes */
#define TMAG5273_DT_AXIS_YZY  0xA /**< Y, Z then Y axes */
#define TMAG5273_DT_AXIS_XZX  0xB /**< X, Z then X axes */
/** @} */

/**
 * @name Magnetic measurement range
 *
 * Values for the `range` devicetree property.
 * @{
 */
#define TMAG5273_DT_AXIS_RANGE_LOW     0 /**< Low range */
#define TMAG5273_DT_AXIS_RANGE_HIGH    1 /**< High range */
#define TMAG5273_DT_AXIS_RANGE_RUNTIME 2 /**< Range selected at runtime */
/** @} */

/**
 * @name Interrupt routing mode
 *
 * Interrupt routing mode.
 * @{
 */
#define TMAG5273_DT_INT_THROUGH_INT         0 /**< Interrupt through the INT pin */
#define TMAG5273_DT_INT_THROUGH_INT_EXC_I2C 1 /**< INT pin, except during I2C bus access */
#define TMAG5273_DT_INT_THROUGH_SCL         2 /**< Interrupt through the SCL pin */
#define TMAG5273_DT_INT_THROUGH_SCL_EXC_I2C 3 /**< SCL pin, except during I2C bus access */
/** @} */

/**
 * @name Threshold crossing count
 *
 * Number of threshold crossings before an interrupt.
 * @{
 */
#define TMAG5273_DT_THRX_COUNT_1 0 /**< Trigger after 1 crossing */
#define TMAG5273_DT_THRX_COUNT_4 1 /**< Trigger after 4 crossings */
/** @} */

/**
 * @name Threshold crossing direction
 *
 * Threshold-crossing direction that triggers an interrupt.
 * @{
 */
#define TMAG5273_DT_THRX_ABOVE   0 /**< Field rises above the threshold */
#define TMAG5273_DT_THRX_BELOW   1 /**< Field falls below the threshold */
#define TMAG5273_DT_THRX_OUTSIDE 2 /**< Field leaves the threshold window */
#define TMAG5273_DT_THRX_INSIDE  3 /**< Field enters the threshold window */
/** @} */

/**
 * @name Magnet temperature coefficient
 *
 * Values for the `temperature-coefficient` devicetree property.
 * @{
 */
#define TMAG5273_DT_TEMP_COEFF_NONE    0 /**< No compensation */
#define TMAG5273_DT_TEMP_COEFF_NDBFE   1 /**< NdBFe magnet (0.12 %/°C) */
#define TMAG5273_DT_TEMP_COEFF_CERAMIC 2 /**< Ceramic magnet (0.2 %/°C) */
/** @} */

/**
 * @name Angle and magnitude calculation
 *
 * Values for the `angle-magnitude-axis` devicetree property.
 * @{
 */
#define TMAG5273_DT_ANGLE_MAG_NONE    0 /**< Disabled */
#define TMAG5273_DT_ANGLE_MAG_XY      1 /**< From the X and Y axes */
#define TMAG5273_DT_ANGLE_MAG_YZ      2 /**< From the Y and Z axes */
#define TMAG5273_DT_ANGLE_MAG_XZ      3 /**< From the X and Z axes */
#define TMAG5273_DT_ANGLE_MAG_RUNTIME 4 /**< Axis pair selected at runtime */
/** @} */

/**
 * @name Magnitude gain correction channel
 *
 * Values for the `ch-mag-gain-correction` devicetree property.
 * @{
 */
#define TMAG5273_DT_CORRECTION_CH_1 0 /**< Apply gain correction to channel 1 */
#define TMAG5273_DT_CORRECTION_CH_2 1 /**< Apply gain correction to channel 2 */
/** @} */

/**
 * @name Sample averaging
 *
 * Values for the `average-mode` devicetree property.
 * @{
 */
#define TMAG5273_DT_AVERAGING_NONE 0 /**< No averaging (1 sample) */
#define TMAG5273_DT_AVERAGING_2X   1 /**< 2 samples averaged */
#define TMAG5273_DT_AVERAGING_4X   2 /**< 4 samples averaged */
#define TMAG5273_DT_AVERAGING_8X   3 /**< 8 samples averaged */
#define TMAG5273_DT_AVERAGING_16X  4 /**< 16 samples averaged */
#define TMAG5273_DT_AVERAGING_32X  5 /**< 32 samples averaged */
/** @} */

/**
 * @name Sleep time between conversions
 *
 * Sleep time between conversions in standby mode.
 * @{
 */
#define TMAG5273_DT_SLEEPTIME_1MS     0  /**< 1 ms */
#define TMAG5273_DT_SLEEPTIME_5MS     1  /**< 5 ms */
#define TMAG5273_DT_SLEEPTIME_10MS    2  /**< 10 ms */
#define TMAG5273_DT_SLEEPTIME_15MS    3  /**< 15 ms */
#define TMAG5273_DT_SLEEPTIME_20MS    4  /**< 20 ms */
#define TMAG5273_DT_SLEEPTIME_30MS    5  /**< 30 ms */
#define TMAG5273_DT_SLEEPTIME_50MS    6  /**< 50 ms */
#define TMAG5273_DT_SLEEPTIME_100MS   7  /**< 100 ms */
#define TMAG5273_DT_SLEEPTIME_500MS   8  /**< 500 ms */
#define TMAG5273_DT_SLEEPTIME_1000MS  9  /**< 1000 ms */
#define TMAG5273_DT_SLEEPTIME_2000MS  10 /**< 2000 ms */
#define TMAG5273_DT_SLEEPTIME_5000MS  11 /**< 5000 ms */
#define TMAG5273_DT_SLEEPTIME_20000MS 12 /**< 20000 ms */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_TMAG5273_H_ */
