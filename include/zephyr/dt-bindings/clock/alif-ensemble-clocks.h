/*
 * Copyright (c) 2025 Alif Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ALIF_ENSEMBLE_CLOCKS_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ALIF_ENSEMBLE_CLOCKS_H_

/**
 * @file
 * @brief Clock IDs for Alif Ensemble SoC family
 *
 * Defines peripheral clock identifiers for the Alif Ensemble family.
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
/** APSS clock (APSS_CLK), gated by CLK_ENA[CPUPLL] */
#define ALIF_PARENT_CLK_APSS        4U
/** 25 MHz clock (25M_CLK), sourced from 100M_CLK/4, gated by CLK_ENA[CLK25M] */
#define ALIF_PARENT_CLK_25M         5U
/** 50 MHz RMII reference clock from the internal PLL output, gated by CLK_ENA[CLK50M] */
#define ALIF_PARENT_CLK_50M         6U
/** 100 MHz clock (100M_CLK), PLL_CLK1 divided by eight, gated by CLK_ENA[CLK100M] */
#define ALIF_PARENT_CLK_100M        7U
/** RTSS-HP clock (RTSS_HP_CLK), gated by CLK_ENA[ES0] */
#define ALIF_PARENT_CLK_RTSS_HP     12U
/** RTSS-HE clock (RTSS_HE_CLK), gated by CLK_ENA[ES1] */
#define ALIF_PARENT_CLK_RTSS_HE     13U
/** 400 MHz clock (400M_CLK), PLL_CLK1 divided by two, gated by CLK_ENA[CLK400M] */
#define ALIF_PARENT_CLK_400M        15U
/** Peripheral clock (PERIPH_CLK), gated by CLK_ENA[CLK160M] */
#define ALIF_PARENT_CLK_PERIPH      20U
/** 266 MHz clock (266M_CLK), PLL_CLK1 divided by three, gated by CLK_ENA[CLK266M] */
#define ALIF_PARENT_CLK_266M        21U
/** 20 MHz USB clock (USB_CLK), PLL_CLK1 divided by 40, gated by CLK_ENA[CLK20M] */
#define ALIF_PARENT_CLK_USB         22U
/** HFOSC clock (HFOSC_CLK), gated by CLK_ENA[CLK38P4M] */
#define ALIF_PARENT_CLK_HFOSC       23U
/** 76.8 MHz clock (76M8_CLK), the HFXO output doubled, gated by CLK_ENA[CLK76P8M] */
#define ALIF_PARENT_CLK_76M8        24U
/** System APB clock (SYST_PCLK), gated by CLK_ENA[PCLK] */
#define ALIF_PARENT_CLK_SYST_PCLK   25U
/** System AHB clock (SYST_HCLK), gated by CLK_ENA[HCLK] */
#define ALIF_PARENT_CLK_SYST_HCLK   26U

/** Always-on 32 kHz clock (S32K_CLK), sourced from the LFXO or LFRC oscillator */
#define ALIF_PARENT_CLK_S32K        32U
/** Always-on 128 kHz clock (128K_CLK), derived from the LFRC oscillator */
#define ALIF_PARENT_CLK_128K        33U
/** Gate only clock. Doesn't support parent query */
#define ALIF_PARENT_GATE_ONLY       34U
/** System reference clock (SYST_REFCLK), selected by PLL_CLK_SEL[SYSREF] */
#define ALIF_PARENT_CLK_SYST_REFCLK 35U
/** PLL_CLK1 output */
#define ALIF_PARENT_CLK_PLL_CLK1    36U
/** PLL_CLK2 output, not present on Ensemble */
#define ALIF_PARENT_CLK_PLL_CLK2    37U
/**
 * 10 MHz clock (10M_CLK), PLL_CLK1 divided by 80, gated together with USB_CLK
 * by CLK_ENA[CLK20M]
 */
#define ALIF_PARENT_CLK_10M         38U
/** Audio PLL clock (AUDIO_PLL_CLK) */
#define ALIF_PARENT_CLK_AUDIO_PLL   39U
/** Ethernet reference clock, sourced from external RMII clock */
#define ALIF_PARENT_CLK_ETH_REFCLK  40U

/** @} */

/**
 * @name Register offsets
 * @{
 */

/** Clock enable register offset in CGU */
#define ALIF_CLK_ENA_REG		0x14U

/** Peripheral clock enable register offset in CLKCTL_PER_MST */
#define ALIF_PERIPH_CLK_ENA_REG		0x0CU
/** Ethernet control register offset in CLKCTL_PER_MST */
#define ALIF_ETH_CTRL0_REG		0x80U

/** UART control register offset in CLKCTL_PER_SLV */
#define ALIF_UART_CTRL_REG		0x08U

/** I2C0 control register offset in CLKCTL_PER_SLV */
#define ALIF_I2C0_CTRL_REG		0x50U
/** I2C1 control register offset in CLKCTL_PER_SLV */
#define ALIF_I2C1_CTRL_REG		0x54U
/** I2C2 control register offset in CLKCTL_PER_SLV */
#define ALIF_I2C2_CTRL_REG		0x58U
/** I2C3 control register offset in CLKCTL_PER_SLV */
#define ALIF_I2C3_CTRL_REG		0x5CU

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
/** UART6 clock sourced from system PCLK */
#define ALIF_UART6_SYST_PCLK        \
	ALIF_CLK_CFG(CLKCTL_PER_SLV, UART_CTRL, 6U, 1U, 1U, 1U, 14U, ALIF_PARENT_CLK_SYST_PCLK)
