/*
 * Copyright (c) 2026 Linumiz
 * Copyright (c) 2026 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_ifx.h>

#include <cy_sysclk.h>

#define IFX_OSC_IMO 0
#define IFX_OSC_EXT 1
#define IFX_OSC_ECO 2

/* ECO configure params */
#define IFX_ECO_CSUM          18UL
#define IFX_ECO_ISR           50UL
#define IFX_ECO_DRIVERLEVEL   100UL
#define IFX_ECO_ENABLETIMEOUT 1000  /* 1 ms */
#define IFX_PLL_ENABLETIMEOUT 10000 /* 10 ms */

#define IFX_NUM_PLL    DT_CHILD_NUM_STATUS_OKAY(DT_PATH(clocks, pll))
#define IFX_NUM_HF_CLK DT_CHILD_NUM_STATUS_OKAY(DT_PATH(clocks, rootclocks))

#define IFX_OSC_CALL(node) ifx_configure_oscillators(dev, DT_REG_ADDR(node))

struct ifx_pll_cfg {
	uint8_t clkpath;
	uint16_t clksource;
	cy_stc_pll_manual_config_t pll_config;
};

struct ifx_hf_clk_cfg {
	uint8_t hf_clk_id;
	uint8_t divider;
	uint16_t source;
};

struct ifx_clk_cfg {
	struct ifx_pll_cfg pll_cfgs[IFX_NUM_PLL];
	struct ifx_hf_clk_cfg hf_cfgs[IFX_NUM_HF_CLK];
};

