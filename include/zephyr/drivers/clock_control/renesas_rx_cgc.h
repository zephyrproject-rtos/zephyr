/*
 * Copyright (c) 2024-2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Renesas RX Clock Generator Circuit (CGC) header file
 * @ingroup clock_control_renesas_rx
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_RENESAS_RX_CGC_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_RENESAS_RX_CGC_H_

#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/rx_clock.h>

/**
 * @defgroup clock_control_renesas_rx Renesas RX CGC
 * @ingroup clock_control_interface_ext
 * @{
 */

/**
 * @brief Conditional property getter based on devicetree status.
 *
 * Returns the value of a devicetree property if the node is marked as `okay`.
 * Otherwise, the provided default value is used.
 */
#define RX_CGC_PROP_HAS_STATUS_OKAY_OR(node_id, prop, default_value)                               \
	COND_CODE_1(DT_NODE_HAS_STATUS(node_id, okay), (DT_PROP(node_id, prop)),                   \
		    (default_value))

/**
 * @brief Helper to get clock source form device tree.
 *
 * Expands to a RX_CLOCKS_SOURCE_* constant when the node is enabled, or
 * RX_CLOCKS_CLOCK_DISABLED otherwise.
 */
#ifdef CONFIG_HAS_RENESAS_RX_FSP
#define RX_CGC_CLK_SRC(node_id)                                                                    \
	COND_CODE_1(DT_NODE_HAS_STATUS(node_id, okay),                                             \
		    (UTIL_CAT(BSP_CLOCKS_SOURCE_, DT_NODE_FULL_NAME_UPPER_TOKEN(node_id))),        \
		    (BSP_CLOCKS_CLOCK_DISABLED))

#define RX_CGC_CLK_DIV(clk, prop, default_value)                                                   \
	UTIL_CAT(RX_CGC_DIV_, DT_NODE_FULL_NAME_UPPER_TOKEN(clk))                                  \
	(RX_CGC_PROP_HAS_STATUS_OKAY_OR(clk, prop, default_value))

#else
#define RX_CGC_CLK_SRC(node_id)                                                                    \
	COND_CODE_1(DT_NODE_HAS_STATUS(node_id, okay),                                             \
		    (UTIL_CAT(RX_CLOCKS_SOURCE_, DT_NODE_FULL_NAME_UPPER_TOKEN(node_id))),        \
		    (RX_CLOCKS_CLOCK_DISABLED))
#endif

