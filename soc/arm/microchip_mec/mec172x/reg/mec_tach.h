/*
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MEC_TACH_H
#define _MEC_TACH_H

#include <stdint.h>
#include <stddef.h>

#include "mec_defs.h"

#define MCHP_TACH_BASE_ADDR		0x40006000ul

#define MCHP_TACH_INST_SPACING		0x10ul
#define MCHP_TACH_INST_SPACING_P2	4u

#define MCHP_TACH_ADDR(n)	(MCHP_TACH_BASE_ADDR + \
				 ((uint32_t)(n) * MCHP_TACH_INST_SPACING))

/* TACH interrupts */
#define MCHP_TACH0_GIRQ		17u
#define MCHP_TACH1_GIRQ		17u
#define MCHP_TACH2_GIRQ		17u
#define MCHP_TACH3_GIRQ		17u

/* Bit position in GIRQ Source, Enable-Set/Clr, and Result registers */
#define MCHP_TACH0_GIRQ_POS	1u
#define MCHP_TACH1_GIRQ_POS	2u
#define MCHP_TACH2_GIRQ_POS	3u
#define MCHP_TACH3_GIRQ_POS	4u

#define MCHP_TACH0_GIRQ_VAL	BIT(MCHP_TACH0_GIRQ_POS)
#define MCHP_TACH1_GIRQ_VAL	BIT(MCHP_TACH1_GIRQ_POS)
#define MCHP_TACH2_GIRQ_VAL	BIT(MCHP_TACH2_GIRQ_POS)
#define MCHP_TACH3_GIRQ_VAL	BIT(MCHP_TACH3_GIRQ_POS)

/* TACH GIRQ aggregated NVIC input */
#define MCHP_TACH0_NVIC_AGGR	9u
#define MCHP_TACH1_NVIC_AGGR	9u
#define MCHP_TACH2_NVIC_AGGR	9u
#define MCHP_TACH3_NVIC_AGGR	9u

/* TACH direct NVIC inputs */
#define MCHP_TACH0_NVIC_DIRECT	71u
#define MCHP_TACH1_NVIC_DIRECT	72u
#define MCHP_TACH2_NVIC_DIRECT	73u
#define MCHP_TACH3_NVIC_DIRECT	159u

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

#endif	/* #ifndef _MEC_TACH_H */
