/*
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MEC172X_ESPI_VW_H
#define _MEC172X_ESPI_VW_H

#include <stdint.h>
#include <stddef.h>

/* Master to Slave VW register: 96-bit (3 32 bit registers) */
/* 32-bit word 0 (bits[31:0]) */
#define ESPI_M2SW0_OFS			0u
#define ESPI_M2SW0_IDX_POS		0
#define ESPI_M2SW0_IDX_MASK		0xffu
#define ESPI_M2SW0_MTOS_SRC_POS		8u
#define ESPI_M2SW0_MTOS_SRC_MASK0	0x03u
#define ESPI_M2SW0_MTOS_SRC_MASK	0x300u
#define ESPI_M2SW0_MTOS_SRC_ESPI_RST	0u
#define ESPI_M2SW0_MTOS_SRC_SYS_RST	0x100u
#define ESPI_M2SW0_MTOS_SRC_SIO_RST	0x200u
#define ESPI_M2SW0_MTOS_SRC_PLTRST	0x300u
#define ESPI_M2SW0_MTOS_STATE_POS	12u
#define ESPI_M2SW0_MTOS_STATE_MASK0	0x0fu
#define ESPI_M2SW0_MTOS_STATE_MASK	0xf000u
/* 32-bit word 1 (bits[63:32]) */
#define ESPI_M2SW1_OFS			4u
#define ESPI_M2SW1_SRC0_SEL_POS		0
#define ESPI_M2SW1_SRC_SEL_MASK0	0x0fu
#define ESPI_M2SW1_SRC0_SEL_MASK	0x0fu
#define ESPI_M2SW1_SRC1_SEL_POS		8
#define ESPI_M2SW1_SRC1_SEL_MASK	0x0f00u
#define ESPI_M2SW1_SRC2_SEL_POS		16
#define ESPI_M2SW1_SRC2_SEL_MASK	0x0f0000u
#define ESPI_M2SW1_SRC3_SEL_POS		24
#define ESPI_M2SW1_SRC3_SEL_MASK	0x0f000000u
/* 0 <= n < 4 */
#define ESPI_M2SW1_SRC_SEL_POS(n)	((n) * 8u)
#define ESPI_M2SW1_SRC_SEL_MASK(n)	SHLU32(0xfu, ((n) * 8u))
#define ESPI_M2SW1_SRC_SEL_VAL(n, v)	SHLU32(((v) & 0xfu), ((n) * 8u))
/* 32-bit word 2 (bits[95:64]) */
#define ESPI_M2SW2_OFS			8u
#define ESPI_M2SW2_SRC_MASK0		0x0fu
#define ESPI_M2SW2_SRC0_POS		0
#define ESPI_M2SW2_SRC0_MASK		0x0fu
#define ESPI_M2SW2_SRC1_POS		8u
#define ESPI_M2SW2_SRC1_MASK		0x0f00u
#define ESPI_M2SW2_SRC2_POS		16u
#define ESPI_M2SW2_SRC2_MASK		0x0f0000u
#define ESPI_M2SW2_SRC3_POS		24u
#define ESPI_M2SW2_SRC3_MASK		0x0f000000u
/* 0 <= n < 4 */
#define ESPI_M2SW2_SRC_POS(n)		((n) * 8u)
#define ESPI_M2SW2_SRC_MASK(n)		SHLU32(0xfu, ((n) * 8u))
#define ESPI_M2SW2_SRC_VAL(n, v)	SHLU32(((v) & 0xfu), ((n) * 8u))

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
#define ESPI_IRQ_SEL_REDGE	0x0du
#define ESPI_IRQ_SEL_FEDGE	0x0eu
#define ESPI_IRQ_SEL_BEDGE	0x0fu

