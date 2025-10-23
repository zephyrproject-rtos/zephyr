/*
 * Copyright (c) 2025 Thomas Schmid <tom@lfence.de>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_MICRO_MCP9600_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_MICRO_MCP9600_H_

/**
 * @defgroup mcp9600 MCP9600 DT Options
 * @ingroup sensor_interface
 * @{
 */

/**
 * @defgroup mcp9600_thermocouple_type Thermocouple type selection
 * @{
 */
#define MCP9600_DT_TYPE_K 0x0
#define MCP9600_DT_TYPE_J 0x1
#define MCP9600_DT_TYPE_T 0x2
#define MCP9600_DT_TYPE_N 0x3
#define MCP9600_DT_TYPE_S 0x4
#define MCP9600_DT_TYPE_E 0x5
#define MCP9600_DT_TYPE_B 0x6
#define MCP9600_DT_TYPE_R 0x7
/** @} */

/**
 * @defgroup mcp9600_adc_resolution ADC resolution
 * @{
 */
#define MCP9600_DT_ADC_RES_18BIT 0x0
#define MCP9600_DT_ADC_RES_16BIT 0x1
#define MCP9600_DT_ADC_RES_14BIT 0x2
#define MCP9600_DT_ADC_RES_12BIT 0x3
/** @} */

/**
 * @defgroup mcp9600_cold_junction_temp_resolution Cold junction temperature resultion
 * @{
 */
#define MCP9600_DT_COLD_JUNC_TMP_RES_0_0625C 0x0
#define MCP9600_DT_COLD_JUNC_TMP_RES_0_25C   0x1
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_MICRO_MCP9600_H_ */
