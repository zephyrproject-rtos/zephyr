/*
 * Copyright (c) 2022 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Clock control driver for Infineon CAT1 MCU family.
 */

#include <zephyr/drivers/clock_control.h>
#include <cyhal_clock.h>
#include <cyhal_utils.h>
#include <cyhal_clock_impl.h>

#define GET_CLK_SOURCE_ORD(N)  DT_DEP_ORD(DT_CLOCKS_CTLR_BY_IDX(DT_NODELABEL(N), 0))

/* Enumeration of enabled in device tree Clock, uses for indexing clock info table */
enum {
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_imo))
	INFINEON_CAT1_CLOCK_IMO,
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_iho))
	INFINEON_CAT1_CLOCK_IHO,
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(path_mux0))
	INFINEON_CAT1_CLOCK_PATHMUX0,
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(path_mux1))
	INFINEON_CAT1_CLOCK_PATHMUX1,
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(path_mux2))
	INFINEON_CAT1_CLOCK_PATHMUX2,
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(path_mux3))
	INFINEON_CAT1_CLOCK_PATHMUX3,
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(path_mux4))
	INFINEON_CAT1_CLOCK_PATHMUX4,
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_hf0))
	INFINEON_CAT1_CLOCK_HF0,
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_hf1))
	INFINEON_CAT1_CLOCK_HF1,
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_hf2))
	INFINEON_CAT1_CLOCK_HF2,
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_hf3))
	INFINEON_CAT1_CLOCK_HF3,
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_hf4))
	INFINEON_CAT1_CLOCK_HF4,
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_hf5))
	INFINEON_CAT1_CLOCK_HF5,
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_hf6))
	INFINEON_CAT1_CLOCK_HF6,
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_hf7))
	INFINEON_CAT1_CLOCK_HF7,
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_hf8))
	INFINEON_CAT1_CLOCK_HF8,
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_hf9))
	INFINEON_CAT1_CLOCK_HF9,
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_hf10))
	INFINEON_CAT1_CLOCK_HF10,
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_hf11))
	INFINEON_CAT1_CLOCK_HF11,
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_hf12))
	INFINEON_CAT1_CLOCK_HF12,
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_hf13))
	INFINEON_CAT1_CLOCK_HF13,
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_fast))
	INFINEON_CAT1_CLOCK_FAST,
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_slow))
	INFINEON_CAT1_CLOCK_SLOW,
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_peri))
	INFINEON_CAT1_CLOCK_PERI,
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(pll0))
	INFINEON_CAT1_CLOCK_PLL0,
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(pll1))
	INFINEON_CAT1_CLOCK_PLL1,
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(fll0))
	INFINEON_CAT1_CLOCK_FLL0,
#endif

	/* Count of enabled clock */
	INFINEON_CAT1_ENABLED_CLOCK_COUNT
}; /* infineon_cat1_clock_info_name_t */

/* Clock info structure */
struct infineon_cat1_clock_info_t {
	cyhal_clock_t obj;      /* Hal Clock object */
	uint32_t dt_ord;        /* Device tree node's dependency ordinal */
};

/* Lookup table which presents  clock objects (cyhal_clock_t) correspondence to ordinal
 * number of device tree clock nodes.
 */
