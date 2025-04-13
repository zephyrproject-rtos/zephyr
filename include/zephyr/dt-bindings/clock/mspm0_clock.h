/*
 * Copyright 2025 Texas Instruments Inc.
 * Copyright 2025 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_MSPM0_CLOCK_H
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_MSPM0_CLOCK_H

#define MSPM0_CLOCK(bus, bit) ((bus << 8) | bit)

/* Peripheral clock source selection register mask */
#define MSPM0_CLOCK_PERIPH_REG_MASK(X)	(X & 0xFF)

/* Clock bus references */
#define MSPM0_CLOCK_BUS_LFOSC		MSPM0_CLOCK(0x0, 0x0)
#define MSPM0_CLOCK_BUS_EXLF		MSPM0_CLOCK(0x1, 0x0)
#define MSPM0_CLOCK_BUS_SYSOSC		MSPM0_CLOCK(0x2, 0x0)
#define MSPM0_CLOCK_BUS_SYSOSC_4M	MSPM0_CLOCK(0x3, 0x0)
#define MSPM0_CLOCK_BUS_HFCLK		MSPM0_CLOCK(0x4, 0x0)
#define MSPM0_CLOCK_BUS_SYSPLL0		MSPM0_CLOCK(0x5, 0x0)
#define MSPM0_CLOCK_BUS_SYSPLL1		MSPM0_CLOCK(0x6, 0x0)
#define MSPM0_CLOCK_BUS_SYSPLL2X	MSPM0_CLOCK(0x7, 0x0)
#define MSPM0_CLOCK_BUS_LFCLK		MSPM0_CLOCK(0x8, 0x2)
#define MSPM0_CLOCK_BUS_MFCLK		MSPM0_CLOCK(0x9, 0x4)
#define MSPM0_CLOCK_BUS_BUSCLK		MSPM0_CLOCK(0xA, 0x8)
#define MSPM0_CLOCK_BUS_ULPCLK		MSPM0_CLOCK(0xB, 0x8)
#define MSPM0_CLOCK_BUS_MCLK		MSPM0_CLOCK(0xC, 0x8)
#define MSPM0_CLOCK_BUS_MFPCLK		MSPM0_CLOCK(0xD, 0x0)
#define MSPM0_CLOCK_BUS_CANCLK		MSPM0_CLOCK(0xE, 0x0)
#define MSPM0_CLOCK_BUS_CLK_OUT		MSPM0_CLOCK(0xF, 0x0)

#endif
