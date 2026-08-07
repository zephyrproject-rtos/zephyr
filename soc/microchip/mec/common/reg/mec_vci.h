/*
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MEC_VCI_H
#define _MEC_VCI_H

#include <stdint.h>
#include <stddef.h>

/* VCI Config register */
#define MCHP_VCI_CFG_REG_OFS		0u
#define MCHP_VCI_CFG_REG_MASK		0x71f8fu
#define MCHP_VCI_CFG_IN_MASK		0x7fu
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
#define MCHP_VCI_LER_REG_MASK		0x3007fu
#define MCHP_VCI_LER_IN_MASK		0x7fu
#define MCHP_VCI_LER_IN0		0x01u
#define MCHP_VCI_LER_IN1		0x02u
#define MCHP_VCI_LER_IN2		0x04u
#define MCHP_VCI_LER_IN3		0x08u
#define MCHP_VCI_LER_IN4		0x10u
#define MCHP_VCI_LER_WEEK_ALARM		BIT(16)
#define MCHP_VCI_LER_RTC_ALARM		BIT(17)

/* VCI Input Enable register */
#define MCHP_VCI_INPUT_EN_REG_OFS	0x0cu
#define MCHP_VCI_INPUT_EN_REG_MASK	0x7fu
#define MCHP_VCI_INPUT_EN_IE_MASK	0x7fu
#define MCHP_VCI_INPUT_EN_IN0		0x01u
#define MCHP_VCI_INPUT_EN_IN1		0x02u
#define MCHP_VCI_INPUT_EN_IN2		0x04u
#define MCHP_VCI_INPUT_EN_IN3		0x08u
#define MCHP_VCI_INPUT_EN_IN4		0x10u

/* VCI Hold Off Count register */
#define MCHP_VCI_HDO_REG_OFS		0x10u
#define MCHP_VCI_HDO_REG_MASK		0xffu

/* VCI Polarity register */
#define MCHP_VCI_POL_REG_OFS		0x14u
#define MCHP_VCI_POL_REG_MASK		0x7fu
#define MCHP_VCI_POL_IE30_MASK		0x0Fu
#define MCHP_VCI_POL_ACT_HI_IN0		0x01u
#define MCHP_VCI_POL_ACT_HI_IN1		0x02u
#define MCHP_VCI_POL_ACT_HI_IN2		0x04u
#define MCHP_VCI_POL_ACT_HI_IN3		0x08u
#define MCHP_VCI_POL_ACT_HI_IN4		0x10u

/* VCI Positive Edge Detect register */
#define MCHP_VCI_PDET_REG_OFS		0x18u
#define MCHP_VCI_PDET_REG_MASK		0x7fu
#define MCHP_VCI_PDET_IN0		0x01u
#define MCHP_VCI_PDET_IN1		0x02u
#define MCHP_VCI_PDET_IN2		0x04u
#define MCHP_VCI_PDET_IN3		0x08u
#define MCHP_VCI_PDET_IN4		0x10u

/* VCI Positive Edge Detect register */
#define MCHP_VCI_NDET_REG_OFS		0x1cu
#define MCHP_VCI_NDET_REG_MASK		0x7fu
#define MCHP_VCI_NDET_IN0		0x01u
#define MCHP_VCI_NDET_IN1		0x02u
#define MCHP_VCI_NDET_IN2		0x04u
#define MCHP_VCI_NDET_IN3		0x08u
#define MCHP_VCI_NDET_IN4		0x10u

/* VCI Buffer Enable register */
#define MCHP_VCI_BEN_REG_OFS		0x20u
#define MCHP_VCI_BEN_REG_MASK		0x7fu
#define MCHP_VCI_BEN_IE30_MASK		0x0fu
#define MCHP_VCI_BEN_IN0		0x01u
#define MCHP_VCI_BEN_IN1		0x02u
#define MCHP_VCI_BEN_IN2		0x04u
#define MCHP_VCI_BEN_IN3		0x08u
#define MCHP_VCI_BEN_IN4		0x10u

/** @brief VBAT powered control interface (VCI) */
struct vci_regs {
	volatile uint32_t CONFIG;
	volatile uint32_t LATCH_EN;
	volatile uint32_t LATCH_RST;
	volatile uint32_t INPUT_EN;
	volatile uint32_t HOLD_OFF;
	volatile uint32_t POLARITY;
	volatile uint32_t PEDGE_DET;
	volatile uint32_t NEDGE_DET;
	volatile uint32_t BUFFER_EN;
};

#endif /* #ifndef _MEC_VCI_H */
