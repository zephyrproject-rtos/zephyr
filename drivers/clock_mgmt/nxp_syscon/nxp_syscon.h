/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_CLOCK_MGMT_NXP_SYSCON_NXP_SYSCON_H_
#define ZEPHYR_DRIVERS_CLOCK_MGMT_NXP_SYSCON_NXP_SYSCON_H_


#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

#ifdef CONFIG_SOC_SERIES_LPC55XXX
#include "nxp_lpc55sxx_pll.h"
#endif

/* No data structure needed for mux */
#define Z_CLOCK_MGMT_NXP_SYSCON_CLOCK_MUX_DATA_DEFINE(node_id, prop, idx)
/* Get mux configuration value */
#define Z_CLOCK_MGMT_NXP_SYSCON_CLOCK_MUX_DATA_GET(node_id, prop, idx)         \
	DT_PHA_BY_IDX(node_id, prop, idx, multiplexer)

/* No data structure needed for frgmult */
#define Z_CLOCK_MGMT_NXP_SYSCON_FLEXFRG_DATA_DEFINE(node_id, prop, idx)
/* Get numerator configuration value */
#define Z_CLOCK_MGMT_NXP_SYSCON_FLEXFRG_DATA_GET(node_id, prop, idx)           \
	DT_PHA_BY_IDX(node_id, prop, idx, numerator)

/* No data structure needed for div */
#define Z_CLOCK_MGMT_NXP_SYSCON_CLOCK_DIV_DATA_DEFINE(node_id, prop, idx)
/* Get div configuration value */
#define Z_CLOCK_MGMT_NXP_SYSCON_CLOCK_DIV_DATA_GET(node_id, prop, idx)         \
	DT_PHA_BY_IDX(node_id, prop, idx, divider)

#define Z_CLOCK_MGMT_NXP_LPC55SXX_PLL_PDEC_DATA_DEFINE(node_id, prop, idx)
#define Z_CLOCK_MGMT_NXP_LPC55SXX_PLL_PDEC_DATA_GET(node_id, prop, idx)        \
	DT_PHA_BY_IDX(node_id, prop, idx, pdec)

#define Z_CLOCK_MGMT_NXP_SYSCON_CLOCK_GATE_DATA_DEFINE(node_id, prop, idx)
#define Z_CLOCK_MGMT_NXP_SYSCON_CLOCK_GATE_DATA_GET(node_id, prop, idx)        \
	DT_PHA_BY_IDX(node_id, prop, idx, gate)

#define Z_CLOCK_MGMT_NXP_SYSCON_CLOCK_SOURCE_DATA_DEFINE(node_id, prop, idx)
#define Z_CLOCK_MGMT_NXP_SYSCON_CLOCK_SOURCE_DATA_GET(node_id, prop, idx)      \
	DT_PHA_BY_IDX(node_id, prop, idx, gate)

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_CLOCK_MGMT_NXP_SYSCON_NXP_SYSCON_H_ */