/* Slave to Master VW register: 64-bit (2 32 bit registers) */
/* 32-bit word 0 (bits[31:0]) */
#define ESPI_S2MW0_OFS			0
#define ESPI_S2MW0_IDX_POS		0
#define ESPI_S2MW0_IDX_MASK		0xffu
#define ESPI_S2MW0_STOM_POS		8u
#define ESPI_S2MW0_STOM_SRC_POS		8u
#define ESPI_S2MW0_STOM_MASK0		0xf3u
#define ESPI_S2MW0_STOM_MASK		0xf300u
#define ESPI_S2MW0_STOM_SRC_MASK0	0x03u
#define ESPI_S2MW0_STOM_SRC_MASK	0x0300u
#define ESPI_S2MW0_STOM_SRC_ESPI_RST	0u
#define ESPI_S2MW0_STOM_SRC_SYS_RST	0x0100u
#define ESPI_S2MW0_STOM_SRC_SIO_RST	0x0200u
#define ESPI_S2MW0_STOM_SRC_PLTRST	0x0300u
#define ESPI_S2MW0_STOM_STATE_POS	12u
#define ESPI_S2MW0_STOM_STATE_MASK0	0x0fu
#define ESPI_S2MW0_STOM_STATE_MASK	0x0f000u
#define ESPI_S2MW0_CHG0_POS		16u
#define ESPI_S2MW0_CHG0			BIT(ESPI_S2MW0_CHG0_POS)
#define ESPI_S2MW0_CHG1_POS		17u
#define ESPI_S2MW0_CHG1			BIT(ESPI_S2MW0_CHG1_POS)
#define ESPI_S2MW0_CHG2_POS		18u
#define ESPI_S2MW0_CHG2			BIT(ESPI_S2MW0_CHG2_POS)
#define ESPI_S2MW0_CHG3_POS		19u
#define ESPI_S2MW0_CHG3			BIT(ESPI_S2MW0_CHG3_POS)
#define ESPI_S2MW0_CHG_ALL_POS		16u
#define ESPI_S2MW0_CHG_ALL_MASK0	0x0fu
#define ESPI_S2MW0_CHG_ALL_MASK		0x0f0000u
/* 0 <= n < 4 */
#define ESPI_S2MW1_CHG_POS(n)		((n) + 16u)
#define ESPI_S2MW1_CHG(v, n)		\
	(((uint32_t)(v) >> ESPI_S2MW1_CHG_POS(n)) & 0x01)

/* 32-bit word 1 (bits[63:32]) */
#define ESPI_S2MW1_OFS			4u
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

#define MEC_MSVW_SRC_IRQ_SEL_MASK0	0x0fu
#define MEC_MSVW_SRC0_IRQ_SEL_MASK	\
	SHLU32(MEC_MSVW_SRC_IRQ_SEL_MASK0, MEC_MSVW_SRC0_IRQ_SEL_POS)
#define MEC_MSVW_SRC1_IRQ_SEL_MASK	\
	SHLU32(MEC_MSVW_SRC_IRQ_SEL_MASK0, MEC_MSVW_SRC1_IRQ_SEL_POS)
#define MEC_MSVW_SRC2_IRQ_SEL_MASK	\
	SHLU32(MEC_MSVW_SRC_IRQ_SEL_MASK0, MEC_MSVW_SRC2_IRQ_SEL_POS)
#define MEC_MSVW_SRC3_IRQ_SEL_MASK	\
	SHLU32(MEC_MSVW_SRC_IRQ_SEL_MASK0, MEC_MSVW_SRC3_IRQ_SEL_POS)

#define MEC_MSVW_SRC_IRQ_SEL_LVL_LO	0x00u
#define MEC_MSVW_SRC_IRQ_SEL_LVL_HI	0x01u
#define MEC_MSVW_SRC_IRQ_SEL_DIS	0x04u
#define MEC_MSVW_SRC_IRQ_SEL_EDGE_FALL	0x0du
#define MEC_MSVW_SRC_IRQ_SEL_EDGE_RISE	0x0eu
#define MEC_MSVW_SRC_IRQ_SEL_EDGE_BOTH	0x0fu

/*
 * 0 <= src <= 3
 * isel = MEC_MSVW_SRC_IRQ_SEL_LVL_LO, ...
 */
#define MEC_MSVW_SRC_IRQ_SEL_VAL(src, isel)	\
	((uint32_t)(isel) << ((src) * 8u))

#define MEC_MSVW_SRC0_POS	0u
#define MEC_MSVW_SRC1_POS	8u
#define MEC_MSVW_SRC2_POS	16u
#define MEC_MSVW_SRC3_POS	24u

#define MEC_MSVW_SRC_MASK0	0x01u

#define MEC_MSVW_SRC0_MASK	BIT(0)
#define MEC_MSVW_SRC1_MASK	BIT(8)
#define MEC_MSVW_SRC2_MASK	BIT(16)
#define MEC_MSVW_SRC3_MASK	BIT(24)

/*
 * 0 <= src <= 3
 * val = 0 or 1
 */
#define MEC_MSVW_SRC_VAL(src, val)	\
	((uint32_t)(val & 0x01u) << ((src) * 8u))

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
	MSVW_IRQ_SEL_LVL_LO	= 0x00u,
	MSVW_IRQ_SEL_LVL_HI	= 0x01u,
	MSVW_IRQ_SEL_DIS	= 0x04u,
	MSVW_IRQ_SEL_EDGE_FALL	= 0x0du,
	MSVW_IRQ_SEL_EDGE_RISE	= 0x0eu,
	MSVW_IRQ_SEL_EDGE_BOTH	= 0x0fu
};

