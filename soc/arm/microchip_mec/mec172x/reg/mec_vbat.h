/*
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MEC_VBAT_H
#define _MEC_VBAT_H

#include <stdint.h>
#include <stddef.h>

#include "mec_defs.h"
#include "mec_regaccess.h"

/* VBAT Registers Registers */
#define MCHP_VBAT_REGISTERS_ADDR	0x4000A400ul
#define MCHP_VBAT_MEMORY_ADDR		0x4000A800ul
#define MCHP_VBAT_MEMORY_SIZE		128ul

/* Offset 0x00 Power-Fail and Reset Status */
#define MCHP_VBATR_PFRS_OFS		0ul
#define MCHP_VBATR_PFRS_MASK		0x7Cul
#define MCHP_VBATR_PFRS_SYS_RST_POS	2u
#define MCHP_VBATR_PFRS_JTAG_POS	3u
#define MCHP_VBATR_PFRS_RESETI_POS	4u
#define MCHP_VBATR_PFRS_WDT_POS		5u
#define MCHP_VBATR_PFRS_SYSRESETREQ_POS 6u
#define MCHP_VBATR_PFRS_VBAT_RST_POS	7u

#define MCHP_VBATR_PFRS_SYS_RST		BIT(2)
#define MCHP_VBATR_PFRS_JTAG		BIT(3)
#define MCHP_VBATR_PFRS_RESETI		BIT(4)
#define MCHP_VBATR_PFRS_WDT		BIT(5)
#define MCHP_VBATR_PFRS_SYSRESETREQ	BIT(6)
#define MCHP_VBATR_PFRS_VBAT_RST	BIT(7)

/* Offset 0x08 32K Clock Source register */
#define MCHP_VBATR_CS_OFS		0x08ul
#define MCHP_VBATR_CS_MASK		0x71f1ul
#define MCHP_VBATR_CS_SO_EN_POS 0
#define MCHP_VBATR_CS_XTAL_EN_POS	8
#define MCHP_VBATR_CS_XTAL_SEL_POS	9
#define MCHP_VBATR_CS_XTAL_DHC_POS	10
#define MCHP_VBATR_CS_XTAL_CNTR_POS	11
#define MCHP_VBATR_CS_PCS_POS		16
#define MCHP_VBATR_CS_DI32_VTR_OFF_POS	18

/* Enable and start internal 32KHz Silicon Oscillator */
#define MCHP_VBATR_CS_SO_EN		BIT(0)
/* Enable and start the external crystal */
#define MCHP_VBATR_CS_XTAL_EN		BIT(8)
/* single ended crystal on XTAL2 instead of parallel across XTAL1 and XTAL2 */
#define MCHP_VBATR_CS_XTAL_SE		BIT(9)
/* disable XTAL high startup current */
#define MCHP_VBATR_CS_XTAL_DHC		BIT(10)
/* crystal amplifier gain control */
#define MCHP_VBATR_CS_XTAL_CNTR_MSK	0x1800u
#define MCHP_VBATR_CS_XTAL_CNTR_DG	0x0800u
#define MCHP_VBATR_CS_XTAL_CNTR_RG	0x1000u
#define MCHP_VBATR_CS_XTAL_CNTR_MG	0x1800u
/* Select source of peripheral 32KHz clock */
#define MCHP_VBATR_CS_PCS_MSK		0x30000u
/* 32K silicon OSC when chip powered by VBAT or VTR */
#define MCHP_VBATR_CS_PCS_VTR_VBAT_SO	0x00000u
/* 32K external crystal when chip powered by VBAT or VTR */
#define MCHP_VBATR_CS_PCS_VTR_VBAT_XTAL 0x10000u
/* 32K input pin on VTR. Switch to Silicon OSC on VBAT */
#define MCHP_VBATR_CS_PCS_VTR_PIN_SO	0x20000u
/* 32K input pin on VTR. Switch to crystal on VBAT */
#define MCHP_VBATR_CS_PCS_VTR_PIN_XTAL	0x30000u
/* Disable internal 32K VBAT clock source when VTR is off */
#define MCHP_VBATR_CS_DI32_VTR_OFF	BIT(18)

/*
 * Monotonic Counter least significant word (32-bit), read-only.
 * Increments by one on read.
 */
#define MCHP_VBATR_MCNT_LSW_OFS		0x20ul

/* Monotonic Counter most significant word (32-bit). Read-Write */
#define MCHP_VBATR_MCNT_MSW_OFS		0x24ul

