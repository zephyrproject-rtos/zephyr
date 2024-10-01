/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_CLOCK_MANAGEMENT_DRIVERS_H_
#define ZEPHYR_DRIVERS_CLOCK_MANAGEMENT_DRIVERS_H_

#include <zephyr/devicetree.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

/* Macro definitions for common clock drivers */

#define Z_CLOCK_MANAGEMENT_CLOCK_SOURCE_DATA_DEFINE(node_id, prop, idx)
#define Z_CLOCK_MANAGEMENT_CLOCK_SOURCE_DATA_GET(node_id, prop, idx)      \
	DT_PHA_BY_IDX(node_id, prop, idx, gate)

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_CLOCK_MANAGEMENT_DRIVERS_H_ */
