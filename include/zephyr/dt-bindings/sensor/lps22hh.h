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

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_LPS22HH_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_LPS22HH_H_

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

/**
 * @name Low-pass filter bandwidth options
 *
 * Values for the `lpfp-bandwidth` devicetree property. The two-bit lpfp_cfg
 * field in CTRL_REG1 packs the enable bit together with the bandwidth:
 * 0 disables the filter (the ST HAL labels it ODR/2), 2 and 3 select
 * ODR/9 and ODR/20 respectively.
 * @{
 */
#define LPS22HH_DT_LPF_DISABLED			0 /**< Low-pass filter off */
#define LPS22HH_DT_LPF_ODR_DIV_9		2 /**< Bandwidth = ODR/9 */
#define LPS22HH_DT_LPF_ODR_DIV_20		3 /**< Bandwidth = ODR/20 */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_LPS22HH_H_ */
