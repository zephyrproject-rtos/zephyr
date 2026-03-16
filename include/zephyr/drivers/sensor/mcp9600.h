/*
 * Copyright (c) 2025 Thomas Schmid <tom@lfence.de>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of MCP9600 sensor
 * @ingroup mcp9600_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_MCP9600H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_MCP9600H_

/**
 * @brief Microchip MCP9600 Thermocouple Electromotive Force (EMF) to °C Converter
 * @defgroup mcp9600_interface MCP9600
 * @ingroup sensor_interface_ext
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <zephyr/drivers/sensor.h>

/**
 * @brief Custom sensor channels for MCP9600.
 */
enum sensor_channel_mcp9600 {
	/**
	 * Cold junction (reference) temperature in degrees Celsius.
	 *
	 * - @c sensor_value.val1 is the integer part of the temperature (°C).
	 * - @c sensor_value.val2 is the fractional part (in millionths of a degree).
	 */
	SENSOR_CHAN_MCP9600_COLD_JUNCTION_TEMP = SENSOR_CHAN_PRIV_START,
	/**
	 * Hot junction (thermocouple tip) temperature in degrees Celsius.
	 *
	 * - @c sensor_value.val1 is the integer part of the temperature (°C).
	 * - @c sensor_value.val2 is the fractional part (in millionths of a degree).
	 */
	SENSOR_CHAN_MCP9600_HOT_JUNCTION_TEMP,
	/**
	 * Temperature difference (hot junction minus cold junction) in degrees Celsius.
	 *
	 * - @c sensor_value.val1 is the integer part of the delta temperature (°C).
	 * - @c sensor_value.val2 is the fractional part (in millionths of a degree).
	 */
	SENSOR_CHAN_MCP9600_DELTA_TEMP,
	/**
	 * Raw ADC value (thermocouple EMF in ADC counts).
	 *
	 * - @c sensor_value.val1 is the raw 24-bit signed ADC value.
	 * - @c sensor_value.val2 is unused (always 0).
	 */
	SENSOR_CHAN_MCP9600_RAW_ADC,
};

/**
 * @brief Custom sensor attributes for MCP9600.
 */
enum sensor_attribute_mcp9600 {
	/**
	 * ADC resolution.
	 *
	 * - @c sensor_value.val1 is the resolution value (see @ref mcp9600_adc_resolution).
	 * - @c sensor_value.val2 is unused (always 0).
	 */
	SENSOR_ATTR_MCP9600_ADC_RES = SENSOR_ATTR_PRIV_START,
	/**
	 * IIR filter coefficient (0-7).
	 *
	 * - @c sensor_value.val1 is the filter coefficient.
	 * - @c sensor_value.val2 is unused (always 0).
	 */
	SENSOR_ATTR_MCP9600_FILTER_COEFFICIENT,
	/**
	 * Thermocouple type.
	 *
	 * - @c sensor_value.val1 is the thermocouple type (see @ref mcp9600_thermocouple_type).
	 * - @c sensor_value.val2 is unused (always 0).
	 */
	SENSOR_ATTR_MCP9600_THERMOCOUPLE_TYPE,
	/**
	 * Cold junction temperature resolution.
	 *
	 * - @c sensor_value.val1 is the resolution value (see @ref
	 *   mcp9600_cold_junction_resolution).
	 * - @c sensor_value.val2 is unused (always 0).
	 */
	SENSOR_ATTR_MCP9600_COLD_JUNCTION_RESOLUTION,
	/**
	 * Device ID (read-only).
	 *
	 * - @c sensor_value.val1 is the 16-bit device ID (chip ID in high byte, revision in low
	 *   byte).
	 * - @c sensor_value.val2 is unused (always 0).
	 */
	SENSOR_ATTR_MCP9600_DEV_ID,
};

/**
 * @anchor mcp9600_thermocouple_type
 * @name Thermocouple type selection
 * @brief Possible values for sensor_value.val1 of a @ref
 * SENSOR_ATTR_MCP9600_THERMOCOUPLE_TYPE attribute.
 * @{
 */
#define MCP9600_ATTR_VALUE_TYPE_K 0x0 /**< Type K thermocouple */
#define MCP9600_ATTR_VALUE_TYPE_J 0x1 /**< Type J thermocouple */
#define MCP9600_ATTR_VALUE_TYPE_T 0x2 /**< Type T thermocouple */
#define MCP9600_ATTR_VALUE_TYPE_N 0x3 /**< Type N thermocouple */
#define MCP9600_ATTR_VALUE_TYPE_S 0x4 /**< Type S thermocouple */
#define MCP9600_ATTR_VALUE_TYPE_E 0x5 /**< Type E thermocouple */
#define MCP9600_ATTR_VALUE_TYPE_B 0x6 /**< Type B thermocouple */
#define MCP9600_ATTR_VALUE_TYPE_R 0x7 /**< Type R thermocouple */
/** @} */

/**
 * @anchor mcp9600_adc_resolution
 * @name ADC resolution
 * @brief Possible values for sensor_value.val1 of a @ref
 * SENSOR_ATTR_MCP9600_ADC_RES attribute.
 * @{
 */
#define MCP9600_ATTR_VALUE_ADC_RES_18BIT 0x0 /**< 18-bit resolution */
#define MCP9600_ATTR_VALUE_ADC_RES_16BIT 0x1 /**< 16-bit resolution */
#define MCP9600_ATTR_VALUE_ADC_RES_14BIT 0x2 /**< 14-bit resolution */
#define MCP9600_ATTR_VALUE_ADC_RES_12BIT 0x3 /**< 12-bit resolution */
/** @} */

/**
 * @anchor mcp9600_cold_junction_resolution
 * @name Cold junction temperature resolution
 * @brief Possible values for sensor_value.val1 of a attribute @ref
 * SENSOR_ATTR_MCP9600_COLD_JUNCTION_RESOLUTION attribute.
 * @{
 */
#define MCP9600_ATTR_VALUE_COLD_JUNC_TMP_RES_0_0625C 0x0 /**< 0.0625 °C resolution */
#define MCP9600_ATTR_VALUE_COLD_JUNC_TMP_RES_0_25C   0x1 /**< 0.25 °C resolution */
/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_MCP9600H_ */
