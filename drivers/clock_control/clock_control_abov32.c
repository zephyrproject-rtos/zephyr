/*
 * Copyright (c) 2026 ABOV Semiconductor Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT abov_abov32_cctl

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/abov32-clock.h>
#include <zephyr/kernel.h>

#include <hal_scu_clk.h>
#include <type/scu_clk_type.h>

#include "clock_control_abov32.h"

struct clock_control_abov32_config {
	uint32_t hsi_frequency;
	uint32_t lsi_frequency;
	bool hse_enabled;
	uint32_t hse_frequency;
	bool lse_enabled;
	uint32_t lse_frequency;
	bool pll_enabled;
	SCUCLK_PLL_CFG_t pll_cfg;
	uint32_t pll_output_frequency;
	SCUCLK_SRC_e mclk_source;
	SCUCLK_DIV_e mclk_pre_div;
	SCUCLK_DIV_e mclk_post_div;
	SCUCLK_DIV_e pclk_div;
};

static uint32_t clock_control_abov32_src_rate(const struct clock_control_abov32_config *cfg,
					       SCUCLK_SRC_e src)
{
	switch (src) {
	case SCUCLK_SRC_HSE:
		return cfg->hse_frequency;
	case SCUCLK_SRC_HSI:
		return cfg->hsi_frequency;
	case SCUCLK_SRC_LSI:
		return cfg->lsi_frequency;
	case SCUCLK_SRC_LSE:
		return cfg->lse_frequency;
	case SCUCLK_SRC_PLL:
		return cfg->pll_output_frequency;
	default:
		return 0U;
	}
}

static int clock_control_abov32_get_rate(const struct device *dev, clock_control_subsys_t sys,
					  uint32_t *rate)
{
	const struct clock_control_abov32_config *cfg = dev->config;
	uint32_t id = (uint32_t)(uintptr_t)sys;
	uint32_t mclk_rate;
	uint32_t hclk_rate;

	switch (id) {
	case ABOV32_CLOCK_HSE:
	case ABOV32_CLOCK_HSI:
	case ABOV32_CLOCK_LSI:
	case ABOV32_CLOCK_LSE:
	case ABOV32_CLOCK_PLL:
		*rate = clock_control_abov32_src_rate(cfg, (SCUCLK_SRC_e)id);
		return 0;
	case ABOV32_CLOCK_MCLK:
	case ABOV32_CLOCK_HCLK:
		mclk_rate = clock_control_abov32_src_rate(cfg, cfg->mclk_source) >>
			    cfg->mclk_pre_div;
		hclk_rate = mclk_rate >> cfg->mclk_post_div;
		*rate = (id == ABOV32_CLOCK_MCLK) ? mclk_rate : hclk_rate;
		return 0;
	case ABOV32_CLOCK_PCLK:
		mclk_rate = clock_control_abov32_src_rate(cfg, cfg->mclk_source) >>
			    cfg->mclk_pre_div;
		hclk_rate = mclk_rate >> cfg->mclk_post_div;
		*rate = hclk_rate >> cfg->pclk_div;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static enum clock_control_status clock_control_abov32_get_status(const struct device *dev,
								   clock_control_subsys_t sys)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(sys);

	/* SCU clock sources configured at init are never gated off at runtime. */
	return CLOCK_CONTROL_STATUS_ON;
}

static DEVICE_API(clock_control, clock_control_abov32_api) = {
	.get_rate = clock_control_abov32_get_rate,
	.get_status = clock_control_abov32_get_status,
};

