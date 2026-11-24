/*
 * SPDX-FileCopyrightText: Copyright Alif Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ALIF_BALLETTO_CLOCKS_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ALIF_BALLETTO_CLOCKS_H_

/**
 * @file
 * @brief Clock IDs for Alif Balletto SoC family
 *
 * Defines peripheral clock identifiers for the Alif Balletto family.
 * Each clock ID is encoded using ALIF_CLK_CFG() macro from alif-clocks-common.h.
 */

#include "alif-clocks-common.h"

/**
 * @name Parent clock source identifiers
 *
 * Identifiers 0-31 correspond to bits in the CGU CLK_ENA register
 * that gates the parent clock. Parent clocks without a CLK_ENA gate use
 * identifiers 32-63
 * @{
 */

/** System AXI clock (SYST_ACLK), gated by CLK_ENA[SYSPLL] */
#define ALIF_PARENT_CLK_SYST_ACLK   0U
/** RTSS-HE clock (RTSS_HE_CLK), gated by CLK_ENA[ES1] */
#define ALIF_PARENT_CLK_RTSS_HE     13U
/** Peripheral clock (PERIPH_CLK), gated by CLK_ENA[CLK160M] */
#define ALIF_PARENT_CLK_PERIPH      20U
/** HFOSC clock (HFOSC_CLK), gated by CLK_ENA[CLK38P4M] */
#define ALIF_PARENT_CLK_HFOSC       23U

/** System AHB clock (SYST_HCLK), SYST_ACLK divided by SYSTOP_CLK_DIV[HCLK_DIVISOR] */
#define ALIF_PARENT_CLK_SYST_HCLK   32U
/** System APB clock (SYST_PCLK), SYST_ACLK divided by SYSTOP_CLK_DIV[PCLK_DIVISOR] */
#define ALIF_PARENT_CLK_SYST_PCLK   33U
/** Always-on 32 kHz clock (S32K_CLK), sourced from the LFXO or LFRC oscillator */
#define ALIF_PARENT_CLK_S32K        34U
/** Always-on 128 kHz clock (128K_CLK), derived from the LFRC oscillator */
#define ALIF_PARENT_CLK_128K        35U
/** Gate only clock. Doesn't support parent query */
#define ALIF_PARENT_GATE_ONLY       36U
/** APSS clock, not present on Balletto */
#define ALIF_PARENT_CLK_APSS        37U
/** 50 MHz clock, not present on Balletto */
#define ALIF_PARENT_CLK_50M         38U
/** 100 MHz clock, not present on Balletto */
#define ALIF_PARENT_CLK_100M        39U
/** 400 MHz clock, not present on Balletto */
#define ALIF_PARENT_CLK_400M        40U
/** 25 MHz clock, not present on Balletto */
#define ALIF_PARENT_CLK_25M         41U
/** System reference clock (SYST_REFCLK), selected by ESCLK_SEL[SYSREF] */
#define ALIF_PARENT_CLK_SYST_REFCLK 42U
/** PLL_CLK1 output */
#define ALIF_PARENT_CLK_PLL_CLK1    43U
/** PLL_CLK2 output */
#define ALIF_PARENT_CLK_PLL_CLK2    44U
/** 76.8 MHz clock (76M8_CLK), the HFXO output doubled */
#define ALIF_PARENT_CLK_76M8        45U
/** 20 MHz USB clock (USB_CLK), PLL_CLK1 divided by eight */
#define ALIF_PARENT_CLK_USB         46U
/** 10 MHz clock (10M_CLK), PLL_CLK1 divided by sixteen */
#define ALIF_PARENT_CLK_10M         47U
/** Audio PLL clock (AUDIO_PLL_CLK) */
#define ALIF_PARENT_CLK_AUDIO_PLL   48U
/** RTSS-HP clock, not present on Balletto */
#define ALIF_PARENT_CLK_RTSS_HP     49U
/** 266 MHz clock, not present on Balletto */
#define ALIF_PARENT_CLK_266M        50U
/** Ethernet reference clock, not present on Balletto */
#define ALIF_PARENT_CLK_ETH_REFCLK  51U

/** @} */

/**
 * @name Register offsets
 * @{
 */

/** UART control register offset in CLKCTL_PER_SLV */
#define ALIF_UART_CTRL_REG		0x08U

/** I2C0 control register offset in CLKCTL_PER_SLV */
#define ALIF_I2C0_CTRL_REG		0x50U

