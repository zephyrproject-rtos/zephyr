/*
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MEC_ESPI_VW_H
#define _MEC_ESPI_VW_H

#include <stdint.h>
#include <stddef.h>

#include "mec_defs.h"
#include "mec_regaccess.h"

/* Master to Slave VW register: 96-bit (3 32 bit registers) */
/* 32-bit word 0 (bits[31:0]) */
#define ESPI_M2SW0_OFS			0ul
#define ESPI_M2SW0_IDX_POS		0
#define ESPI_M2SW0_IDX_MASK		0xFFu
#define ESPI_M2SW0_MTOS_SRC_POS		8u
#define ESPI_M2SW0_MTOS_SRC_MASK0	0x03u
#define ESPI_M2SW0_MTOS_SRC_MASK	0x300u
#define ESPI_M2SW0_MTOS_SRC_ESPI_RST	0ul
#define ESPI_M2SW0_MTOS_SRC_SYS_RST	0x100u
#define ESPI_M2SW0_MTOS_SRC_SIO_RST	0x200u
#define ESPI_M2SW0_MTOS_SRC_PLTRST	0x300u
#define ESPI_M2SW0_MTOS_STATE_POS	12u
#define ESPI_M2SW0_MTOS_STATE_MASK0	0x0Ful
#define ESPI_M2SW0_MTOS_STATE_MASK	0xF000ul
/* 32-bit word 1 (bits[63:32]) */
#define ESPI_M2SW1_OFS			4ul
#define ESPI_M2SW1_SRC0_SEL_POS		0
#define ESPI_M2SW1_SRC_SEL_MASK0	0x0Ful
#define ESPI_M2SW1_SRC0_SEL_MASK	0x0Ful
#define ESPI_M2SW1_SRC1_SEL_POS		8
#define ESPI_M2SW1_SRC1_SEL_MASK	0x0F00ul
#define ESPI_M2SW1_SRC2_SEL_POS		16
#define ESPI_M2SW1_SRC2_SEL_MASK	0x0F0000ul
#define ESPI_M2SW1_SRC3_SEL_POS		24
#define ESPI_M2SW1_SRC3_SEL_MASK	0x0F000000ul
/* 0 <= n < 4 */
#define ESPI_M2SW1_SRC_SEL_POS(n)	((n) * 8U)
#define ESPI_M2SW1_SRC_SEL_MASK(n)	SHLU32(0xFu, ((n) * 8U))
#define ESPI_M2SW1_SRC_SEL_VAL(n, v)	SHLU32(((v) & 0xFu), ((n) * 8U))
/* 32-bit word 2 (bits[95:64]) */
#define ESPI_M2SW2_OFS			8ul
#define ESPI_M2SW2_SRC_MASK0		0x0Ful
#define ESPI_M2SW2_SRC0_POS		0
#define ESPI_M2SW2_SRC0_MASK		0x0Ful
#define ESPI_M2SW2_SRC1_POS		8u
#define ESPI_M2SW2_SRC1_MASK		0x0F00ul
#define ESPI_M2SW2_SRC2_POS		16u
#define ESPI_M2SW2_SRC2_MASK		0x0F0000ul
#define ESPI_M2SW2_SRC3_POS		24u
#define ESPI_M2SW2_SRC3_MASK		0x0F000000ul
/* 0 <= n < 4 */
#define ESPI_M2SW2_SRC_POS(n)		((n) * 8U)
#define ESPI_M2SW2_SRC_MASK(n)		SHLU32(0xFu, ((n) * 8U))
#define ESPI_M2SW2_SRC_VAL(n, v)	SHLU32(((v) & 0xFu), ((n) * 8U))

/*
 * Zero based values used for above SRC_SEL fields.
 * These values select the interrupt sensitivity for the VWire.
 * Example: Set SRC1 to Level High
 *
 * r = read MSVW1 register
 * r &= ESPI_M2SW1_SRC_SEL_MASK(1)
 * r |= ESPI_MSVW1_SRC_SEL_VAL(1, ESPI_IRQ_SEL_LVL_HI)
 * write r to MSVW1 register
 */
