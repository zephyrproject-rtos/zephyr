/*
 * Copyright 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_CLOCK_MANAGEMENT_ADI_MAX32_CLOCKS_H_
#define ZEPHYR_DRIVERS_CLOCK_MANAGEMENT_ADI_MAX32_CLOCKS_H_


#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

#define Z_CLOCK_MANAGEMENT_DATA_DEFINE_adi_max32_clock_source(node_id, prop, idx)
#define Z_CLOCK_MANAGEMENT_DATA_GET_adi_max32_clock_source(node_id, prop, idx)      \
	DT_PHA_BY_IDX(node_id, prop, idx, gate)

#define Z_CLOCK_MANAGEMENT_DATA_DEFINE_adi_max32_clock_div(node_id, prop, idx)
#define Z_CLOCK_MANAGEMENT_DATA_GET_adi_max32_clock_div(node_id, prop, idx)      \
	DT_PHA_BY_IDX(node_id, prop, idx, divider)

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_CLOCK_MANAGEMENT_ADI_MAX32_CLOCKS_H_ */
