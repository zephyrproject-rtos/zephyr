/*
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MEC_BCL_H
#define _MEC_BCL_H

#include <stdint.h>
#include <stddef.h>

#include "mec_defs.h"

#define MCHP_BCL_0_BASE_ADDR		0x4000CD00ul

#define MCHP_BCL_0_GIRQ			18u
#define MCHP_BCL_0_NVIC_GIRQ		10u
#define MCHP_BCL_0_BCLR_POS		6u
#define MCHP_BCL_0_ERR_POS		7u
#define MCHP_BCL_0_BCLR_NVIC		97u
#define MCHP_BCL_0_ERR_NVIC		96u
#define MCHP_BCL_0_BCLR_GIRQ_VAL	BIT(6)
#define MCHP_BCL_0_ERR_GIRQ_VAL		BIT(7)

/* EC-only registers */

/* BCL Status register at 0x00 */
#define MCHP_BCL_STS_OFS		0x00u
#define MCHP_BCL_STS_MASK		0xF1u
#define MCHP_BCL_STS_RESET		BIT(7)
#define MCHP_BCL_STS_ERR_RWC		BIT(6)
#define MCHP_BCL_STS_ERR_IEN		BIT(5)
#define MCHP_BCL_STS_BCLR_IEN		BIT(4)
#define MCHP_BCL_STS_BUSY_RO		BIT(0)

/* BCL Target address register at 0x04 */
#define MCHP_BCL_TADDR_OFS		0x04u
#define MCHP_BCL_TADDR_MASK		0xFFu

/* BCL Data register at 0x08 */
#define MCHP_BCL_DATA_OFS		0x08u
#define MCHP_BCL_DATA_MASK		0xFFu

/* BCL Clock select register at 0x0C */
#define MCHP_BCL_CLKSEL_OFS		0x0Cu
#define MCHP_BCL_CLKSEL_DIV_MSK		0xFFu
#define MCHP_BCL_CLKSEL_DFLT		0x04u
#define MCHP_BCL_CLKSEL_48M		0x00u
#define MCHP_BCL_CLKSEL_24M		0x01u
#define MCHP_BCL_CLKSEL_16M		0x02u
#define MCHP_BCL_CLKSEL_12M		0x03u
#define MCHP_BCL_CLKSEL_8M		0x05u
#define MCHP_BCL_CLKSEL_6M		0x07u
#define MCHP_BCL_CLKSEL_1M		0x2Fu

#endif	/* #ifndef _MEC_BCL_H */