static struct infineon_cat1_clock_info_t
	clock_info_table[INFINEON_CAT1_ENABLED_CLOCK_COUNT] = {
	/* We always have IMO */
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_imo))
	[INFINEON_CAT1_CLOCK_IMO] = { .dt_ord = DT_DEP_ORD(DT_NODELABEL(clk_imo)) },
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_iho))
	[INFINEON_CAT1_CLOCK_IHO] = { .dt_ord = DT_DEP_ORD(DT_NODELABEL(clk_iho)) },
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(path_mux0))
	[INFINEON_CAT1_CLOCK_PATHMUX0] = { .dt_ord = DT_DEP_ORD(DT_NODELABEL(path_mux0)) },
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(path_mux1))
	[INFINEON_CAT1_CLOCK_PATHMUX1] = { .dt_ord = DT_DEP_ORD(DT_NODELABEL(path_mux1)) },
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(path_mux2))
	[INFINEON_CAT1_CLOCK_PATHMUX2] = { .dt_ord = DT_DEP_ORD(DT_NODELABEL(path_mux2)) },
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(path_mux3))
	[INFINEON_CAT1_CLOCK_PATHMUX3] = { .dt_ord = DT_DEP_ORD(DT_NODELABEL(path_mux3)) },
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(path_mux4))
	[INFINEON_CAT1_CLOCK_PATHMUX4] = { .dt_ord = DT_DEP_ORD(DT_NODELABEL(path_mux4)) },
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_hf0))
	[INFINEON_CAT1_CLOCK_HF0] = { .dt_ord = DT_DEP_ORD(DT_NODELABEL(clk_hf0)) },
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_hf1))
	[INFINEON_CAT1_CLOCK_HF1] = { .dt_ord = DT_DEP_ORD(DT_NODELABEL(clk_hf1)) },
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_hf2))
	[INFINEON_CAT1_CLOCK_HF2] = { .dt_ord = DT_DEP_ORD(DT_NODELABEL(clk_hf2)) },
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_hf3))
	[INFINEON_CAT1_CLOCK_HF3] = { .dt_ord = DT_DEP_ORD(DT_NODELABEL(clk_hf3)) },
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_hf4))
	[INFINEON_CAT1_CLOCK_HF4] = { .dt_ord = DT_DEP_ORD(DT_NODELABEL(clk_hf4)) },
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_hf5))
	[INFINEON_CAT1_CLOCK_HF5] = { .dt_ord = DT_DEP_ORD(DT_NODELABEL(clk_hf5)) },
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_hf6))
	[INFINEON_CAT1_CLOCK_HF6] = { .dt_ord = DT_DEP_ORD(DT_NODELABEL(clk_hf6)) },
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_hf7))
	[INFINEON_CAT1_CLOCK_HF7] = { .dt_ord = DT_DEP_ORD(DT_NODELABEL(clk_hf7)) },
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_hf8))
	[INFINEON_CAT1_CLOCK_HF8] = { .dt_ord = DT_DEP_ORD(DT_NODELABEL(clk_hf8)) },
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_hf9))
	[INFINEON_CAT1_CLOCK_HF9] = { .dt_ord = DT_DEP_ORD(DT_NODELABEL(clk_hf9)) },
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_hf10))
	[INFINEON_CAT1_CLOCK_HF10] = { .dt_ord = DT_DEP_ORD(DT_NODELABEL(clk_hf10)) },
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_hf11))
	[INFINEON_CAT1_CLOCK_HF11] = { .dt_ord = DT_DEP_ORD(DT_NODELABEL(clk_hf11)) },
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_hf12))
	[INFINEON_CAT1_CLOCK_HF12] = { .dt_ord = DT_DEP_ORD(DT_NODELABEL(clk_hf12)) },
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_hf13))
	[INFINEON_CAT1_CLOCK_HF13] = { .dt_ord = DT_DEP_ORD(DT_NODELABEL(clk_hf13)) },
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_fast))
	[INFINEON_CAT1_CLOCK_FAST] = { .dt_ord = DT_DEP_ORD(DT_NODELABEL(clk_fast)) },
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_slow))
	[INFINEON_CAT1_CLOCK_SLOW] = { .dt_ord = DT_DEP_ORD(DT_NODELABEL(clk_slow)) },
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_peri))
	[INFINEON_CAT1_CLOCK_PERI] = { .dt_ord = DT_DEP_ORD(DT_NODELABEL(clk_peri)) },
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(pll0))
	[INFINEON_CAT1_CLOCK_PLL0] = { .dt_ord = DT_DEP_ORD(DT_NODELABEL(pll0)) },
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(pll1))
	[INFINEON_CAT1_CLOCK_PLL1] = { .dt_ord = DT_DEP_ORD(DT_NODELABEL(pll1)) },
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(fll0))
	[INFINEON_CAT1_CLOCK_FLL0] = { .dt_ord = DT_DEP_ORD(DT_NODELABEL(fll0)) },
