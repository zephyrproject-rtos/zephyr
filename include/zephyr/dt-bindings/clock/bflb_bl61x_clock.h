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

#define BL61X_AUPLL_DIV2	0
#define BL61X_AUPLL_DIV1	1
#define BL61X_WIFIPLL_240MHz	2
#define BL61X_WIFIPLL_320MHz	3

/* Overclocked
 * Overclock PLL to 480MHz for div ID 1 and 360MHz for div ID 2
 * BCLK divider MUST be set so BCLK is 80MHz or slower
 * Breaks most complex peripherals (Wifi)
 */
#define BL61X_WIFIPLL_OC_360MHz	(2 | 0x10)
#define BL61X_WIFIPLL_OC_480MHz	(3 | 0x10)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_BFLB_BL61X_CLOCK_H_ */