static int clock_control_abov32_configure(const struct clock_control_abov32_config *cfg)
{
	SCUCLK_MCLK_CFG_t mclk_cfg = {
		.eMClk = cfg->mclk_source,
		.ePreMClkDiv = cfg->mclk_pre_div,
		.ePostMClkDiv = cfg->mclk_post_div,
	};

	if (cfg->hse_enabled && HAL_SCU_CLK_SetSrcEnable(SCUCLK_SRC_HSE, true) != HAL_ERR_OK) {
		return -EIO;
	}

	if (cfg->lse_enabled && HAL_SCU_CLK_SetSrcEnable(SCUCLK_SRC_LSE, true) != HAL_ERR_OK) {
		return -EIO;
	}

	if (cfg->pll_enabled) {
		SCUCLK_PLL_CFG_t pll_cfg = cfg->pll_cfg;

		if (HAL_SCU_CLK_SetPLLConfig(true, &pll_cfg) != HAL_ERR_OK) {
			return -EIO;
		}
	}

	if (HAL_SCU_CLK_SetMClk(&mclk_cfg) != HAL_ERR_OK) {
		return -EIO;
	}

	if (HAL_SCU_CLK_SetPClkDiv(cfg->pclk_div) != HAL_ERR_OK) {
		return -EIO;
	}

	return 0;
}

static int clock_control_abov32_init(const struct device *dev)
{
	return clock_control_abov32_configure(dev->config);
}

static const struct clock_control_abov32_config clock_control_abov32_cfg_0 = {
	.hsi_frequency = DT_INST_PROP(0, hsi_frequency),
	.lsi_frequency = DT_INST_PROP(0, lsi_frequency),
	.hse_enabled = DT_INST_NODE_HAS_PROP(0, hse_frequency),
	.hse_frequency = DT_INST_PROP_OR(0, hse_frequency, 0),
	.lse_enabled = DT_INST_NODE_HAS_PROP(0, lse_frequency),
	.lse_frequency = DT_INST_PROP_OR(0, lse_frequency, 0),
	.pll_enabled = DT_INST_NODE_HAS_PROP(0, pll_output_frequency),
	.pll_cfg = {
		.eSrc = DT_INST_PROP(0, pll_source_hse) ? SCUCLK_PLL_SRC_HSE : SCUCLK_PLL_SRC_HSI,
		.eSrcDiv = DT_INST_PROP_OR(0, pll_source_div, 0),
		.un8OutDiv = DT_INST_PROP_OR(0, pll_out_div, 0),
		.un8PostDiv1 = DT_INST_PROP_OR(0, pll_post_div1, 0),
		.un8PostDiv2 = DT_INST_PROP_OR(0, pll_post_div2, 0),
		.un8PreDiv = DT_INST_PROP_OR(0, pll_pre_div, 0),
		.eCurOpt = SCUCLK_PLL_CTRLOPT_10UA,
		.eVcoBias = SCUCLK_PLL_VCOBIAS_NONE,
		.ePllMode = DT_INST_PROP(0, pll_vco2x) ? SCUCLK_PLL_MODE_VCO2X
							: SCUCLK_PLL_MODE_VCO,
	},
	.pll_output_frequency = DT_INST_PROP_OR(0, pll_output_frequency, 0),
	.mclk_source = DT_INST_PROP_OR(0, mclk_source, ABOV32_CLOCK_HSI),
	.mclk_pre_div = DT_INST_PROP_OR(0, mclk_pre_div, 0),
	.mclk_post_div = DT_INST_PROP_OR(0, mclk_post_div, 0),
	.pclk_div = DT_INST_PROP_OR(0, pclk_div, 0),
};

DEVICE_DT_INST_DEFINE(0, clock_control_abov32_init, NULL, NULL, &clock_control_abov32_cfg_0,
		       PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		       &clock_control_abov32_api);

void clock_control_abov32_reinit(void)
{
	/*
	 * Only one cctl instance exists on this SoC (DT_DRV_COMPAT is
	 * single-instance here), so there's no device pointer to look up --
	 * go straight at the same config struct clock_control_abov32_init()
	 * used. A failure here has nowhere useful to report to (this runs
	 * from pm_state_exit_post_ops(), which returns void), so it's
	 * intentionally not checked.
	 */
	(void)clock_control_abov32_configure(&clock_control_abov32_cfg_0);
}