/* ROM Feature register */
#define MCHP_VBATR_ROM_FEAT_OFS		0x28ul

/* Embedded Reset Debounce Enable register */
#define MCHP_VBATR_EMBRD_EN_OFS		0x34ul
#define MCHP_VBATR_EMBRD_EN		BIT(0)

/* Register access */
#define MCHP_VBATR_PFRS() \
	REG8(MCHP_VBAT_REGISTERS_ADDR + MCHP_VBATR_PFRS_OFS)

#define MCHP_VBATR_CS()	\
	REG32(MCHP_VBAT_REGISTERS_ADDR + MCHP_VBATR_CS_OFS)

#define MCHP_VBATR_MCNT_LO() \
	REG32(MCHP_VBAT_REGISTERS_ADDR + MCHP_VBATR_MCNT_LSW_OFS)

#define MCHP_VBATR_MCNT_HI() \
	REG32(MCHP_VBAT_REGISTERS_ADDR + MCHP_VBATR_MCNT_MSW_OFS)

#define MCHP_VBATR_EMBRD() \
	REG32(MCHP_VBAT_REGISTERS_ADDR + MCHP_VBATR_EMBRD_EN_OFS)

/* -------------- VBAT Powered Control Interface (VCI) ------------------ */
#define MCHP_VCI_BASE_ADDR	0x4000AE00ul

/* VCI interrupts */
#define MCHP_VCI_OVRD_GIRQ	21u
#define MCHP_VCI_IN0_GIRQ	21u
#define MCHP_VCI_IN1_GIRQ	21u
#define MCHP_VCI_IN2_GIRQ	21u
#define MCHP_VCI_IN3_GIRQ	21u
#define MCHP_VCI_IN4_GIRQ	21u

/* Bit position in GIRQ Source, Enable-Set/Clr, and Result registers */
#define MCHP_VCI_OVRD_GIRQ_POS	10u
#define MCHP_VCI_IN0_GIRQ_POS	11u
#define MCHP_VCI_IN1_GIRQ_POS	12u
#define MCHP_VCI_IN2_GIRQ_POS	13u
#define MCHP_VCI_IN3_GIRQ_POS	14u
#define MCHP_VCI_IN4_GIRQ_POS	15u

#define MCHP_VCI_OVRD_GIRQ_VAL	BIT(MCHP_VCI_OVRD_GIRQ_POS)
#define MCHP_VCI_IN0_GIRQ_VAL	BIT(MCHP_VCI_IN0_GIRQ_POS)
#define MCHP_VCI_IN1_GIRQ_VAL	BIT(MCHP_VCI_IN1_GIRQ_POS)
#define MCHP_VCI_IN2_GIRQ_VAL	BIT(MCHP_VCI_IN2_GIRQ_POS)
#define MCHP_VCI_IN3_GIRQ_VAL	BIT(MCHP_VCI_IN3_GIRQ_POS)
#define MCHP_VCI_IN4_GIRQ_VAL	BIT(MCHP_VCI_IN4_GIRQ_POS)

/* VCI GIRQ aggregated NVIC input */
#define MCHP_VCI_OVRD_NVIC_AGGR 13u
#define MCHP_VCI_IN0_NVIC_AGGR	13u
#define MCHP_VCI_IN1_NVIC_AGGR	13u
#define MCHP_VCI_IN2_NVIC_AGGR	13u
#define MCHP_VCI_IN3_NVIC_AGGR	13u
#define MCHP_VCI_IN4_NVIC_AGGR	13u

/* VCI direct NVIC inputs */
#define MCHP_VCI_OVRD_NVIC_DIRECT	121u
#define MCHP_VCI_IN0_NVIC_DIRECT	122u
#define MCHP_VCI_IN1_NVIC_DIRECT	123u
#define MCHP_VCI_IN2_NVIC_DIRECT	124u
#define MCHP_VCI_IN3_NVIC_DIRECT	125u
#define MCHP_VCI_IN4_NVIC_DIRECT	126u