/** UART7 clock sourced from system PCLK */
#define ALIF_UART7_SYST_PCLK        \
	ALIF_CLK_CFG(CLKCTL_PER_SLV, UART_CTRL, 7U, 1U, 1U, 1U, 15U, ALIF_PARENT_CLK_SYST_PCLK)

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
/** I2C2 clock sourced from system PCLK */
#define ALIF_I2C2_SYST_PCLK         \
	ALIF_CLK_CFG(CLKCTL_PER_SLV, I2C2_CTRL, 0U, 1U, 0U, 0U, 0U, ALIF_PARENT_CLK_SYST_PCLK)
/** I2C3 clock sourced from system PCLK */
#define ALIF_I2C3_SYST_PCLK         \
	ALIF_CLK_CFG(CLKCTL_PER_SLV, I2C3_CTRL, 0U, 1U, 0U, 0U, 0U, ALIF_PARENT_CLK_SYST_PCLK)

/** @} */

/**
 * @name SPI peripheral clocks
 * @{
 */

/** SPI0 to SPI3 clock sourced from system HCLK */
#define ALIF_SPI_SYST_HCLK          ALIF_CLK_CFG_FIXED(ALIF_PARENT_CLK_SYST_HCLK)

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
/** LPTIMER2 clock sourced from S32K_CLK */
#define ALIF_LPTIMER2_S32K_CLK      \
	ALIF_CLK_CFG(VBAT, TIMER_CLKSEL, 0U, 0U, 0U, 2U, 8U, ALIF_PARENT_CLK_S32K)
/** LPTIMER2 clock sourced from 128K_CLK */
#define ALIF_LPTIMER2_128K_CLK      \
	ALIF_CLK_CFG(VBAT, TIMER_CLKSEL, 0U, 0U, 1U, 2U, 8U, ALIF_PARENT_CLK_128K)
/** LPTIMER3 clock sourced from S32K_CLK */
#define ALIF_LPTIMER3_S32K_CLK      \
	ALIF_CLK_CFG(VBAT, TIMER_CLKSEL, 0U, 0U, 0U, 2U, 12U, ALIF_PARENT_CLK_S32K)
/** LPTIMER3 clock sourced from 128K_CLK */
#define ALIF_LPTIMER3_128K_CLK      \
	ALIF_CLK_CFG(VBAT, TIMER_CLKSEL, 0U, 0U, 1U, 2U, 12U, ALIF_PARENT_CLK_128K)

/** @} */

/**
 * @name Ethernet clocks
 *
 * The 50 MHz RMII clock, from which the MAC derives its transmit and
 * receive clocks, is gated in PERIPH_CLK_ENA and selected in
 * ETH_CTRL0[RMII_CLKSEL] from either the ETH_REFCLK pin or the internal
 * 50 MHz PLL output. The source selection has no enable bit; only the
 * source is programmed.
 *
 * The CSR clock, which clocks the register interface of the MAC and from
 * which the MDIO clock is divided, is SYST_HCLK. It has neither a gate nor a
 * source selection, so its identifier names no register and only reports the
 * rate.
 *
 * The PTP reference clock, which drives the timestamp counter of the MAC, is
 * 100M_CLK. Its gate in the CGU clock enable register also gates 100M_CLK for
 * the other peripherals it feeds.
 *
 * @{
 */

/** Ethernet RMII clock gate */
#define ALIF_ETH_RMII_CLK           \
	ALIF_CLK_CFG(CLKCTL_PER_MST, PERIPH_CLK_ENA, 12U, 1U, 0U, 0U, 0U, ALIF_PARENT_CLK_50M)
/** Ethernet RMII clock sourced from the ETH_REFCLK pin */
#define ALIF_ETH_RMII_REFCLK        \
	ALIF_CLK_CFG(CLKCTL_PER_MST, ETH_CTRL0, 0U, 0U, 0U, 1U, 4U, ALIF_PARENT_CLK_50M)
/** Ethernet RMII clock sourced from the internal 50 MHz PLL output */
#define ALIF_ETH_RMII_PLL_50M       \
	ALIF_CLK_CFG(CLKCTL_PER_MST, ETH_CTRL0, 0U, 0U, 1U, 1U, 4U, ALIF_PARENT_CLK_50M)
/** Ethernet CSR clock sourced from system HCLK */
#define ALIF_ETH_SYST_HCLK          ALIF_CLK_CFG_FIXED(ALIF_PARENT_CLK_SYST_HCLK)
/** Ethernet PTP reference clock sourced from 100M_CLK */
#define ALIF_ETH_PTP_100M_CLK       \
	ALIF_CLK_CFG(CGU, CLK_ENA, 7U, 1U, 0U, 0U, 0U, ALIF_PARENT_CLK_100M)

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ALIF_ENSEMBLE_CLOCKS_H_ */
