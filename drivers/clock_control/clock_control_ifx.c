/*
 * Copyright (c) 2025 Linumiz GmbH
 * Copyright (c) 2026 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_ifx_cat1.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include <cy_sysclk.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ifx_clock_control, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#define IFX_ECO_CSUM          18UL
#define IFX_ECO_ISR           50UL
#define IFX_ECO_DRIVERLEVEL   100UL
#define IFX_ECO_ENABLETIMEOUT 1000  /* 1 ms */
#define IFX_PLL_ENABLETIMEOUT 10000 /* 10 ms */

struct ifx_clock_hfg_config {
	uint8_t source;
	uint8_t divider;
	bool enable;
};
struct ifx_clock_config {
	cy_en_clkpath_in_sources_t clkpath[SRSS_NUM_CLKPATH];
	bool pll_enable[SRSS_NUM_TOTAL_PLL];
	cy_stc_pll_manual_config_t pll_configs[SRSS_NUM_TOTAL_PLL];
	struct ifx_clock_hfg_config hf_configs[SRSS_NUM_HFROOT];
};

struct ifx_clock_data {
};

static int ifx_clock_get_rate(const struct device *dev, clock_control_subsys_t sys, uint32_t *rate)
{
	return -EINVAL;
}

static int ifx_clock_init(const struct device *dev)
{
	int ret;

#if ( (CONFIG_SOC_FAMILY_INFINEON_CAT1C && CONFIG_CPU_CORTEX_M7) || (CONFIG_SOC_FAMILY_CYT2B7) )
	/* The ECO was configured by the CORTEX_M0P, and the frequency is stored in a variable.
	 * For the M7, we need to update that variable with this function.  The frequency
	 * is needed by the UART driver to calculate the BAUD.
	 */
	Cy_SysClk_EcoSetFrequency(DT_PROP(DT_NODELABEL(clk_eco), clock_frequency));
	return 0;
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(clk_eco), okay)
	ret = Cy_SysClk_EcoConfigure(DT_PROP(DT_NODELABEL(clk_eco), clock_frequency), IFX_ECO_CSUM,
				     IFX_ECO_ISR, IFX_ECO_DRIVERLEVEL);
	if (ret) {
		LOG_ERR("ECO config failed %d\n", ret);
		return ret;
	}
	ret = Cy_SysClk_EcoEnable(IFX_ECO_ENABLETIMEOUT);
	if (ret) {
		LOG_ERR("ECO enable failed %d\n", ret);
		return ret;
	}
#endif

	int i;
	for (i = 0; i < SRSS_NUM_CLKPATH; i++) {
		ret = Cy_SysClk_ClkPathSetSource(
			i, ((struct ifx_clock_config *)dev->config)->clkpath[i]);
		if (ret) {
			LOG_ERR("Clock Path %d source set failed %d\n", i, ret);
			return ret;
		}
	}

	for (i = 1; i <= SRSS_NUM_TOTAL_PLL; i++) {
		if (!((struct ifx_clock_config *)dev->config)->pll_enable[i - 1]) {
			continue;
		}

		ret = Cy_SysClk_PllDisable(i);
		if (ret) {
			LOG_ERR("PLL %d disable failed\n", i);
			return ret;
		}
		ret = Cy_SysClk_PllManualConfigure(
			i, &((struct ifx_clock_config *)dev->config)->pll_configs[i - 1]);
		if (ret) {
			LOG_ERR("PLL %d config failed %d\n", i, ret);
			return ret;
		}
		ret = Cy_SysClk_PllEnable(i, IFX_PLL_ENABLETIMEOUT);
		if (ret) {
			LOG_ERR("PLL %d enable failed %d\n", i, ret);
			return ret;
		}
	}

	for (i = 0; i < SRSS_NUM_HFROOT; i++) {
		if (!((struct ifx_clock_config *)dev->config)->hf_configs[i].enable) {
			continue;
		}
		ret = Cy_SysClk_ClkHfSetDivider(
			i, ((struct ifx_clock_config *)dev->config)->hf_configs[i].divider);
		if (ret) {
			LOG_ERR("HF Clock %d divider set failed %d\n", i, ret);
			return ret;
		}
		ret = Cy_SysClk_ClkHfSetSource(
			i, ((struct ifx_clock_config *)dev->config)->hf_configs[i].source);
		if (ret) {
			LOG_ERR("HF Clock %d source set failed %d\n", i, ret);
			return ret;
		}
#if !defined(CONFIG_SOC_FAMILY_CYT2B7)
		ret = Cy_SysClk_ClkHfDirectSel(i, false);
		if (ret) {
			LOG_ERR("HF Clock %d direct select failed %d\n", i, ret);
			return ret;
		}
#endif
		ret = Cy_SysClk_ClkHfEnable(i);
		if (ret) {
			LOG_ERR("HF Clock %d enable failed %d\n", i, ret);
			return ret;
		}
	}

	return 0;
}

