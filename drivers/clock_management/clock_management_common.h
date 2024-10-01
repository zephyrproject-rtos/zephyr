/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_CLOCK_MANAGEMENT_CLOCK_MANAGEMENT_COMMON_H_
#define ZEPHYR_DRIVERS_CLOCK_MANAGEMENT_CLOCK_MANAGEMENT_COMMON_H_

/**
 * @brief Defines clock management data for a specific clock
 *
 * Defines clock management data for a clock, based on the clock's compatible
 * string. Given clock nodes with compatibles like so:
 *
 * @code{.dts}
 *     a {
 *             compatible = "vnd,source";
 *     };
 *
 *     b {
 *             compatible = "vnd,mux";
 *     };
 *
 *     c {
 *             compatible = "vnd,div";
 *     };
 * @endcode
 *
 * The clock driver must provide definitions like so:
 *
 * @code{.c}
 *     #define Z_CLOCK_MANAGEMENT_VND_SOURCE_DATA_DEFINE(node_id, prop, idx)
 *     #define Z_CLOCK_MANAGEMENT_VND_MUX_DATA_DEFINE(node_id, prop, idx)
 *     #define Z_CLOCK_MANAGEMENT_VND_DIV_DATA_DEFINE(node_id, prop, idx)
 * @endcode
 *
 * All macros take the node id of the node with the clock-state-i, the name of
 * the clock-state-i property, and the index of the phandle for this clock node
 * as arguments. The _DATA_DEFINE macros should initialize any data structure
 * needed by the clock.
 *
 * @param node_id Node identifier
 * @param prop clock property name
 * @param idx property index
 */
#define Z_CLOCK_MANAGEMENT_CLK_DATA_DEFINE(node_id, prop, idx)                       \
	_CONCAT(_CONCAT(Z_CLOCK_MANAGEMENT_, DT_STRING_UPPER_TOKEN(                  \
		DT_PHANDLE_BY_IDX(node_id, prop, idx), compatible_IDX_0)),     \
		_DATA_DEFINE)(node_id, prop, idx);

/**
 * @brief Gets clock management data for a specific clock
 *
 * Reads clock management data for a clock, based on the clock's compatible
 * string. Given clock nodes with compatibles like so:
 *
 * @code{.dts}
 *     a {
 *             compatible = "vnd,source";
 *     };
 *
 *     b {
 *             compatible = "vnd,mux";
 *     };
 *
 *     c {
 *             compatible = "vnd,div";
 *     };
 * @endcode
 *
 * The clock driver must provide definitions like so:
 *
 * @code{.c}
 *     #define Z_CLOCK_MANAGEMENT_VND_SOURCE_DATA_GET(node_id, prop, idx)
 *     #define Z_CLOCK_MANAGEMENT_VND_MUX_DATA_GET(node_id, prop, idx)
 *     #define Z_CLOCK_MANAGEMENT_VND_DIV_DATA_GET(node_id, prop, idx)
 * @endcode
 *
 * All macros take the node id of the node with the clock-state-i, the name of
 * the clock-state-i property, and the index of the phandle for this clock node
 * as arguments.
 * The _DATA_GET macros should get a reference to the clock data structure
 * data structure, which will be cast to a void pointer by the clock management
 * subsystem.
 * @param node_id Node identifier
 * @param prop clock property name
 * @param idx property index
 */
#define Z_CLOCK_MANAGEMENT_CLK_DATA_GET(node_id, prop, idx)                          \
	(void *)_CONCAT(_CONCAT(Z_CLOCK_MANAGEMENT_, DT_STRING_UPPER_TOKEN(          \
		DT_PHANDLE_BY_IDX(node_id, prop, idx), compatible_IDX_0)),     \
		_DATA_GET)(node_id, prop, idx)

#endif /* ZEPHYR_DRIVERS_CLOCK_MANAGEMENT_CLOCK_MANAGEMENT_COMMON_H_ */
