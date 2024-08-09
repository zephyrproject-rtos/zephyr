/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_CLOCK_MGMT_DRIVERS_H_
#define ZEPHYR_DRIVERS_CLOCK_MGMT_DRIVERS_H_

#include <zephyr/devicetree.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

/* Macro definitions for common clock drivers */

/* No data structure needed for clock output */
#define Z_CLOCK_MGMT_CLOCK_OUTPUT_DATA_DEFINE(node_id, prop, idx)
/* Get clock output frequency value */
#define Z_CLOCK_MGMT_CLOCK_OUTPUT_DATA_GET(node_id, prop, idx)         \
	DT_PHA_BY_IDX(node_id, prop, idx, frequency)

/* Include individual clock driver headers here */

#ifdef CONFIG_CLOCK_MGMT_NXP_SYSCON
#include "nxp_syscon/nxp_syscon.h"
#endif

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_CLOCK_MGMT_DRIVERS_H_ */
