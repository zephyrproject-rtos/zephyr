/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TEST_DRIVERS_CLOCK_MANAGEMENT_CLOCK_DRIVERS_H_
#define ZEPHYR_TEST_DRIVERS_CLOCK_MANAGEMENT_CLOCK_DRIVERS_H_

#include <zephyr/devicetree.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

/* Macro definitions for emulated clock drivers */

/* No data structure needed for clock mux */
#define Z_CLOCK_MANAGEMENT_DATA_DEFINE_vnd_emul_clock_mux(node_id, prop, idx)
/* Get clock mux selector value */
#define Z_CLOCK_MANAGEMENT_DATA_GET_vnd_emul_clock_mux(node_id, prop, idx)           \
	DT_PHA_BY_IDX(node_id, prop, idx, multiplexer)

/* No data structure needed for clock mux */
#define Z_CLOCK_MANAGEMENT_DATA_DEFINE_vnd_emul_clock_div(node_id, prop, idx)
/* Get clock mux selector value */
#define Z_CLOCK_MANAGEMENT_DATA_GET_vnd_emul_clock_div(node_id, prop, idx)           \
	DT_PHA_BY_IDX(node_id, prop, idx, divider)

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_TEST_DRIVERS_CLOCK_MANAGEMENT_CLOCK_DRIVERS_H_ */