#define ESPI_IRQ_SEL_LVL_LO	0
#define ESPI_IRQ_SEL_LVL_HI	1
#define ESPI_IRQ_SEL_DIS	4
/* NOTE: Edge trigger modes allow VWires to wake from deep sleep */
#define ESPI_IRQ_SEL_REDGE	0x0Du
#define ESPI_IRQ_SEL_FEDGE	0x0Eu
#define ESPI_IRQ_SEL_BEDGE	0x0Fu

/* Slave to Master VW register: 64-bit (2 32 bit registers) */
/* 32-bit word 0 (bits[31:0]) */
#define ESPI_S2MW0_OFS			0
#define ESPI_S2MW0_IDX_POS		0
#define ESPI_S2MW0_IDX_MASK		0xFFu
#define ESPI_S2MW0_STOM_POS		8u
#define ESPI_S2MW0_STOM_SRC_POS		8u
#define ESPI_S2MW0_STOM_MASK0		0xF3u
#define ESPI_S2MW0_STOM_MASK		0xF300u
#define ESPI_S2MW0_STOM_SRC_MASK0	0x03ul
#define ESPI_S2MW0_STOM_SRC_MASK	0x0300ul
#define ESPI_S2MW0_STOM_SRC_ESPI_RST	0ul
#define ESPI_S2MW0_STOM_SRC_SYS_RST	0x0100ul
#define ESPI_S2MW0_STOM_SRC_SIO_RST	0x0200ul
#define ESPI_S2MW0_STOM_SRC_PLTRST	0x0300ul
#define ESPI_S2MW0_STOM_STATE_POS	12u
#define ESPI_S2MW0_STOM_STATE_MASK0	0x0Ful
#define ESPI_S2MW0_STOM_STATE_MASK	0x0F000ul
#define ESPI_S2MW0_CHG0_POS		16u
#define ESPI_S2MW0_CHG0			BIT(ESPI_S2MW0_CHG0_POS)
#define ESPI_S2MW0_CHG1_POS		17u
#define ESPI_S2MW0_CHG1			BIT(ESPI_S2MW0_CHG1_POS)
#define ESPI_S2MW0_CHG2_POS		18u
#define ESPI_S2MW0_CHG2			BIT(ESPI_S2MW0_CHG2_POS)
#define ESPI_S2MW0_CHG3_POS		19u
#define ESPI_S2MW0_CHG3			BIT(ESPI_S2MW0_CHG3_POS)
#define ESPI_S2MW0_CHG_ALL_POS		16u
#define ESPI_S2MW0_CHG_ALL_MASK0	0x0Ful
#define ESPI_S2MW0_CHG_ALL_MASK		0x0F0000ul
/* 0 <= n < 4 */
#define ESPI_S2MW1_CHG_POS(n)		((n) + 16u)
#define ESPI_S2MW1_CHG(v, n)		\
	(((uint32_t)(v) >> ESPI_S2MW1_CHG_POS(n)) & 0x01)

/* 32-bit word 1 (bits[63:32]) */
#define ESPI_S2MW1_OFS			4ul
#define ESPI_S2MW1_SRC0_POS		0u
#define ESPI_S2MW1_SRC0			BIT(ESPI_S2MW1_SRC0_POS)
#define ESPI_S2MW1_SRC1_POS		8u
#define ESPI_S2MW1_SRC1			BIT(ESPI_S2MW1_SRC1_POS)
#define ESPI_S2MW1_SRC2_POS		16u
#define ESPI_S2MW1_SRC2			BIT(ESPI_S2MW1_SRC2_POS)
#define ESPI_S2MW1_SRC3_POS		24u
#define ESPI_S2MW1_SRC3			BIT(ESPI_S2MW1_SRC3_POS)
/* 0 <= n < 4 */
#define ESPI_S2MW1_SRC_POS(n)		SHLU32((n), 3)
#define ESPI_S2MW1_SRC(v, n)		\
	SHLU32(((uint32_t)(v) & 0x01), (ESPI_S2MW1_SRC_POS(n)))

/* =========================================================================*/
/* ================	      ESPI_VW			   ================ */
/* =========================================================================*/

/**
 * @brief eSPI Virtual Wires (ESPI_VW)
 */

#define ESPI_MSVW_IDX_MAX	10u
#define ESPI_SMVW_IDX_MAX	10u

