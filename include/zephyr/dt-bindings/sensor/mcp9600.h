/*
 * Copyright (c) 2025 Thomas Schmid <tom@lfence.de>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the Microchip MCP9600 thermocouple converter.
 * @ingroup mcp9600_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_MCP9600_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_MCP9600_H_

/**
 * @addtogroup mcp9600_interface
 * @{
 */

/**
 * @name Thermocouple type selection
 *
 * Values for the `thermocouple-type` devicetree property.
 * @{
 */
#define MCP9600_DT_TYPE_K 0x0 /**< Type K thermocouple */
#define MCP9600_DT_TYPE_J 0x1 /**< Type J thermocouple */
#define MCP9600_DT_TYPE_T 0x2 /**< Type T thermocouple */
#define MCP9600_DT_TYPE_N 0x3 /**< Type N thermocouple */
#define MCP9600_DT_TYPE_S 0x4 /**< Type S thermocouple */
#define MCP9600_DT_TYPE_E 0x5 /**< Type E thermocouple */
#define MCP9600_DT_TYPE_B 0x6 /**< Type B thermocouple */
#define MCP9600_DT_TYPE_R 0x7 /**< Type R thermocouple */
/** @} */

/**
 * @name ADC resolution options
 *
 * Values for the `adc-resolution` devicetree property.
 * @{
 */
#define MCP9600_DT_ADC_RES_18BIT 0x0 /**< 18-bit resolution */
#define MCP9600_DT_ADC_RES_16BIT 0x1 /**< 16-bit resolution */
#define MCP9600_DT_ADC_RES_14BIT 0x2 /**< 14-bit resolution */
#define MCP9600_DT_ADC_RES_12BIT 0x3 /**< 12-bit resolution */
/** @} */

/**
 * @name Cold-junction temperature resolution
 *
 * Values for the `cold-junction-temp-resolution` devicetree property.
 * @{
 */
#define MCP9600_DT_COLD_JUNC_TMP_RES_0_0625C 0x0 /**< 0.0625 °C resolution */
#define MCP9600_DT_COLD_JUNC_TMP_RES_0_25C   0x1 /**< 0.25 °C resolution */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_MCP9600_H_ */
