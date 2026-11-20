/*
 * Copyright (c) 2026 Dotcom IoT LLP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_MIS2DU12_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_MIS2DU12_H_

/**
 * @file
 * @brief MIS2DU12 sensor devicetree binding macros.
 * @ingroup mis2du12_dt_bindings
 */

#include <zephyr/dt-bindings/sensor/lis2du12.h>

/**
 * @defgroup mis2du12_dt_bindings MIS2DU12 DT Bindings
 * @ingroup sensor_interface
 * @brief STMicroelectronics MIS2DU12 DT binding macros
 * @{
 */

/** Full scale range: +/-2g */
#define MIS2DU12_DT_FS_2G  LIS2DU12_DT_FS_2G
/** Full scale range: +/-4g */
#define MIS2DU12_DT_FS_4G  LIS2DU12_DT_FS_4G
/** Full scale range: +/-8g */
#define MIS2DU12_DT_FS_8G  LIS2DU12_DT_FS_8G
/** Full scale range: +/-16g */
#define MIS2DU12_DT_FS_16G LIS2DU12_DT_FS_16G

/** Output data rate: power-down */
#define MIS2DU12_DT_ODR_OFF         LIS2DU12_DT_ODR_OFF
/** Output data rate: 1.6 Hz, ultra-low power */
#define MIS2DU12_DT_ODR_AT_1Hz6_ULP LIS2DU12_DT_ODR_AT_1Hz6_ULP
/** Output data rate: 3 Hz, ultra-low power */
#define MIS2DU12_DT_ODR_AT_3Hz_ULP  LIS2DU12_DT_ODR_AT_3Hz_ULP
/** Output data rate: 6 Hz, ultra-low power */
#define MIS2DU12_DT_ODR_AT_6Hz_ULP  LIS2DU12_DT_ODR_AT_6Hz_ULP
/** Output data rate: 6 Hz */
#define MIS2DU12_DT_ODR_AT_6Hz      LIS2DU12_DT_ODR_AT_6Hz
/** Output data rate: 12 Hz */
#define MIS2DU12_DT_ODR_AT_12Hz     LIS2DU12_DT_ODR_AT_12Hz
/** Output data rate: 25 Hz */
#define MIS2DU12_DT_ODR_AT_25Hz     LIS2DU12_DT_ODR_AT_25Hz
/** Output data rate: 50 Hz */
#define MIS2DU12_DT_ODR_AT_50Hz     LIS2DU12_DT_ODR_AT_50Hz
/** Output data rate: 100 Hz */
#define MIS2DU12_DT_ODR_AT_100Hz    LIS2DU12_DT_ODR_AT_100Hz
/** Output data rate: 200 Hz */
#define MIS2DU12_DT_ODR_AT_200Hz    LIS2DU12_DT_ODR_AT_200Hz
/** Output data rate: 400 Hz */
#define MIS2DU12_DT_ODR_AT_400Hz    LIS2DU12_DT_ODR_AT_400Hz
/** Output data rate: 800 Hz */
#define MIS2DU12_DT_ODR_AT_800Hz    LIS2DU12_DT_ODR_AT_800Hz

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_MIS2DU12_H_ */