#define ESPI_NUM_MSVW		11u
#define ESPI_NUM_SMVW		11u

/*
 * ESPI MSVW interrupts
 * GIRQ24 contains MSVW 0 - 6
 * GIRQ25 contains MSVW 7 - 10
 */
#define MEC_ESPI_MSVW_NUM_GIRQS     2u

#define MEC_ESPI_MSVW_00_06_GIRQ	24u
#define MEC_ESPI_MSVW_00_06_NVIC	15u

#define MEC_ESPI_MSVW00_SRC0_POS	0u
#define MEC_ESPI_MSVW00_SRC1_POS	1u
#define MEC_ESPI_MSVW00_SRC2_POS	2u
#define MEC_ESPI_MSVW00_SRC3_POS	3u
#define MEC_ESPI_MSVW01_SRC0_POS	4u
#define MEC_ESPI_MSVW01_SRC1_POS	5u
#define MEC_ESPI_MSVW01_SRC2_POS	6u
#define MEC_ESPI_MSVW01_SRC3_POS	7u
#define MEC_ESPI_MSVW02_SRC0_POS	8u
#define MEC_ESPI_MSVW02_SRC1_POS	9u
#define MEC_ESPI_MSVW02_SRC2_POS	10u
#define MEC_ESPI_MSVW02_SRC3_POS	11u
#define MEC_ESPI_MSVW03_SRC0_POS	12u
#define MEC_ESPI_MSVW03_SRC1_POS	13u
#define MEC_ESPI_MSVW03_SRC2_POS	14u
#define MEC_ESPI_MSVW03_SRC3_POS	15u
#define MEC_ESPI_MSVW04_SRC0_POS	16u
#define MEC_ESPI_MSVW04_SRC1_POS	17u
#define MEC_ESPI_MSVW04_SRC2_POS	16u
#define MEC_ESPI_MSVW04_SRC3_POS	19u
#define MEC_ESPI_MSVW05_SRC0_POS	20u
#define MEC_ESPI_MSVW05_SRC1_POS	21u
#define MEC_ESPI_MSVW05_SRC2_POS	22u
#define MEC_ESPI_MSVW05_SRC3_POS	23u
#define MEC_ESPI_MSVW06_SRC0_POS	24u
#define MEC_ESPI_MSVW06_SRC1_POS	25u
#define MEC_ESPI_MSVW06_SRC2_POS	26u
#define MEC_ESPI_MSVW06_SRC3_POS	27u

