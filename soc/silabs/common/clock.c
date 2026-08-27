/*
 * Copyright (c) 2018, Christian Taedcke
 * Copyright (c) 2026 sevenlab engineering GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SoC initialization for SoCs using the gecko SDK.
 */

#define DT_DRV_COMPAT silabs_series0_cmu

#include <zephyr/kernel.h>

#include <em_cmu.h>
#include <em_device.h>
#include <soc_common.h>

#define CLK_SRC_IS(clk, src) DT_SAME_NODE(DT_CLOCKS_CTLR_BY_IDX(clk, 0), src)

/* Basic clock sources. */
#define CLK_LFXO         DT_INST_CLOCKS_CTLR_BY_NAME(0, lfxo)
#define CLK_LFXO_ENABLED DT_NODE_HAS_STATUS(CLK_LFXO, okay)
#define CLK_LFXO_FREQ    DT_PROP_OR(CLK_LFXO, clock_frequency, 0)

#define CLK_LFRCO         DT_INST_CLOCKS_CTLR_BY_NAME(0, lfrco)
#define CLK_LFRCO_ENABLED DT_NODE_HAS_STATUS(CLK_LFRCO, okay)

#define CLK_HFRCO         DT_INST_CLOCKS_CTLR_BY_NAME(0, hfrco)
#define CLK_HFRCO_ENABLED DT_NODE_HAS_STATUS(CLK_HFRCO, okay)
#define CLK_HFRCO_FREQ    DT_PROP_OR(CLK_HFRCO, clock_frequency, 0)

#define CLK_HFXO         DT_INST_CLOCKS_CTLR_BY_NAME(0, hfxo)
#define CLK_HFXO_ENABLED DT_NODE_HAS_STATUS(CLK_HFXO, okay)
#define CLK_HFXO_FREQ    DT_PROP_OR(CLK_HFXO, clock_frequency, 0)

/* Derived Clocks. */
#define CLK_HF      DT_INST_CLOCKS_CTLR_BY_NAME(0, hf)
#define CLK_HF_DIV  DT_PROP_OR(CLK_HF, clock_div, 1)
#define CLK_HF_MULT DT_PROP_OR(CLK_HF, clock_mult, 1)

#define CLK_LFA         DT_INST_CLOCKS_CTLR_BY_NAME(0, lfa)
#define CLK_LFA_ENABLED DT_NODE_HAS_STATUS(CLK_LFA, okay)
#define CLK_LFA_DIV     DT_PROP_OR(CLK_LFA, clock_div, 1)
#define CLK_LFA_MULT    DT_PROP_OR(CLK_LFA, clock_mult, 1)

#define CLK_LFB         DT_INST_CLOCKS_CTLR_BY_NAME(0, lfb)
#define CLK_LFB_ENABLED DT_NODE_HAS_STATUS(CLK_LFB, okay)
#define CLK_LFB_DIV     DT_PROP_OR(CLK_LFB, clock_div, 1)
#define CLK_LFB_MULT    DT_PROP_OR(CLK_LFB, clock_mult, 1)

#define CLK_LFC         DT_INST_CLOCKS_CTLR_BY_NAME(0, lfc)
#define CLK_LFC_ENABLED DT_NODE_HAS_STATUS(CLK_LFC, okay)
#define CLK_LFC_DIV     DT_PROP_OR(CLK_LFC, clock_div, 1)
#define CLK_LFC_MULT    DT_PROP_OR(CLK_LFC, clock_mult, 1)

#define CLK_LFE         DT_INST_CLOCKS_CTLR_BY_NAME(0, lfe)
#define CLK_LFE_ENABLED DT_NODE_HAS_STATUS(CLK_LFE, okay)
#define CLK_LFE_DIV     DT_PROP_OR(CLK_LFE, clock_div, 1)
#define CLK_LFE_MULT    DT_PROP_OR(CLK_LFE, clock_mult, 1)

