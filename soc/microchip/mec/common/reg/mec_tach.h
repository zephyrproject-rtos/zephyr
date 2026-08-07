/*
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MEC_TACH_H
#define _MEC_TACH_H

#include <stdint.h>
#include <stddef.h>

#define MCHP_TACH_INST_SPACING		0x10ul
#define MCHP_TACH_INST_SPACING_P2	4u

/* TACH Control register */
#define MCHP_TACH_CONTROL_REG_OFS		0U
#define MCHP_TACH_CONTROL_MASK			0xffffdd03U

/* Enable exceed high or low limit detection */
#define MCHP_TACH_CTRL_EXCEED_LIM_EN_POS	0
#define MCHP_TACH_CTRL_EXCEED_LIM_EN		BIT(0)

/* Enable TACH operation */
#define MCHP_TACH_CTRL_EN_POS		1
#define MCHP_TACH_CTRL_EN		BIT(MCHP_TACH_CTRL_EN_POS)

/* Enable input filter */
#define MCHP_TACH_CTRL_FILTER_EN_POS	8
#define MCHP_TACH_CTRL_FILTER_EN	BIT(MCHP_TACH_CTRL_FILTER_EN_POS)

/* Select read mode. Latch data on rising edge of selected trigger */
#define MCHP_TACH_CTRL_READ_MODE_SEL_POS	10
#define MCHP_TACH_CTRL_READ_MODE_INPUT		0U
#define MCHP_TACH_CTRL_READ_MODE_100K_CLOCK	BIT(10)

/* Select TACH edges for counter increment */
#define MCHP_TACH_CTRL_NUM_EDGES_POS	11
#define MCHP_TACH_CTRL_NUM_EDGES_MASK0	0x03U
#define MCHP_TACH_CTRL_NUM_EDGES_MASK	SHLU32(0x03U, 11)
#define MCHP_TACH_CTRL_EDGES_2		0U
#define MCHP_TACH_CTRL_EDGES_3		SHLU32(1u, 11)
#define MCHP_TACH_CTRL_EDGES_5		SHLU32(2u, 11)
#define MCHP_TACH_CTRL_EDGES_9		SHLU32(3u, 11)

/* Enable count ready interrupt */
#define MCHP_TACH_CTRL_CNT_RDY_INT_EN_POS	14
#define MCHP_TACH_CTRL_CNT_RDY_INT_EN		BIT(14)

/* Enable input toggle interrupt */
#define MCHP_TACH_CTRL_TOGGLE_INT_EN_POS	15
#define MCHP_TACH_CTRL_TOGGLE_INT_EN		BIT(15)

/* Read-only latched TACH pulse counter */
#define MCHP_TACH_CTRL_COUNTER_POS	16
#define MCHP_TACH_CTRL_COUNTER_MASK0	0xfffful
#define MCHP_TACH_CTRL_COUNTER_MASK	SHLU32(0xffffU, 16)

/*
 * TACH Status register
 * bits[0, 2-3] are R/W1C
 * bit[1] is Read-Only
 */
#define MCHP_TACH_STATUS_REG_OFS	4U
#define MCHP_TACH_STATUS_MASK		0x0FU
#define MCHP_TACH_STS_EXCEED_LIMIT_POS	0
#define MCHP_TACH_STS_EXCEED_LIMIT	BIT(MCHP_TACH_STS_EXCEED_LIMIT_POS)
#define MCHP_TACH_STS_PIN_STATE_POS	1
#define MCHP_TACH_STS_PIN_STATE_HI	BIT(MCHP_TACH_STS_PIN_STATE_POS)
#define MCHP_TACH_STS_TOGGLE_POS	2
#define MCHP_TACH_STS_TOGGLE		BIT(MCHP_TACH_STS_TOGGLE_POS)
#define MCHP_TACH_STS_CNT_RDY_POS	3
#define MCHP_TACH_STS_CNT_RDY		BIT(MCHP_TACH_STS_CNT_RDY_POS)

/* TACH High Limit Register */
#define MCHP_TACH_HI_LIMIT_REG_OFS	8U
#define MCHP_TACH_HI_LIMIT_MASK		0xffffU

/* TACH Low Limit Register */
#define MCHP_TACH_LO_LIMIT_REG_OFS	0x0CU
#define MCHP_TACH_LO_LIMIT_MASK		0xffffU

/** @brief Tachometer Registers (TACH) */
struct tach_regs {
	volatile uint32_t CONTROL;
	volatile uint32_t STATUS;
	volatile uint32_t LIMIT_HI;
	volatile uint32_t LIMIT_LO;
};

#endif	/* #ifndef _MEC_TACH_H */
