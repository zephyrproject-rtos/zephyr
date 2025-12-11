/*
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MEC_PECI_H
#define _MEC_PECI_H

#include <stdint.h>
#include <stddef.h>

/* Write Data register */
#define MCHP_PECI_WR_DATA_REG_OFS	0u
#define MCHP_PECI_WR_DATA_MASK		0xffu

/* Read Data register */
#define MCHP_PECI_RD_DATA_REG_OFS	4u
#define MCHP_PECI_RD_DATA_MASK		0xffu

/* Control register */
#define MCHP_PECI_CTRL_REG_OFS		8u
#define MCHP_PECI_CTRL_MASK		0xe9u
#define MCHP_PECI_CTRL_PD_POS		0
#define MCHP_PECI_CTRL_PD		BIT(MCHP_PECI_CTRL_PD_POS)
#define MCHP_PECI_CTRL_RST_POS		3
#define MCHP_PECI_CTRL_RST		BIT(MCHP_PECI_CTRL_RST_POS)
#define MCHP_PECI_CTRL_FRST_POS		5
#define MCHP_PECI_CTRL_FRST		BIT(MCHP_PECI_CTRL_FRST_POS)
#define MCHP_PECI_CTRL_TXEN_POS		6
#define MCHP_PECI_CTRL_TXEN		BIT(MCHP_PECI_CTRL_TXEN_POS)
#define MCHP_PECI_CTRL_MIEN_POS		7
#define MCHP_PECI_CTRL_MIEN		BIT(MCHP_PECI_CTRL_MIEN_POS)

/* Status 1 register. RW1C and read-only bits. */
#define MCHP_PECI_STS1_REG_OFS		0x0cu
#define MCHP_PECI_STS1_MASK		0xbfu
#define MCHP_PECI_STS1_BOF_POS		0
#define MCHP_PECI_STS1_BOF		BIT(MCHP_PECI_STS1_BOF_POS)
#define MCHP_PECI_STS1_EOF_POS		1
#define MCHP_PECI_STS1_EOF		BIT(MCHP_PECI_STS1_EOF_POS)
/* Error is read-only */
#define MCHP_PECI_STS1_ERR_POS		2
#define MCHP_PECI_STS1_ERR		BIT(MCHP_PECI_STS1_ERR_POS)
/* Ready is read-only */
#define MCHP_PECI_STS1_RDY_POS		3
#define MCHP_PECI_STS1_RDY		BIT(MCHP_PECI_STS1_RDY_POS)
#define MCHP_PECI_STS1_RDYLO_POS	4
#define MCHP_PECI_STS1_RDYLO		BIT(MCHP_PECI_STS1_RDYLO_POS)
#define MCHP_PECI_STS1_RDYHI_POS	5
#define MCHP_PECI_STS1_RDYHI		BIT(MCHP_PECI_STS1_RDYHI_POS)
/* MINT is read-only */
#define MCHP_PECI_STS1_MINT_POS		7
#define MCHP_PECI_STS1_MINT		BIT(MCHP_PECI_STS1_MINT_POS)

/* Status 2 register. Read-only bits. */
#define MCHP_PECI_STS2_REG_OFS		0x10u
#define MCHP_PECI_STS2_MASK		0x8fu
#define MCHP_PECI_STS2_WFF_POS		0
#define MCHP_PECI_STS2_WFF		BIT(MCHP_PECI_STS2_WFF_POS)
#define MCHP_PECI_STS2_WFE_POS		1
#define MCHP_PECI_STS2_WFE		BIT(MCHP_PECI_STS2_WFE_POS)
#define MCHP_PECI_STS2_RFF_POS		2
#define MCHP_PECI_STS2_RFF		BIT(MCHP_PECI_STS2_RFF_POS)
#define MCHP_PECI_STS2_RFE_POS		3
#define MCHP_PECI_STS2_RFE		BIT(MCHP_PECI_STS2_RFE_POS)
#define MCHP_PECI_STS2_IDLE_POS		7
#define MCHP_PECI_STS2_IDLE		BIT(MCHP_PECI_STS2_IDLE_POS)