BUILD_ASSERT(DT_NODE_HAS_STATUS(DT_CLOCKS_CTLR_BY_IDX(CLK_HF, 0), okay),
	     "HF clock source is not enabled.");
BUILD_ASSERT(!CLK_LFA_ENABLED || DT_NODE_HAS_STATUS(DT_CLOCKS_CTLR_BY_IDX(CLK_LFA, 0), okay),
	     "LFA clock source is not enabled.");
BUILD_ASSERT(!CLK_LFB_ENABLED || DT_NODE_HAS_STATUS(DT_CLOCKS_CTLR_BY_IDX(CLK_LFB, 0), okay),
	     "LFB clock source is not enabled.");
BUILD_ASSERT(!CLK_LFC_ENABLED || DT_NODE_HAS_STATUS(DT_CLOCKS_CTLR_BY_IDX(CLK_LFC, 0), okay),
	     "LFC clock source is not enabled.");
BUILD_ASSERT(!CLK_LFE_ENABLED || DT_NODE_HAS_STATUS(DT_CLOCKS_CTLR_BY_IDX(CLK_LFE, 0), okay),
	     "LFE clock source is not enabled.");

BUILD_ASSERT(CLK_HF_DIV == 1, "Unsupported HF divider");
BUILD_ASSERT(CLK_HF_MULT == 1, "Unsupported HF multiplier");

BUILD_ASSERT(CLK_LFA_DIV == 1, "Unsupported LFA divider");
BUILD_ASSERT(CLK_LFA_MULT == 1, "Unsupported LFA multiplier");

BUILD_ASSERT(CLK_LFB_DIV == 1, "Unsupported LFB divider");
BUILD_ASSERT(CLK_LFB_MULT == 1, "Unsupported LFB multiplier");

BUILD_ASSERT(CLK_LFC_DIV == 1, "Unsupported LFC divider");
BUILD_ASSERT(CLK_LFC_MULT == 1, "Unsupported LFC multiplier");

BUILD_ASSERT(CLK_LFE_DIV == 1, "Unsupported LFA divider");
BUILD_ASSERT(CLK_LFE_MULT == 1, "Unsupported LFA multiplier");

/**
 * @brief Initialization parameters for the external high frequency oscillator
 */
static CMU_HFXOInit_TypeDef hfxoInit = CMU_HFXOINIT_DEFAULT;

/**
 * @brief Initialization parameters for the external low frequency oscillator
 */
static CMU_LFXOInit_TypeDef lfxoInit = CMU_LFXOINIT_DEFAULT;

static void init_lfxo(void)
{
	/*
	 * Configuring LFXO disables it, so we can do that only if it's not
	 * used as a SYSCLK/HFCLK source.
	 */
	if (CMU_ClockSelectGet(cmuClock_HF) != cmuSelect_LFXO) {
		CMU_LFXOInit(&lfxoInit);
		CMU_OscillatorEnable(cmuOsc_LFXO, true, true);
	}

	SystemLFXOClockSet(CLK_LFXO_FREQ);
}

static void init_hfxo(void)
{
	if (CMU_ClockSelectGet(cmuClock_HF) != cmuSelect_HFXO) {
		CMU_HFXOInit(&hfxoInit);
		CMU_OscillatorEnable(cmuOsc_HFXO, true, true);
	}

	SystemHFXOClockSet(CLK_HFXO_FREQ);
}

