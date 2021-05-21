/*
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MEC_RPMFAN_H
#define _MEC_RPMFAN_H

#include <stdint.h>
#include <stddef.h>

#include "mec_defs.h"

#define MCHP_RPMFAN_BASE_ADDR		0x4000A000ul
#define MCHP_RPMFAN_INST_SPACING	0x80ul
#define MCHP_RPMFAN_ADDR(n) \
	(MCHP_RPMFAN_BASE_ADDR + ((n) * MCHP_RPMFAN_INST_SPACING))

/* RPMFAN has two interrupt signals: Fail and Stall */
#define MCHP_RPMFAN_0_GIRQ		17u
#define MCHP_RPMFAN_1_GIRQ		17u
#define MCHP_RPMFAN_0_GIRQ_NVIC		9u
#define MCHP_RPMFAN_1_GIRQ_NVIC		9u

#define MCHP_RPMFAN_0_FAIL_GIRQ_POS	20u
#define MCHP_RPMFAN_0_STALL_GIRQ_POS	21u
#define MCHP_RPMFAN_1_FAIL_GIRQ_POS	22u
#define MCHP_RPMFAN_1_STALL_GIRQ_POS	23u

#define MCHP_RPMFAN_0_FAIL_GIRQ		BIT(20)
#define MCHP_RPMFAN_0_STALL_GIRQ	BIT(21)
#define MCHP_RPMFAN_1_FAIL_GIRQ		BIT(22)
#define MCHP_RPMFAN_1_STALL_GIRQ	BIT(23)

#define MCHP_RPMFAN_0_FAIL_NVIC		74u
#define MCHP_RPMFAN_0_STALL_NVIC	75u
#define MCHP_RPMFAN_1_FAIL_NVIC		76u
#define MCHP_RPMFAN_1_STALL_NVIC	77u

/* Fan Setting register 16-bit */
#define MCHP_RPMFAN_FS_OFS	0x00u
#define MCHP_RPMFAN_FS_MSK	0xFFC0u
#define MCHP_RPMFAN_FSO_POS	6

/* Fan Configuration register 16-bit */
#define MCHP_RPMFAN_FC_OFS		0x02u
#define MCHP_RPMFAN_FC_MSK		0xFEFFu
#define MCHP_RPMFAN_FC_DFLT		0x1C3Du
#define MCHP_RPMFAN_FC_UPD_POS		0
#define MCHP_RPMFAN_FC_UPD_MSK		0x07u
#define MCHP_RPMFAN_FC_UPD_100MS	0u
#define MCHP_RPMFAN_FC_UPD_200MS	0x01u
#define MCHP_RPMFAN_FC_UPD_300MS	0x02u
#define MCHP_RPMFAN_FC_UPD_400MS	0x03u
#define MCHP_RPMFAN_FC_UPD_500MS	0x04u
#define MCHP_RPMFAN_FC_UPD_800MS	0x05u
#define MCHP_RPMFAN_FC_UPD_1200MS	0x06u
#define MCHP_RPMFAN_FC_UPD_1600MS	0x07u
/* number of edges per rotation on TACH input */
#define MCHP_RPMFAN_FC_NE_POS		3
#define MCHP_RPMFAN_FC_NE_MSK		0x18u
#define MCHP_RPMFAN_FC_NE_MSK		0x18u
#define MCHP_RPMFAN_FC_NE_3		0u
#define MCHP_RPMFAN_FC_NE_4		0x08u
#define MCHP_RPMFAN_FC_NE_7		0x10u
#define MCHP_RPMFAN_FC_NE_9		0x18u
/* weight the TACH values */
#define MCHP_RPMFAN_RNG_POS		5
#define MCHP_RPMFAN_RNG_MSK		0x60u
#define MCHP_RPMFAN_RNG_500_1X		0u
#define MCHP_RPMFAN_RNG_1000_2X		0x20u
#define MCHP_RPMFAN_RNG_2000_4X		0x40u
#define MCHP_RPMFAN_RNG_4000_8X		0x60u
/* enable selected HW algorithm */
#define MCHP_RPMFAN_ALGO_EN		BIT(7)
/* PWM driver polarity */
#define MCHP_RPMFAN_PWM_INV		BIT(9)
/* HW algorithm error window adjust */
#define MCHP_RPMFAN_ERR_RNG_POS		10
#define MCHP_RPMFAN_ERR_RNG_MSK		0xC00u
#define MCHP_RPMFAN_ERR_RNG_0		0x00u
#define MCHP_RPMFAN_ERR_RNG_50		0x400u
#define MCHP_RPMFAN_ERR_RNG_100		0x800u
#define MCHP_RPMFAN_ERR_RNG_200		0xC00u
/* HW algorithm derivative options */
#define MCHP_RPMFAN_DER_OPT_POS		12
#define MCHP_RPMFAN_DER_OPT_MSK		0x3000u
#define MCHP_RPMFAN_DER_OPT_NONE	0x00u
#define MCHP_RPMFAN_DER_OPT_BASIC	0x1000u
#define MCHP_RPMFAN_DER_OPT_STEP	0x2000u
#define MCHP_RPMFAN_DER_OPT_BOTH	0x3000u
/* disable glitch filter on TACH input pin */
#define MCHP_RPMFAN_DIS_GLITCH		BIT(14)
/* enable ramp rate control in manual mode operation */
#define MCHP_RPMFAN_EN_RRC		BIT(15)

/* PWM Divider register 8-bit */
#define MCHP_RPMFAN_PWM_DIV_OFS		0x04u
#define MCHP_RPMFAN_PWM_DIV_MSK		0xffu
#define MCHP_RPMFAN_PWM_DIV_DFLT	0x01u

