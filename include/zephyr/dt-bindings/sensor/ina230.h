/*
 * Copyright 2021 The Chromium OS Authors
 * Copyright (c) 2021 Grinn
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the TI INA230 power monitor.
 * @ingroup ina230_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_INA230_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_INA230_H_

#include <zephyr/dt-bindings/dt-util.h>

/**
 * @defgroup ina230_interface INA230
 * @ingroup sensor_interface_ext_ti
 * @brief Texas Instruments INA230 current and power monitor
 * @{
 */

/**
 * @name Mask/enable bits that assert the ALERT pin
 *
 * Mask/enable flags that assert the ALERT pin.
 * @{
 */
#define INA230_SHUNT_VOLTAGE_OVER    BIT(15) /**< Shunt voltage over-limit */
#define INA230_SHUNT_VOLTAGE_UNDER   BIT(14) /**< Shunt voltage under-limit */
#define INA230_BUS_VOLTAGE_OVER      BIT(13) /**< Bus voltage over-limit */
#define INA230_BUS_VOLTAGE_UNDER     BIT(12) /**< Bus voltage under-limit */
#define INA230_OVER_LIMIT_POWER      BIT(11) /**< Power over-limit */
#define INA230_CONVERSION_READY      BIT(10) /**< Conversion-ready enable */
#define INA230_ALERT_FUNCTION_FLAG   BIT(4)  /**< Alert function flag */
#define INA230_CONVERSION_READY_FLAG BIT(3)  /**< Conversion-ready flag */
#define INA230_MATH_OVERFLOW_FLAG    BIT(2)  /**< Math overflow flag */
#define INA230_ALERT_POLARITY        BIT(1)  /**< Alert pin polarity */
#define INA230_ALERT_LATCH_ENABLE    BIT(0)  /**< Alert latch enable */
/** @} */

/**
 * @name Operating mode
 * @anchor ina230_oper_mode
 *
 * Operating-mode argument for the INA230_CONFIG() macro.
 * @{
 */
#define INA230_OPER_MODE_POWER_DOWN             0x00 /**< Power-down */
#define INA230_OPER_MODE_SHUNT_VOLTAGE_TRIG     0x01 /**< Shunt voltage, triggered */
#define INA230_OPER_MODE_BUS_VOLTAGE_TRIG       0x02 /**< Bus voltage, triggered */
#define INA230_OPER_MODE_SHUNT_BUS_VOLTAGE_TRIG 0x03 /**< Shunt and bus, triggered */
#define INA230_OPER_MODE_SHUNT_VOLTAGE_CONT     0x05 /**< Shunt voltage, continuous */
#define INA230_OPER_MODE_BUS_VOLTAGE_CONT       0x06 /**< Bus voltage, continuous */
#define INA230_OPER_MODE_SHUNT_BUS_VOLTAGE_CONT 0x07 /**< Shunt and bus, continuous */
/** @} */

/**
 * @name Bus and shunt voltage conversion time
 * @anchor ina230_conv_time
 *
 * Conversion-time argument for the INA230_CONFIG() macro.
 * @{
 */
#define INA230_CONV_TIME_140  0x00 /**< 140 µs */
#define INA230_CONV_TIME_204  0x01 /**< 204 µs */
#define INA230_CONV_TIME_332  0x02 /**< 332 µs */
#define INA230_CONV_TIME_588  0x03 /**< 588 µs */
#define INA230_CONV_TIME_1100 0x04 /**< 1100 µs */
#define INA230_CONV_TIME_2116 0x05 /**< 2116 µs */
#define INA230_CONV_TIME_4156 0x06 /**< 4156 µs */
#define INA230_CONV_TIME_8244 0x07 /**< 8244 µs */
/** @} */

/**
 * @name Averaging mode (number of samples)
 * @anchor ina230_avg
 *
 * Averaging-mode argument for the INA230_CONFIG() macro.
 * @{
 */
#define INA230_AVG_MODE_1    0x00 /**< 1 sample */
#define INA230_AVG_MODE_4    0x01 /**< 4 samples */
#define INA230_AVG_MODE_16   0x02 /**< 16 samples */
#define INA230_AVG_MODE_64   0x03 /**< 64 samples */
#define INA230_AVG_MODE_128  0x04 /**< 128 samples */
#define INA230_AVG_MODE_256  0x05 /**< 256 samples */
#define INA230_AVG_MODE_512  0x06 /**< 512 samples */
#define INA230_AVG_MODE_1024 0x07 /**< 1024 samples */
/** @} */

/**
 * @brief Build the INA230 configuration register value.
 *
 * @param mode  Operating mode (see @ref ina230_oper_mode).
 * @param svct  Shunt voltage conversion time (see @ref ina230_conv_time).
 * @param bvct  Bus voltage conversion time (see @ref ina230_conv_time).
 * @param avg   Averaging mode (see @ref ina230_avg).
 */
#define INA230_CONFIG(mode, \
		      svct, \
		      bvct, \
		      avg)  \
	(((avg) << 9) | ((bvct) << 6) | ((svct) << 3) | (mode))

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_INA230_H_ */
