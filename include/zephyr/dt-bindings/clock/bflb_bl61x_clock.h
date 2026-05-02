/*
 * Copyright (c) 2025-2026 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_BFLB_BL61X_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_BFLB_BL61X_CLOCK_H_

#include "bflb_clock_common.h"

/**
 * @file
 * BCLK divider MUST be set so BCLK is 80MHz or slower to operate safely,
 * However BCLK of up to 130 MHz have been observed to be tolerated.
 * Drivers are able to compose a clock viable for them from clock sources (typically BCLK)
 * so no additional settings are required for the peripherals.
 * Breaks most complex peripherals (Wifi)
 * and peripherals that can use the peripheral bus as master (DMA and DMA-using drivers).
 * Flash input clock must be tuned to fit the new clocks.
 */

/** Root Clock */
#define BL61X_CLKID_CLK_ROOT    BFLB_CLKID_CLK_ROOT
/** 32MHz RC Oscillator Clock */
#define BL61X_CLKID_CLK_RC32M   BFLB_CLKID_CLK_RC32M
/** Crystal as clock */
#define BL61X_CLKID_CLK_CRYSTAL BFLB_CLKID_CLK_CRYSTAL
/** Bus Clock */
#define BL61X_CLKID_CLK_BCLK    BFLB_CLKID_CLK_BCLK
/** F32K Clock */
#define BL61X_CLKID_CLK_F32K	BFLB_CLKID_CLK_F32K
/** XTAL32K Clock */
#define BL61X_CLKID_CLK_XTAL32K	BFLB_CLKID_CLK_XTAL32K
/** RC32K Clock */
#define BL61X_CLKID_CLK_RC32K	BFLB_CLKID_CLK_RC32K
/** WIFIPLL clock, the standard root frequency of this PLL is 960MHz */
#define BL61X_CLKID_CLK_WIFIPLL BFLB_CLKID_CLK_PRIVATE
/** AUPLL clock, it has no standard root frequency */
#define BL61X_CLKID_CLK_AUPLL   (BFLB_CLKID_CLK_PRIVATE + 1)
/** This clock is muxed off the PLLs to provide 160MHz */
#define BL61X_CLKID_CLK_160M    (BFLB_CLKID_CLK_PRIVATE + 2)

/** ID 0, AUPLL / 2  */
#define BL61X_AUPLL_ID_DIV2	0
/** ID 1, AUPLL / 1  */
#define BL61X_AUPLL_ID_DIV1	1
/** ID 2, WIFIPLL root / 4 or 3 * top / 4 */
#define BL61X_WIFIPLL_ID_DIV3_4	2
/** ID 3, WIFIPLL root / 3 or top / 1 */
#define BL61X_WIFIPLL_ID_DIV1	3

/** The reference top frequency for the WIFIPLL at the root clock (WIFIPLL root / 3 here) */
#define BL61X_WIFIPLL_TOP_FREQ	(DT_FREQ_M(320))

/** The reference top frequency for the AUPLL at the root clock (undefined) */
#define BL61X_AUPLL_TOP_FREQ	(DT_FREQ_M(1))

/** Overclocked PLL example 1, standard voltages can be used */
#define BL61X_WIFIPLL_TOP_FREQ_OC1	(DT_FREQ_M(480))

/** Overclocked PLL example 2, increased voltages (1.25v) must be used */
#define BL61X_WIFIPLL_TOP_FREQ_OC2	(DT_FREQ_M(640))

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_BFLB_BL61X_CLOCK_H_ */