/* Used for both MSVW MTOS and SMVW STOM fields */
enum espi_vw_rst_src {
	VW_RST_SRC_ESPI_RESET = 0u,
	VW_RST_SRC_SYS_RESET,
	VW_RST_SRC_SIO_RESET,
	VW_RST_SRC_PLTRST,
};

enum espi_msvw_byte_idx {
	MSVW_BI_INDEX = 0,
	MSVW_BI_MTOS,
	MSVW_BI_RSVD2,
	MSVW_BI_RSVD3,
	MSVW_BI_IRQ_SEL0,
	MSVW_BI_IRQ_SEL1,
	MSVW_BI_IRQ_SEL2,
	MSVW_BI_IRQ_SEL3,
	MSVW_BI_SRC0,
	MSVW_BI_SRC1,
	MSVW_BI_SRC2,
	MSVW_BI_SRC3,
	MSVW_IDX_MAX
};

enum espi_smvw_byte_idx {
	SMVW_BI_INDEX = 0,
	SMVW_BI_STOM,
	SMVW_BI_SRC_CHG,
	SMVW_BI_RSVD3,
	SMVW_BI_SRC0,
	SMVW_BI_SRC1,
	SMVW_BI_SRC2,
	SMVW_BI_SRC3,
	SMVW_IDX_MAX
};

/** @brief eSPI 96-bit Host-to-Target Virtual Wire register */
struct espi_msvw_reg {
	volatile uint8_t INDEX;
	volatile uint8_t MTOS;
	uint8_t RSVD1[2];
	volatile uint32_t SRC_IRQ_SEL;
	volatile uint32_t SRC;
};

/** @brief eSPI 96-bit Host-to-Target Virtual Wire register as bytes */
struct espi_msvwb_reg {
	volatile uint8_t HTVWB[12];
};

/** @brief HW implements 11 Host-to-Target VW registers as an array */
struct espi_msvw_ar_regs {
	struct espi_msvw_reg MSVW[11];
};

/** @brief HW implements 11 Host-to-Target VW registers as named registers */
struct espi_msvw_named_regs {
	struct espi_msvw_reg MSVW00;
	struct espi_msvw_reg MSVW01;
	struct espi_msvw_reg MSVW02;
	struct espi_msvw_reg MSVW03;
	struct espi_msvw_reg MSVW04;
	struct espi_msvw_reg MSVW05;
	struct espi_msvw_reg MSVW06;
	struct espi_msvw_reg MSVW07;
	struct espi_msvw_reg MSVW08;
	struct espi_msvw_reg MSVW09;
	struct espi_msvw_reg MSVW10;
};

/** @brief eSPI M2S VW registers as an array of words at 0x400F9C00 */
struct espi_msvw32_regs {
	volatile uint32_t MSVW32[11 * 3];
};

/** @brief eSPI 64-bit Target-to-Host Virtual Wire register */
struct espi_smvw_reg {
	volatile uint8_t INDEX;
	volatile uint8_t STOM;
	volatile uint8_t SRC_CHG;
	uint8_t RSVD1[1];
	volatile uint32_t SRC;
};

/** @brief eSPI 64-bit Target-to-Host Virtual Wire register as bytes */
struct espi_smvwb_reg {
	volatile uint8_t THVWB[8];
};


/** @brief HW implements 11 Target-to-Host VW registers as an array */
struct espi_smvw_ar_regs {
	struct espi_smvw_reg SMVW[11];
};

/** @brief HW implements 11 Target-to-Host VW registers as named registers */
struct espi_smvw_named_regs {
	struct espi_smvw_reg SMVW00;
	struct espi_smvw_reg SMVW01;
	struct espi_smvw_reg SMVW02;
	struct espi_smvw_reg SMVW03;
	struct espi_smvw_reg SMVW04;
	struct espi_smvw_reg SMVW05;
	struct espi_smvw_reg SMVW06;
	struct espi_smvw_reg SMVW07;
	struct espi_smvw_reg SMVW08;
	struct espi_smvw_reg SMVW09;
	struct espi_smvw_reg SMVW10;
};

/** @brief eSPI S2M VW registers as an array of words at 0x400F9E00 */
struct espi_smvw32_regs {
	volatile uint32_t SMVW[11 * 2];
};

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _MEC172X_ESPI_VW_H */