#define MEC_ESPI_MSVW00_SRC0_VAL	BIT(MEC_ESPI_MSVW00_SRC0_POS)
#define MEC_ESPI_MSVW00_SRC1_VAL	BIT(MEC_ESPI_MSVW00_SRC1_POS)
#define MEC_ESPI_MSVW00_SRC2_VAL	BIT(MEC_ESPI_MSVW00_SRC2_POS)
#define MEC_ESPI_MSVW00_SRC3_VAL	BIT(MEC_ESPI_MSVW00_SRC3_POS)
#define MEC_ESPI_MSVW01_SRC0_VAL	BIT(MEC_ESPI_MSVW01_SRC0_POS)
#define MEC_ESPI_MSVW01_SRC1_VAL	BIT(MEC_ESPI_MSVW01_SRC1_POS)
#define MEC_ESPI_MSVW01_SRC2_VAL	BIT(MEC_ESPI_MSVW01_SRC2_POS)
#define MEC_ESPI_MSVW01_SRC3_VAL	BIT(MEC_ESPI_MSVW01_SRC3_POS)
#define MEC_ESPI_MSVW02_SRC0_VAL	BIT(MEC_ESPI_MSVW02_SRC0_POS)
#define MEC_ESPI_MSVW02_SRC1_VAL	BIT(MEC_ESPI_MSVW02_SRC1_POS)
#define MEC_ESPI_MSVW02_SRC2_VAL	BIT(MEC_ESPI_MSVW02_SRC2_POS)
#define MEC_ESPI_MSVW02_SRC3_VAL	BIT(MEC_ESPI_MSVW02_SRC3_POS)
#define MEC_ESPI_MSVW03_SRC0_VAL	BIT(MEC_ESPI_MSVW03_SRC0_POS)
#define MEC_ESPI_MSVW03_SRC1_VAL	BIT(MEC_ESPI_MSVW03_SRC1_POS)
#define MEC_ESPI_MSVW03_SRC2_VAL	BIT(MEC_ESPI_MSVW03_SRC2_POS)
#define MEC_ESPI_MSVW03_SRC3_VAL	BIT(MEC_ESPI_MSVW03_SRC3_POS)
#define MEC_ESPI_MSVW04_SRC0_VAL	BIT(MEC_ESPI_MSVW04_SRC0_POS)
#define MEC_ESPI_MSVW04_SRC1_VAL	BIT(MEC_ESPI_MSVW04_SRC1_POS)
#define MEC_ESPI_MSVW04_SRC2_VAL	BIT(MEC_ESPI_MSVW04_SRC2_POS)
#define MEC_ESPI_MSVW04_SRC3_VAL	BIT(MEC_ESPI_MSVW04_SRC3_POS)
#define MEC_ESPI_MSVW05_SRC0_VAL	BIT(MEC_ESPI_MSVW05_SRC0_POS)
#define MEC_ESPI_MSVW05_SRC1_VAL	BIT(MEC_ESPI_MSVW05_SRC1_POS)
#define MEC_ESPI_MSVW05_SRC2_VAL	BIT(MEC_ESPI_MSVW05_SRC2_POS)
#define MEC_ESPI_MSVW05_SRC3_VAL	BIT(MEC_ESPI_MSVW05_SRC3_POS)
#define MEC_ESPI_MSVW06_SRC0_VAL	BIT(MEC_ESPI_MSVW06_SRC0_POS)
#define MEC_ESPI_MSVW06_SRC1_VAL	BIT(MEC_ESPI_MSVW06_SRC1_POS)
#define MEC_ESPI_MSVW06_SRC2_VAL	BIT(MEC_ESPI_MSVW06_SRC2_POS)
#define MEC_ESPI_MSVW06_SRC3_VAL	BIT(MEC_ESPI_MSVW06_SRC3_POS)

/*
 * 0 <= v <= 6
 * 0 <= s <= 3
 */
#define MEC_ESPI_MSVW_00_06_GIRQ_POS(v, s)	\
	(((uint32_t)(v) * 4U) + (uint32_t)(s))

#define MEC_ESPI_VSVW_07_10_GIRQ	25u
#define MEC_ESPI_VSVW_07_10_NVIC	16u

#define MEC_ESPI_MSVW07_SRC0_POS	0u
#define MEC_ESPI_MSVW07_SRC1_POS	1u
#define MEC_ESPI_MSVW07_SRC2_POS	2u
#define MEC_ESPI_MSVW07_SRC3_POS	3u
#define MEC_ESPI_MSVW08_SRC0_POS	4u
#define MEC_ESPI_MSVW08_SRC1_POS	5u
#define MEC_ESPI_MSVW08_SRC2_POS	6u
#define MEC_ESPI_MSVW08_SRC3_POS	7u
#define MEC_ESPI_MSVW09_SRC0_POS	8u
#define MEC_ESPI_MSVW09_SRC1_POS	9u
#define MEC_ESPI_MSVW09_SRC2_POS	10u
#define MEC_ESPI_MSVW09_SRC3_POS	11u
#define MEC_ESPI_MSVW10_SRC0_POS	12u
#define MEC_ESPI_MSVW10_SRC1_POS	13u
#define MEC_ESPI_MSVW10_SRC2_POS	14u
#define MEC_ESPI_MSVW10_SRC3_POS	15u

