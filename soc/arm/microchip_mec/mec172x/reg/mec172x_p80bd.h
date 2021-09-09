/*
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MEC172X_P80BD_H
#define _MEC172X_P80BD_H

#include <stdint.h>
#include <stddef.h>

#define MCHP_P80BD_0_BASE_ADDR		0x400f8000u

/* HDATA - Write-Only 32-bit */
#define MCHP_P80BD_HDATA_OFS		0x00u
#define MCHP_P80BD_HDATA_MASK		GENMASK(31, 0)

/*
 * EC-only Data/Attributes 16-bit
 * b[7:0]  = data byte from capture FIFO
 * b[15:8] = data attributes
 */
#define MCHP_P80BD_ECDA_OFS		0x100u
#define MCHP_P80BD_ECDA_MASK		0x7fffu
#define MCHP_P80BD_ECDA_DPOS		0
#define MCHP_P80BD_ECDA_APOS		8
#define MCHP_P80BD_ECDA_DMSK		0xffu
#define MCHP_P80BD_ECDA_AMSK		0x7f00u
#define MCHP_P80BD_ECDA_LANE_POS	8
#define MCHP_P80BD_ECDA_LANE_MSK	0x0300u
#define MCHP_P80BD_ECDA_LANE_0		0x0000u
#define MCHP_P80BD_ECDA_LANE_1		0x0100u
#define MCHP_P80BD_ECDA_LANE_2		0x0200u
#define MCHP_P80BD_ECDA_LANE_3		0x0300u
#define MCHP_P80BD_ECDA_LEN_POS		10
#define MCHP_P80BD_ECDA_LEN_MSK		0x0c00u
#define MCHP_P80BD_ECDA_LEN_1		0x0000u
#define MCHP_P80BD_ECDA_LEN_2		0x0400u
#define MCHP_P80BD_ECDA_LEN_4		0x0800u
#define MCHP_P80BD_ECDA_LEN_INVAL	0x0c00u
#define MCHP_P80BD_ECDA_NE		BIT(12)
#define MCHP_P80BD_ECDA_OVR		BIT(13)
#define MCHP_P80BD_ECDA_THR		BIT(14)

/* Configuration */
#define MCHP_P80BD_CFG_OFS		0x104u
#define MCHP_P80BD_CFG_MASK		0x80000703u
#define MCHP_P80BD_CFG_FLUSH_FIFO	BIT(0)	/* WO */
#define MCHP_P80BD_CFG_SNAP_CLR		BIT(1)	/* WO */
#define MCHP_P80BD_CFG_FIFO_THR_POS	8
#define MCHP_P80BD_CFG_FIFO_THR_MSK	0x700u
#define MCHP_P80BD_CFG_FIFO_THR_1	0x000u
#define MCHP_P80BD_CFG_FIFO_THR_4	0x100u
#define MCHP_P80BD_CFG_FIFO_THR_8	0x200u
#define MCHP_P80BD_CFG_FIFO_THR_16	0x300u
#define MCHP_P80BD_CFG_FIFO_THR_20	0x400u
#define MCHP_P80BD_CFG_FIFO_THR_24	0x500u
#define MCHP_P80BD_CFG_FIFO_THR_28	0x600u
#define MCHP_P80BD_CFG_FIFO_THR_30	0x700u
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
#define MCHP_P80BD_SS_MASK		0xffffffffu

/* Capture 32-bit (RO). Current 4-byte Port 80 capture value */
#define MCHP_P80BD_CAP_OFS		0x110u

/** @brief BIOS Debug Port 80h and Alias port capture registers. */
struct p80bd_regs {
	volatile uint32_t HDATA;
	uint8_t RSVD1[0x100 - 0x04];
	volatile uint32_t EC_DA;
	volatile uint32_t CONFIG;
	volatile uint32_t STS_IEN;
	volatile uint32_t SNAPSHOT;
	volatile uint32_t CAPTURE;
	uint8_t RSVD2[0x330 - 0x114];
	volatile uint32_t ACTV;
	uint8_t RSVD3[0x400 - 0x334];
	volatile uint8_t ALIAS_HDATA;
	uint8_t RSVD4[0x730 - 0x401];
	volatile uint32_t ALIAS_ACTV;
	uint8_t RSVD5[0x7f0 - 0x734];
	volatile uint32_t ALIAS_BLS;
};

#endif /* #ifndef _MEC172X_P80BD_H */
