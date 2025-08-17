/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_CLOCK_MANAGEMENT_NXP_LPC55SXX_PLL_H_
#define ZEPHYR_DRIVERS_CLOCK_MANAGEMENT_NXP_LPC55SXX_PLL_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

/* Include fsl_common.h for register definitions */
#include <fsl_common.h>

struct lpc55sxx_pll0_cfg {
	uint32_t CTRL;
	uint32_t NDEC;
	uint32_t SSCG0;
	uint32_t SSCG1;
};

struct lpc55sxx_pll1_cfg {
	uint32_t CTRL;
	uint32_t NDEC;
	uint32_t MDEC;
};

#define Z_CLOCK_MANAGEMENT_DATA_DEFINE_nxp_lpc55sxx_pll0(node_id, prop, idx)		\
	const struct lpc55sxx_pll0_cfg _CONCAT(_CONCAT(node_id, idx), pll0_regs) = {	\
		.CTRL = SYSCON_PLL0CTRL_CLKEN_MASK |					\
			SYSCON_PLL0CTRL_SELI(DT_PHA_BY_IDX(node_id, prop, idx, seli)) | \
			SYSCON_PLL0CTRL_SELP(DT_PHA_BY_IDX(node_id, prop, idx, selp)) |	\
			SYSCON_PLL0CTRL_SELR(DT_PHA_BY_IDX(node_id, prop, idx, selr)) | \
			SYSCON_PLL0CTRL_LIMUPOFF(DT_PHA_BY_IDX(node_id, prop, idx, sscg_en)), \
		.NDEC = SYSCON_PLL0NDEC_NDIV(DT_PHA_BY_IDX(node_id, prop, idx, ndec)),	\
		.SSCG0 = DT_PHA_BY_IDX(node_id, prop, idx, sscg_en) ?                   \
			DT_PHA_BY_IDX(node_id, prop, idx, sscg0) : 0x0,                 \
		.SSCG1 = DT_PHA_BY_IDX(node_id, prop, idx, mdec) ?                      \
			(SYSCON_PLL0SSCG1_SEL_EXT_MASK | SYSCON_PLL0SSCG1_MDIV_EXT(     \
			DT_PHA_BY_IDX(node_id, prop, idx, mdec))) :                     \
			DT_PHA_BY_IDX(node_id, prop, idx, sscg1),                       \
	};
#define Z_CLOCK_MANAGEMENT_DATA_GET_nxp_lpc55sxx_pll0(node_id, prop, idx)               \
	&_CONCAT(_CONCAT(node_id, idx), pll0_regs)

#define Z_CLOCK_MANAGEMENT_DATA_DEFINE_nxp_lpc55sxx_pll1(node_id, prop, idx)		\
	const struct lpc55sxx_pll1_cfg _CONCAT(_CONCAT(node_id, idx), pll1_regs) = {	\
		.CTRL = SYSCON_PLL1CTRL_CLKEN_MASK |					\
			SYSCON_PLL1CTRL_SELI(DT_PHA_BY_IDX(node_id, prop, idx, seli)) | \
			SYSCON_PLL1CTRL_SELP(DT_PHA_BY_IDX(node_id, prop, idx, selp)) |	\
			SYSCON_PLL1CTRL_SELR(DT_PHA_BY_IDX(node_id, prop, idx, selr)),  \
		.NDEC = SYSCON_PLL1NDEC_NDIV(DT_PHA_BY_IDX(node_id, prop, idx, ndec)),	\
		.MDEC = SYSCON_PLL1MDEC_MDIV(DT_PHA_BY_IDX(node_id, prop, idx, mdec)),	\
	};
#define Z_CLOCK_MANAGEMENT_DATA_GET_nxp_lpc55sxx_pll1(node_id, prop, idx)        \
	&_CONCAT(_CONCAT(node_id, idx), pll1_regs)

#define Z_CLOCK_MANAGEMENT_DATA_DEFINE_nxp_lpc55sxx_pll_pdec(node_id, prop, idx)
#define Z_CLOCK_MANAGEMENT_DATA_GET_nxp_lpc55sxx_pll_pdec(node_id, prop, idx)        \
	DT_PHA_BY_IDX(node_id, prop, idx, pdec)

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_CLOCK_MANAGEMENT_NXP_LPC55SXX_PLL_H_ */
