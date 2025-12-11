/*
 * Copyright 2025 Texas Instruments Inc.
 * Copyright 2025 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_MSPM0_CLOCK_H
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_MSPM0_CLOCK_H

#define MSPM0_CLOCK(clk, bit) ((clk << 8) | bit)

/* Peripheral clock source selection register mask */
#define MSPM0_CLOCK_PERIPH_REG_MASK(X)	(X & 0xFF)

/* Clock references */
#define MSPM0_CLOCK_SYSOSC		MSPM0_CLOCK(0x0, 0x0)
#define MSPM0_CLOCK_LFCLK		MSPM0_CLOCK(0x1, 0x2)
#define MSPM0_CLOCK_MFCLK		MSPM0_CLOCK(0x2, 0x4)
#define MSPM0_CLOCK_BUSCLK		MSPM0_CLOCK(0x3, 0x8)
#define MSPM0_CLOCK_ULPCLK		MSPM0_CLOCK(0x4, 0x8)
#define MSPM0_CLOCK_MCLK		MSPM0_CLOCK(0x5, 0x8)
#define MSPM0_CLOCK_MFPCLK		MSPM0_CLOCK(0x6, 0x0)
#define MSPM0_CLOCK_CANCLK		MSPM0_CLOCK(0x7, 0x0)
#define MSPM0_CLOCK_CLK_OUT		MSPM0_CLOCK(0x8, 0x0)

#endif
