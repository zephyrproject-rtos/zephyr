/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT renesas_rx_cgc_pclk

#include <string.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <zephyr/dt-bindings/clock/rx_clock.h>
#include <zephyr/drivers/clock_control/renesas_rx_cgc.h>

#if DT_NODE_HAS_STATUS(DT_NODELABEL(pclkblock), okay)
#define MSTP_REGS_ELEM(node_id, prop, idx)                                                         \
	[DT_STRING_TOKEN_BY_IDX(node_id, prop, idx)] =                                             \
		(volatile uint32_t *)DT_REG_ADDR_BY_IDX(node_id, idx),

static volatile uint32_t *mstp_regs[] = {
	DT_FOREACH_PROP_ELEM(DT_NODELABEL(pclkblock), reg_names, MSTP_REGS_ELEM)};
#else
static volatile uint32_t *mstp_regs[] = {};
#endif

static int clock_control_renesas_rx_on(const struct device *dev, clock_control_subsys_t sys)
{
	struct clock_control_rx_subsys_cfg *subsys_clk = (struct clock_control_rx_subsys_cfg *)sys;

	if (!dev || !sys) {
		return -EINVAL;
	}

	renesas_rx_register_protect_disable(RENESAS_RX_REG_PROTECT_LPC_CGC_SWR);
	WRITE_BIT(*mstp_regs[subsys_clk->mstp], subsys_clk->stop_bit, false);
	renesas_rx_register_protect_enable(RENESAS_RX_REG_PROTECT_LPC_CGC_SWR);

	return 0;
}

static int clock_control_renesas_rx_off(const struct device *dev, clock_control_subsys_t sys)
{
	struct clock_control_rx_subsys_cfg *subsys_clk = (struct clock_control_rx_subsys_cfg *)sys;

	if (!dev || !sys) {
		return -EINVAL;
	}

	renesas_rx_register_protect_disable(RENESAS_RX_REG_PROTECT_LPC_CGC_SWR);
	WRITE_BIT(*mstp_regs[subsys_clk->mstp], subsys_clk->stop_bit, true);
	renesas_rx_register_protect_enable(RENESAS_RX_REG_PROTECT_LPC_CGC_SWR);

	return 0;
}

static int clock_control_renesas_rx_get_rate(const struct device *dev, clock_control_subsys_t sys,
					     uint32_t *rate)
{
	const struct clock_control_rx_pclk_cfg *config = dev->config;
	uint32_t clk_src_rate;
	uint32_t clk_div_val;
	int ret;

	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	ret = clock_control_get_rate(config->clock_src_dev, NULL, &clk_src_rate);
	if (ret) {
		return ret;
	}

	clk_div_val = config->clk_div;
	*rate = clk_src_rate / clk_div_val;

	return 0;
}

static DEVICE_API(clock_control, clock_control_renesas_rx_api) = {
	.on = clock_control_renesas_rx_on,
	.off = clock_control_renesas_rx_off,
	.get_rate = clock_control_renesas_rx_get_rate,
};

#define RENESAS_RX_CLOCK_SOURCE(node_id)                                                           \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, clocks), (DEVICE_DT_GET(DT_CLOCKS_CTLR(node_id))),   \
		DEVICE_DT_GET(DT_CLOCKS_CTLR(DT_INST_PARENT(node_id))))

#define INIT_PCLK(node_id)                                                                         \
	static const struct clock_control_rx_pclk_cfg clock_control_cfg_##node_id = {              \
		.clock_src_dev = RENESAS_RX_CLOCK_SOURCE(node_id),                                 \
		.clk_div = DT_INST_PROP_OR(node_id, div, 1),                                       \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(node_id, NULL, NULL, NULL, &clock_control_cfg_##node_id,             \
			      PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS,                   \
			      &clock_control_renesas_rx_api);

DT_INST_FOREACH_STATUS_OKAY(INIT_PCLK);
