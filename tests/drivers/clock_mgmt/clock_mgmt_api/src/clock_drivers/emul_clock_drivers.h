/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TEST_DRIVERS_CLOCK_MGMT_CLOCK_DRIVERS_H_
#define ZEPHYR_TEST_DRIVERS_CLOCK_MGMT_CLOCK_DRIVERS_H_

#include <zephyr/devicetree.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

/* Macro definitions for emulated clock drivers */

/* No data structure needed for clock mux */
#define Z_CLOCK_MGMT_VND_EMUL_CLOCK_MUX_DATA_DEFINE(node_id, prop, idx)
/* Get clock mux selector value */
#define Z_CLOCK_MGMT_VND_EMUL_CLOCK_MUX_DATA_GET(node_id, prop, idx)           \
	DT_PHA_BY_IDX(node_id, prop, idx, multiplexer)

/* No data structure needed for clock mux */
#define Z_CLOCK_MGMT_VND_EMUL_CLOCK_DIV_DATA_DEFINE(node_id, prop, idx)
/* Get clock mux selector value */
#define Z_CLOCK_MGMT_VND_EMUL_CLOCK_DIV_DATA_GET(node_id, prop, idx)           \
	DT_PHA_BY_IDX(node_id, prop, idx, divider)

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_TEST_DRIVERS_CLOCK_MGMT_CLOCK_DRIVERS_H_ */
