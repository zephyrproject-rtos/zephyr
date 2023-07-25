/*
 * Copyright (c) 2021 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_MCHP_XEC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_MCHP_XEC_H_

/* PLL 32KHz clock source VTR rail ON. */
#define MCHP_XEC_PLL_CLK32K_SRC_SIL_OSC		0U
#define MCHP_XEC_PLL_CLK32K_SRC_XTAL		1U
#define MCHP_XEC_PLL_CLK32K_SRC_PIN		2U

/* Peripheral 32KHz clock source for VTR rail ON and off(VBAT operation) */
#define MCHP_XEC_PERIPH_CLK32K_SRC_SO_SO	0U
#define MCHP_XEC_PERIPH_CLK32K_SRC_XTAL_XTAL	1U
#define MCHP_XEC_PERIPH_CLK32K_SRC_PIN_SO	2U
#define MCHP_XEC_PERIPH_CLK32K_SRC_PIN_XTAL	3U

/* clocks supported by the driver */
#define MCHP_XEC_PCR_CLK_CORE			0
#define MCHP_XEC_PCR_CLK_CPU			1
#define MCHP_XEC_PCR_CLK_BUS			2
#define MCHP_XEC_PCR_CLK_PERIPH			3
#define MCHP_XEC_PCR_CLK_PERIPH_FAST		4
#define MCHP_XEC_PCR_CLK_PERIPH_SLOW		5

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_MCHP_XEC_H_ */
