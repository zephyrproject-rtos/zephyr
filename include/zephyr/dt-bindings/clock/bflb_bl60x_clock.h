/*
 * Copyright (c) 2025-2026 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_BFLB_BL60X_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_BFLB_BL60X_CLOCK_H_

#include "bflb_clock_common.h"

#define BL60X_CLKID_CLK_ROOT    BFLB_CLKID_CLK_ROOT
#define BL60X_CLKID_CLK_RC32M   BFLB_CLKID_CLK_RC32M
#define BL60X_CLKID_CLK_CRYSTAL BFLB_CLKID_CLK_CRYSTAL
#define BL60X_CLKID_CLK_BCLK    BFLB_CLKID_CLK_BCLK
#define BL60X_CLKID_CLK_PLL     4

/** The root frequency of the PLL is 480MHz */

/** ID 0, 48MHz = 480 / 10 */
#define BL60X_PLL_48MHz		0
/** ID 1, 120MHz = 480 / 4 */
#define BL60X_PLL_120MHz	1
/** ID 2, 160MHz = 480 / 3 */
#define BL60X_PLL_160MHz	2
/** ID 3, 192MHz = 480 / 2.5 */
#define BL60X_PLL_192MHz	3

/** Overclocked x 1.25
 * BCLK divider is the most important scaler for BL60x, so remember to tune the dividers
 */

/** ID 0, 60MHz = 480 / 10 * 1.25 */
#define BL60X_PLL_OC_60MHz	(0 | 0x10)
/** ID 1, 150MHz = 480 / 4 * 1.25 */
#define BL60X_PLL_OC_150MHz	(1 | 0x10)
/** ID 2, 200MHz = 480 / 3 * 1.25 */
#define BL60X_PLL_OC_200MHz	(2 | 0x10)
/** ID 3, 240MHz = 480 / 2.5 * 1.25 */
#define BL60X_PLL_OC_240MHz	(3 | 0x10)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_BFLB_BL60X_CLOCK_H_ */