static inline int ifx_clk_get_rate(const struct device *dev, clock_control_subsys_t sys,
				   uint32_t *rate)
{
	struct ifx_clk *clk = (struct ifx_clk *)sys;

	if ((clk == NULL) || (rate == NULL)) {
		return -EINVAL;
	}

	switch (clk->clk) {
	case IFX_CLK_PLL:
		*rate = Cy_SysClk_PllGetFrequency(clk->clk_id);
		break;
	case IFX_CLK_HF:
		*rate = Cy_SysClk_ClkHfGetFrequency(clk->clk_id);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static inline int ifx_configure_eco(const struct device *dev)
{
	cy_en_sysclk_status_t ret;

	ARG_UNUSED(dev);

	Cy_SysClk_EcoDisable();
	ret = Cy_SysClk_EcoConfigure(DT_PROP(DT_NODELABEL(eco), clock_frequency), IFX_ECO_CSUM,
				     IFX_ECO_ISR, IFX_ECO_DRIVERLEVEL);
	if (ret != 0) {
		return -EINVAL;
	}

	ret = Cy_SysClk_EcoEnable(IFX_ECO_ENABLETIMEOUT);
	if (ret != 0) {
		return -EINVAL;
	}

	return 0;
};

static inline int ifx_configure_oscillators(const struct device *dev, uint8_t osc)
{
	switch (osc) {
	case IFX_OSC_ECO:
#if defined(CONFIG_CPU_CORTEX_M0PLUS)
		return ifx_configure_eco(dev);
#else
		Cy_SysClk_EcoSetFrequency(DT_PROP(DT_NODELABEL(eco), clock_frequency));
		return 0;
#endif
	default:
		return -EINVAL;
	}
}

#if defined(CONFIG_CPU_CORTEX_M0PLUS)
static int ifx_configure_pll(const struct ifx_pll_cfg *pll_configs)
{
	const struct ifx_pll_cfg *pll_cfg = NULL;
	cy_en_sysclk_status_t ret;

	for (int i = 0; i < IFX_NUM_PLL; i++) {
		pll_cfg = &pll_configs[i];
		if (pll_cfg == NULL) {
			continue;
		}

		ret = Cy_SysClk_ClkPathSetSource(pll_cfg->clkpath, pll_cfg->clksource);
		if (ret != 0) {
			return -EINVAL;
		}

		ret = Cy_SysClk_PllDisable(pll_cfg->clkpath);
		if (ret != 0) {
			return -EINVAL;
		}

		ret = Cy_SysClk_PllManualConfigure(pll_cfg->clkpath, &pll_cfg->pll_config);
		if (ret != 0) {
			return -EINVAL;
		}

		ret = Cy_SysClk_PllEnable(pll_cfg->clkpath, IFX_PLL_ENABLETIMEOUT);
		if (ret != 0) {
			return -EINVAL;
		}
	}

	return 0;
}

static int ifx_configure_hf_clk(const struct ifx_hf_clk_cfg *hf_configs)
{
	const struct ifx_hf_clk_cfg *hf_cfg = NULL;
	cy_en_sysclk_status_t ret;

	for (int i = 0; i < IFX_NUM_HF_CLK; i++) {
		hf_cfg = &hf_configs[i];
		if (hf_cfg == NULL) {
			continue;
		}

		ret = Cy_SysClk_ClkHfSetSource(hf_cfg->hf_clk_id, hf_cfg->source);
		if (ret != 0) {
			return -EINVAL;
		}

		ret = Cy_SysClk_ClkHfSetDivider(hf_cfg->hf_clk_id, hf_cfg->divider);
		if (ret != 0) {
			return -EINVAL;
		}

		ret = Cy_SysClk_ClkHfDirectSel(hf_cfg->hf_clk_id, false);
		if (ret != 0) {
			return -EINVAL;
		}

		ret = Cy_SysClk_ClkHfEnable(hf_cfg->hf_clk_id);
		if (ret != 0) {
			return -EINVAL;
		}

		if (hf_cfg->hf_clk_id == 0) {
			SystemCoreClockUpdate();
		}
	}

	return 0;
}
#endif

static int ifx_clk_init(const struct device *dev)
{
	int ret;

	ret = DT_FOREACH_CHILD_STATUS_OKAY_SEP(DT_PATH(clocks, oscillators), IFX_OSC_CALL, (?:));
	if (ret != 0) {
		return ret;
	}

#if defined(CONFIG_CPU_CORTEX_M0PLUS)
	const struct ifx_clk_cfg *cfg = (const struct ifx_clk_cfg *)dev->config;

	ret = ifx_configure_pll(cfg->pll_cfgs);
	if (ret != 0) {
		return ret;
	}

#if DT_NODE_HAS_PROP(DT_NODELABEL(clk_hf0), slow_clk_divider)
	Cy_SysClk_ClkSlowSetDivider(DT_PROP(DT_NODELABEL(clk_hf0), slow_clk_divider));
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(clk_hf0), mem_clk_divider)
	Cy_SysClk_ClkMemSetDivider(DT_PROP(DT_NODELABEL(clk_hf0), mem_clk_divider));
#endif
	ret = ifx_configure_hf_clk(cfg->hf_cfgs);
	if (ret != 0) {
		return ret;
	}
#endif

	return 0;
}

static DEVICE_API(clock_control, ifx_clock_control_api) = {
	.get_rate = ifx_clk_get_rate,
};

#define IFX_CLOCK_PLL_CONFIG(node_id)                                                              \
	{                                                                                          \
		.clkpath = DT_REG_ADDR(node_id),                                                   \
		.clksource = DT_REG_ADDR(DT_CLOCKS_CTLR(node_id)),                                 \
		.pll_config =                                                                      \
			{                                                                          \
				.lfMode = 0,                                                       \
				.feedbackDiv = DT_PROP(node_id, p_div),                            \
				.referenceDiv = DT_PROP(node_id, q_div),                           \
				.outputDiv = DT_PROP_OR(node_id, output_div, 0),                   \
				.outputMode = CY_SYSCLK_FLLPLL_OUTPUT_AUTO,                        \
				.fracDiv = DT_PROP_OR(node_id, frac_div, 0),                       \
				.fracEn = DT_NODE_HAS_PROP(node_id, frac_div),			   \
			},                                                                         \
	}

#define IFX_CLOCK_HF_CONFIG(node_id)                                                               \
	{                                                                                          \
		.hf_clk_id = DT_REG_ADDR(node_id),                                                 \
		.source = DT_REG_ADDR(DT_CLOCKS_CTLR(node_id)),                                    \
		.divider = DT_ENUM_IDX_OR(node_id, divider, 0),                                    \
	}

#define IFX_CLOCK_PLL_FOREACH_SEP(fn, sep)                                                         \
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(DT_PATH(clocks, pll), fn, sep)

#define IFX_CLOCK_HF_CLK_FOREACH_SEP(fn, sep)                                                      \
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(DT_PATH(clocks, rootclocks), fn, sep)

static const struct ifx_clk_cfg ifx_clk_config = {
	.pll_cfgs = {IFX_CLOCK_PLL_FOREACH_SEP(IFX_CLOCK_PLL_CONFIG, (,))},
	.hf_cfgs = {IFX_CLOCK_HF_CLK_FOREACH_SEP(IFX_CLOCK_HF_CONFIG, (,))},
};

DEVICE_DT_DEFINE(DT_NODELABEL(clocks), &ifx_clk_init, NULL, NULL, &ifx_clk_config, PRE_KERNEL_1,
		 CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &ifx_clock_control_api);