#ifdef CONFIG_HAS_RENESAS_RX_FSP
#define RX_CGC_DIV_CPUCLK(n)     UTIL_CAT(BSP_CLOCKS_SYS_CLOCK_DIV_, n)
#define RX_CGC_DIV_MRICLK(n)     UTIL_CAT(BSP_CLOCKS_SYS_CLOCK_DIV_, n)
#define RX_CGC_DIV_ICLK(n)       UTIL_CAT(BSP_CLOCKS_SYS_CLOCK_DIV_, n)
#define RX_CGC_DIV_PCLKA(n)      UTIL_CAT(BSP_CLOCKS_SYS_CLOCK_DIV_, n)
#define RX_CGC_DIV_PCLKB(n)      UTIL_CAT(BSP_CLOCKS_SYS_CLOCK_DIV_, n)
#define RX_CGC_DIV_PCLKC(n)      UTIL_CAT(BSP_CLOCKS_SYS_CLOCK_DIV_, n)
#define RX_CGC_DIV_PCLKD(n)      UTIL_CAT(BSP_CLOCKS_SYS_CLOCK_DIV_, n)
#define RX_CGC_DIV_PCLKE(n)      UTIL_CAT(BSP_CLOCKS_SYS_CLOCK_DIV_, n)
#define RX_CGC_DIV_BCLK(n)       UTIL_CAT(BSP_CLOCKS_SYS_CLOCK_DIV_, n)
#define RX_CGC_DIV_BCLKA(n)      UTIL_CAT(BSP_CLOCKS_SYS_CLOCK_DIV_, n)
#define RX_CGC_DIV_MRPCLK(n)     UTIL_CAT(BSP_CLOCKS_SYS_CLOCK_DIV_, n)
#define RX_CGC_DIV_CLKOUT(n)     UTIL_CAT(BSP_CLOCKS_SYS_CLOCK_DIV_, n)
#define RX_CGC_DIV_SCICLK(n)     UTIL_CAT(BSP_CLOCKS_SCI_CLOCK_DIV_, n)
#define RX_CGC_DIV_SPICLK(n)     UTIL_CAT(BSP_CLOCKS_SPI_CLOCK_DIV_, n)
#define RX_CGC_DIV_CANFDCLK(n)   UTIL_CAT(BSP_CLOCKS_CANFD_CLOCK_DIV_, n)
#define RX_CGC_DIV_GPTCLK(n)     UTIL_CAT(BSP_CLOCKS_GPT_CLOCK_DIV_, n)
#define RX_CGC_DIV_I3CCLK(n)     UTIL_CAT(BSP_CLOCKS_I3C_CLOCK_DIV_, n)
#define RX_CGC_DIV_UCLK(n)       UTIL_CAT(BSP_CLOCKS_USB_CLOCK_DIV_, n)
#define RX_CGC_DIV_USB60CLK(n)   UTIL_CAT(BSP_CLOCKS_USB60_CLOCK_DIV_, n)
#define RX_CGC_DIV_OCTASPICLK(n) UTIL_CAT(BSP_CLOCKS_OCTA_CLOCK_DIV_, n)
#define RX_CGC_DIV_PLL(n)        UTIL_CAT(BSP_CLOCKS_PLL_DIV_, n)
#define RX_CGC_DIV_PLLP(n)       UTIL_CAT(BSP_CLOCKS_PLL_DIV_, n)
#define RX_CGC_DIV_PLLQ(n)       UTIL_CAT(BSP_CLOCKS_PLL_DIV_, n)
#define RX_CGC_DIV_PLLR(n)       UTIL_CAT(BSP_CLOCKS_PLL_DIV_, n)
#define RX_CGC_DIV_PLL2(n)       UTIL_CAT(BSP_CLOCKS_PLL_DIV_, n)
#define RX_CGC_DIV_PLL2P(n)      UTIL_CAT(BSP_CLOCKS_PLL_DIV_, n)
#define RX_CGC_DIV_PLL2Q(n)      UTIL_CAT(BSP_CLOCKS_PLL_DIV_, n)
#define RX_CGC_DIV_PLL2R(n)      UTIL_CAT(BSP_CLOCKS_PLL_DIV_, n)
#define RX_CGC_DIV_ADCCLK(n)     UTIL_CAT(BSP_CLOCKS_ADC_CLOCK_DIV_, n)
#define RX_CGC_DIV_ESWCLK(n)     UTIL_CAT(BSP_CLOCKS_ESW_CLOCK_DIV_, n)
#define RX_CGC_DIV_ESWPHYCLK(n)  UTIL_CAT(BSP_CLOCKS_ESWPHY_CLOCK_DIV_, n)
#define RX_CGC_DIV_ETHPHYCLK(n)  UTIL_CAT(BSP_CLOCKS_ETHPHY_CLOCK_DIV_, n)
#define RX_CGC_DIV_ESCCLK(n)     UTIL_CAT(BSP_CLOCKS_ESC_CLOCK_DIV_, n)
#define RX_CGC_DIV_DSMIFCLK(n)   UTIL_CAT(BSP_CLOCKS_DSMIF_CLOCK_DIV_, n)