#define MEC_ESPI_MSVW07_SRC0_VAL	BIT(MEC_ESPI_MSVW07_SRC0_POS)
#define MEC_ESPI_MSVW07_SRC1_VAL	BIT(MEC_ESPI_MSVW07_SRC1_POS)
#define MEC_ESPI_MSVW07_SRC2_VAL	BIT(MEC_ESPI_MSVW07_SRC2_POS)
#define MEC_ESPI_MSVW07_SRC3_VAL	BIT(MEC_ESPI_MSVW07_SRC3_POS)
#define MEC_ESPI_MSVW08_SRC0_VAL	BIT(MEC_ESPI_MSVW08_SRC0_POS)
#define MEC_ESPI_MSVW08_SRC1_VAL	BIT(MEC_ESPI_MSVW08_SRC1_POS)
#define MEC_ESPI_MSVW08_SRC2_VAL	BIT(MEC_ESPI_MSVW08_SRC2_POS)
#define MEC_ESPI_MSVW08_SRC3_VAL	BIT(MEC_ESPI_MSVW08_SRC3_POS)
#define MEC_ESPI_MSVW09_SRC0_VAL	BIT(MEC_ESPI_MSVW09_SRC0_POS)
#define MEC_ESPI_MSVW09_SRC1_VAL	BIT(MEC_ESPI_MSVW09_SRC1_POS)
#define MEC_ESPI_MSVW09_SRC2_VAL	BIT(MEC_ESPI_MSVW09_SRC2_POS)
#define MEC_ESPI_MSVW09_SRC3_VAL	BIT(MEC_ESPI_MSVW09_SRC3_POS)
#define MEC_ESPI_MSVW10_SRC0_VAL	BIT(MEC_ESPI_MSVW10_SRC0_POS)
#define MEC_ESPI_MSVW10_SRC1_VAL	BIT(MEC_ESPI_MSVW10_SRC1_POS)
#define MEC_ESPI_MSVW10_SRC2_VAL	BIT(MEC_ESPI_MSVW10_SRC2_POS)
#define MEC_ESPI_MSVW10_SRC3_VAL	BIT(MEC_ESPI_MSVW10_SRC3_POS)

/*
 * 7 <= v <= 10
 * 0 <= s <= 3
 */
#define MEC_ESPI_MSVW_07_10_GIRQ_POS(v, s)	\
	((((uint32_t)(v) - 7ul) * 4U) + (uint32_t)(s))

/* Master-to-Slave VW byte indices(offsets) */
#define MSVW_INDEX_OFS		0u
#define MSVW_MTOS_OFS		1u
#define MSVW_SRC0_ISEL_OFS	4u
#define MSVW_SRC1_ISEL_OFS	5u
#define MSVW_SRC2_ISEL_OFS	6u
#define MSVW_SRC3_ISEL_OFS	7u
#define MSVW_SRC0_OFS		8u
#define MSVW_SRC1_OFS		9u
#define MSVW_SRC2_OFS		10u
#define MSVW_SRC3_OFS		11u

/* Slave-to-Master VW byte indices(offsets) */
#define SMVW_INDEX_OFS		0u
#define SMVW_STOM_OFS		1u
#define SMVW_CHANGED_OFS	2u
#define SMVW_SRC0_OFS		4u
#define SMVW_SRC1_OFS		5u
#define SMVW_SRC2_OFS		6u
#define SMVW_SRC3_OFS		7u


/* Master-to-Slave Virtual Wire 96-bit register */
#define MEC_MSVW_SRC0_IRQ_SEL_POS	0u
#define MEC_MSVW_SRC1_IRQ_SEL_POS	8u
#define MEC_MSVW_SRC2_IRQ_SEL_POS	16u
#define MEC_MSVW_SRC3_IRQ_SEL_POS	24u

#define MEC_MSVW_SRC_IRQ_SEL_MASK0	0x0Ful
#define MEC_MSVW_SRC0_IRQ_SEL_MASK	\
	SHLU32(MEC_MSVW_SRC_IRQ_SEL_MASK0, MEC_MSVW_SRC0_IRQ_SEL_POS)
#define MEC_MSVW_SRC1_IRQ_SEL_MASK	\
	SHLU32(MEC_MSVW_SRC_IRQ_SEL_MASK0, MEC_MSVW_SRC1_IRQ_SEL_POS)
#define MEC_MSVW_SRC2_IRQ_SEL_MASK	\
	SHLU32(MEC_MSVW_SRC_IRQ_SEL_MASK0, MEC_MSVW_SRC2_IRQ_SEL_POS)
#define MEC_MSVW_SRC3_IRQ_SEL_MASK	\
	SHLU32(MEC_MSVW_SRC_IRQ_SEL_MASK0, MEC_MSVW_SRC3_IRQ_SEL_POS)

