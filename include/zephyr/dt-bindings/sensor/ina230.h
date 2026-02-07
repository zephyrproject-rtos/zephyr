/*
 * Copyright 2021 The Chromium OS Authors
 * Copyright (c) 2021 Grinn
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for INA230 Devicetree constants
 * @ingroup ina230_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INA230_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INA230_H_

/**
 * @defgroup ina230_interface INA230
 * @ingroup sensor_interface_ext
 * @brief Texas Instruments INA230 Bidirectional Current and Power Monitor
 * @{
 */

#include <zephyr/dt-bindings/dt-util.h>

/**
 * @name Mask/Enable bits that asserts the ALERT pin
 * @{
 */
#define INA230_SHUNT_VOLTAGE_OVER    BIT(15) /**< Shunt Voltage Over Limit (SOL) */
#define INA230_SHUNT_VOLTAGE_UNDER   BIT(14) /**< Shunt Voltage Under Limit (SUL) */
#define INA230_BUS_VOLTAGE_OVER      BIT(13) /**< Bus Voltage Over Limit (BOL) */
#define INA230_BUS_VOLTAGE_UNDER     BIT(12) /**< Bus Voltage Under Limit (BUL) */
#define INA230_OVER_LIMIT_POWER      BIT(11) /**< Power Over Limit (POL) */
#define INA230_CONVERSION_READY      BIT(10) /**< Conversion Ready (CNVR) */
#define INA230_ALERT_FUNCTION_FLAG   BIT(4)  /**< Alert Function Flag (AFF) */
#define INA230_CONVERSION_READY_FLAG BIT(3)  /**< Conversion Ready Flag (CVRF) */
#define INA230_MATH_OVERFLOW_FLAG    BIT(2)  /**< Math Overflow Flag (OVF) */
#define INA230_ALERT_POLARITY        BIT(1)  /**< Alert Polarity (APOL) */
#define INA230_ALERT_LATCH_ENABLE    BIT(0)  /**< Alert Latch Enable (LEN )*/
					     /** @} */

/**
 * @name Operating Mode
 * @{
 */
#define INA230_OPER_MODE_POWER_DOWN             0x00 /**< Power-down */
#define INA230_OPER_MODE_SHUNT_VOLTAGE_TRIG     0x01 /**< Shunt Voltage, triggered */
#define INA230_OPER_MODE_BUS_VOLTAGE_TRIG       0x02 /**< Bus Voltage, triggered */
#define INA230_OPER_MODE_SHUNT_BUS_VOLTAGE_TRIG 0x03 /**< Shunt and Bus, triggered */
#define INA230_OPER_MODE_SHUNT_VOLTAGE_CONT     0x05 /**< Shunt Voltage, continuous */
#define INA230_OPER_MODE_BUS_VOLTAGE_CONT       0x06 /**< Bus Voltage, continuous */
#define INA230_OPER_MODE_SHUNT_BUS_VOLTAGE_CONT 0x07 /**< Shunt and Bus, continuous */
/** @} */

/**
 * @name Conversion time for bus and shunt (in micro-seconds)
 * @{
 */
#define INA230_CONV_TIME_140  0x00 /**< 140 us */
#define INA230_CONV_TIME_204  0x01 /**< 204 us */
#define INA230_CONV_TIME_332  0x02 /**< 332 us */
#define INA230_CONV_TIME_588  0x03 /**< 588 us */
#define INA230_CONV_TIME_1100 0x04 /**< 1100 us */
#define INA230_CONV_TIME_2116 0x05 /**< 2116 us */
#define INA230_CONV_TIME_4156 0x06 /**< 4156 us */
#define INA230_CONV_TIME_8244 0x07 /**< 8244 us */
/** @} */

/**
 * @name Averaging Mode
 *
 * Number of samples that are collected and averaged together.
 *
 * @{
 */
#define INA230_AVG_MODE_1    0x00 /**< No averaging, 1 sample */
#define INA230_AVG_MODE_4    0x01 /**< 4 samples */
#define INA230_AVG_MODE_16   0x02 /**< 16 samples */
#define INA230_AVG_MODE_64   0x03 /**< 64 samples */
#define INA230_AVG_MODE_128  0x04 /**< 128 samples */
#define INA230_AVG_MODE_256  0x05 /**< 256 samples */
#define INA230_AVG_MODE_512  0x06 /**< 512 samples */
#define INA230_AVG_MODE_1024 0x07 /**< 1024 samples */
/** @} */

/**
 * @brief Macro for creating the INA230 configuration value
 *
 * @param mode  Operating mode.
 * @param svct  Conversion time for shunt voltage.
 * @param bvct  Conversion time for bus voltage.
 * @param avg   Averaging mode.
 */
#define INA230_CONFIG(mode, \
		      svct, \
		      bvct, \
		      avg)  \
	(((avg) << 9) | ((bvct) << 6) | ((svct) << 3) | (mode))

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INA230_H_ */