/* Error register. R/W1C bits. */
#define MCHP_PECI_ERR_REG_OFS		0x14u
#define MCHP_PECI_ERR_MASK		0xf3u
#define MCHP_PECI_ERR_FERR_POS		0
#define MCHP_PECI_ERR_FERR		BIT(MCHP_PECI_ERR_FERR_POS)
#define MCHP_PECI_ERR_BERR_POS		1
#define MCHP_PECI_ERR_BERR		BIT(MCHP_PECI_ERR_BERR_POS)
#define MCHP_PECI_ERR_WROV_POS		4
#define MCHP_PECI_ERR_WROV		BIT(MCHP_PECI_ERR_WROV_POS)
#define MCHP_PECI_ERR_WRUN_POS		5
#define MCHP_PECI_ERR_WRUN		BIT(MCHP_PECI_ERR_WRUN_POS)
#define MCHP_PECI_ERR_RDOV_POS		6
#define MCHP_PECI_ERR_RDOV		BIT(MCHP_PECI_ERR_RDOV_POS)
#define MCHP_PECI_ERR_CLK_POS		7
#define MCHP_PECI_ERR_CLK		BIT(MCHP_PECI_ERR_CLK_POS)

/* Interrupt Enable 1 register. */
#define MCHP_PECI_IEN1_REG_OFS		0x18u
#define MCHP_PECI_IEN1_MASK		0x37u
#define MCHP_PECI_IEN1_BIEN_POS		0
#define MCHP_PECI_IEN1_BIEN		BIT(MCHP_PECI_IEN1_BIEN_POS)
#define MCHP_PECI_IEN1_EIEN_POS		1
#define MCHP_PECI_IEN1_EIEN		BIT(MCHP_PECI_IEN1_EIEN_POS)
#define MCHP_PECI_IEN1_EREN_POS		2
#define MCHP_PECI_IEN1_EREN		BIT(MCHP_PECI_IEN1_EREN_POS)
#define MCHP_PECI_IEN1_RLEN_POS		4
#define MCHP_PECI_IEN1_RLEN		BIT(MCHP_PECI_IEN1_RLEN_POS)
#define MCHP_PECI_IEN1_RHEN_POS		5
#define MCHP_PECI_IEN1_RHEN		BIT(MCHP_PECI_IEN1_RHEN_POS)

/* Interrupt Enable 2 register. */
#define MCHP_PECI_IEN2_REG_OFS		0x1cu
#define MCHP_PECI_IEN2_MASK		0x06u
#define MCHP_PECI_IEN2_ENWFE_POS	1
#define MCHP_PECI_IEN2_ENWFE		BIT(MCHP_PECI_IEN2_ENWFE_POS)
#define MCHP_PECI_IEN2_ENRFF_POS	2
#define MCHP_PECI_IEN2_ENRFF		BIT(MCHP_PECI_IEN2_ENRFF_POS)

/* Optimal Bit Time LSB register. */
#define MCHP_PECI_OPT_BT_LSB_REG_OFS	0x20u
#define MCHP_PECI_OPT_BT_LSB_MASK	0xffu

/* Optimal Bit Time MSB register. */
#define MCHP_PECI_OPT_BT_MSB_REG_OFS	0x24u
#define MCHP_PECI_OPT_BT_MSB_MASK	0xffu

/** @brief PECI controller. Size = 76(0x4c) */
struct peci_regs {
	volatile uint8_t WR_DATA;
	uint8_t RSVD1[3];
	volatile uint8_t RD_DATA;
	uint8_t RSVD2[3];
	volatile uint8_t CONTROL;
	uint8_t RSVD3[3];
	volatile uint8_t STATUS1;
	uint8_t RSVD4[3];
	volatile uint8_t STATUS2;
	uint8_t RSVD5[3];
	volatile uint8_t ERROR;
	uint8_t RSVD6[3];
	volatile uint8_t INT_EN1;
	uint8_t RSVD7[3];
	volatile uint8_t INT_EN2;
	uint8_t RSVD8[3];
	volatile uint8_t OPT_BIT_TIME_LSB;
	uint8_t RSVD9[3];
	volatile uint8_t OPT_BIT_TIME_MSB;
	uint8_t RSVD10[3];
	volatile uint8_t REQ_TMR_LSB;
	uint8_t RSVD11[3];
	volatile uint8_t REQ_TMR_MSB;
	uint8_t RSVD12[3];
	volatile uint8_t BAUD_CTRL;
	uint8_t RSVD13[3];
	uint32_t RSVD14[3];
	volatile uint8_t BLK_ID;
	uint8_t RSVD15[3];
	volatile uint8_t BLK_REV;
	uint8_t RSVD16[3];
	volatile uint8_t SST_CTL1;
	uint8_t RSVD17[3];
};

#endif	/* #ifndef _MEC_PECI_H */
