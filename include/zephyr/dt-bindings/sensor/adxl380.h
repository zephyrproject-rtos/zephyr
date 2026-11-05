/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding macros for the Analog Devices ADXL380 accelerometer.
 *
 * Defines the ADXL380 measurement-range selectors. Shared options common to the
 * ADXL380/ADXL382 live in adxl38x.h, which this file includes.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_ADXL380_H
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_ADXL380_H

#include <zephyr/dt-bindings/sensor/adxl38x.h>

/**
 * @defgroup adxl380_range ADXL380 measurement range options
 * @ingroup adxl38x
 *
 * Values map to OP_MODE (0x26) bits [7:6].
 * @{
 */
/** Select ±4 g measurement range */
#define ADXL380_DT_RANGE_4G  0
/** Select ±8 g measurement range */
#define ADXL380_DT_RANGE_8G  1
/** Select ±16 g measurement range */
#define ADXL380_DT_RANGE_16G 2
/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_ADXL380_H */
