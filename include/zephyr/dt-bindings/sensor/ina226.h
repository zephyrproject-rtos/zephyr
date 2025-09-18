/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for INA226 Devicetree constants
 * @ingroup ina226_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INA226_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INA226_H_

/**
 * @defgroup ina226_interface INA226
 * @ingroup sensor_interface_ext
 * @brief Texas Instruments INA226 Bidirectional Current and Power Monitor
 * @{
 */

#include <zephyr/dt-bindings/dt-util.h>

/**
 * @name Reset Modes
 * @{
 */
#define INA226_RST_NORMAL_OPERATION	0x00 /**< Normal Operation */
#define INA226_RST_SYSTEM_RESET		0x01 /**< System Reset */
/** @} */

/**
 * @name Averaging Modes
 *
 * @{
 */
#define INA226_AVG_MODE_1		0x00 /**< 1 sample */
#define INA226_AVG_MODE_4		0x01 /**< 4 samples */
#define INA226_AVG_MODE_16		0x02 /**< 16 samples */
#define INA226_AVG_MODE_64		0x03 /**< 64 samples */
#define INA226_AVG_MODE_128		0x04 /**< 128 samples */
#define INA226_AVG_MODE_256		0x05 /**< 256 samples */
#define INA226_AVG_MODE_512		0x06 /**< 512 samples */
#define INA226_AVG_MODE_1024		0x07 /**< 1024 samples */
/** @} */

/**
 * @name Conversion Time for Bus and Shunt Voltage
 * @{
 */
#define INA226_CONV_TIME_140		0x00 /**< 140 μs */
#define INA226_CONV_TIME_204		0x01 /**< 204 μs */
#define INA226_CONV_TIME_332		0x02 /**< 332 μs */
#define INA226_CONV_TIME_588		0x03 /**< 588 μs */
#define INA226_CONV_TIME_1100		0x04 /**< 1.1 ms */
#define INA226_CONV_TIME_2116		0x05 /**< 2.116 ms */
#define INA226_CONV_TIME_4156		0x06 /**< 4.156 ms */
#define INA226_CONV_TIME_8244		0x07 /**< 8.244 ms */
/** @} */

/**
 * @name Operating Modes
 * @{
 */
#define INA226_OPER_MODE_POWER_DOWN			0x00 /**< Power-Down (or Shutdown) */
#define INA226_OPER_MODE_SHUNT_VOLTAGE_TRIG		0x01 /**< Shunt Voltage, Triggered */
#define INA226_OPER_MODE_BUS_VOLTAGE_TRIG		0x02 /**< Bus Voltage, Triggered */
#define INA226_OPER_MODE_SHUNT_BUS_VOLTAGE_TRIG		0x03 /**< Shunt and Bus, Triggered */
#define INA226_OPER_MODE_SHUNT_VOLTAGE_CONT		0x05 /**< Shunt Voltage, Continuous */
#define INA226_OPER_MODE_BUS_VOLTAGE_CONT		0x06 /**< Bus Voltage, Continuous */
#define INA226_OPER_MODE_SHUNT_BUS_VOLTAGE_CONT		0x07 /**< Shunt and Bus, Continuous */
/** @} */

/**
 * @}
 */

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INA226_H_ */
