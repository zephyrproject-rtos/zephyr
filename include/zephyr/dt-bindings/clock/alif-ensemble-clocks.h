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
 * @name Register offsets
 * @{
 */

/** UART control register offset in CLKCTL_PER_SLV */
#define ALIF_UART_CTRL_REG		0x08U

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

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ALIF_ENSEMBLE_CLOCKS_H_ */
