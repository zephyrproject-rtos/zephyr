/*
 * Copyright 2025 Texas Instruments Inc.
 * Copyright 2025 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_MSPM_CLOCK_H
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_MSPM_CLOCK_H

#define MSPM_CLOCK(clk, bit) ((clk << 8) | bit)

/* Peripheral clock source selection register mask */
#define MSPM_CLOCK_PERIPH_REG_MASK(X)	(X & 0xFF)

/* Clock references */
#define MSPM_CLOCK_SYSOSC		MSPM_CLOCK(0x0, 0x0)
#define MSPM_CLOCK_LFCLK		MSPM_CLOCK(0x1, 0x2)
#define MSPM_CLOCK_MFCLK		MSPM_CLOCK(0x2, 0x4)
#define MSPM_CLOCK_BUSCLK		MSPM_CLOCK(0x3, 0x8)
#define MSPM_CLOCK_ULPCLK		MSPM_CLOCK(0x4, 0x8)
#define MSPM_CLOCK_MCLK		MSPM_CLOCK(0x5, 0x8)
#define MSPM_CLOCK_MFPCLK		MSPM_CLOCK(0x6, 0x0)
#define MSPM_CLOCK_CANCLK		MSPM_CLOCK(0x7, 0x0)
#define MSPM_CLOCK_CLK_OUT		MSPM_CLOCK(0x8, 0x0)
/** @brief High frequency clock reference */
#define MSPM_CLOCK_HFCLK		MSPM_CLOCK(0x9, 0x2)

#endif
