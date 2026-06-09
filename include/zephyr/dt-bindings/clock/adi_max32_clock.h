/*
 * Copyright (c) 2023-2024 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ADI_MAX32_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ADI_MAX32_CLOCK_H_

/** Peripheral clock register */
#define ADI_MAX32_CLOCK_BUS0 0
#define ADI_MAX32_CLOCK_BUS1 1
#define ADI_MAX32_CLOCK_BUS2 2

/** Clock source for peripheral interfaces like UART, WDT... */
#define ADI_MAX32_PRPH_CLK_SRC_PCLK      0 /* Peripheral clock */
#define ADI_MAX32_PRPH_CLK_SRC_EXTCLK    1 /* External clock */
#define ADI_MAX32_PRPH_CLK_SRC_IBRO      2 /* Internal Baud Rate Oscillator*/
#define ADI_MAX32_PRPH_CLK_SRC_ERFO      3 /* External RF Oscillator */
#define ADI_MAX32_PRPH_CLK_SRC_ERTCO     4 /* External RTC Oscillator */
#define ADI_MAX32_PRPH_CLK_SRC_INRO      5 /* Internal Nano Ring Oscillator */
#define ADI_MAX32_PRPH_CLK_SRC_ISO       6 /* Internal Secondary Oscillator */
#define ADI_MAX32_PRPH_CLK_SRC_IBRO_DIV8 7 /* IBRO/8 */
#define ADI_MAX32_PRPH_CLK_SRC_IPLL      8 /* Internal Phase Lock Loop Oscillator */
#define ADI_MAX32_PRPH_CLK_SRC_EBO       9 /* External Base Oscillator */

/** Clock source enable */
#define ADI_MAX32_CLK_SRC_EN_ISO        0x0 /* Internal Secondary Oscillator */
#define ADI_MAX32_CLK_SRC_EN_IPLL       0x1 /* Internal Phase Lock Loop Oscillator */
#if defined(CONFIG_SOC_MAX78002)
#define ADI_MAX32_CLK_SRC_EN_EBO        0x2 /* External Base Oscillator */
#else
#define ADI_MAX32_CLK_SRC_EN_ERFO       0x2 /* External RF Oscillator */
#endif
#define ADI_MAX32_CLK_SRC_EN_INRO       0x3 /* Internal Nano Ring Oscillator */
#if defined(CONFIG_SOC_MAX32520) || defined(CONFIG_SOC_MAX32657)
#define ADI_MAX32_CLK_SRC_EN_IPO        0x0 /* Internal Primary Oscillator */
#else
#define ADI_MAX32_CLK_SRC_EN_IPO        0x4 /* Internal Primary Oscillator */
#endif
#define ADI_MAX32_CLK_SRC_EN_IBRO       0x5 /* Internal Baud Rate Oscillator */
#define ADI_MAX32_CLK_SRC_EN_ERTCO      0x6 /* External RTC Oscillator */
#define ADI_MAX32_CLK_SRC_EN_EXTCLK     0x7 /* External Clock */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ADI_MAX32_CLOCK_H_ */
