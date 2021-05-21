/*
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MEC_PROCHOT_H
#define _MEC_PROCHOT_H

#include <stdint.h>
#include <stddef.h>

#include "mec_defs.h"

#define MCHP_PROCHOT_BASE_ADDR		0x40003400ul

/* PROCHOT interrupt signal */
#define MCHP_PROCHOT_GIRQ		17u
/* Bit position in GIRQ Source, Enable-Set/Clr, and Result registers */
#define MCHP_PROCHOT_GIRQ_POS		17
#define MCHP_PROCHOT_GIRQ_VAL		BIT(MCHP_PROCHOT_GIRQ_POS)

/* PROCHOT GIRQ aggregated NVIC input */
#define MCHP_PROCHOT_NVIC_AGGR		9u
/* PROCHOT direct NVIC inputs */
#define MCHP_PROCHOT_NVIC_DIRECT	87u

/* Cumulative Count register */
#define MCHP_PROCHOT_CCNT_REG_OFS	0
#define MCHP_PROCHOT_CCNT_REG_MASK	0x00fffffful

/* Duty cycle count register */
#define MCHP_PROCHOT_DCCNT_REG_OFS	4
#define MCHP_PROCHOT_DCCNT_REG_MASK	0x00fffffful

/* Duty cycle period register */
#define MCHP_PROCHOT_DCPER_REG_OFS	8
#define MCHP_PROCHOT_DCPER_REG_MASK	0x00fffffful

/* Control and Status register */
#define MCHP_PROCHOT_CTRLS_REG_OFS		0x0C
#define MCHP_PROCHOT_CTRLS_REG_MASK		0x00000C3Ful
#define MCHP_PROCHOT_CTRLS_EN_POS		0
#define MCHP_PROCHOT_CTRLS_EN			BIT(MCHP_PROCHOT_CTRLS_EN_POS)
#define MCHP_PROCHOT_CTRLS_PIN_POS		1
#define MCHP_PROCHOT_CTRLS_PIN_HI		BIT(MCHP_PROCHOT_CTRLS_PIN_POS)
#define MCHP_PROCHOT_CTRLS_ASSERT_EN_POS	2
#define MCHP_PROCHOT_CTRLS_ASSERT_EN		\
	BIT(MCHP_PROCHOT_CTRLS_ASSERT_EN_POS)
#define MCHP_PROCHOT_CTRLS_PERIOD_EN_POS	3
#define MCHP_PROCHOT_CTRLS_PERIOD_EN		\
	BIT(MCHP_PROCHOT_CTRLS_PERIOD_EN_POS)
#define MCHP_PROCHOT_CTRLS_RESET_EN_POS		4
#define MCHP_PROCHOT_CTRLS_RESET_EN		\
	BIT(MCHP_PROCHOT_CTRLS_RESET_EN_POS)
#define MCHP_PROCHOT_CTRLS_FILT_EN_POS		5
#define MCHP_PROCHOT_CTRLS_FILT_EN		\
	BIT(MCHP_PROCHOT_CTRLS_FILT_EN_POS)

/* Assertion counter register */
#define MCHP_PROCHOT_ASSERT_CNT_REG_OFS		0x10
#define MCHP_PROCHOT_ASSERT_CNT_REG_MASK	0x0000fffful

/* Assertion counter limit register */
#define MCHP_PROCHOT_ASSERT_LIM_REG_OFS		0x14
#define MCHP_PROCHOT_ASSERT_LIM_REG_MASK	0x0000fffful

#endif	/* #ifndef _MEC_PROCHOT_H */
