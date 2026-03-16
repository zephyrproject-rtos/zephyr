/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_BFLB_BL808_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_BFLB_BL808_CLOCK_H_

#include "bflb_clock_common.h"

/**
 * @file
 * BL808 clock IDs and PLL configuration constants for device tree bindings.
 */

/** Root Clock */
#define BL808_CLKID_CLK_ROOT    BFLB_CLKID_CLK_ROOT
/** 32MHz RC Oscillator Clock */
#define BL808_CLKID_CLK_RC32M   BFLB_CLKID_CLK_RC32M
/** Crystal Clock */
#define BL808_CLKID_CLK_CRYSTAL BFLB_CLKID_CLK_CRYSTAL
/** Bus Clock */
#define BL808_CLKID_CLK_BCLK    BFLB_CLKID_CLK_BCLK
/** F32K Clock */
#define BL808_CLKID_CLK_F32K    BFLB_CLKID_CLK_F32K
/** XTAL32K Clock */
#define BL808_CLKID_CLK_XTAL32K BFLB_CLKID_CLK_XTAL32K
/** RC32K Clock */
#define BL808_CLKID_CLK_RC32K   BFLB_CLKID_CLK_RC32K
/** WiFi PLL Clock */
#define BL808_CLKID_CLK_WIFIPLL (BFLB_CLKID_CLK_PRIVATE + 0)
/** Audio PLL Clock */
#define BL808_CLKID_CLK_AUPLL   (BFLB_CLKID_CLK_PRIVATE + 1)
/** CPU PLL Clock */
#define BL808_CLKID_CLK_CPUPLL  (BFLB_CLKID_CLK_PRIVATE + 2)
/** 160MHz Clock */
#define BL808_CLKID_CLK_160M    (BFLB_CLKID_CLK_PRIVATE + 3)

/** CPU PLL output select: top frequency (400 MHz) */
#define BL808_CPUPLL_ID_400M    0
/** AUPLL output select: divide by 1 */
#define BL808_AUPLL_ID_DIV1     1
/** WiFi PLL output select: 3/4 of top frequency */
#define BL808_WIFIPLL_ID_DIV3_4 2
/** WiFi PLL output select: top frequency */
#define BL808_WIFIPLL_ID_DIV1   3

/** WiFi PLL top frequency (960MHz VCO / 3 = 320MHz) */
#define BL808_WIFIPLL_TOP_FREQ DT_FREQ_M(320)
/** Audio PLL top frequency */
#define BL808_AUPLL_TOP_FREQ   DT_FREQ_M(1)

/** Overclocked WiFi PLL top frequency (960MHz VCO / 2 = 480MHz) */
#define BL808_WIFIPLL_TOP_FREQ_OC1 DT_FREQ_M(480)
/** Overclocked WiFi PLL top frequency (no divider = 640MHz) */
#define BL808_WIFIPLL_TOP_FREQ_OC2 DT_FREQ_M(640)

/** CPU PLL top frequency (800 MHz VCO / 2 = 400 MHz) */
#define BL808_CPUPLL_TOP_FREQ DT_FREQ_M(400)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_BFLB_BL808_CLOCK_H_ */