#endif
};

static cy_rslt_t _configure_path_mux(cyhal_clock_t *clock_obj,
				     cyhal_clock_t *clock_source_obj,
				     const cyhal_clock_t *reserve_obj)
{
	cy_rslt_t rslt;

	ARG_UNUSED(clock_source_obj);

	rslt = cyhal_clock_reserve(clock_obj, reserve_obj);

	if (rslt == CY_RSLT_SUCCESS) {
		rslt = cyhal_clock_set_source(clock_obj, clock_source_obj);
	}

	return rslt;
}

static cy_rslt_t _configure_clk_hf(cyhal_clock_t *clock_obj,
				   cyhal_clock_t *clock_source_obj,
				   const cyhal_clock_t *reserve_obj,
				   uint32_t clock_div)
{
	cy_rslt_t rslt;

	rslt = cyhal_clock_reserve(clock_obj, reserve_obj);

	if (rslt == CY_RSLT_SUCCESS) {
		rslt = cyhal_clock_set_source(clock_obj, clock_source_obj);
	}

	if (rslt == CY_RSLT_SUCCESS) {
		rslt = cyhal_clock_set_divider(clock_obj, clock_div);
	}

	if (rslt == CY_RSLT_SUCCESS) {
		rslt = cyhal_clock_set_enabled(clock_obj, true, true);
	}

	return rslt;
}

static cy_rslt_t _configure_clk_frequency_and_enable(cyhal_clock_t *clock_obj,
						     cyhal_clock_t *clock_source_obj,
						     const cyhal_clock_t *reserve_obj,
						     uint32_t frequency)
{
	ARG_UNUSED(clock_source_obj);
	cy_rslt_t rslt;

	rslt = cyhal_clock_reserve(clock_obj, reserve_obj);

	if (rslt == CY_RSLT_SUCCESS) {
		rslt = cyhal_clock_set_frequency(clock_obj, frequency, NULL);
	}

	if (rslt == CY_RSLT_SUCCESS) {
		rslt = cyhal_clock_set_enabled(clock_obj, true, true);
	}

	return rslt;
}

static cyhal_clock_t *_get_hal_obj_from_ord(uint32_t dt_ord)
{
	cyhal_clock_t *ret_obj = NULL;

	for (uint32_t i = 0u; i < INFINEON_CAT1_ENABLED_CLOCK_COUNT; i++) {
		if (clock_info_table[i].dt_ord == dt_ord) {
			ret_obj = &clock_info_table[i].obj;
			return ret_obj;
		}
	}
	return ret_obj;
}

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(dpll_hp))
__WEAK void cycfg_ClockStartupError(uint32_t error)
{
	(void)error; /* Suppress the compiler warning */
	while (1) {
	}
}

void Cy_SysClk_Dpll_Hp0_Init(void)
{
#define CY_CFG_SYSCLK_PLL_ERROR 3

	static cy_stc_dpll_hp_config_t srss_0_clock_0_pll500m_0_hp_pllConfig = {
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
	static cy_stc_pll_manual_config_t srss_0_clock_0_pll500m_0_pllConfig = {
		.hpPllCfg = &srss_0_clock_0_pll500m_0_hp_pllConfig,
	};

#if !defined(CY_PDL_TZ_ENABLED)
	if (Cy_SysClk_PllIsEnabled(SRSS_DPLL_HP_0_PATH_NUM)) {
		return;
	}
#endif
	Cy_SysClk_PllDisable(SRSS_DPLL_HP_0_PATH_NUM);
	if (CY_SYSCLK_SUCCESS !=
	    Cy_SysClk_PllManualConfigure(SRSS_DPLL_HP_0_PATH_NUM,
					 &srss_0_clock_0_pll500m_0_pllConfig)) {
		cycfg_ClockStartupError(CY_CFG_SYSCLK_PLL_ERROR);
	}
	if (CY_SYSCLK_SUCCESS != Cy_SysClk_PllEnable(SRSS_DPLL_HP_0_PATH_NUM, 10000u)) {
		cycfg_ClockStartupError(CY_CFG_SYSCLK_PLL_ERROR);
	}
}
#endif

static int clock_control_infineon_cat1_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	cy_rslt_t rslt;
	cyhal_clock_t *clock_obj = NULL;
	cyhal_clock_t *clock_source_obj = NULL;

	__attribute__((unused)) uint32 frequency;
	uint32 clock_div;

	/* Configure IMO */
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_imo))
	clock_obj = &clock_info_table[INFINEON_CAT1_CLOCK_IMO].obj;
	if (cyhal_clock_get(clock_obj, &CYHAL_CLOCK_RSC_IMO)) {
		return -EIO;
	}
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_iho))
	clock_obj = &clock_info_table[INFINEON_CAT1_CLOCK_IHO].obj;
	if (cyhal_clock_get(clock_obj, &CYHAL_CLOCK_RSC_IHO)) {
		return -EIO;
	}
