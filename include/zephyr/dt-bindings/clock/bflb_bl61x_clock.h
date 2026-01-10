/*
 * Copyright (c) 2025 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_BFLB_BL61X_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_BFLB_BL61X_CLOCK_H_

#include "bflb_clock_common.h"

#define BL61X_CLKID_CLK_ROOT    BFLB_CLKID_CLK_ROOT
#define BL61X_CLKID_CLK_RC32M   BFLB_CLKID_CLK_RC32M
#define BL61X_CLKID_CLK_CRYSTAL BFLB_CLKID_CLK_CRYSTAL
#define BL61X_CLKID_CLK_BCLK    BFLB_CLKID_CLK_BCLK
#define BL61X_CLKID_CLK_WIFIPLL 4
#define BL61X_CLKID_CLK_AUPLL   5
#define BL61X_CLKID_CLK_160M    6

#define BL61X_AUPLL_DIV2	0
#define BL61X_AUPLL_DIV1	1
#define BL61X_WIFIPLL_240MHz	2
#define BL61X_WIFIPLL_320MHz	3

/* Overclocked
 * BCLK divider MUST be set so BCLK is 80MHz or slower to operate safely,
 * However BCLK of up to 130 MHz have been observed to be tolerated.
 * Drivers are able to compose a clock viable for them from clock sources (typically BCLK)
 * so no additional settings are required for the peripherals.
 * Breaks most complex peripherals (Wifi)
 * and peripherals that can use the peripheral bus as master (DMA and DMA-using drivers).
 * Flash input clock must be tuned to fit the new clocks.
 */

/* Overclock PLL to 480MHz for div ID 1 and 360MHz for div ID 2
 * Safe (default) core voltages are used (1.10v).
 */
#define BL61X_WIFIPLL_OC_360MHz	(2 | 0x10)
#define BL61X_WIFIPLL_OC_480MHz	(3 | 0x10)

/* Overclock PLL to 640MHz for div ID 1 and 480MHz for div ID 2.
 * This is the absolute maximum the PLL can provide.
 */
#define BL61X_WIFIPLL_OCMAX_480MHz	(2 | 0x20)
#define BL61X_WIFIPLL_OCMAX_640MHz	(3 | 0x20)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_BFLB_BL61X_CLOCK_H_ */
