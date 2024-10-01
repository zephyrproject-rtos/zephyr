/*
 * Copyright 2024 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_BATTERY_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_BATTERY_H_

#include <stdint.h>
#include <errno.h>

#include <zephyr/devicetree.h>
#include <zephyr/math/interpolation.h>
#include <zephyr/sys/util_macro.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief battery API
 * @defgroup battery_apis battery APIs
 * @{
 */

/* Battery chemistry enumeration.
 * Value names must match those from dts/bindings/battery.yaml
 */
enum battery_chemistry {
	BATTERY_CHEMISTRY_UNKNOWN = 0,
	BATTERY_CHEMISTRY_NICKEL_CADMIUM,
	BATTERY_CHEMISTRY_NICKEL_METAL_HYDRIDE,
	BATTERY_CHEMISTRY_LITHIUM_ION,
	BATTERY_CHEMISTRY_LITHIUM_ION_POLYMER,
	BATTERY_CHEMISTRY_LITHIUM_ION_IRON_PHOSPHATE,
	BATTERY_CHEMISTRY_LITHIUM_ION_MANGANESE_OXIDE,
};

/* Length of open circuit voltage table */
#define BATTERY_OCV_TABLE_LEN 11

/**
 * @brief Get the battery chemistry enum value
 *
 * @param node_id node identifier
 */
#define BATTERY_CHEMISTRY_DT_GET(node_id)                                                          \
	UTIL_CAT(BATTERY_CHEMISTRY_, DT_STRING_UPPER_TOKEN_OR(node_id, device_chemistry, UNKNOWN))

/**
 * @brief Get the OCV curve for a given table
 *
 * @param node_id node identifier
 * @param table table to retrieve
 */
#define BATTERY_OCV_TABLE_DT_GET(node_id, table)                                                   \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, table),                                              \
		    ({DT_FOREACH_PROP_ELEM_SEP(node_id, table, DT_PROP_BY_IDX, (,))}), ({-1}))

/**
 * @brief Convert an OCV table and battery voltage to a charge percentage
 *
 * @param ocv_table Open circuit voltage curve
 * @param voltage_uv Battery voltage in microVolts
 *
 * @returns Battery state of charge in milliPercent
 */
static inline int32_t battery_soc_lookup(const int32_t ocv_table[BATTERY_OCV_TABLE_LEN],
					 uint32_t voltage_uv)
{
	static const int32_t soc_axis[BATTERY_OCV_TABLE_LEN] = {
		0, 10000, 20000, 30000, 40000, 50000, 60000, 70000, 80000, 90000, 100000};

	/* Convert voltage to SoC */
	return linear_interpolate(ocv_table, soc_axis, BATTERY_OCV_TABLE_LEN, voltage_uv);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_BATTERY_H_ */