static DEVICE_API(clock_control, clock_control_ifx_cat1_api) = {
	.get_rate = ifx_clock_get_rate,
};

#define IFX_CLOCK_PATH_MUX(node_id) DT_REG_ADDR(DT_CLOCKS_CTLR(node_id))
#define IFX_CLOCK_ROOT_MUX(node_id) DT_REG_ADDR(DT_CLOCKS_CTLR(node_id))

#define IFX_CLOCK_PLL_FOREACH(fn)          DT_FOREACH_CHILD(DT_PATH(clocks, pll), fn)
#define IFX_CLOCK_PLL_FOREACH_SEP(fn, sep) DT_FOREACH_CHILD_SEP(DT_PATH(clocks, pll), fn, sep)

#define IFX_CLOCK_DIRECT_FOREACH(fn)          DT_FOREACH_CHILD(DT_PATH(clocks, direct), fn)
#define IFX_CLOCK_DIRECT_FOREACH_SEP(fn, sep) DT_FOREACH_CHILD_SEP(DT_PATH(clocks, direct), fn, sep)

#define IFX_CLOCH_HF_FOREACH(fn)          DT_FOREACH_CHILD(DT_PATH(clocks, clk_hf), fn)
#define IFX_CLOCK_HF_FOREACH_SEP(fn, sep) DT_FOREACH_CHILD_SEP(DT_PATH(clocks, clk_hf), fn, sep)

#define IFX_CLOCK_PLL_CONFIG(node_id)                                                              \
	{                                                                                          \
		.feedbackDiv = DT_PROP(node_id, p_div), .referenceDiv = DT_PROP(node_id, q_div),   \
		.outputDiv = DT_PROP_OR(node_id, output_div, 0),                                   \
		.outputMode = CY_SYSCLK_FLLPLL_OUTPUT_AUTO, .lfMode = 0,                           \
		.fracDiv = DT_PROP_OR(node_id, frac_div, 0),                                       \
		.fracEn = DT_PROP_OR(node_id, frac_div, 0) != 0,                                   \
	}

#define IFX_CLOCK_HF_CONFIG(node_id)                                                               \
	{                                                                                          \
		.source = IFX_CLOCK_ROOT_MUX(node_id),                                             \
		.divider = DT_ENUM_IDX_OR(node_id, divider, 0),                                    \
		.enable = DT_NODE_HAS_STATUS_OKAY(node_id),                                        \
	}

static const struct ifx_clock_config ifx_clock_config = {
	.clkpath = {IFX_CLOCK_PLL_FOREACH_SEP(IFX_CLOCK_PATH_MUX, (, )),
		    IFX_CLOCK_DIRECT_FOREACH_SEP(IFX_CLOCK_PATH_MUX, (, ))},
	.pll_configs = {IFX_CLOCK_PLL_FOREACH_SEP(IFX_CLOCK_PLL_CONFIG, (, ))},
	.pll_enable = {IFX_CLOCK_PLL_FOREACH_SEP(DT_NODE_HAS_STATUS_OKAY, (, ))},
	.hf_configs = {IFX_CLOCK_HF_FOREACH_SEP(IFX_CLOCK_HF_CONFIG, (, ))}};
static struct ifx_clock_data ifx_clock_data = {};

DEVICE_DT_DEFINE(DT_NODELABEL(clocking), ifx_clock_init, NULL, &ifx_clock_data, &ifx_clock_config,
		 PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &clock_control_ifx_cat1_api);