#define BSP_CLOCKS_SOURCE_PLL  BSP_CLOCKS_SOURCE_CLOCK_PLL
#define BSP_CLOCKS_SOURCE_PLLP BSP_CLOCKS_SOURCE_CLOCK_PLL
#define BSP_CLOCKS_SOURCE_PLLQ BSP_CLOCKS_SOURCE_CLOCK_PLL1Q
#define BSP_CLOCKS_SOURCE_PLLR BSP_CLOCKS_SOURCE_CLOCK_PLL1R

#define BSP_CLOCKS_SOURCE_PLL2  BSP_CLOCKS_SOURCE_CLOCK_PLL2
#define BSP_CLOCKS_SOURCE_PLL2P BSP_CLOCKS_SOURCE_CLOCK_PLL2
#define BSP_CLOCKS_SOURCE_PLL2Q BSP_CLOCKS_SOURCE_CLOCK_PLL2Q
#define BSP_CLOCKS_SOURCE_PLL2R BSP_CLOCKS_SOURCE_CLOCK_PLL2R

#endif

/**
 * @brief Helper to get IF clock source form device tree.
 *
 * Expands to a RX_IF_CLOCKS_SOURCE_* constant when the node is enabled, or
 * RX_CLOCKS_CLOCK_DISABLED otherwise.
 */
#define RX_IF_CLK_SRC(node_id)                                                                     \
	COND_CODE_1(DT_NODE_HAS_STATUS(node_id, okay),\
			(UTIL_CAT(RX_IF_CLOCKS_SOURCE_, DT_NODE_FULL_NAME_UPPER_TOKEN(node_id))),\
			(RX_CLOCKS_CLOCK_DISABLED))

/**
 * @brief Helper to get LPT clock source form device tree.
 *
 * Expands to a RX_LPT_CLOCKS_SOURCE_* constant when the node is enabled, or
 * RX_LPT_CLOCKS_NON_USE otherwise.
 */
#define RX_LPT_CLK_SRC(node_id)                                                                    \
	COND_CODE_1(DT_NODE_HAS_STATUS(node_id, okay),\
			(UTIL_CAT(RX_LPT_CLOCKS_SOURCE_, DT_NODE_FULL_NAME_UPPER_TOKEN(node_id))),\
			(RX_LPT_CLOCKS_NON_USE))

/**
 * @brief Helper to get PLL clock source form device tree.
 *
 * Expands to a RX_PLL_CLOCKS_SOURCE_* constant when the node is enabled, or
 * RX_CLOCKS_CLOCK_DISABLED otherwise.
 */
#define RX_CGC_PLL_CLK_SRC(node_id)                                                                \
	COND_CODE_1(DT_NODE_HAS_STATUS(node_id, okay),                                             \
		(UTIL_CAT(RX_PLL_CLOCKS_SOURCE_, DT_NODE_FULL_NAME_UPPER_TOKEN(node_id))),        \
		    (RX_CLOCKS_CLOCK_DISABLED))

/**
 * @brief Peripheral clock configuration (PCLK).
 */
struct clock_control_rx_pclk_cfg {
	const struct device *clock_src_dev; /**< Clock source. */
	uint32_t clk_div;                   /**< Divider configuration. */
};

/**
 * @brief Subsystem clock control configuration.
 */
struct clock_control_rx_subsys_cfg {
	uint32_t mstp;     /**< MSTP register index. */
	uint32_t stop_bit; /**< Clock stop bit. */
};

/**
 * @brief PLL configuration structure.
 */
struct clock_control_rx_pll_cfg {
	const struct device *clock_dev; /**< Device providing PLL source. */
};

/**
 * @brief PLL control parameters.
 */
struct clock_control_rx_pll_data {
	uint32_t pll_div; /**< PLL divider. */
	uint32_t pll_mul; /**< PLL multiplier. */
};

/**
 * @brief Root clock configuration.
 */
struct clock_control_rx_root_cfg {
	uint32_t rate; /**< Target clock rate in Hz. */
};

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_RENESAS_RX_CGC_H_ */
