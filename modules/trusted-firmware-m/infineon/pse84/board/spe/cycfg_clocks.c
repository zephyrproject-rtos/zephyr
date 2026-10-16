/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * PSE84 system clock configuration for TF-M, built from the devicetree of the
 * Zephyr non-secure image. Replaces cycfg_clocks.c of the TF-M board design
 * and provides init_cycfg_clocks() and the HF clock configurations declared
 * by its cycfg_clocks.h, the parts used by the secure image.
 */

#include "cycfg_clocks.h"
#include "cycfg_clocks_dt.h"

/* SoC facts */

#define NUM_CLKPATH 6
#define NUM_HFROOT  14

/* CM33 runs from CLK_HF0; paths below CY_SRSS_NUM_PLL each carry a DPLL */
#define CORE_HF           0U
#define FIRST_DIRECT_PATH ((cy_en_clkhf_in_sources_t)CY_SRSS_NUM_PLL)
#define CLK_HF0_MAX_HZ    200000000U

BUILD_ASSERT(CY_SRSS_NUM_CLKPATH == NUM_CLKPATH, "unexpected number of clock paths");
BUILD_ASSERT(CY_SRSS_NUM_HFROOT == NUM_HFROOT, "unexpected number of HF clocks");
/* Keeps PATH_DPLL_n in line with the silicon */
BUILD_ASSERT((SRSS_DPLL_LP_0_PATH_NUM == 0U) && (SRSS_DPLL_LP_1_PATH_NUM == 1U) &&
		     (SRSS_DPLL_HP_0_PATH_NUM == 2U),
	     "unexpected DPLL to clock path mapping");
/* MXS22SRSS 1.0 needs a DPLL_LP enable workaround that is not implemented */
BUILD_ASSERT(CY_IP_MXS22SRSS_VERSION_MINOR != 0U, "MXS22SRSS 1.0 is not supported");

/* Oscillator pins: WCO on P18.0/P18.1, ECO on P19.0/P19.1, EXT_CLK on P7.4 */
#define WCO_PORT      GPIO_PRT18
#define ECO_PORT      GPIO_PRT19
#define EXT_CLK_PORT  GPIO_PRT7
#define EXT_CLK_PIN   4U
#define EXT_CLK_HSIOM P7_4_SRSS_EXT_CLK

#define WCO_START_TIMEOUT_US 1000000UL
#define ECO_START_TIMEOUT_US 3000UL
#define PLL_LOCK_TIMEOUT_US  10000U
/* Four ILO cycles, for an unfinished CLK_LF source transition */
#define CLK_LF_SETTLE_US     200U

/* DPLL_LP trim update, register stride 0x20 per instance */
#define DPLL_LP_TEST3(n)   (0x52403218U + ((n) * 0x20U))
#define DPLL_LP_TEST4(n)   (0x5240321CU + ((n) * 0x20U))
#define DPLL_LP_TEST3_TRIM 0x921F190AU
#define DPLL_LP_TEST4_TRIM 0x08100000U

#define CY_CFG_SYSCLK_ECO_ERROR 1
#define CY_CFG_SYSCLK_PLL_ERROR 3
#define CY_CFG_SYSCLK_WCO_ERROR 5

/* Allowed difference between a computed frequency and the devicetree value */
#define FREQ_TOLERANCE_HZ 1000U
#define FREQ_MATCH(a, b)  (((a) > (b) ? (a) - (b) : (b) - (a)) <= FREQ_TOLERANCE_HZ)

/* Checks: clock path sources */

