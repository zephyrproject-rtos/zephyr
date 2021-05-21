/*
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MEC_EMI_H
#define _MEC_EMI_H

#include <stdint.h>
#include <stddef.h>

#include "mec_defs.h"

#define MCHP_EMI_BASE_ADDR	0x400F4000ul

#define MCHP_EMI_SPACING	0x0400ul
#define MCHP_EMI_SPACING_PWROF2 10u

#define MCHP_EMI0_ADDR		0x400F4000ul
#define MCHP_EMI1_ADDR		0x400F4400ul
#define MCHP_EMI2_ADDR		0x400F4800ul

/* EMI interrupts */
#define MCHP_EMI_GIRQ		15u
#define MCHP_EMI_GIRQ_NVIC	7u
/* Direct NVIC connections */
#define MCHP_EMI0_NVIC		42u
#define MCHP_EMI1_NVIC		43u
#define MCHP_EMI2_NVIC		44u

/* GIRQ Source, Enable_Set/Clr, Result registers bit positions */
#define MCHP_EMI0_GIRQ_POS	2u
#define MCHP_EMI1_GIRQ_POS	3u
#define MCHP_EMI2_GIRQ_POS	4u

#define MCHP_EMI0_GIRQ		BIT(2)
#define MCHP_EMI1_GIRQ		BIT(3)
#define MCHP_EMI2_GIRQ		BIT(4)

/* OS_INT_SRC_LSB */
#define MCHP_EMI_OSIS_LSB_EC_WR_POS	0u
#define MCHP_EMI_OSIS_LSB_EC_WR		BIT(0)
/* the following bits are also apply to OS_INT_MASK_LSB */
#define MCHP_EMI_OSIS_LSB_SWI_POS	1u
#define MCHP_EMI_OSIS_LSB_SWI_MASK0	0x7Ful
#define MCHP_EMI_OSIS_LSB_SWI_MASK	0xFEul
#define MCHP_EMI_OSIS_LSB_SWI1		BIT(1)
#define MCHP_EMI_OSIS_LSB_SWI2		BIT(2)
#define MCHP_EMI_OSIS_LSB_SWI3		BIT(3)
#define MCHP_EMI_OSIS_LSB_SWI4		BIT(4)
#define MCHP_EMI_OSIS_LSB_SWI5		BIT(5)
#define MCHP_EMI_OSIS_LSB_SWI6		BIT(6)
#define MCHP_EMI_OSIS_LSB_SWI7		BIT(7)

/* OS_INT_SRC_MSB and OS_INT_MASK_MSB */
#define MCHP_EMI_OSIS_MSB_SWI_POS	0u
#define MCHP_EMI_OSIS_MSB_SWI_MASK0	0xFFul
#define MCHP_EMI_OSIS_MSB_SWI_MASK	0xFFul
#define MCHP_EMI_OSIS_MSB_SWI8		BIT(0)
#define MCHP_EMI_OSIS_MSB_SWI9		BIT(1)
#define MCHP_EMI_OSIS_MSB_SWI10		BIT(2)
#define MCHP_EMI_OSIS_MSB_SWI11		BIT(3)
#define MCHP_EMI_OSIS_MSB_SWI12		BIT(4)
#define MCHP_EMI_OSIS_MSB_SWI13		BIT(5)
#define MCHP_EMI_OSIS_MSB_SWI14		BIT(6)
#define MCHP_EMI_OSIS_MSB_SWI15		BIT(7)

/*  OS_APP_ID */
#define MCHP_EMI_OS_APP_ID_MASK		0xFFul

/*
 * MEM_BASE_0 and MEM_BASE_1 registers
 * bits[1:0] = 00b read-only forcing EC SRAM location to
 * be aligned >= 4 bytes.
 */
#define MCHP_EMI_MEM_BASE_MASK		0xFFFFFFFCul

/*
 * MEM_LIMIT_0 and MEM_LIMIT_1 registers are split into two fields
 * bits[15:0] = read limit
 *	bits[1:0]=00b read-only forcing >= 4 byte alignment
 * bits[31:16] = write limit
 *	bits[17:16]=00b read-only forcing >= 4 byte alignment
 */
#define MEM_EMI_MEM_LIMIT_MASK		0xFFFCFFFCul
#define MEM_EMI_MEM_LIMIT_RD_POS	0u
#define MEM_EMI_MEM_LIMIT_RD_MASK0	0xFFFCul
#define MEM_EMI_MEM_LIMIT_RD_MASK	0xFFFCul
#define MEM_EMI_MEM_LIMIT_WR_POS	16u
#define MEM_EMI_MEM_LIMIT_WR_MASK0	0xFFFCul
#define MEM_EMI_MEM_LIMIT_WR_MASK	SHLU32(0xFFFCu, 16)

/* EC_SET_OS_INT and EC_OS_INT_CLR_EN */
#define MCHP_EMI_EC_OS_SWI_MASK 0xFFFEul
#define MCHP_EMI_EC_OS_SWI1	BIT(1)
#define MCHP_EMI_EC_OS_SWI2	BIT(2)
#define MCHP_EMI_EC_OS_SWI3	BIT(3)
#define MCHP_EMI_EC_OS_SWI4	BIT(4)
#define MCHP_EMI_EC_OS_SWI5	BIT(5)
#define MCHP_EMI_EC_OS_SWI6	BIT(6)
#define MCHP_EMI_EC_OS_SWI7	BIT(7)
#define MCHP_EMI_EC_OS_SWI8	BIT(8)
#define MCHP_EMI_EC_OS_SWI9	BIT(9)
#define MCHP_EMI_EC_OS_SWI10	BIT(10)
#define MCHP_EMI_EC_OS_SWI11	BIT(11)
#define MCHP_EMI_EC_OS_SWI12	BIT(12)
#define MCHP_EMI_EC_OS_SWI13	BIT(13)
#define MCHP_EMI_EC_OS_SWI14	BIT(14)
#define MCHP_EMI_EC_OS_SWI15	BIT(15)

#endif	/* #ifndef _MEC_EMI_H */
