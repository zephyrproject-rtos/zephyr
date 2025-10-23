/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Clock control driver for Infineon CAT1 MCU family.
 */

#include <infineon_kconfig.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/kernel.h>
#include <stdlib.h>

#include <zephyr/drivers/clock_control/clock_control_ifx_cat1.h>
#include <zephyr/dt-bindings/clock/ifx_clock_source_common.h>
#include <zephyr/dt-bindings/clock/ifx_clock_source_boards.h>

#include <cy_sysclk.h>

#define DT_DRV_COMPAT infineon_fixed_clock

struct fixed_rate_clock_config {
	uint32_t rate;
	uint32_t system_clock; /* ifx_cat1_clock_block */
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(dpll_lp0)) ||                                             \
	DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(dpll_lp1))
	cy_stc_dpll_lp_config_t dpll_lp_config;
#endif
};

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(dpll_hp)) ||                                              \
	DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(dpll_lp0)) ||                                         \
	DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(dpll_lp1))
static void clock_startup_error(uint32_t error)
{
	(void)error; /* Suppress the compiler warning */
	while (1) {
	}
}
#endif

#define CY_CFG_SYSCLK_PLL_ERROR 3

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(dpll_lp0)) ||                                             \
	DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(dpll_lp1))
#define DPLL_LP0 SRSS_PLL_250M_0_PATH_NUM
#define DPLL_LP1 SRSS_PLL_250M_1_PATH_NUM
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(dpll_lp0)) ||                                             \
	DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(dpll_lp1))
static void clk_dpll_lp_init(uint32_t dpll_lp, cy_stc_dpll_lp_config_t dpll_lp_config)
{
	cy_stc_pll_manual_config_t dpll_config = {
		.lpPllCfg = &dpll_lp_config,
	};

#if !defined(CY_PDL_TZ_ENABLED)
	if (Cy_SysClk_PllIsEnabled(dpll_lp)) {
		return;
	}
#endif

	Cy_SysClk_PllDisable(dpll_lp);
	if (CY_SYSCLK_SUCCESS != Cy_SysClk_PllManualConfigure(dpll_lp, &dpll_config)) {
		clock_startup_error(CY_CFG_SYSCLK_PLL_ERROR);
	}

#if (defined(CY_IP_MXS22SRSS_VERSION) && defined(CY_IP_MXS22SRSS_VERSION)) &&                      \
	((CY_IP_MXS22SRSS_VERSION == 1) && (CY_IP_MXS22SRSS_VERSION_MINOR == 0))
	/* The workaround for devices with MXS22SRSS block 1.0 */
	uint32_t clk_hf_mask = Cy_SysClk_ClkHfGetMaskOnPath((cy_en_clkhf_in_sources_t)dpll_lp);

	if (clk_hf_mask) {
		Cy_SysClk_ClkHfEnableDirectMuxWithMask(clk_hf_mask, true);
	}
#endif /* ((CY_IP_MXS22SRSS_VERSION == 1) && (CY_IP_MXS22SRSS_VERSION_MINOR == 0)) */

	if (CY_SYSCLK_SUCCESS != Cy_SysClk_PllEnable(dpll_lp, 10000u)) {
		clock_startup_error(CY_CFG_SYSCLK_PLL_ERROR);
	} else {
		/* The workaround for devices with MXS22SRSS block 1.0 */
#if (defined(CY_IP_MXS22SRSS_VERSION) && defined(CY_IP_MXS22SRSS_VERSION)) &&                      \
	((CY_IP_MXS22SRSS_VERSION == 1) && (CY_IP_MXS22SRSS_VERSION_MINOR == 0))
		Cy_SysLib_DelayUs(SRSS_DPLL_LP_INIT_DELAY_USEC);
		Cy_SysClk_ClkHfEnableDirectMuxWithMask(clk_hf_mask, false);
#endif /* ((CY_IP_MXS22SRSS_VERSION == 1) && (CY_IP_MXS22SRSS_VERSION_MINOR == 0)) */
	}
}
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(dpll_hp))
static void ifx_clk_dpll_hp0_init(void)
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
		clock_startup_error(CY_CFG_SYSCLK_PLL_ERROR);
	}
	if (CY_SYSCLK_SUCCESS != Cy_SysClk_PllEnable(SRSS_DPLL_HP_0_PATH_NUM, 10000u)) {
		clock_startup_error(CY_CFG_SYSCLK_PLL_ERROR);
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

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(dpll_lp0))
	case IFX_DPLL250_0:
