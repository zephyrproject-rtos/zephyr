/*
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the Broadcom AFBR-S50 ToF sensor.
 * @ingroup afbr_s50_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_AFBR_S50_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_AFBR_S50_H_

/**
 * @defgroup afbr_s50_interface AFBR-S50
 * @ingroup sensor_interface_ext_brcm
 * @brief Broadcom AFBR-S50 time-of-flight distance sensor
 * @{
 */

/**
 * @name Dual-frequency mode options
 *
 * Values for the `dual-freq-mode` devicetree property.
 *
 * Dual-frequency mode extends the unambiguous range by combining measurements
 * at two modulation frequencies.
 * @{
 */
#define AFBR_S50_DT_DFM_MODE_OFF		0 /**< Disabled */
#define AFBR_S50_DT_DFM_MODE_4X			1 /**< 4x range extension */
#define AFBR_S50_DT_DFM_MODE_8X			2 /**< 8x range extension */
/** @} */

/**
 * @name Measurement modes
 *
 * Values for the `measurement-mode` devicetree property.
 *
 * Modes with the laser-class-2 suffix raise the optical output power.
 * @{
 */
/** Short range */
#define AFBR_S50_DT_MODE_SHORT_RANGE					1
/** Long range */
#define AFBR_S50_DT_MODE_LONG_RANGE					2
/** High-speed short range */
#define AFBR_S50_DT_MODE_HIGH_SPEED_SHORT_RANGE				5
/** High-speed long range */
#define AFBR_S50_DT_MODE_HIGH_SPEED_LONG_RANGE				6
/** High-precision short range */
#define AFBR_S50_DT_MODE_HIGH_PRECISION_SHORT_RANGE			9
/** Short range, laser class 2 */
#define AFBR_S50_DT_MODE_SHORT_RANGE_LASER_CLASS_2			129
/** Long range, laser class 2 */
#define AFBR_S50_DT_MODE_LONG_RANGE_LASER_CLASS_2			130
/** High-speed short range, laser class 2 */
#define AFBR_S50_DT_MODE_HIGH_SPEED_SHORT_RANGE_LASER_CLASS_2		133
/** High-speed long range, laser class 2 */
#define AFBR_S50_DT_MODE_HIGH_SPEED_LONG_RANGE_LASER_CLASS_2		134
/** High-precision short range, laser class 2 */
#define AFBR_S50_DT_MODE_HIGH_PRECISION_SHORT_RANGE_LASER_CLASS_2	137
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_AFBR_S50_H_ */
