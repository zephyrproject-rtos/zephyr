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

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ADI_MAX32_CLOCK_H_ */