/* VCI Config register */
#define MCHP_VCI_CFG_REG_OFS		0u
#define MCHP_VCI_CFG_REG_MASK		0x71F8Ful
#define MCHP_VCI_CFG_IN_MASK		0x7Fu
#define MCHP_VCI_CFG_IN0_HI		0x01u
#define MCHP_VCI_CFG_IN1_HI		0x02u
#define MCHP_VCI_CFG_IN2_HI		0x04u
#define MCHP_VCI_CFG_IN3_HI		0x08u
#define MCHP_VCI_CFG_IN4_HI		0x10u
#define MCHP_VCI_VCI_OVRD_IN_HI		BIT(8)
#define MCHP_VCI_VCI_OUT_HI		BIT(9)
#define MCHP_VCI_FW_CTRL_EN		BIT(10)
#define MCHP_VCI_FW_EXT_SEL		BIT(11)
#define MCHP_VCI_FILTER_BYPASS		BIT(12)
#define MCHP_VCI_WEEK_ALARM		BIT(16)
#define MCHP_VCI_RTC_ALARM		BIT(17)
#define MCHP_VCI_SYS_PWR_PRES		BIT(18)

/* VCI Latch Enable register */
/* VCI Latch Reset register */
#define MCHP_VCI_LE_REG_OFS		4u
#define MCHP_VCI_LR_REG_OFS		8u
#define MCHP_VCI_LER_REG_MASK		0x3007Ful
#define MCHP_VCI_LER_IN_MASK		0x7Fu
#define MCHP_VCI_LER_IN0		0x01u
#define MCHP_VCI_LER_IN1		0x02u
#define MCHP_VCI_LER_IN2		0x04u
#define MCHP_VCI_LER_IN3		0x08u
#define MCHP_VCI_LER_IN4		0x10u
#define MCHP_VCI_LER_WEEK_ALARM		BIT(16)
#define MCHP_VCI_LER_RTC_ALARM		BIT(17)

/* VCI Input Enable register */
#define MCHP_VCI_INPUT_EN_REG_OFS	0x0Cu
#define MCHP_VCI_INPUT_EN_REG_MASK	0x7Ful
#define MCHP_VCI_INPUT_EN_IE_MASK	0x7Fu
#define MCHP_VCI_INPUT_EN_IN0		0x01u
#define MCHP_VCI_INPUT_EN_IN1		0x02u
#define MCHP_VCI_INPUT_EN_IN2		0x04u
#define MCHP_VCI_INPUT_EN_IN3		0x08u
#define MCHP_VCI_INPUT_EN_IN4		0x10u

/* VCI Hold Off Count register */
#define MCHP_VCI_HDO_REG_OFS		0x10u
#define MCHP_VCI_HDO_REG_MASK		0xFFul

/* VCI Polarity register */
#define MCHP_VCI_POL_REG_OFS		0x14u
#define MCHP_VCI_POL_REG_MASK		0x7Ful
#define MCHP_VCI_POL_IE30_MASK		0x0Fu
#define MCHP_VCI_POL_ACT_HI_IN0		0x01u
#define MCHP_VCI_POL_ACT_HI_IN1		0x02u
#define MCHP_VCI_POL_ACT_HI_IN2		0x04u
#define MCHP_VCI_POL_ACT_HI_IN3		0x08u
#define MCHP_VCI_POL_ACT_HI_IN4		0x10u

/* VCI Positive Edge Detect register */
#define MCHP_VCI_PDET_REG_OFS		0x18u
#define MCHP_VCI_PDET_REG_MASK		0x7Ful
#define MCHP_VCI_PDET_IN0		0x01u
#define MCHP_VCI_PDET_IN1		0x02u
#define MCHP_VCI_PDET_IN2		0x04u
#define MCHP_VCI_PDET_IN3		0x08u
#define MCHP_VCI_PDET_IN4		0x10u

/* VCI Positive Edge Detect register */
#define MCHP_VCI_NDET_REG_OFS		0x1Cu
#define MCHP_VCI_NDET_REG_MASK		0x7Ful
#define MCHP_VCI_NDET_IN0		0x01u
#define MCHP_VCI_NDET_IN1		0x02u
#define MCHP_VCI_NDET_IN2		0x04u
#define MCHP_VCI_NDET_IN3		0x08u
#define MCHP_VCI_NDET_IN4		0x10u

/* VCI Buffer Enable register */
#define MCHP_VCI_BEN_REG_OFS		0x20u
#define MCHP_VCI_BEN_REG_MASK		0x7Ful
#define MCHP_VCI_BEN_IE30_MASK		0x0Fu
#define MCHP_VCI_BEN_IN0		0x01u
#define MCHP_VCI_BEN_IN1		0x02u
#define MCHP_VCI_BEN_IN2		0x04u
#define MCHP_VCI_BEN_IN3		0x08u
#define MCHP_VCI_BEN_IN4		0x10u

#endif /* #ifndef _MEC_VBAT_H */
