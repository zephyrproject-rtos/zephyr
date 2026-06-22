/*
 * Copyright (c) The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file bflb_bl70xl_clock.h
 * @brief Clock IDs for the BL70XL SoC series
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_BFLB_BL70XL_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_BFLB_BL70XL_CLOCK_H_

#include "bflb_clock_common.h"

/** Root Clock */
#define BL70XL_CLKID_CLK_ROOT    BFLB_CLKID_CLK_ROOT
/** 32MHz RC Oscillator Clock */
#define BL70XL_CLKID_CLK_RC32M   BFLB_CLKID_CLK_RC32M
/** Crystal as clock */
#define BL70XL_CLKID_CLK_CRYSTAL BFLB_CLKID_CLK_CRYSTAL
/** Bus Clock */
#define BL70XL_CLKID_CLK_BCLK    BFLB_CLKID_CLK_BCLK
/** F32K Clock */
#define BL70XL_CLKID_CLK_F32K	BFLB_CLKID_CLK_F32K
/** XTAL32K Clock */
#define BL70XL_CLKID_CLK_XTAL32K	BFLB_CLKID_CLK_XTAL32K
/** RC32K Clock */
#define BL70XL_CLKID_CLK_RC32K	BFLB_CLKID_CLK_RC32K
/** DLL clock, the standard root frequency of the DLL is 128MHz */
#define BL70XL_CLKID_CLK_DLL     BFLB_CLKID_CLK_PRIVATE

/** ID 0, DLL 25.6MHz output  */
#define BL70XL_DLL_25P6MHZ	0
/** ID 1, DLL 42.67MHz output  */
#define BL70XL_DLL_42P67MHZ	1
/** ID 2, DLL 64MHz output */
#define BL70XL_DLL_64MHZ	2
/** ID 3, DLL 128MHz output */
#define BL70XL_DLL_128MHZ	3

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_BFLB_BL70XL_CLOCK_H_ */
