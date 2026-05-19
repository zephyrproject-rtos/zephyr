/*
 * Copyright (c) 2025-2026 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_BFLB_CLOCK_COMMON_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_BFLB_CLOCK_COMMON_H_

/** @cond INTERNAL_HIDDEN */

#ifndef DT_FREQ_M
#define DT_FREQ_M MHZ
#endif

/** @endcond */

/** Root Clock */
#define BFLB_CLKID_CLK_ROOT	0
/** 32MHz RC Oscillator Clock */
#define BFLB_CLKID_CLK_RC32M	1
/** Crystal as clock */
#define BFLB_CLKID_CLK_CRYSTAL	2
/** Bus Clock */
#define BFLB_CLKID_CLK_BCLK	3
/** F32K Clock */
#define BFLB_CLKID_CLK_F32K	4
/** XTAL32K Clock */
#define BFLB_CLKID_CLK_XTAL32K	5
/** RC32K Clock */
#define BFLB_CLKID_CLK_RC32K	6
/** Start of soc-specific clock ID entries */
#define BFLB_CLKID_CLK_PRIVATE	7

/** Pass to clock API to switch root clock to RC32M  */
#define BFLB_FORCE_ROOT_RC32M	32
/** Pass to clock API to switch root clock to Crystal  */
#define BFLB_FORCE_ROOT_CRYSTAL	33
/** Pass to clock API to switch root clock to dts-configured PLL  */
#define BFLB_FORCE_ROOT_PLL	34

/** Constant frequency of RC32M */
#define BFLB_RC32M_FREQUENCY	(DT_FREQ_M(32))

/** Constant frequency of F32K */
#define BFLB_F32K_FREQUENCY	32768U

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_BFLB_CLOCK_COMMON_H_ */
