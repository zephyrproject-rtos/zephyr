/*
 * Copyright (c) 2024 Fredrik Gihl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the TI TMP114 temperature sensor.
 * @ingroup tmp114_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_TI_TMP114_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_TI_TMP114_H_

/**
 * @defgroup tmp114_interface TMP114
 * @ingroup sensor_interface_ext_ti
 * @brief Texas Instruments TMP114 temperature sensor
 * @{
 */

/**
 * @name Output data rate options
 *
 * Values for the `odr` devicetree property.
 * @{
 */
#define TMP114_DT_ODR_6_4_MS   0 /**< 6.4 ms */
#define TMP114_DT_ODR_31_25_MS 1 /**< 31.25 ms */
#define TMP114_DT_ODR_62_5_MS  2 /**< 62.5 ms */
#define TMP114_DT_ODR_125_MS   3 /**< 125 ms */
#define TMP114_DT_ODR_250_MS   4 /**< 250 ms */
#define TMP114_DT_ODR_500_MS   5 /**< 500 ms */
#define TMP114_DT_ODR_1000_MS  6 /**< 1000 ms */
#define TMP114_DT_ODR_2000_MS  7 /**< 2000 ms */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_TI_TMP114_H_ */
