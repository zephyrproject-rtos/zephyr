/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding macros for the Analog Devices ADXL382 accelerometer.
 *
 * Defines the ADXL382 measurement-range selectors. Shared options common to the
 * ADXL380/ADXL382 live in adxl38x.h, which this file includes.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_ADXL382_H
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_ADXL382_H

#include <zephyr/dt-bindings/sensor/adxl38x.h>

/**
 * @defgroup adxl382_range ADXL382 measurement range options
 * @ingroup adxl38x
 *
 * Values map to OP_MODE (0x26) bits [7:6].
 * @{
 */
/** Select ±15 g measurement range */
#define ADXL382_DT_RANGE_15G 0
/** Select ±30 g measurement range */
#define ADXL382_DT_RANGE_30G 1
/** Select ±60 g measurement range */
#define ADXL382_DT_RANGE_60G 2
/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_ADXL382_H */
