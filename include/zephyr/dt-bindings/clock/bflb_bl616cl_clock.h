/*
 * Copyright (c) 2026 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_BFLB_BL616CL_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_BFLB_BL616CL_CLOCK_H_

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
#define BL616CL_CLKID_CLK_ROOT		BFLB_CLKID_CLK_ROOT
/** 32MHz RC Oscillator Clock */
#define BL616CL_CLKID_CLK_RC32M		BFLB_CLKID_CLK_RC32M
/** Crystal as clock */
#define BL616CL_CLKID_CLK_CRYSTAL	BFLB_CLKID_CLK_CRYSTAL
/** Bus Clock */
#define BL616CL_CLKID_CLK_BCLK		BFLB_CLKID_CLK_BCLK
/** F32K Clock */
#define BL616CL_CLKID_CLK_F32K		BFLB_CLKID_CLK_F32K
/** XTAL32K Clock */
#define BL616CL_CLKID_CLK_XTAL32K	BFLB_CLKID_CLK_XTAL32K
/** RC32K Clock */
#define BL616CL_CLKID_CLK_RC32K		BFLB_CLKID_CLK_RC32K
/** PLL clock, the standard root frequency of this PLL is 960MHz */
#define BL616CL_CLKID_CLK_PLL		BFLB_CLKID_CLK_PRIVATE
/** This clock is muxed off the PLLs to provide 160MHz */
#define BL616CL_CLKID_CLK_160M		(BFLB_CLKID_CLK_PRIVATE + 1)

/** ID 0, PLL root / 10 or 3 * top / 10 */
#define BL616CL_PLL_ID_DIV3_10	0
/** ID 1, PLL root / 5 or 3 * top / 5 */
#define BL616CL_PLL_ID_DIV3_5	1
/** ID 2, PLL root / 4 or 3 * top / 4 */
#define BL616CL_PLL_ID_DIV3_4	2
/** ID 3, PLL root / 3 or top / 1 */
#define BL616CL_PLL_ID_DIV1	3

/** The reference top frequency for the PLL at the root clock (PLL root / 3 here) */
#define BL616CL_PLL_TOP_FREQ	(DT_FREQ_M(320))

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_BFLB_BL616CL_CLOCK_H_ */
