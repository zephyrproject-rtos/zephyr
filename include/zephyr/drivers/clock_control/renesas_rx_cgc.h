/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_RENESAS_RX_CGC_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_RENESAS_RX_CGC_H_

#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/rx_clock.h>

#define RX_CGC_PROP_HAS_STATUS_OKAY_OR(node_id, prop, default_value)                               \
	COND_CODE_1(DT_NODE_HAS_STATUS(node_id, okay), (DT_PROP(node_id, prop)), (default_value))

#define RX_CGC_CLK_SRC(node_id)                                                                    \
	COND_CODE_1(DT_NODE_HAS_STATUS(node_id, okay),                                             \
		    (UTIL_CAT(RX_CLOCKS_SOURCE_, DT_NODE_FULL_NAME_UPPER_TOKEN(node_id))),        \
		    (RX_CLOCKS_CLOCK_DISABLED))

#define RX_IF_CLK_SRC(node_id)                                                                     \
	COND_CODE_1(DT_NODE_HAS_STATUS(node_id, okay),\
			(UTIL_CAT(RX_IF_CLOCKS_SOURCE_, DT_NODE_FULL_NAME_UPPER_TOKEN(node_id))),\
			(RX_CLOCKS_CLOCK_DISABLED))

#define RX_LPT_CLK_SRC(node_id)                                                                    \
	COND_CODE_1(DT_NODE_HAS_STATUS(node_id, okay),\
			(UTIL_CAT(RX_LPT_CLOCKS_SOURCE_, DT_NODE_FULL_NAME_UPPER_TOKEN(node_id))),\
			(RX_LPT_CLOCKS_NON_USE))

#define RX_CGC_PLL_CLK_SRC(node_id)                                                                \
	COND_CODE_1(DT_NODE_HAS_STATUS(node_id, okay),                                             \
		(UTIL_CAT(RX_PLL_CLOCKS_SOURCE_, DT_NODE_FULL_NAME_UPPER_TOKEN(node_id))),        \
		    (RX_CLOCKS_CLOCK_DISABLED))

struct clock_control_rx_pclk_cfg {
	const struct device *clock_src_dev;
	uint32_t clk_div;
};

struct clock_control_rx_subsys_cfg {
	uint32_t mstp;
	uint32_t stop_bit;
};

struct clock_control_rx_pll_cfg {
	const struct device *clock_dev;
};

struct clock_control_rx_pll_data {
	uint32_t pll_div;
	uint32_t pll_mul;
};

struct clock_control_rx_root_cfg {
	uint32_t rate;
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_RENESAS_RX_CGC_H_ */
