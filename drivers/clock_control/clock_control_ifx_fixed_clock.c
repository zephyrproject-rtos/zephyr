/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Clock control driver for Infineon CAT1 MCU family.
 */

#include <zephyr/drivers/clock_control.h>
#include <zephyr/kernel.h>
#include <stdlib.h>

#include <infineon_kconfig.h>
#include <zephyr/drivers/clock_control/clock_control_ifx_cat1.h>
#include <zephyr/dt-bindings/clock/ifx_clock_source_common.h>
#include <zephyr/dt-bindings/clock/ifx_clock_source_boards.h>

#include <cy_sysclk.h>

#define DT_DRV_COMPAT infineon_fixed_clock

struct fixed_rate_clock_config {
	uint32_t rate;
	uint32_t system_clock; /* ifx_cat1_clock_block */
};

__WEAK void ifx_clock_startup_error(uint32_t error)
{
	(void)error; /* Suppress the compiler warning */
	while (1) {
	}
}

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(dpll_hp))
void ifx_clk_dpll_hp0_init(void)
{
#define CY_CFG_SYSCLK_PLL_ERROR 3

	static cy_stc_dpll_hp_config_t dpll_hp_config = {
		.pDiv = 0,
		.nDiv = 15,
		.kDiv = 1,
		.nDivFract = 0,
		.freqModeSel = CY_SYSCLK_DPLL_HP_CLK50MHZ_1US_CNT_VAL,
		.ivrTrim = 0x8U,
		.clkrSel = 0x1U,
		.alphaCoarse = 0xCU,
		.betaCoarse = 0x5U,
		.flockThresh = 0x3U,
		.flockWait = 0x6U,
		.flockLkThres = 0x7U,
		.flockLkWait = 0x4U,
		.alphaExt = 0x14U,
		.betaExt = 0x14U,
		.lfEn = 0x1U,
		.dcEn = 0x1U,
		.outputMode = CY_SYSCLK_FLLPLL_OUTPUT_AUTO,
	};
	static cy_stc_pll_manual_config_t dpll_config = {
		.hpPllCfg = &dpll_hp_config,
	};

#if !defined(CY_PDL_TZ_ENABLED)
	if (Cy_SysClk_PllIsEnabled(SRSS_DPLL_HP_0_PATH_NUM)) {
		return;
	}
#endif
	Cy_SysClk_PllDisable(SRSS_DPLL_HP_0_PATH_NUM);
	if (CY_SYSCLK_SUCCESS !=
	    Cy_SysClk_PllManualConfigure(SRSS_DPLL_HP_0_PATH_NUM, &dpll_config)) {
		ifx_clock_startup_error(CY_CFG_SYSCLK_PLL_ERROR);
	}
	if (CY_SYSCLK_SUCCESS != Cy_SysClk_PllEnable(SRSS_DPLL_HP_0_PATH_NUM, 10000u)) {
		ifx_clock_startup_error(CY_CFG_SYSCLK_PLL_ERROR);
	}
}
#endif

static int fixed_rate_clk_init(const struct device *dev)
{
	const struct fixed_rate_clock_config *const config = dev->config;

	switch (config->system_clock) {

	case IFX_IMO:
		break;

	case IFX_FLL:
		break;

	case IFX_IHO:
		Cy_SysClk_IhoEnable();
		break;

	case IFX_PILO:
		Cy_SysClk_PiloEnable();
		break;

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(dpll_hp))
	case IFX_DPLL500:
		ifx_clk_dpll_hp0_init();
		SystemCoreClockUpdate();
		break;
#endif
	default:
		break;
	}

	return 0;
}

#define FIXED_CLK_INIT(n)                                                                          \
	static const struct fixed_rate_clock_config fixed_rate_clock_config_##n = {                \
		.rate = DT_INST_PROP(n, clock_frequency),                                          \
		.system_clock = DT_INST_PROP(n, system_clock),                                     \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, fixed_rate_clk_init, NULL, NULL, &fixed_rate_clock_config_##n,    \
			      PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(FIXED_CLK_INIT)
