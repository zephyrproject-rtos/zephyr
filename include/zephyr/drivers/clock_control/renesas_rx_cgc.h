/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Renesas RX Clock Generator Circuit (CGC) header file
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_RENESAS_RX_CGC_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_RENESAS_RX_CGC_H_

#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/rx_clock.h>

/**
 * @name Clock source and divider helpers for Renesas RX devices.
 * @{
 */

/**
 * @brief Conditional property getter based on devicetree status.
 *
 * Returns the value of a devicetree property if the node is marked as `okay`.
 * Otherwise, the provided default value is used.
 */
#define RX_CGC_PROP_HAS_STATUS_OKAY_OR(node_id, prop, default_value)                               \
	COND_CODE_1(DT_NODE_HAS_STATUS(node_id, okay), (DT_PROP(node_id, prop)), (default_value))

/**
 * @brief Helper to get clock source form device tree.
 *
 * Expands to a RX_CLOCKS_SOURCE_* constant when the node is enabled, or
 * RX_CLOCKS_CLOCK_DISABLED otherwise.
 */
#define RX_CGC_CLK_SRC(node_id)                                                                    \
	COND_CODE_1(DT_NODE_HAS_STATUS(node_id, okay),                                             \
		    (UTIL_CAT(RX_CLOCKS_SOURCE_, DT_NODE_FULL_NAME_UPPER_TOKEN(node_id))),        \
		    (RX_CLOCKS_CLOCK_DISABLED))

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