#endif
#if !DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_imo)) && \
	!DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_iho))
	#error "IMO clock or IHO clock must be enabled"
#endif

	/* Configure the PathMux[0] to source defined in tree device 'path_mux0' node */
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(path_mux0))
	clock_obj = &clock_info_table[INFINEON_CAT1_CLOCK_PATHMUX0].obj;
	clock_source_obj = _get_hal_obj_from_ord(GET_CLK_SOURCE_ORD(path_mux0));

	if (_configure_path_mux(clock_obj, clock_source_obj, &CYHAL_CLOCK_PATHMUX[0])) {
		return -EIO;
	}
#endif

	/* Configure the PathMux[1] to source defined in tree device 'path_mux1' node */
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(path_mux1))
	clock_obj = &clock_info_table[INFINEON_CAT1_CLOCK_PATHMUX1].obj;
	clock_source_obj = _get_hal_obj_from_ord(GET_CLK_SOURCE_ORD(path_mux1));

	if (_configure_path_mux(clock_obj, clock_source_obj, &CYHAL_CLOCK_PATHMUX[1])) {
		return -EIO;
	}
#endif

	/* Configure the PathMux[2] to source defined in tree device 'path_mux2' node */
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(path_mux2))
	clock_obj = &clock_info_table[INFINEON_CAT1_CLOCK_PATHMUX2].obj;
	clock_source_obj = _get_hal_obj_from_ord(GET_CLK_SOURCE_ORD(path_mux2));

	if (_configure_path_mux(clock_obj, clock_source_obj, &CYHAL_CLOCK_PATHMUX[2])) {
		return -EIO;
	}
#endif

	/* Configure the PathMux[3] to source defined in tree device 'path_mux3' node */
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(path_mux3))
	clock_obj = &clock_info_table[INFINEON_CAT1_CLOCK_PATHMUX3].obj;
	clock_source_obj = _get_hal_obj_from_ord(GET_CLK_SOURCE_ORD(path_mux3));

	if (_configure_path_mux(clock_obj, clock_source_obj, &CYHAL_CLOCK_PATHMUX[3])) {
		return -EIO;
	}
#endif

	/* Configure the PathMux[4] to source defined in tree device 'path_mux4' node */
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(path_mux4))
	clock_obj = &clock_info_table[INFINEON_CAT1_CLOCK_PATHMUX4].obj;
	clock_source_obj = _get_hal_obj_from_ord(GET_CLK_SOURCE_ORD(path_mux4));

	if (_configure_path_mux(clock_obj, clock_source_obj, &CYHAL_CLOCK_PATHMUX[4])) {
		return -EIO;
	}
#endif

	/* Configure FLL0 */
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(fll0))
	clock_obj = &clock_info_table[INFINEON_CAT1_CLOCK_FLL0].obj;
	frequency = DT_PROP(DT_NODELABEL(fll0), clock_frequency);

	rslt = _configure_clk_frequency_and_enable(clock_obj, clock_source_obj,
						   &CYHAL_CLOCK_FLL, frequency);
	if (rslt) {
		return -EIO;
	}
