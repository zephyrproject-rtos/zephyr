/*
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for AFBR-S50 Devicetree constants
 * @ingroup afbr_s50_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_BRCM_AFBR_S50_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_BRCM_AFBR_S50_H_

/**
 * @defgroup afbr_s50_interface AFBR-S50
 * @ingroup sensor_interface_ext
 * @brief AFBR-S50 3D ToF sensor
 * @{
 */

/**
 * @name Dual Frequency Mode Settings
 *
 * Dual Frequency Mode is a feature that allows the sensor to use two different modulation
 * frequencies for the same measurement. This is useful to reduce the noise in the measurement.
 *
 * @{
 */
#define AFBR_S50_DT_DFM_MODE_OFF		0 /**< Off (single-frequency mode) */
#define AFBR_S50_DT_DFM_MODE_4X			1 /**< x4 mode */
#define AFBR_S50_DT_DFM_MODE_8X			2 /**< x8 mode */
/** @} */

/**
 * @name Measurement Modes
 * @{
 */

/** Short range mode */
#define AFBR_S50_DT_MODE_SHORT_RANGE					1
/** Long range mode */
#define AFBR_S50_DT_MODE_LONG_RANGE					2
/** High speed short range mode */
#define AFBR_S50_DT_MODE_HIGH_SPEED_SHORT_RANGE				5
/** High speed long range mode */
#define AFBR_S50_DT_MODE_HIGH_SPEED_LONG_RANGE				6
/** High precision short range mode */
#define AFBR_S50_DT_MODE_HIGH_PRECISION_SHORT_RANGE			9
/** Short range mode (laser class 2) */
#define AFBR_S50_DT_MODE_SHORT_RANGE_LASER_CLASS_2			129
/** Long range mode (laser class 2) */
#define AFBR_S50_DT_MODE_LONG_RANGE_LASER_CLASS_2			130
/** High speed short range mode (laser class 2) */
#define AFBR_S50_DT_MODE_HIGH_SPEED_SHORT_RANGE_LASER_CLASS_2		133
/** High speed long range mode (laser class 2) */
#define AFBR_S50_DT_MODE_HIGH_SPEED_LONG_RANGE_LASER_CLASS_2		134
/** High precision short range mode (laser class 2) */
#define AFBR_S50_DT_MODE_HIGH_PRECISION_SHORT_RANGE_LASER_CLASS_2	137
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_BRCM_AFBR_S50_H_ */