#ifdef WA__DRIVERS_21925
		/* Workaround: update DPLL_LP trim values */
		CY_SET_REG32(0x52403218, 0x921F190A); /* DPLL_LP0_TEST3 */
		CY_SET_REG32(0x5240321C, 0x08100000); /* DPLL_LP0_TEST4 */
#endif
		clk_dpll_lp_init(DPLL_LP0, config->dpll_lp_config);
		SystemCoreClockUpdate();
		break;
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(dpll_lp1))
	case IFX_DPLL250_1:
#ifdef WA__DRIVERS_21925
		/* Workaround: update DPLL_LP trim values */
		CY_SET_REG32(0x52403238, 0x921F190A); /* DPLL_LP1_TEST3 */
		CY_SET_REG32(0x5240323C, 0x08100000); /* DPLL_LP1_TEST4 */
#endif
		clk_dpll_lp_init(DPLL_LP1, config->dpll_lp_config);
		SystemCoreClockUpdate();
		break;
#endif

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

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(dpll_lp0)) ||                                             \
	DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(dpll_lp1))
#define DPLL_LP_INIT(n)                                                                            \
	.dpll_lp_config = {                                                                        \
		.feedbackDiv = DT_INST_PROP_OR(n, feedback_div, 0),                                \
		.referenceDiv = DT_INST_PROP_OR(n, reference_div, 0),                              \
		.outputDiv = DT_INST_PROP_OR(n, output_div, 0),                                    \
		.pllDcoMode = DT_INST_PROP_OR(n, dco_mode_enable, false),                          \
		.outputMode = CY_SYSCLK_FLLPLL_OUTPUT_AUTO,                                        \
		.fracDiv = DT_INST_PROP_OR(n, fraction_div, 0),                                    \
		.fracDitherEn = false,                                                             \
		.fracEn = true,                                                                    \
		.sscgDepth = 0x0U,                                                                 \
		.sscgRate = 0x0U,                                                                  \
		.sscgDitherEn = 0x0U,                                                              \
		.sscgMode = 0x0U,                                                                  \
		.sscgEn = 0x0U,                                                                    \
		.dcoCode = 0x0U,                                                                   \
		.accMode = 0x1U,                                                                   \
		.tdcMode = 0x1U,                                                                   \
		.pllTg = 0x0U,                                                                     \
		.accCntLock = 0x0U,                                                                \
		.kiInt = 0x24U,                                                                    \
		.kpInt = 0x1CU,                                                                    \
		.kiAccInt = 0x23U,                                                                 \
		.kpAccInt = 0x1AU,                                                                 \
		.kiFrac = 0x24U,                                                                   \
		.kpFrac = 0x20U,                                                                   \
		.kiAccFrac = 0x23U,                                                                \
		.kpAccFrac = 0x1AU,                                                                \
		.kiSscg = 0x18U,                                                                   \
		.kpSscg = 0x18U,                                                                   \
		.kiAccSscg = 0x16U,                                                                \
		.kpAccSscg = 0x14U,                                                                \
	},
#else
#define DPLL_LP_INIT(n)
#endif

#define FIXED_CLK_INIT(n)                                                                          \
	static const struct fixed_rate_clock_config fixed_rate_clock_config_##n = {                \
		.rate = DT_INST_PROP(n, clock_frequency),                                          \
		.system_clock = DT_INST_PROP(n, system_clock),                                     \
		DPLL_LP_INIT(n)                                                                    \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, fixed_rate_clk_init, NULL, NULL, &fixed_rate_clock_config_##n,    \
			      PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(FIXED_CLK_INIT)
