/*
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MEC_ADC_H
#define _MEC_ADC_H

#include <stdint.h>
#include <stddef.h>

/* Eight ADC channels numbered 0 - 7 */
#define MCHP_ADC_MAX_CHAN		8u
#define MCHP_ADC_MAX_CHAN_MASK		0x07u

/* Control register */
#define MCHP_ADC_CTRL_REG_OFS		0u
#define MCHP_ADC_CTRL_REG_MASK		0xdfu
#define MCHP_ADC_CTRL_REG_RW_MASK	0x1fu
#define MCHP_ADC_CTRL_REG_RW1C_MASK	0xc0u
#define MCHP_ADC_CTRL_ACTV		BIT(0)
#define MCHP_ADC_CTRL_START_SNGL	BIT(1)
#define MCHP_ADC_CTRL_START_RPT		BIT(2)
#define MCHP_ADC_CTRL_PWRSV_DIS		BIT(3)
#define MCHP_ADC_CTRL_SRST		BIT(4)
#define MCHP_ADC_CTRL_RPT_DONE_STS	BIT(6) /* R/W1C */
#define MCHP_ADC_CTRL_SNGL_DONE_STS	BIT(7) /* R/W1C */

/* Delay register. Start and repeat delays in units of 40 us */
#define MCHP_ADC_DELAY_REG_OFS		4u
#define MCHP_ADC_DELAY_REG_MASK		0xffffffffu
#define MCHP_ADC_DELAY_START_POS	0u
#define MCHP_ADC_DELAY_START_MASK	0xffffu
#define MCHP_ADC_DELAY_RPT_POS		16u
#define MCHP_ADC_DELAY_RPT_MASK		0xffff0000u

/* Status register. 0 <= n < MCHP_ADC_MAX_CHAN */
#define MCHP_ADC_STATUS_REG_OFS		8u
#define MCHP_ADC_STATUS_REG_MASK	0xffffu
#define MCHP_ADC_STATUS_CHAN(n)		BIT(n)

/* Single Conversion Select register */
#define MCHP_ADC_SCS_REG_OFS		0x0cu
#define MCHP_ADC_SCS_REG_MASK		0xffu
#define MCHP_ADC_SCS_CH_0_7		0xffu
#define MCHP_ADC_SCS_CH(n)		BIT(((n) & 0x07u))

/* Repeat Conversion Select register */
#define MCHP_ADC_RCS_REG_OFS		0x10u
#define MCHP_ADC_RCS_REG_MASK		0xffu
#define MCHP_ADC_RCS_CH_0_7		0xffu
#define MCHP_ADC_RCS_CH(n)		BIT(((n) & 0x07u))

/* Channel reading registers */
#define MCHP_ADC_RDCH_REG_MASK		0xfffu
#define MCHP_ADC_RDCH0_REG_OFS		0x14u
#define MCHP_ADC_RDCH1_REG_OFS		0x18u
#define MCHP_ADC_RDCH2_REG_OFS		0x1cu
#define MCHP_ADC_RDCH3_REG_OFS		0x20u
#define MCHP_ADC_RDCH4_REG_OFS		0x24u
#define MCHP_ADC_RDCH5_REG_OFS		0x28u
#define MCHP_ADC_RDCH6_REG_OFS		0x2cu
#define MCHP_ADC_RDCH7_REG_OFS		0x30u

/* Configuration register */
#define MCHP_ADC_CFG_REG_OFS		0x7cu
#define MCHP_ADC_CFG_REG_MASK		0xffffu
#define MCHP_ADC_CFG_CLK_LO_TIME_POS	0
#define MCHP_ADC_CFG_CLK_LO_TIME_MASK0	0xffu
#define MCHP_ADC_CFG_CLK_LO_TIME_MASK	0xffu
#define MCHP_ADC_CFG_CLK_HI_TIME_POS	8
#define MCHP_ADC_CFG_CLK_HI_TIME_MASK0	0xffu
#define MCHP_ADC_CFG_CLK_HI_TIME_MASK	SHLU32(0xffu, 8)

/* Channel Vref Select register */
#define MCHP_ADC_CH_VREF_SEL_REG_OFS	0x80u
#define MCHP_ADC_CH_VREF_SEL_REG_MASK	0x00ffffffu
#define MCHP_ADC_CH_VREF_SEL_MASK(n)	SHLU32(0x03u, (((n) & 0x07) * 2u))
#define MCHP_ADC_CH_VREF_SEL_PAD(n)	0u
#define MCHP_ADC_CH_VREF_SEL_GPIO(n)	SHLU32(0x01u, (((n) & 0x07) * 2u))

