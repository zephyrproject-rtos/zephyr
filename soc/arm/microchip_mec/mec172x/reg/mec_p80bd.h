/*
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MEC_P80BD_H
#define _MEC_P80BD_H

#include <stdint.h>
#include <stddef.h>

#include "mec_defs.h"

#define MCHP_P80BD_0_BASE_ADDR		0x400F8000ul

#define MCHP_P80BD_0_GIRQ		15u
#define MCHP_P80BD_0_GIRQ_POS		22u
#define MCHP_P80BD_0_GIRQ_VAL		BIT(22)
#define MCHP_P80BD_0_GIRQ_NVIC		7u
#define MCHP_P80BD_0_NVIC		62u

/* HDATA - Write-Only 32-bit */
#define MCHP_P80BD_HDATA_OFS		0x00ul
#define MCHP_P80BD_HDATA_MASK		0xFFFFFFFFul

/*
 * EC-only Data/Attributes 16-bit
 * b[7:0]  = data byte from capture FIFO
 * b[15:8] = data attributes
 */
#define MCHP_P80BD_ECDA_OFS		0x100ul
#define MCHP_P80BD_ECDA_MASK		0x7FFFul
#define MCHP_P80BD_ECDA_DPOS		0
#define MCHP_P80BD_ECDA_APOS		8
#define MCHP_P80BD_ECDA_DMSK		0xFFul
#define MCHP_P80BD_ECDA_AMSK		0x7F00ul
#define MCHP_P80BD_ECDA_LANE_POS	8
#define MCHP_P80BD_ECDA_LANE_MSK	0x0300ul
#define MCHP_P80BD_ECDA_LANE_0		0x0000ul
#define MCHP_P80BD_ECDA_LANE_1		0x0100ul
#define MCHP_P80BD_ECDA_LANE_2		0x0200ul
#define MCHP_P80BD_ECDA_LANE_3		0x0300ul
#define MCHP_P80BD_ECDA_LEN_POS		10
#define MCHP_P80BD_ECDA_LEN_MSK		0x0C00ul
#define MCHP_P80BD_ECDA_LEN_1		0x0000ul
#define MCHP_P80BD_ECDA_LEN_2		0x0400ul
#define MCHP_P80BD_ECDA_LEN_4		0x0800ul
#define MCHP_P80BD_ECDA_LEN_INVAL	0x0C00ul
#define MCHP_P80BD_ECDA_NE		BIT(12)
#define MCHP_P80BD_ECDA_OVR		BIT(13)
#define MCHP_P80BD_ECDA_THR		BIT(14)

/* Configuration */
#define MCHP_P80BD_CFG_OFS		0x104ul
#define MCHP_P80BD_CFG_MASK		0x80000703ul
#define MCHP_P80BD_CFG_FLUSH_FIFO	BIT(0)	/* WO */
#define MCHP_P80BD_CFG_SNAP_CLR		BIT(1)	/* WO */
#define MCHP_P80BD_CFG_FIFO_THR_POS	8
#define MCHP_P80BD_CFG_FIFO_THR_MSK	0x700ul
#define MCHP_P80BD_CFG_FIFO_THR_1	0x000ul
#define MCHP_P80BD_CFG_FIFO_THR_4	0x100ul
#define MCHP_P80BD_CFG_FIFO_THR_8	0x200ul
#define MCHP_P80BD_CFG_FIFO_THR_16	0x300ul
#define MCHP_P80BD_CFG_FIFO_THR_20	0x400ul
#define MCHP_P80BD_CFG_FIFO_THR_24	0x500ul
#define MCHP_P80BD_CFG_FIFO_THR_28	0x600ul
#define MCHP_P80BD_CFG_FIFO_THR_30	0x700ul
#define MCHP_P80BD_CFG_SRST		BIT(31) /* WO */

/* Status and Interrupt Enable 16-bit */
#define MCHP_P80BD_SI_OFS		0x108u
#define MCHP_P80BD_SI_MASK		0x107u
#define MCHP_P80BD_SI_STS_MASK		0x007u
#define MCHP_P80BD_SI_IEN_MASK		0x100u
#define MCHP_P80BD_SI_NE_STS		BIT(0)
#define MCHP_P80BD_SI_OVR_STS		BIT(1)
#define MCHP_P80BD_SI_THR_STS		BIT(2)
#define MCHP_P80BD_SI_THR_IEN		BIT(8)

/* Snapshot 32-bit (RO) */
#define MCHP_P80BD_SS_OFS		0x10Cu
#define MCHP_P80BD_SS_MASK		0xFFFFFFFFul

/* Capture 32-bit (RO). Current 4-byte Port 80 capture value */
#define MCHP_P80BD_CAP_OFS		0x110u

#endif /* #ifndef _MEC_P80BD_H */