#endif

	/* Configure PLL0 */
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(pll0))
	clock_obj = &clock_info_table[INFINEON_CAT1_CLOCK_PLL0].obj;
	frequency = DT_PROP(DT_NODELABEL(pll0), clock_frequency);

	rslt = _configure_clk_frequency_and_enable(clock_obj, clock_source_obj,
						   &CYHAL_CLOCK_PLL[0], frequency);

	if (rslt) {
		return -EIO;
	}
#endif

	/* Configure PLL1 */
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(pll1))
	clock_obj = &clock_info_table[INFINEON_CAT1_CLOCK_PLL1].obj;
	frequency = DT_PROP(DT_NODELABEL(pll1), clock_frequency);

	rslt = _configure_clk_frequency_and_enable(clock_obj, clock_source_obj,
						   &CYHAL_CLOCK_PLL[1], frequency);
	if (rslt) {
		return -EIO;
	}
#endif

	/* Configure the HF[0] to source defined in tree device 'clk_hf0' node */
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_hf0))
	clock_obj = &clock_info_table[INFINEON_CAT1_CLOCK_HF0].obj;
	clock_source_obj = _get_hal_obj_from_ord(GET_CLK_SOURCE_ORD(clk_hf0));
	clock_div = DT_PROP(DT_NODELABEL(clk_hf0), clock_div);

	if (_configure_clk_hf(clock_obj, clock_source_obj, &CYHAL_CLOCK_HF[0], clock_div)) {
		return -EIO;
	}
#endif

	/* Configure the HF[1] to source defined in tree device 'clk_hf1' node */
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_hf1))
	clock_obj = &clock_info_table[INFINEON_CAT1_CLOCK_HF1].obj;
	clock_source_obj = _get_hal_obj_from_ord(GET_CLK_SOURCE_ORD(clk_hf1));
	clock_div = DT_PROP(DT_NODELABEL(clk_hf1), clock_div);

	if (_configure_clk_hf(clock_obj, clock_source_obj, &CYHAL_CLOCK_HF[1], clock_div)) {
		return -EIO;
	}
#endif

	/* Configure the HF[2] to source defined in tree device 'clk_hf2' node */
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_hf2))
	clock_obj = &clock_info_table[INFINEON_CAT1_CLOCK_HF2].obj;
	clock_source_obj = _get_hal_obj_from_ord(GET_CLK_SOURCE_ORD(clk_hf2));
	clock_div = DT_PROP(DT_NODELABEL(clk_hf2), clock_div);

	if (_configure_clk_hf(clock_obj, clock_source_obj, &CYHAL_CLOCK_HF[2], clock_div)) {
		return -EIO;
	}
#endif

	/* Configure the HF[3] to source defined in tree device 'clk_hf3' node */
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_hf3))
	clock_obj = &clock_info_table[INFINEON_CAT1_CLOCK_HF3].obj;
	clock_source_obj = _get_hal_obj_from_ord(GET_CLK_SOURCE_ORD(clk_hf3));
	clock_div = DT_PROP(DT_NODELABEL(clk_hf3), clock_div);

	if (_configure_clk_hf(clock_obj, clock_source_obj, &CYHAL_CLOCK_HF[3], clock_div)) {
		return -EIO;
	}
#endif

	/* Configure the HF[4] to source defined in tree device 'clk_hf4' node */
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_hf4))
	clock_obj = &clock_info_table[INFINEON_CAT1_CLOCK_HF4].obj;
	clock_source_obj = _get_hal_obj_from_ord(GET_CLK_SOURCE_ORD(clk_hf4));
	clock_div = DT_PROP(DT_NODELABEL(clk_hf4), clock_div);

	if (_configure_clk_hf(clock_obj, clock_source_obj, &CYHAL_CLOCK_HF[4], clock_div)) {
		return -EIO;
	}
