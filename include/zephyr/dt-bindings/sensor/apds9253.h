/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2018 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for APDS9253 Devicetree constants
 * @ingroup apds9253_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_APDS9253_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_APDS9253_H_

/**
 * @defgroup apds9253_interface APDS9253
 * @ingroup sensor_interface_ext
 * @brief APDS9253 ambient light RGB & IR sensor
 * @{
 */

#include <zephyr/dt-bindings/dt-util.h>

/**
 * @name APDS9253 Resolution and Integration Time Settings
 * @brief ADC resolution and integration time configuration constants
 *
 * These constants define the ADC resolution (13-20 bits) and corresponding
 * integration times (3.125ms - 400ms). Higher resolution provides better
 * accuracy but requires longer integration times.
 *
 * @{
 */

/** 20-bit resolution with 400ms integration time (highest accuracy) */
#define APDS9253_RESOLUTION_20BIT_400MS 0
/** 19-bit resolution with 200ms integration time */
#define APDS9253_RESOLUTION_19BIT_200MS BIT(4)
/** 18-bit resolution with 100ms integration time (default) */
#define APDS9253_RESOLUTION_18BIT_100MS BIT(5)
/** 17-bit resolution with 50ms integration time */
#define APDS9253_RESOLUTION_17BIT_50MS  (BIT(5) | BIT(4))
/** 16-bit resolution with 25ms integration time */
#define APDS9253_RESOLUTION_16BIT_25MS  BIT(6)
/** 13-bit resolution with 3ms integration time (fastest response) */
#define APDS9253_RESOLUTION_13BIT_3MS   (BIT(6) | BIT(4))

/** @} */

/**
 * @name APDS9253 Measurement Rate Settings
 * @brief Periodic measurement timing configuration constants

 * These constants control the timing of periodic measurements in active mode.
 * The measurement rate determines how frequently new sensor readings are taken.
 *
 * @{
 */

/** Measurement every 2 seconds - lowest power consumption */
#define APDS9253_MEASUREMENT_RATE_2000MS (BIT(2) | BIT(1) | BIT(0))
/** Measurement every 1 second */
#define APDS9253_MEASUREMENT_RATE_1000MS (BIT(2) | BIT(0))
/** Measurement every 500ms */
#define APDS9253_MEASUREMENT_RATE_500MS  BIT(2)
/** Measurement every 200ms */
#define APDS9253_MEASUREMENT_RATE_200MS  (BIT(1) | BIT(0))
/** Measurement every 100ms (default) */
#define APDS9253_MEASUREMENT_RATE_100MS  BIT(1)
/** Measurement every 50ms */
#define APDS9253_MEASUREMENT_RATE_50MS   BIT(0)
/** Measurement every 25ms */
#define APDS9253_MEASUREMENT_RATE_25MS   0

/** @} */

/**
 * @name APDS9253 Analog Gain Range Settings
 * @brief Sensor sensitivity and dynamic range configuration constants
 *
 * These constants configure the analog gain of the light sensor, affecting
 * the sensitivity and dynamic range. Higher gain provides better sensitivity
 * for low-light conditions but reduces the maximum measurable light intensity.
 *
 * @{
 */

/** Gain factor 18× - highest sensitivity (max lux ~7,700) */
#define APDS9253_GAIN_RANGE_18 BIT(2)
/** Gain factor 9× */
#define APDS9253_GAIN_RANGE_9  (BIT(1) | BIT(0))
/** Gain factor 6× */
#define APDS9253_GAIN_RANGE_6  BIT(1)
/** Gain factor 3× (default) */
#define APDS9253_GAIN_RANGE_3  BIT(0)
/** Gain factor 1× - lowest sensitivity (max lux ~144,000 lux)*/
#define APDS9253_GAIN_RANGE_1  0

/** @} */

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_APDS9253_H_*/
