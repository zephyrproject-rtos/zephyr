/*
 * Copyright (c) 2024 Intel Corporation
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the PNI RM3100 magnetometer.
 * @ingroup rm3100_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_RM3100_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_RM3100_H_

/**
 * @defgroup rm3100_interface RM3100
 * @ingroup sensor_interface_ext_pni
 * @brief PNI RM3100 3-axis magnetometer
 * @{
 */

/**
 * @name Output data rate options
 *
 * Values for the `odr` devicetree property.
 * @{
 */
#define RM3100_DT_ODR_600	0x92 /**< 600 Hz */
#define RM3100_DT_ODR_300	0x93 /**< 300 Hz */
#define RM3100_DT_ODR_150	0x94 /**< 150 Hz */
#define RM3100_DT_ODR_75	0x95 /**< 75 Hz */
#define RM3100_DT_ODR_37_5	0x96 /**< 37.5 Hz */
#define RM3100_DT_ODR_18	0x97 /**< 18 Hz */
#define RM3100_DT_ODR_9		0x98 /**< 9 Hz */
#define RM3100_DT_ODR_4_5	0x99 /**< 4.5 Hz */
#define RM3100_DT_ODR_2_3	0x9A /**< 2.3 Hz */
#define RM3100_DT_ODR_1_2	0x9B /**< 1.2 Hz */
#define RM3100_DT_ODR_0_6	0x9C /**< 0.6 Hz */
#define RM3100_DT_ODR_0_3	0x9D /**< 0.3 Hz */
#define RM3100_DT_ODR_0_015	0x9E /**< 0.015 Hz */
#define RM3100_DT_ODR_0_0075	0x9F /**< 0.0075 Hz */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_RM3100_H_ */
