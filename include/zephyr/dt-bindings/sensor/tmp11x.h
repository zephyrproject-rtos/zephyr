/*
 * Copyright (c) 2025 Prevas A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the TI TMP11x temperature sensors.
 * @ingroup tmp11x_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_TMP11X_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_TMP11X_H_

/**
 * @addtogroup tmp11x_interface
 * @{
 */

/**
 * @name Output data rate options
 *
 * Values for the `odr` devicetree property.
 * @{
 */
#define TMP11X_DT_ODR_15_5_MS  0     /**< 15.5 ms */
#define TMP11X_DT_ODR_125_MS   0x80  /**< 125 ms */
#define TMP11X_DT_ODR_250_MS   0x100 /**< 250 ms */
#define TMP11X_DT_ODR_500_MS   0x180 /**< 500 ms */
#define TMP11X_DT_ODR_1000_MS  0x200 /**< 1000 ms */
#define TMP11X_DT_ODR_4000_MS  0x280 /**< 4000 ms */
#define TMP11X_DT_ODR_8000_MS  0x300 /**< 8000 ms */
#define TMP11X_DT_ODR_16000_MS 0x380 /**< 16000 ms */
/** @} */

/**
 * @name Averaging (oversampling) options
 *
 * Values for the `oversampling` devicetree property.
 * @{
 */
#define TMP11X_DT_OVERSAMPLING_1  0    /**< 1 conversion averaged */
#define TMP11X_DT_OVERSAMPLING_8  0x20 /**< 8 conversions averaged */
#define TMP11X_DT_OVERSAMPLING_32 0x40 /**< 32 conversions averaged */
#define TMP11X_DT_OVERSAMPLING_64 0x60 /**< 64 conversions averaged */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_TMP11X_H_ */
