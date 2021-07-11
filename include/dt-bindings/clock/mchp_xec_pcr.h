/*
 * Copyright (c) 2021 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_MCHP_XEC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_MCHP_XEC_H_

/* PLL and Peripheral 32 KHz clock source */
#define MCHP_XEC_CLK32K_SRC_SIL_OSC	0U
#define MCHP_XEC_CLK32K_SRC_XTAL	1U
#define MCHP_XEC_CLK32K_SRC_PIN		2U

/* Crystal connection */
#define MCHP_XEC_XTAL_PARALLEL		0U
#define MCHP_XEC_XTAL_SINGLE_ENDED	1U

/*
 * When the 32KHz pin goes down fall back
 * to either internal silicon oscillator
 * or crystal.
 */
#define MCHP_XEC_PIN32K_FB_SIL_OSC	0U
#define MCHP_XEC_PIN32K_FB_XTAL		1U

/* clocks supported by the driver */
#define MCHP_XEC_PCR_CLK_CORE		0
#define MCHP_XEC_PCR_CLK_CPU		1
#define MCHP_XEC_PCR_CLK_BUS		2
#define MCHP_XEC_PCR_CLK_PERIPH		3
#define MCHP_XEC_PCR_CLK_PERIPH_FAST	4
#define MCHP_XEC_PCR_CLK_PERIPH_SLOW	5

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_MCHP_XEC_H_ */
