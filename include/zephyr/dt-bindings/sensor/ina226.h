/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the TI INA226 power monitor.
 * @ingroup ina226_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INA226_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INA226_H_

#include <zephyr/dt-bindings/dt-util.h>

/**
 * @defgroup ina226_interface INA226
 * @ingroup sensor_interface_ext_ti
 * @brief Texas Instruments INA226 current and power monitor
 * @{
 */

/**
 * @name Reset mode
 *
 * Reset-mode selection.
 * @{
 */
#define INA226_RST_NORMAL_OPERATION	0x00 /**< Normal operation */
#define INA226_RST_SYSTEM_RESET		0x01 /**< Trigger a system reset */
/** @} */

/**
 * @name Averaging mode (number of samples)
 *
 * Averaging-mode selection.
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
 * @name Bus and shunt voltage conversion time
 *
 * Bus and shunt voltage conversion-time selection.
 * @{
 */
#define INA226_CONV_TIME_140		0x00 /**< 140 µs */
#define INA226_CONV_TIME_204		0x01 /**< 204 µs */
#define INA226_CONV_TIME_332		0x02 /**< 332 µs */
#define INA226_CONV_TIME_588		0x03 /**< 588 µs */
#define INA226_CONV_TIME_1100		0x04 /**< 1100 µs */
#define INA226_CONV_TIME_2116		0x05 /**< 2116 µs */
#define INA226_CONV_TIME_4156		0x06 /**< 4156 µs */
#define INA226_CONV_TIME_8244		0x07 /**< 8244 µs */
/** @} */

/**
 * @name Operating mode
 *
 * Operating-mode selection.
 * @{
 */
#define INA226_OPER_MODE_POWER_DOWN			0x00 /**< Power-down */
#define INA226_OPER_MODE_SHUNT_VOLTAGE_TRIG		0x01 /**< Shunt voltage, triggered */
#define INA226_OPER_MODE_BUS_VOLTAGE_TRIG		0x02 /**< Bus voltage, triggered */
#define INA226_OPER_MODE_SHUNT_BUS_VOLTAGE_TRIG		0x03 /**< Shunt and bus, triggered */
#define INA226_OPER_MODE_SHUNT_VOLTAGE_CONT		0x05 /**< Shunt voltage, continuous */
#define INA226_OPER_MODE_BUS_VOLTAGE_CONT		0x06 /**< Bus voltage, continuous */
#define INA226_OPER_MODE_SHUNT_BUS_VOLTAGE_CONT		0x07 /**< Shunt and bus, continuous */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INA226_H_ */