#define CHECK_PATH(n, ...)                                                                         \
	BUILD_ASSERT(!PATH_OKAY(n) || PATH_IN_FREQ(n) != 0U,                                       \
		     "path_mux" #n ": source clock is disabled or not supported")

LISTIFY(NUM_CLKPATH, CHECK_PATH, (;));

/* Checks: HF clock sources and limits */

#define CHECK_HF(n, ...)                                                                           \
	BUILD_ASSERT(!DT_NODE_HAS_STATUS_OKAY(HF_NODE(n)) ||                                       \
			     (PATH_OKAY(HF_SRC(n)) && HF_FREQ(n) != 0U),                           \
		     "clk_hf" #n ": source clock path is disabled")

LISTIFY(NUM_HFROOT, CHECK_HF, (;));

BUILD_ASSERT(CLK_OKAY(clk_hf0), "clk_hf0 must be enabled");
BUILD_ASSERT(HF_FREQ(0) <= CLK_HF0_MAX_HZ, "clk_hf0 exceeds 200 MHz");
BUILD_ASSERT(FREQ_MATCH(HF_FREQ(0), (uint64_t)DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency)),
	     "cpu@0 clock-frequency does not match clk_hf0");

/* Checks: DPLL dividers */

#define CHECK_DPLL(n, model)                                                                       \
	BUILD_ASSERT(!CLK_OKAY(PATH_DPLL(n)) ||                                                    \
			     (PATH_OKAY(n) && FREQ_MATCH(model(n), CLK_FREQ(PATH_DPLL(n)))),       \
		     STRINGIFY(PATH_DPLL(n)) ": dividers do not produce clock-frequency")

CHECK_DPLL(0, DPLL_LP_FREQ);
CHECK_DPLL(1, DPLL_LP_FREQ);
CHECK_DPLL(2, DPLL_HP_FREQ);

/* Checks: LF and backup clock sources */

#if CLK_OKAY(clk_lf)
BUILD_ASSERT((CLK_LF_SRC == IFX_CAT1_CLKLF_IN_PILO && CLK_OKAY(clk_pilo)) ||
		     (CLK_LF_SRC == IFX_CAT1_CLKLF_IN_WCO && CLK_OKAY(clk_wco)),
	     "clk_lf: source clock is disabled or not supported");
#endif

#if CLK_OKAY(clk_bak)
BUILD_ASSERT((CLK_BAK_SRC == IFX_CAT1_CLKBAK_IN_WCO && CLK_OKAY(clk_wco)) ||
		     (CLK_BAK_SRC == IFX_CAT1_CLKBAK_IN_PILO && CLK_OKAY(clk_pilo)) ||
		     (CLK_BAK_SRC == IFX_CAT1_CLKBAK_IN_CLKLF && CLK_OKAY(clk_lf)),
	     "clk_bak: source clock is disabled or not supported");
#endif

/* Configuration data */

#define HF_CONFIG(n, ...)                                                                          \
	const cycfg_clkhf_config_t cycfg_hf##n##Config = {                                         \
		.source = (cy_en_clkhf_in_sources_t)HF_SRC(n),                                     \
		.divider = (cy_en_clkhf_dividers_t)HF_DIV(n),                                      \
	}

LISTIFY(NUM_HFROOT, HF_CONFIG, (;));

