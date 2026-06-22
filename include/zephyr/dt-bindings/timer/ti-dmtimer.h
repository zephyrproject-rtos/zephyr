/*
 * Copyright (c) 2026 Texas Instruments
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Texas Instruments DM Timer Clock Source Definitions
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_TIMER_TI_DMTIMER_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_TIMER_TI_DMTIMER_H_

/**
 * @name TI DM Timer Clock Sources
 * @{
 */

/** High-frequency oscillator 0 clock output */
#define HFOSC0_CLKOUT           0x0
/** High-frequency oscillator 0 clock output 32K */
#define HFOSC0_CLKOUT_32K       0x1
/** Main PLL0 HSDIV7 clock output */
#define MAIN_PLL0_HSDIV7_CLKOUT 0x2
/** 12MHz RC clock */
#define CLK_12M_RC              0x3
/** MCU external reference clock 0 */
#define MCU_EXT_REFCLK0         0x4
/** External reference clock 1 */
#define EXT_REFCLK1             0x5
/** CPTS reference frequency test clock */
#define CPTS_RFT_CLK            0x6
/** CPSW 3-port Gigabit Switch CPTS reference frequency test clock */
#define CPSW3G_CPTS_RFT_CLK     0x7
/** Main PLL1 HSDIV3 clock output */
#define MAIN_PLL1_HSDIV3_CLKOUT 0x8
/** Main PLL2 HSDIV6 clock output */
#define MAIN_PLL2_HSDIV6_CLKOUT 0x9
/** CPSW 3-port Gigabit Switch CPTS function 0 clock */
#define CPSW3G_CPTS_GENF0       0xA
/** CPSW 3-port Gigabit Switch CPTS function 1 clock */
#define CPSW3G_CPTS_GENF1       0xB
/** CPTS function 1 clock */
#define CPTS_GENF1              0xC
/** CPTS function 2 clock */
#define CPTS_GENF2              0xD
/** CPTS function 3 clock */
#define CPTS_GENF3              0xE
/** CPTS function 4 clock */
#define CPTS_GENF4              0xF

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_TIMER_TI_DMTIMER_H_ */