/* Gain register 8-bit */
#define MCHP_RPMFAN_GAIN_OFS		0x05u
#define MCHP_RPMFAN_GAIN_MSK		0x3Fu
#define MCHP_RPMFAN_GAIN_DFLT		0x2Au
#define MCHP_RPMFAN_GAINP_POS		0
#define MCHP_RPMFAN_GAINP_MSK		0x03u
#define MCHP_RPMFAN_GAINP_1X		0x00u
#define MCHP_RPMFAN_GAINP_2X		0x01u
#define MCHP_RPMFAN_GAINP_4X		0x02u
#define MCHP_RPMFAN_GAINP_8X		0x03u
#define MCHP_RPMFAN_GAINI_POS		2
#define MCHP_RPMFAN_GAINI_MSK		0x0Cu
#define MCHP_RPMFAN_GAINI_1X		0x00u
#define MCHP_RPMFAN_GAINI_2X		0x04u
#define MCHP_RPMFAN_GAINI_4X		0x08u
#define MCHP_RPMFAN_GAINI_8X		0x0Cu
#define MCHP_RPMFAN_GAIND_POS		4
#define MCHP_RPMFAN_GAIND_MSK		0x30u
#define MCHP_RPMFAN_GAIND_1X		0x00u
#define MCHP_RPMFAN_GAIND_2X		0x10u
#define MCHP_RPMFAN_GAIND_4X		0x20u
#define MCHP_RPMFAN_GAIND_8X		0x30u

/* Fan spin up register 8-bit */
#define MCHP_RPMFAN_SU_OFS		0x06u
#define MCHP_RPMFAN_SU_MSK		0xffu
#define MCHP_RPMFAN_SU_DFLT		0x19u
#define MCHP_RPMFAN_SU_TIME_POS		0
#define MCHP_RPMFAN_SU_TIME_MSK		0x03u
#define MCHP_RPMFAN_SU_TIME_250MS	0x00u
#define MCHP_RPMFAN_SU_TIME_500MS	0x01u
#define MCHP_RPMFAN_SU_TIME_1S		0x02u
#define MCHP_RPMFAN_SU_TIME_2S		0x03u
#define MCHP_RPMFAN_SU_LVL_POS		2
#define MCHP_RPMFAN_SU_LVL_MSK		0x1Cu
#define MCHP_RPMFAN_SU_LVL_30P		0x00u
#define MCHP_RPMFAN_SU_LVL_35P		0x04u
#define MCHP_RPMFAN_SU_LVL_40P		0x08u
#define MCHP_RPMFAN_SU_LVL_45P		0x0Cu
#define MCHP_RPMFAN_SU_LVL_50P		0x10u
#define MCHP_RPMFAN_SU_LVL_55P		0x14u
#define MCHP_RPMFAN_SU_LVL_60P		0x18u
#define MCHP_RPMFAN_SU_LVL_66P		0x1Cu
#define MCHP_RPMFAN_SU_NOKICK		BIT(5)
#define MCHP_RPMFAN_SU_DFC_POS		6
#define MCHP_RPMFAN_SU_DFC_MSK		0xC0u
#define MCHP_RPMFAN_SU_DFC_DIS		0x00u
#define MCHP_RPMFAN_SU_DFC_16		0x40u
#define MCHP_RPMFAN_SU_DFC_32		0x80u
#define MCHP_RPMFAN_SU_DFC_64		0xC0u

/* Fan step register 8-bit */
#define MCHP_RPMFAN_FSTEP_OFS		0x07u
#define MCHP_RPMFAN_FSTEP_MSK		0xffu
#define MCHP_RPMFAN_FSTEP_DFLT		0x10u

/* Fan minimum driver register 8-bit */
#define MCHP_RPMFAN_MDRV_OFS		0x08u
#define MCHP_RPMFAN_MDRV_MSK		0xffu
#define MCHP_RPMFAN_MDRV_DFLT		0x66u

/* Valid Tach Count register 8-bit */
#define MCHP_RPMFAN_VTC_OFS		0x09u
#define MCHP_RPMFAN_VTC_MSK		0xffu
#define MCHP_RPMFAN_VTC_DFLT		0xF5u

/* Fan drive fail band register 16-bit */
#define MCHP_RPMFAN_FDFB_OFS		0x0Au
#define MCHP_RPMFAN_FDFB_MSK		0xFFF8u
#define MCHP_RPMFAN_FDFB_SHIFT		3

/* Tach target register 16-bit */
#define MCHP_RPMFAN_TT_OFS		0x0Cu
#define MCHP_RPMFAN_TT_MSK		0xfff8u
#define MCHP_RPMFAN_TT_SHIFT		3

/* Tach reading register 16-bit */
#define MCHP_RPMFAN_TR_OFS		0x0Eu
#define MCHP_RPMFAN_TR_MSK		0xfff8u
#define MCHP_RPMFAN_TR_SHIFT		3

/* PWM driver base frequency register 8-bit */
#define MCHP_RPMFAN_PBF_OFS		0x10u
#define MCHP_RPMFAN_PBF_MSK		0x03u
#define MCHP_RPMFAN_PBF_26K		0x00u
#define MCHP_RPMFAN_PBF_23K		0x01u
#define MCHP_RPMFAN_PBF_4K		0x02u
#define MCHP_RPMFAN_PBF_2K		0x03u

/* Fan status register 8-bit */
#define MCHP_RPMFAN_FSTS_OFS		0x11u
#define MCHP_RPMFAN_FSTS_MSK		0x23u
#define MCHP_RPMFAN_FSTS_STALL		BIT(0)
#define MCHP_RPMFAN_FSTS_SU_FAIL	BIT(1)
#define MCHP_RPMFAN_FSTS_DRV_FAIL	BIT(5)

#endif	/* #ifndef _MEC_RPMFAN_H */
