/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the ST LPS22HH pressure sensor.
 * @ingroup lps22hh_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ST_LPS22HH_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ST_LPS22HH_H_

/**
 * @defgroup lps22hh_interface LPS22HH
 * @ingroup sensor_interface_ext_st
 * @brief STMicroelectronics LPS22HH pressure and temperature sensor
 * @{
 */

/**
 * @name Output data rate options
 *
 * Values for the `odr` devicetree property.
 * @{
 */
#define LPS22HH_DT_ODR_POWER_DOWN		0 /**< Power-down */
#define LPS22HH_DT_ODR_1HZ			1 /**< 1 Hz */
#define LPS22HH_DT_ODR_10HZ			2 /**< 10 Hz */
#define LPS22HH_DT_ODR_25HZ			3 /**< 25 Hz */
#define LPS22HH_DT_ODR_50HZ			4 /**< 50 Hz */
#define LPS22HH_DT_ODR_75HZ			5 /**< 75 Hz */
#define LPS22HH_DT_ODR_100HZ			6 /**< 100 Hz */
#define LPS22HH_DT_ODR_200HZ			7 /**< 200 Hz */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ST_LPS22HH_H_ */