#define HF_CONFIG_PTR(n, ...)                                                                      \
	COND_CODE_1(DT_NODE_HAS_STATUS_OKAY(HF_NODE(n)), (&cycfg_hf##n##Config), (NULL))

static const cycfg_clkhf_config_t *const cycfg_clkhf_configs[] = {
	LISTIFY(NUM_HFROOT, HF_CONFIG_PTR, (,))
};

#define PATH_CONFIG(n, ...) ((cy_en_clkpath_in_sources_t)PATH_SRC(n))

static const cy_en_clkpath_in_sources_t cycfg_clkpath_configs[] = {
	LISTIFY(NUM_CLKPATH, PATH_CONFIG, (,))
};

#if CLK_OKAY(clk_eco)
static const cy_stc_clk_eco_config_t eco_config = {
	.ecoClkfreq = DT_PROP(CLK_NODE(clk_eco), clock_frequency),
	.ecoCtrim = DT_PROP(CLK_NODE(clk_eco), eco_ctrim),
	.ecoGtrim = DT_PROP(CLK_NODE(clk_eco), eco_gtrim),
	.ecoIboost = DT_PROP(CLK_NODE(clk_eco), eco_iboost),
};
#endif

/*
 * DPLL loop filter and tuning fields not described in devicetree use fixed
 * default values.
 */
#define DPLL_LP_CONFIG(label)                                                                      \
	{                                                                                          \
		.feedbackDiv = DT_PROP(CLK_NODE(label), feedback_div),                             \
		.referenceDiv = DT_PROP(CLK_NODE(label), reference_div),                           \
		.outputDiv = DT_PROP(CLK_NODE(label), output_div),                                 \
		.pllDcoMode = DT_PROP(CLK_NODE(label), dco_mode_enable),                           \
		.outputMode = CY_SYSCLK_FLLPLL_OUTPUT_AUTO,                                        \
		.fracDiv = DT_PROP_OR(CLK_NODE(label), fraction_div, 0),                           \
		.fracDitherEn = false,                                                             \
		.fracEn = true,                                                                    \
		.dcoCode = 0xFU,                                                                   \
		.kiInt = 0xAU,                                                                     \
		.kiFrac = 0xBU,                                                                    \
		.kiSscg = 0x7U,                                                                    \
		.kpInt = 0x8U,                                                                     \
		.kpFrac = 0x9U,                                                                    \
		.kpSscg = 0x7U,                                                                    \
	}

#if CLK_OKAY(dpll_lp0)
static cy_stc_dpll_lp_config_t dpll_lp0_lp_config = DPLL_LP_CONFIG(dpll_lp0);
static cy_stc_pll_manual_config_t dpll_lp0_config = {
	.lpPllCfg = &dpll_lp0_lp_config,
};
#endif

#if CLK_OKAY(dpll_lp1)
static cy_stc_dpll_lp_config_t dpll_lp1_lp_config = DPLL_LP_CONFIG(dpll_lp1);
static cy_stc_pll_manual_config_t dpll_lp1_config = {
	.lpPllCfg = &dpll_lp1_lp_config,
};
#endif

#if CLK_OKAY(dpll_hp)
static cy_stc_dpll_hp_config_t dpll_hp_hp_config = {
	.pDiv = DT_PROP(CLK_NODE(dpll_hp), div_p),
	.nDiv = DT_PROP(CLK_NODE(dpll_hp), div_n),
	.kDiv = DT_PROP(CLK_NODE(dpll_hp), div_k),
	.nDivFract = DT_PROP_OR(CLK_NODE(dpll_hp), fraction_div, 0),
	.freqModeSel = (cy_en_wait_mode_select_t)DT_PROP(CLK_NODE(dpll_hp), freq_mode_sel),
	.ivrTrim = 0x8U,
	.clkrSel = 0x1U,
	.alphaCoarse = 0xCU,
	.betaCoarse = 0x5U,
	.flockThresh = DT_PROP(CLK_NODE(dpll_hp), flock_enable_threshold),
	.flockWait = 0x6U,
	.flockLkThres = 0x7U,
	.flockLkWait = 0x4U,
	.alphaExt = 0x14U,
	.betaExt = DT_PROP(CLK_NODE(dpll_hp), lf_beta_value),
	.lfEn = 0x1U,
	.dcEn = 0x1U,
	.outputMode = CY_SYSCLK_FLLPLL_OUTPUT_AUTO,
};
static cy_stc_pll_manual_config_t dpll_hp_config = {
	.hpPllCfg = &dpll_hp_hp_config,
};
#endif

/* Initialization */

__WEAK void __NO_RETURN cycfg_ClockStartupError(uint32_t error)
{
	(void)error;
	while (true) {
	}
}

static void clk_hf_init(uint32_t clk_hf, const cycfg_clkhf_config_t *hf)
{
	Cy_SysClk_ClkHfSetDivider(clk_hf, hf->divider);
	Cy_SysClk_ClkHfSetSource(clk_hf, hf->source);
	if (clk_hf != 0U) {
		Cy_SysClk_ClkHfEnable(clk_hf);
	}
}

/* Run the core from the IHO through the first direct path while clocks change */
static void iho_failsafe(void)
{
	Cy_SysClk_IhoEnable();
	Cy_SysClk_ClkPathSetSource(FIRST_DIRECT_PATH, CY_SYSCLK_CLKPATH_IN_IHO);
	Cy_SysClk_ClkHfSetSource(CORE_HF, FIRST_DIRECT_PATH);
	Cy_SysClk_ClkHfSetDivider(CORE_HF, CY_SYSCLK_CLKHF_NO_DIVIDE);
}

static void plls_disable(void)
{
	for (uint32_t pll = CY_SRSS_NUM_PLL - 1U; pll > 0U; --pll) {
		(void)Cy_SysClk_PllDisable(pll);
	}
}

static void clk_sources_init(void)
{
#if CLK_OKAY(clk_pilo)
	Cy_SysClk_PiloEnable();
#endif

#if CLK_OKAY(clk_wco)
	(void)Cy_GPIO_Pin_FastInit(WCO_PORT, 1U, 0x00U, 0x00U, HSIOM_SEL_GPIO);
	(void)Cy_GPIO_Pin_FastInit(WCO_PORT, 0U, 0x00U, 0x00U, HSIOM_SEL_GPIO);
	if (Cy_SysClk_WcoEnable(WCO_START_TIMEOUT_US) != CY_SYSCLK_SUCCESS) {
		cycfg_ClockStartupError(CY_CFG_SYSCLK_WCO_ERROR);
	}
#endif

#if CLK_OKAY(clk_eco)
	(void)Cy_GPIO_Pin_FastInit(ECO_PORT, 0U, CY_GPIO_DM_ANALOG, 0UL, HSIOM_SEL_GPIO);
	(void)Cy_GPIO_Pin_FastInit(ECO_PORT, 1U, CY_GPIO_DM_ANALOG, 0UL, HSIOM_SEL_GPIO);
	if (Cy_SysClk_EcoManualConfigure(&eco_config) == CY_SYSCLK_BAD_PARAM) {
		cycfg_ClockStartupError(CY_CFG_SYSCLK_ECO_ERROR);
	}
	if (Cy_SysClk_EcoEnable(ECO_START_TIMEOUT_US) == CY_SYSCLK_TIMEOUT) {
		cycfg_ClockStartupError(CY_CFG_SYSCLK_ECO_ERROR);
	}
	Cy_SysClk_EcoSetFrequency(DT_PROP(CLK_NODE(clk_eco), clock_frequency));
#endif

#if CLK_OKAY(clk_lf)
	Cy_SysClk_ClkLfSetSource((cy_en_clklf_in_sources_t)CLK_LF_SRC);
#endif

#if CLK_OKAY(clk_ext)
	(void)Cy_GPIO_Pin_SecFastInit(EXT_CLK_PORT, EXT_CLK_PIN, CY_GPIO_DM_HIGHZ, 0UL,
				      EXT_CLK_HSIOM);
	Cy_SysClk_ExtClkSetFrequency(DT_PROP(CLK_NODE(clk_ext), clock_frequency));
#endif
}

/* All clock paths except the first direct path, which still carries the IHO */
static void clk_paths_init(void)
{
	for (uint32_t i = 0U; i < CY_SRSS_NUM_CLKPATH; i++) {
		if (i != FIRST_DIRECT_PATH) {
			Cy_SysClk_ClkPathSetSource(i, cycfg_clkpath_configs[i]);
		}
	}
}

/* All enabled HF clocks except the one running the core */
static void clk_hf_init_all(void)
{
	for (uint32_t i = 0U; i < CY_SRSS_NUM_HFROOT; i++) {
		if ((i != CORE_HF) && (cycfg_clkhf_configs[i] != NULL)) {
			clk_hf_init(i, cycfg_clkhf_configs[i]);
		}
	}
}

#if CLK_OKAY(dpll_lp0) || CLK_OKAY(dpll_lp1) || CLK_OKAY(dpll_hp)
static void dpll_init(uint32_t path, cy_stc_pll_manual_config_t *config)
{
	(void)Cy_SysClk_PllDisable(path);
	if (Cy_SysClk_PllManualConfigure(path, config) != CY_SYSCLK_SUCCESS) {
		cycfg_ClockStartupError(CY_CFG_SYSCLK_PLL_ERROR);
	}

	if (Cy_SysClk_PllEnable(path, PLL_LOCK_TIMEOUT_US) != CY_SYSCLK_SUCCESS) {
		cycfg_ClockStartupError(CY_CFG_SYSCLK_PLL_ERROR);
	}
}

#if CLK_OKAY(dpll_lp0) || CLK_OKAY(dpll_lp1)
static void dpll_lp_init(uint32_t n, cy_stc_pll_manual_config_t *config)
{
#ifdef UPDATE_DPLL_LP_TRIM_VALUES
	CY_SET_REG32(DPLL_LP_TEST3(n), DPLL_LP_TEST3_TRIM);
	CY_SET_REG32(DPLL_LP_TEST4(n), DPLL_LP_TEST4_TRIM);
#endif
	dpll_init(n, config);
}
#endif
#endif

static void dplls_init(void)
{
#if CLK_OKAY(dpll_hp)
	dpll_init(SRSS_DPLL_HP_0_PATH_NUM, &dpll_hp_config);
#endif
#if CLK_OKAY(dpll_lp0)
	dpll_lp_init(SRSS_DPLL_LP_0_PATH_NUM, &dpll_lp0_config);
#endif
#if CLK_OKAY(dpll_lp1)
	dpll_lp_init(SRSS_DPLL_LP_1_PATH_NUM, &dpll_lp1_config);
#endif
}

/* Move the core to its configured HF source and disable unused HF clocks */
static void core_hf_init(void)
{
	for (uint32_t i = 0U; i < CY_SRSS_NUM_HFROOT; i++) {
		if (cycfg_clkhf_configs[i] == NULL) {
			(void)Cy_SysClk_ClkHfDisable(i);
		} else if (i == CORE_HF) {
			clk_hf_init(i, cycfg_clkhf_configs[i]);
		}
	}
}

static void clk_hf_csv_disable(void)
{
	for (uint32_t i = 0U; i < CY_SRSS_NUM_HFROOT; i++) {
		Cy_SysClk_ClkHfCsvDisable(i);
	}
}

void init_cycfg_clocks(void)
{
	iho_failsafe();

	/*
	 * CLK_LF selection is write-protected while the WDT is locked. The WDT
	 * is left unlocked.
	 */
	Cy_WDT_Unlock();

	plls_disable();
	clk_sources_init();
	clk_paths_init();
	clk_hf_init_all();

#if CLK_OKAY(clk_bak)
	Cy_SysClk_ClkBakSetSource((cy_en_clkbak_in_sources_t)CLK_BAK_SRC);
#endif

	dplls_init();
	core_hf_init();

	/* Release the first direct path from the IHO fail-safe */
	Cy_SysClk_ClkPathSetSource(FIRST_DIRECT_PATH, cycfg_clkpath_configs[FIRST_DIRECT_PATH]);

	Cy_SysLib_DelayUs(CLK_LF_SETTLE_US);
	SystemCoreClockUpdate();
	clk_hf_csv_disable();
}
