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

/** Pass to clock API to switch root clock to RC32M  */
#define BFLB_FORCE_ROOT_RC32M	10
/** Pass to clock API to switch root clock to Crystal  */
#define BFLB_FORCE_ROOT_CRYSTAL	11
/** Pass to clock API to switch root clock to dts-configured PLL  */
#define BFLB_FORCE_ROOT_PLL	12

/** Constant frequency of RC32M */
#define BFLB_RC32M_FREQUENCY	(DT_FREQ_M(32))

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_BFLB_CLOCK_COMMON_H_ */
