/*
 * Copyright (c) 2025 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_BFLB_BL70X_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_BFLB_BL70X_CLOCK_H_

#include "bflb_clock_common.h"

/** Root Clock */
#define BL70X_CLKID_CLK_ROOT    BFLB_CLKID_CLK_ROOT
/** 32MHz RC Oscillator Clock */
#define BL70X_CLKID_CLK_RC32M   BFLB_CLKID_CLK_RC32M
/** Crystal as clock */
#define BL70X_CLKID_CLK_CRYSTAL BFLB_CLKID_CLK_CRYSTAL
/** Bus Clock */
#define BL70X_CLKID_CLK_BCLK    BFLB_CLKID_CLK_BCLK
/** F32K Clock */
#define BL70X_CLKID_CLK_F32K	BFLB_CLKID_CLK_F32K
/** XTAL32K Clock */
#define BL70X_CLKID_CLK_XTAL32K	BFLB_CLKID_CLK_XTAL32K
/** RC32K Clock */
#define BL70X_CLKID_CLK_RC32K	BFLB_CLKID_CLK_RC32K
/** DLL clock, the standard root frequency of the DLL is 288MHz */
#define BL70X_CLKID_CLK_DLL     BFLB_CLKID_CLK_PRIVATE

/** ID 0, DLL 57MHz output  */
#define BL70X_DLL_57MHz		0
/** ID 1, DLL 96MHz output  */
#define BL70X_DLL_96MHz		1
/** ID 2, DLL 144MHz output */
#define BL70X_DLL_144MHz	2
/** ID 3, DLL 120MHz output, Invalid */
#define BL70X_DLL_120MHz	3

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_BFLB_BL70X_CLOCK_H_ */