#endif

	/* Configure the HF[5] to source defined in tree device 'clk_hf5' node */
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_hf5))
	clock_obj = &clock_info_table[INFINEON_CAT1_CLOCK_HF5].obj;
	clock_source_obj = _get_hal_obj_from_ord(GET_CLK_SOURCE_ORD(clk_hf5));
	clock_div = DT_PROP(DT_NODELABEL(clk_hf5), clock_div);

	if (_configure_clk_hf(clock_obj, clock_source_obj, &CYHAL_CLOCK_HF[5], clock_div)) {
		return -EIO;
	}
#endif

	/* Configure the HF[6] to source defined in tree device 'clk_hf6' node */
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_hf6))
	clock_obj = &clock_info_table[INFINEON_CAT1_CLOCK_HF6].obj;
	clock_source_obj = _get_hal_obj_from_ord(GET_CLK_SOURCE_ORD(clk_hf6));
	clock_div = DT_PROP(DT_NODELABEL(clk_hf6), clock_div);

	if (_configure_clk_hf(clock_obj, clock_source_obj, &CYHAL_CLOCK_HF[6], clock_div)) {
		return -EIO;
	}
#endif

	/* Configure the HF[7] to source defined in tree device 'clk_hf7' node */
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_hf7))
	clock_obj = &clock_info_table[INFINEON_CAT1_CLOCK_HF7].obj;
	clock_source_obj = _get_hal_obj_from_ord(GET_CLK_SOURCE_ORD(clk_hf7));
	clock_div = DT_PROP(DT_NODELABEL(clk_hf7), clock_div);

	if (_configure_clk_hf(clock_obj, clock_source_obj, &CYHAL_CLOCK_HF[7], clock_div)) {
		return -EIO;
	}
#endif

	/* Configure the HF[8] to source defined in tree device 'clk_hf8' node */
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_hf8))
	clock_obj = &clock_info_table[INFINEON_CAT1_CLOCK_HF8].obj;
	clock_source_obj = _get_hal_obj_from_ord(GET_CLK_SOURCE_ORD(clk_hf8));
	clock_div = DT_PROP(DT_NODELABEL(clk_hf8), clock_div);

	if (_configure_clk_hf(clock_obj, clock_source_obj, &CYHAL_CLOCK_HF[8], clock_div)) {
		return -EIO;
	}
#endif

	/* Configure the HF[9] to source defined in tree device 'clk_hf9' node */
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_hf9))
	clock_obj = &clock_info_table[INFINEON_CAT1_CLOCK_HF9].obj;
	clock_source_obj = _get_hal_obj_from_ord(GET_CLK_SOURCE_ORD(clk_hf9));
	clock_div = DT_PROP(DT_NODELABEL(clk_hf9), clock_div);

	if (_configure_clk_hf(clock_obj, clock_source_obj, &CYHAL_CLOCK_HF[9], clock_div)) {
		return -EIO;
	}
#endif

	/* Configure the HF[10] to source defined in tree device 'clk_hf10' node */
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_hf10))
	clock_obj = &clock_info_table[INFINEON_CAT1_CLOCK_HF10].obj;
	clock_source_obj = _get_hal_obj_from_ord(GET_CLK_SOURCE_ORD(clk_hf10));
	clock_div = DT_PROP(DT_NODELABEL(clk_hf10), clock_div);

	if (_configure_clk_hf(clock_obj, clock_source_obj, &CYHAL_CLOCK_HF[10], clock_div)) {
		return -EIO;
	}
#endif

	/* Configure the HF[11] to source defined in tree device 'clk_hf11' node */
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_hf11))
	clock_obj = &clock_info_table[INFINEON_CAT1_CLOCK_HF11].obj;
	clock_source_obj = _get_hal_obj_from_ord(GET_CLK_SOURCE_ORD(clk_hf11));
	clock_div = DT_PROP(DT_NODELABEL(clk_hf11), clock_div);

	if (_configure_clk_hf(clock_obj, clock_source_obj, &CYHAL_CLOCK_HF[11], clock_div)) {
		return -EIO;
	}
