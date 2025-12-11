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
 * @brief Custom sensor channels for MCP9600
 */
enum sensor_channel_mcp9600 {
	SENSOR_CHAN_MCP9600_COLD_JUNCTION_TEMP = SENSOR_CHAN_PRIV_START,
	SENSOR_CHAN_MCP9600_HOT_JUNCTION_TEMP,
	SENSOR_CHAN_MCP9600_DELTA_TEMP,
	SENSOR_CHAN_MCP9600_RAW_ADC,
};

/**
 * @brief Custom sensor attributes for MCP9600
 */
enum sensor_attribute_mcp9600 {
	SENSOR_ATTR_MCP9600_ADC_RES = SENSOR_ATTR_PRIV_START,
	SENSOR_ATTR_MCP9600_FILTER_COEFFICIENT,
	SENSOR_ATTR_MCP9600_THERMOCOUPLE_TYPE,
	SENSOR_ATTR_MCP9600_COLD_JUNCTION_RESOLUTION,
	SENSOR_ATTR_MCP9600_DEV_ID, /** @brief read only */
};

/**
 * @name Thermocouple type selection
 * @brief Values for attribute SENSOR_CHAN_MCP9600_THERMOCOUPLE_TYPE
 * @{
 */
#define MCP9600_ATTR_VALUE_TYPE_K 0x0
#define MCP9600_ATTR_VALUE_TYPE_J 0x1
#define MCP9600_ATTR_VALUE_TYPE_T 0x2
#define MCP9600_ATTR_VALUE_TYPE_N 0x3
#define MCP9600_ATTR_VALUE_TYPE_S 0x4
#define MCP9600_ATTR_VALUE_TYPE_E 0x5
#define MCP9600_ATTR_VALUE_TYPE_B 0x6
#define MCP9600_ATTR_VALUE_TYPE_R 0x7
/** @} */


/**
 * @name ADC resolution
 * @brief MCP9600 values for attribute SENSOR_ATTR_MCP9600_ADC_RES
 * @{
 */
#define MCP9600_ATTR_VALUE_ADC_RES_18BIT 0x0
#define MCP9600_ATTR_VALUE_ADC_RES_16BIT 0x1
#define MCP9600_ATTR_VALUE_ADC_RES_14BIT 0x2
#define MCP9600_ATTR_VALUE_ADC_RES_12BIT 0x3
/**
 * @}
 */


/**
 * @name Cold junction temperature resolution
 * @brief MCP9600 values for attribute SENSOR_ATTR_MCP9600_COLD_JUNCTION_RESOLUTION
 * @{
 */
#define MCP9600_ATTR_VALUE_COLD_JUNC_TMP_RES_0_0625C 0x0
#define MCP9600_ATTR_VALUE_COLD_JUNC_TMP_RES_0_25C   0x1
/**
 * @}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_MCP9600H_ */
