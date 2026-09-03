/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Infineon clock control driver public API
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_IFX_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_IFX_H_

#include <stdint.h>
#include <zephyr/devicetree.h>

/** PLL clock type */
#define IFX_CLK_PLL 0

/** High-frequency clock type */
#define IFX_CLK_HF  1

/** Low-frequency clock type */
#define IFX_CLK_LF  2

/** Infineon clock specifier */
struct ifx_clk {
	/** Clock type */
	uint8_t clk;
	/** Clock ID */
	uint8_t clk_id;
};

/** Infineon peripheral clock specifier */
struct ifx_clk_peri {
	/** Dependency ordinal of the referenced peri-div divider node */
	uint32_t div_ord;
	/** Root clock ID (peripheral clock ID / peripheral mux ID) */
	uint8_t rootclk_id;
};

/**
 * @brief Build a struct ifx_clk_peri from a peripheral's clocks
 * property and clock destination property (peripheral clock ID).
 */
#define IFX_CLK_PERI_DT_SPEC_GET(node_id)			\
	{							\
		.rootclk_id = DT_PROP(node_id, clk_dst),	\
		.div_ord = DT_DEP_ORD(DT_CLOCKS_CTLR(node_id)),	\
	}

/** @brief Like IFX_CLK_PERI_DT_SPEC_GET(), by DT_DRV_COMPAT instance number. */
#define IFX_CLK_PERI_DT_INST_SPEC_GET(inst)	\
	IFX_CLK_PERI_DT_SPEC_GET(DT_DRV_INST(inst))

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_IFX_H_ */