/** I2C1 control register offset in CLKCTL_PER_SLV */
#define ALIF_I2C1_CTRL_REG		0x54U

/** LPTIMER clock select register offset in VBAT */
#define ALIF_TIMER_CLKSEL_REG		0x04U

/** @} */

/**
 * @name UART peripheral clocks
 * @{
 */

/** UART0 clock sourced from system PCLK */
#define ALIF_UART0_SYST_PCLK        \
	ALIF_CLK_CFG(CLKCTL_PER_SLV, UART_CTRL, 0U, 1U, 1U, 1U, 8U, ALIF_PARENT_CLK_SYST_PCLK)
/** UART1 clock sourced from system PCLK */
#define ALIF_UART1_SYST_PCLK        \
	ALIF_CLK_CFG(CLKCTL_PER_SLV, UART_CTRL, 1U, 1U, 1U, 1U, 9U, ALIF_PARENT_CLK_SYST_PCLK)
/** UART2 clock sourced from system PCLK */
#define ALIF_UART2_SYST_PCLK        \
	ALIF_CLK_CFG(CLKCTL_PER_SLV, UART_CTRL, 2U, 1U, 1U, 1U, 10U, ALIF_PARENT_CLK_SYST_PCLK)
/** UART3 clock sourced from system PCLK */
#define ALIF_UART3_SYST_PCLK        \
	ALIF_CLK_CFG(CLKCTL_PER_SLV, UART_CTRL, 3U, 1U, 1U, 1U, 11U, ALIF_PARENT_CLK_SYST_PCLK)
/** UART4 clock sourced from system PCLK */
#define ALIF_UART4_SYST_PCLK        \
	ALIF_CLK_CFG(CLKCTL_PER_SLV, UART_CTRL, 4U, 1U, 1U, 1U, 12U, ALIF_PARENT_CLK_SYST_PCLK)
/** UART5 clock sourced from system PCLK */
#define ALIF_UART5_SYST_PCLK        \
	ALIF_CLK_CFG(CLKCTL_PER_SLV, UART_CTRL, 5U, 1U, 1U, 1U, 13U, ALIF_PARENT_CLK_SYST_PCLK)

/** @} */

/**
 * @name I2C peripheral clocks
 * @{
 */

/** I2C0 clock sourced from system PCLK */
#define ALIF_I2C0_SYST_PCLK         \
	ALIF_CLK_CFG(CLKCTL_PER_SLV, I2C0_CTRL, 0U, 1U, 0U, 0U, 0U, ALIF_PARENT_CLK_SYST_PCLK)
/** I2C1 clock sourced from system PCLK */
#define ALIF_I2C1_SYST_PCLK         \
	ALIF_CLK_CFG(CLKCTL_PER_SLV, I2C1_CTRL, 0U, 1U, 0U, 0U, 0U, ALIF_PARENT_CLK_SYST_PCLK)

/** @} */

/**
 * @name LPTIMER peripheral clocks
 *
 * The channels have no clock enable bit; only the source selection is
 * programmed.
 *
 * @{
 */

/** LPTIMER0 clock sourced from S32K_CLK */
#define ALIF_LPTIMER0_S32K_CLK      \
	ALIF_CLK_CFG(VBAT, TIMER_CLKSEL, 0U, 0U, 0U, 2U, 0U, ALIF_PARENT_CLK_S32K)
/** LPTIMER0 clock sourced from 128K_CLK */
#define ALIF_LPTIMER0_128K_CLK      \
	ALIF_CLK_CFG(VBAT, TIMER_CLKSEL, 0U, 0U, 1U, 2U, 0U, ALIF_PARENT_CLK_128K)
/** LPTIMER1 clock sourced from S32K_CLK */
#define ALIF_LPTIMER1_S32K_CLK      \
	ALIF_CLK_CFG(VBAT, TIMER_CLKSEL, 0U, 0U, 0U, 2U, 4U, ALIF_PARENT_CLK_S32K)
/** LPTIMER1 clock sourced from 128K_CLK */
#define ALIF_LPTIMER1_128K_CLK      \
	ALIF_CLK_CFG(VBAT, TIMER_CLKSEL, 0U, 0U, 1U, 2U, 4U, ALIF_PARENT_CLK_128K)

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ALIF_BALLETTO_CLOCKS_H_ */