#define MEC_MSVW_SRC_IRQ_SEL_LVL_LO	0x00ul
#define MEC_MSVW_SRC_IRQ_SEL_LVL_HI	0x01ul
#define MEC_MSVW_SRC_IRQ_SEL_DIS	0x04ul
#define MEC_MSVW_SRC_IRQ_SEL_EDGE_FALL	0x0Dul
#define MEC_MSVW_SRC_IRQ_SEL_EDGE_RISE	0x0Eul
#define MEC_MSVW_SRC_IRQ_SEL_EDGE_BOTH	0x0Ful

/*
 * 0 <= src <= 3
 * isel = MEC_MSVW_SRC_IRQ_SEL_LVL_LO, ...
 */
#define MEC_MSVW_SRC_IRQ_SEL_VAL(src, isel)	\
	((uint32_t)(isel) << ((src) * 8U))

#define MEC_MSVW_SRC0_POS	0ul
#define MEC_MSVW_SRC1_POS	8ul
#define MEC_MSVW_SRC2_POS	16ul
#define MEC_MSVW_SRC3_POS	24ul

#define MEC_MSVW_SRC_MASK0	0x01ul

#define MEC_MSVW_SRC0_MASK	BIT(0)
#define MEC_MSVW_SRC1_MASK	BIT(8)
#define MEC_MSVW_SRC2_MASK	BIT(16)
#define MEC_MSVW_SRC3_MASK	BIT(24)

/*
 * 0 <= src <= 3
 * val = 0 or 1
 */
#define MEC_MSVW_SRC_VAL(src, val)	\
	((uint32_t)(val & 0x01u) << ((src) * 8U))

/* Slave-to-Master Virtual Wire 64-bit register */

/* MSVW helper inline functions */

/* Interfaces to any C modules */
#ifdef __cplusplus
extern "C" {
#endif

enum espi_msvw_src {
	MSVW_SRC0 = 0u,
	MSVW_SRC1,
	MSVW_SRC2,
	MSVW_SRC3
};

enum espi_smvw_src {
	SMVW_SRC0 = 0u,
	SMVW_SRC1,
	SMVW_SRC2,
	SMVW_SRC3
};

enum espi_msvw_irq_sel {
	MSVW_IRQ_SEL_LVL_LO	= 0x00ul,
	MSVW_IRQ_SEL_LVL_HI	= 0x01ul,
	MSVW_IRQ_SEL_DIS	= 0x04ul,
	MSVW_IRQ_SEL_EDGE_FALL	= 0x0Dul,
	MSVW_IRQ_SEL_EDGE_RISE	= 0x0Eul,
	MSVW_IRQ_SEL_EDGE_BOTH	= 0x0Ful
};

/* Used for both MSVW MTOS and SMVW STOM fields */
enum espi_vw_rst_src {
	VW_RST_SRC_ESPI_RESET = 0ul,
	VW_RST_SRC_SYS_RESET,
	VW_RST_SRC_SIO_RESET,
	VW_RST_SRC_PLTRST,
};

/*
 * Set the specified Master-to-Slave VWire's interrupt select field for
 * the specified edge, level, or disabled.
 * MSVW IRQ_SELECT is a 32-bit register where the four VWire's
 * interrupt select fields are located on byte boundaries.
 * Param p is a pointer to the 96-bit MSVW register.
 * Param src is in [0:3] specifying one of the four VWire's.
 * Param isel is the new value for the specified VWire's interrupt
 * select field. IRQ_SELECT is the 32-bit word at bits[63:32] in the
 * 96-bit MSVW register.
 * IRQ_SELECT[3:0]   = SRC0_IRQ_SELECT
 * IRQ_SELECT[11:8]  = SRC1_IRQ_SELECT
 * IRQ_SELECT[19:16] = SRC1_IRQ_SELECT
 * IRQ_SELECT[27:24] = SRC1_IRQ_SELECT
 * The MSVW registers are byte accessible allowing us to avoid a
 * read-modify-write.
 */
static __attribute__ ((always_inline))
inline void
mec_espi_msvw_irq_sel_set(struct espi_msvw_reg *p, enum espi_msvw_src src,
			  enum espi_msvw_irq_sel isel)
{
	volatile uint8_t *psrc = (volatile uint8_t *)&p->SRC_IRQ_SEL;

	*(psrc + (uintptr_t) src) = isel;
}

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _MEC_ESPI_VW_H */
