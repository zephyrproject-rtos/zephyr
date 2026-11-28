/*
 * Copyright (c) 2026 Dotcom IoT LLP
 * Copyright (c) 2026 Mahendra Sondagar <mahendra@dotcom.co.in>
 * Copyright (c) 2026 Milan Pipaliya <milan.pipaliya@dnkmail.in>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree bindings for STMicroelectronics MIS2DU12 accelerometer.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_MIS2DU12_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_MIS2DU12_H_

#include <zephyr/dt-bindings/sensor/lis2du12.h>

/**
 * @name MIS2DU12 full-scale range definitions
 * @{
 */
#define MIS2DU12_DT_FS_2G  LIS2DU12_DT_FS_2G  /**< +/- 2g full scale */
#define MIS2DU12_DT_FS_4G  LIS2DU12_DT_FS_4G  /**< +/- 4g full scale */
#define MIS2DU12_DT_FS_8G  LIS2DU12_DT_FS_8G  /**< +/- 8g full scale */
#define MIS2DU12_DT_FS_16G LIS2DU12_DT_FS_16G /**< +/- 16g full scale */
/** @} */

/**
 * @name MIS2DU12 output data rate (ODR) definitions
 * @{
 */
#define MIS2DU12_DT_ODR_OFF         LIS2DU12_DT_ODR_OFF         /**< Power-down mode */
#define MIS2DU12_DT_ODR_AT_1Hz6_ULP LIS2DU12_DT_ODR_AT_1Hz6_ULP /**< 1.6 Hz (ultra-low power) */
#define MIS2DU12_DT_ODR_AT_3Hz_ULP  LIS2DU12_DT_ODR_AT_3Hz_ULP  /**< 3 Hz (ultra-low power) */
#define MIS2DU12_DT_ODR_AT_6Hz_ULP  LIS2DU12_DT_ODR_AT_6Hz_ULP  /**< 6 Hz (ultra-low power) */
#define MIS2DU12_DT_ODR_AT_6Hz      LIS2DU12_DT_ODR_AT_6Hz      /**< 6 Hz */
#define MIS2DU12_DT_ODR_AT_12Hz     LIS2DU12_DT_ODR_AT_12Hz     /**< 12.5 Hz */
#define MIS2DU12_DT_ODR_AT_25Hz     LIS2DU12_DT_ODR_AT_25Hz     /**< 25 Hz */
#define MIS2DU12_DT_ODR_AT_50Hz     LIS2DU12_DT_ODR_AT_50Hz     /**< 50 Hz */
#define MIS2DU12_DT_ODR_AT_100Hz    LIS2DU12_DT_ODR_AT_100Hz    /**< 100 Hz */
#define MIS2DU12_DT_ODR_AT_200Hz    LIS2DU12_DT_ODR_AT_200Hz    /**< 200 Hz */
#define MIS2DU12_DT_ODR_AT_400Hz    LIS2DU12_DT_ODR_AT_400Hz    /**< 400 Hz */
#define MIS2DU12_DT_ODR_AT_800Hz    LIS2DU12_DT_ODR_AT_800Hz    /**< 800 Hz */
/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_MIS2DU12_H_ */
