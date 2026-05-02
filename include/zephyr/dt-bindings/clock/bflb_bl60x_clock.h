/*
 * Copyright (c) 2025-2026 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_BFLB_BL60X_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_BFLB_BL60X_CLOCK_H_

#include "bflb_clock_common.h"

/**
 * @file
 * Performance on BL60x is a complex affair:
 * BCLK divider is the most important scaler for performance on BL60x due to what appears as
 * a hardware design issue relating to feeding the CPU, as BCLK is mostly unimportant for
 * raw CPU performance on all other BFLB SoCs, including BL70x and BL70xL which use the same cache
 * and CPU schemes.
 * So remember to tune the dividers, keeping in mind BFLB recommends not running BCLK over 96MHz.
 * Here are some example setups that work well:
 *
 * Base:
 * Root: 192MHz (Standard)
 * BCLK: 96MHz (Root / 2, Standard)
 * Flash CLK: 48MHz (BCLK / 2, may vary depending on module or PCB but this is a safe bet)
 * Performance 1 (351 Cmrks), Unrolled: 1.125 (395 Cmrks)
 *
 * Best when unrolling loops with higher voltage (nonstandard usage):
 * Root: 200MHz (Standard)
 * BCLK: 200MHz (Overclocked)
 * Flash CLK: 66MHz (BCLK / 3)
 * SoC voltage: 1.25 V (Increased)
 * Performance 1.52 (534 Cmrks), Unrolled: 1.9 (666 Cmrks)
 *
 * Best when unrolling loops (nonstandard usage, maximum BCLK without increased voltage):
 * Root: 150MHz (Value from overclock but within Standard range)
 * BCLK: 150MHz (Overclocked)
 * Flash CLK: 75MHz (BCLK / 2)
 * Performance 1.14 (400 Cmrks), Unrolled: 1.42 (500 Cmrks)
 *
 * Best when not unrolling loops with higher voltage:
 * Root: 320MHz (Overclocked)
 * BCLK: 160MHz (Root / 2, Overclocked)
 * Flash CLK: 80MHz (BCLK / 2)
 * SoC voltage: 1.25 V (Increased)
 * Performance 1.67 (585 Cmrks), Unrolled: 1.88 (660 Cmrks)
 *
 * Best when not unrolling loops:
 * Root: 240MHz (Overclocked)
 * BCLK: 120MHz (Root / 2, Overclocked)
 * Flash CLK: 60MHz (BCLK / 2)
 * Performance 1.25 (440 Cmrks), Unrolled: 1.41 (494 Cmrks)
 *
 * On this SoC, the main CPU can be clocked higher, but this lacks relevance as the performance is
 * limited by BCLK speed and 200 MHz BCLK is already close to the highest it can get.
 * The maximum reached stable frequency is 360MHz at 1.35v.
 * Using 360/180 results in only 10-22% of improvement (1.8 (660 Cmrks), Unrolled: 2.1 (740 Cmrks))
 * over the 200/200 setup, on top of needing the maximum voltage possible applied to the SoC.
 * Finally, due to requirements of the PLL, lower speed reference clocks than the usual
 * 40MHz crystal may not allow it to lock.
 *
 * When overclocking, most complex peripherals (Wifi) and peripherals that can use
 * the peripheral bus as a master (DMA and DMA-using drivers) do not work properly.
 */

/** Root Clock */
#define BL60X_CLKID_CLK_ROOT	BFLB_CLKID_CLK_ROOT
/** 32MHz RC Oscillator Clock */
#define BL60X_CLKID_CLK_RC32M	BFLB_CLKID_CLK_RC32M
/** Crystal as clock */
#define BL60X_CLKID_CLK_CRYSTAL	BFLB_CLKID_CLK_CRYSTAL
/** Bus Clock */
#define BL60X_CLKID_CLK_BCLK	BFLB_CLKID_CLK_BCLK
/** F32K Clock */
#define BL60X_CLKID_CLK_F32K	BFLB_CLKID_CLK_F32K
/** XTAL32K Clock */
#define BL60X_CLKID_CLK_XTAL32K	BFLB_CLKID_CLK_XTAL32K
/** RC32K Clock */
#define BL60X_CLKID_CLK_RC32K	BFLB_CLKID_CLK_RC32K
/** PLL clock, the standard root frequency of the PLL is 480MHz */
#define BL60X_CLKID_CLK_PLL	BFLB_CLKID_CLK_PRIVATE

/** The reference top frequency for the PLL at the root clock (PLL root / 2.5 here) */
#define BL60X_PLL_TOP_FREQ	(DT_FREQ_M(192))

/** ID 0, PLL root / 10 or top / 4  */
#define BL60X_PLL_ID_DIV4	0
/** ID 1, PLL root / 4 or 5 * top / 8 */
#define BL60X_PLL_ID_DIV5_8	1
/** ID 2, PLL root / 3 or 5 * top / 3 */
#define BL60X_PLL_ID_DIV5_3	2
/** ID 3, PLL root / 2.5 or top / 1 */
#define BL60X_PLL_ID_DIV1	3

/** Overclocked PLL example 1, standard voltages can be used */
#define BL60X_PLL_TOP_FREQ_OC1	(DT_FREQ_M(240))

/** Overclocked PLL example 2, increased voltages (1.25v) must be used */
#define BL60X_PLL_TOP_FREQ_OC2	(DT_FREQ_M(320))

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_BFLB_BL60X_CLOCK_H_ */