/* Vref Control register */
#define MCHP_ADC_VREF_CTRL_REG_OFS		0x84u
#define MCHP_ADC_VREF_CTRL_REG_MASK		0xffffffffu
#define MCHP_ADC_VREF_CTRL_CHRG_DEL_POS		0
#define MCHP_ADC_VREF_CTRL_CHRG_DEL_MASK0	0xffffu
#define MCHP_ADC_VREF_CTRL_CHRG_DEL_MASK	0xffffu
#define MCHP_ADC_VREF_CTRL_SW_DEL_POS		16
#define MCHP_ADC_VREF_CTRL_SW_DEL_MASK0		0x1fffu
#define MCHP_ADC_VREF_CTRL_SW_DEL_MASK		SHLU32(0x1fffu, 16)
#define MCHP_ADC_VREF_CTRL_PAD_POS		29
#define MCHP_ADC_VREF_CTRL_PAD_UNUSED_FLOAT	0u
#define MCHP_ADC_VREF_CTRL_PAD_UNUSED_DRIVE_LO	BIT(29)
#define MCHP_ADC_VREF_CTRL_SEL_STS_POS		30
#define MCHP_ADC_VREF_CTRL_SEL_STS_MASK0	0x03u
#define MCHP_ADC_VREF_CTRL_SEL_STS_MASK		SHLU32(3u, 30)

/* SAR ADC Control register */
#define MCHP_ADC_SAR_CTRL_REG_OFS	0x88u
#define MCHP_ADC_SAR_CTRL_REG_MASK	0x0001ff8fu
/* Select single ended or differential operation */
#define MCHP_ADC_SAR_CTRL_SELDIFF_POS	0
#define MCHP_ADC_SAR_CTRL_SELDIFF_DIS	0u
#define MCHP_ADC_SAR_CTRL_SELDIFF_EN	BIT(0)
/* Select resolution */
#define MCHP_ADC_SAR_CTRL_RES_POS	1
#define MCHP_ADC_SAR_CTRL_RES_MASK0	0x03u
#define MCHP_ADC_SAR_CTRL_RES_MASK	0x06u
#define MCHP_ADC_SAR_CTRL_RES_10_BITS	0x04u
#define MCHP_ADC_SAR_CTRL_RES_12_BITS	0x06u
/* Shift data in reading register */
#define MCHP_ADC_SAR_CTRL_SHIFTD_POS	3
#define MCHP_ADC_SAR_CTRL_SHIFTD_DIS	0u
#define MCHP_ADC_SAR_CTRL_SHIFTD_EN	BIT(3)
/* Warm up delay in ADC clock cycles */
#define MCHP_ADC_SAR_CTRL_WUP_DLY_POS	7
#define MCHP_ADC_SAR_CTRL_WUP_DLY_MASK0 0x3ffu
#define MCHP_ADC_SAR_CTRL_WUP_DLY_MASK	SHLU32(0x3ffu, 7)
#define MCHP_ADC_SAR_CTRL_WUP_DLY_DFLT	SHLU32(0x202u, 7)

/* Register interface */
#define MCHP_ADC_CH_NUM(n)	((n) & MCHP_ADC_MAX_CHAN_MASK)
#define MCHP_ADC_CH_OFS(n)	(MCHP_ADC_CH_NUM(n) * 4u)
#define MCHP_ADC_CH_ADDR(n)	(MCHP_ADC_BASE_ADDR + MCHP_ADC_CH_OFS(n))

/** @brief Analog to Digital Converter Registers (ADC) */
struct adc_regs {
	volatile uint32_t CONTROL;
	volatile uint32_t DELAY;
	volatile uint32_t STATUS;
	volatile uint32_t SINGLE;
	volatile uint32_t REPEAT;
	volatile uint32_t RD[8];
	uint8_t RSVD1[0x7c - 0x34];
	volatile uint32_t CONFIG;
	volatile uint32_t VREF_CHAN_SEL;
	volatile uint32_t VREF_CTRL;
	volatile uint32_t SARADC_CTRL;
};

#endif	/* #ifndef _MEC_ADC_H */
