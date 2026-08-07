/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2018 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the Broadcom APDS9253 light sensor.
 * @ingroup apds9253_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_APDS9253_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_APDS9253_H_

#include <zephyr/dt-bindings/dt-util.h>

/**
 * @defgroup apds9253_interface APDS9253
 * @ingroup sensor_interface_ext_avago
 * @brief Broadcom APDS9253 RGB and ambient light sensor
 * @{
 */

/**
 * @name ADC resolution and integration time
 *
 * Values for the `resolution` devicetree property.
 * @{
 */
#define APDS9253_RESOLUTION_20BIT_400MS 0                 /**< 20-bit, 400 ms */
#define APDS9253_RESOLUTION_19BIT_200MS BIT(4)            /**< 19-bit, 200 ms */
#define APDS9253_RESOLUTION_18BIT_100MS BIT(5)            /**< 18-bit, 100 ms (default) */
#define APDS9253_RESOLUTION_17BIT_50MS  (BIT(5) | BIT(4)) /**< 17-bit, 50 ms */
#define APDS9253_RESOLUTION_16BIT_25MS  BIT(6)            /**< 16-bit, 25 ms */
#define APDS9253_RESOLUTION_13BIT_3MS   (BIT(6) | BIT(4)) /**< 13-bit, 3 ms */
/** @} */

/**
 * @name Measurement rate
 *
 * Values for the `rate` devicetree property.
 * @{
 */
#define APDS9253_MEASUREMENT_RATE_2000MS (BIT(2) | BIT(1) | BIT(0)) /**< 2000 ms */
#define APDS9253_MEASUREMENT_RATE_1000MS (BIT(2) | BIT(0))          /**< 1000 ms */
#define APDS9253_MEASUREMENT_RATE_500MS  BIT(2)                     /**< 500 ms */
#define APDS9253_MEASUREMENT_RATE_200MS  (BIT(1) | BIT(0))          /**< 200 ms */
#define APDS9253_MEASUREMENT_RATE_100MS  BIT(1)                     /**< 100 ms (default) */
#define APDS9253_MEASUREMENT_RATE_50MS   BIT(0)                     /**< 50 ms */
#define APDS9253_MEASUREMENT_RATE_25MS   0                          /**< 25 ms */
/** @} */

/**
 * @name Gain range
 *
 * Values for the `gain` devicetree property.
 * @{
 */
#define APDS9253_GAIN_RANGE_18 BIT(2)            /**< 18x gain */
#define APDS9253_GAIN_RANGE_9  (BIT(1) | BIT(0)) /**< 9x gain */
#define APDS9253_GAIN_RANGE_6  BIT(1)            /**< 6x gain */
#define APDS9253_GAIN_RANGE_3  BIT(0)            /**< 3x gain (default) */
#define APDS9253_GAIN_RANGE_1  0                 /**< 1x gain */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_APDS9253_H_*/