#endif

	/* Configure the HF[12] to source defined in tree device 'clk_hf12' node */
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_hf12))
	clock_obj = &clock_info_table[INFINEON_CAT1_CLOCK_HF12].obj;
	clock_source_obj = _get_hal_obj_from_ord(GET_CLK_SOURCE_ORD(clk_hf12));
	clock_div = DT_PROP(DT_NODELABEL(clk_hf12), clock_div);

	if (_configure_clk_hf(clock_obj, clock_source_obj, &CYHAL_CLOCK_HF[12], clock_div)) {
		return -EIO;
	}
#endif

	/* Configure the HF[13] to source defined in tree device 'clk_hf13' node */
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_hf13))
	clock_obj = &clock_info_table[INFINEON_CAT1_CLOCK_HF13].obj;
	clock_source_obj = _get_hal_obj_from_ord(GET_CLK_SOURCE_ORD(clk_hf13));
	clock_div = DT_PROP(DT_NODELABEL(clk_hf13), clock_div);

	if (_configure_clk_hf(clock_obj, clock_source_obj, &CYHAL_CLOCK_HF[13], clock_div)) {
		return -EIO;
	}
#endif

	/* Configure the clock fast to source defined in tree device 'clk_fast' node */
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_fast))
	clock_obj = &clock_info_table[INFINEON_CAT1_CLOCK_FAST].obj;
	clock_div = DT_PROP(DT_NODELABEL(clk_fast), clock_div);

	rslt = cyhal_clock_reserve(clock_obj, &CYHAL_CLOCK_FAST);
	if (rslt == CY_RSLT_SUCCESS) {
		rslt = cyhal_clock_set_divider(clock_obj, clock_div);
	}
	if (rslt) {
		return -EIO;
	}
#endif

	/* Configure the clock peri to source defined in tree device 'clk_peri' node */
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_peri))
	clock_obj = &clock_info_table[INFINEON_CAT1_CLOCK_PERI].obj;
	clock_div = DT_PROP(DT_NODELABEL(clk_peri), clock_div);

	rslt = cyhal_clock_reserve(clock_obj, &CYHAL_CLOCK_PERI);
	if (rslt == CY_RSLT_SUCCESS) {
		rslt = cyhal_clock_set_divider(clock_obj, clock_div);
	}
	if (rslt) {
		return -EIO;
	}
#endif

	/* Configure the clock slow to source defined in tree device 'clk_slow' node */
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_slow))
	clock_obj = &clock_info_table[INFINEON_CAT1_CLOCK_SLOW].obj;
	clock_div = DT_PROP(DT_NODELABEL(clk_slow), clock_div);

	rslt = cyhal_clock_reserve(clock_obj, &CYHAL_CLOCK_SLOW);
	if (rslt == CY_RSLT_SUCCESS) {
		rslt = cyhal_clock_set_divider(clock_obj, clock_div);
	}
	if (rslt) {
		return -EIO;
	}
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(dpll_hp))
	Cy_SysClk_Dpll_Hp0_Init();
	SystemCoreClockUpdate();
#endif

	return (int) rslt;
}

static int clock_control_infineon_cat_on_off(const struct device *dev,
					     clock_control_subsys_t sys)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(sys);

	/* On/off functionality are not supported */
	return -ENOSYS;
}

static DEVICE_API(clock_control, clock_control_infineon_cat1_api) = {
	.on = clock_control_infineon_cat_on_off,
	.off = clock_control_infineon_cat_on_off
};

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_imo))
DEVICE_DT_DEFINE(DT_NODELABEL(clk_imo),
		 clock_control_infineon_cat1_init,
		 NULL,
		 NULL,
		 NULL,
		 PRE_KERNEL_1,
		 CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		 &clock_control_infineon_cat1_api);
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_iho))
DEVICE_DT_DEFINE(DT_NODELABEL(clk_iho),
		 clock_control_infineon_cat1_init,
		 NULL,
		 NULL,
		 NULL,
		 PRE_KERNEL_1,
		 CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		 &clock_control_infineon_cat1_api);
#endif