#if CLK_HFRCO_ENABLED
static void init_hfrco(void)
{
#if defined(_CMU_HFRCOCTRL_FREQRANGE_MASK)
	/* Setting system HFRCO frequency */
	CMU_HFRCOBandSet(CLK_HFRCO_FREQ);
#elif defined(_CMU_HFRCOCTRL_BAND_MASK)

#if CLK_HFRCO_FREQ == 1000000
	CMU_HFRCOBandSet(cmuHFRCOBand_1MHz);
#elif CLK_HFRCO_FREQ == 7000000
	CMU_HFRCOBandSet(cmuHFRCOBand_7MHz);
#elif CLK_HFRCO_FREQ == 11000000
	CMU_HFRCOBandSet(cmuHFRCOBand_11MHz);
#elif CLK_HFRCO_FREQ == 14000000
	CMU_HFRCOBandSet(cmuHFRCOBand_14MHz);
#elif CLK_HFRCO_FREQ == 21000000
	CMU_HFRCOBandSet(cmuHFRCOBand_21MHz);
#elif CLK_HFRCO_FREQ == 24000000
	CMU_HFRCOBandSet(cmuHFRCOBand_24MHz);
#else
#error "Unsupported HFRCO frequency"
#endif

#else
#error "HFRCO is not supported on this SoC"
#endif
}
#endif

static void init_corele(void)
{
#if defined(cmuClock_CORELE)
	/* Ensure LE modules are clocked. */
	CMU_ClockEnable(cmuClock_CORELE, true);
#endif
}

void silabs_gecko_init_clocks(void)
{
	/* Init base clocks. */
	if (CLK_LFXO_ENABLED) {
		init_lfxo();
	}
	if (CLK_HFXO_ENABLED) {
		init_hfxo();
	}
#if CLK_HFRCO_ENABLED
	init_hfrco();
#endif

	/* Set HF clock source. */
	if (CLK_SRC_IS(CLK_HF, CLK_HFXO)) {
		CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFXO);
	} else if (CLK_SRC_IS(CLK_HF, CLK_LFXO)) {
		CMU_ClockSelectSet(cmuClock_HF, cmuSelect_LFXO);
	} else if (CLK_SRC_IS(CLK_HF, CLK_HFRCO)) {
		/* This is the default clock, the controller starts with. */
		CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFRCO);
	}

	if (!CLK_HFRCO_ENABLED) {
		CMU_OscillatorEnable(cmuOsc_HFRCO, false, false);
	}

	/* Enable the High Frequency Peripheral Clock */
	CMU_ClockEnable(cmuClock_HFPER, true);

	if (IS_ENABLED(CONFIG_GPIO_GECKO) || IS_ENABLED(CONFIG_LOG_BACKEND_SWO)) {
		CMU_ClockEnable(cmuClock_GPIO, true);
	}

	if (CLK_LFA_ENABLED || CLK_LFB_ENABLED || CLK_LFE_ENABLED) {
		init_corele();
	}

#if CLK_LFA_ENABLED
	if (CLK_SRC_IS(CLK_LFA, CLK_LFRCO)) {
		CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_LFRCO);
	} else if (CLK_SRC_IS(CLK_LFA, CLK_LFXO)) {
		CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_LFXO);
	}
#endif

#if CLK_LFB_ENABLED
	if (CLK_SRC_IS(CLK_LFB, CLK_LFRCO)) {
		CMU_ClockSelectSet(cmuClock_LFB, cmuSelect_LFRCO);
	} else if (CLK_SRC_IS(CLK_LFB, CLK_LFXO)) {
		CMU_ClockSelectSet(cmuClock_LFB, cmuSelect_LFXO);
	}
#endif

#if CLK_LFC_ENABLED
	if (CLK_SRC_IS(CLK_LFC, CLK_LFRCO)) {
		CMU_ClockSelectSet(cmuClock_LFC, cmuSelect_LFRCO);
	} else if (CLK_SRC_IS(CLK_LFC, CLK_LFXO)) {
		CMU_ClockSelectSet(cmuClock_LFC, cmuSelect_LFXO);
	}
#endif

#if CLK_LFE_ENABLED
	if (CLK_SRC_IS(CLK_LFE, CLK_LFRCO)) {
		CMU_ClockSelectSet(cmuClock_LFE, cmuSelect_LFRCO);
	} else if (CLK_SRC_IS(CLK_LFE, CLK_LFXO)) {
		CMU_ClockSelectSet(cmuClock_LFE, cmuSelect_LFXO);
	}
#endif
}
