/*
 * Copyright (c) 2025 Realtek, SIBG-SD7
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _REALTEK_RTS5912_REG_SPIC_H
#define _REALTEK_RTS5912_REG_SPIC_H

/**
 * @brief SPIC Controller (SPIC)
 */

struct reg_spic_reg {
	uint32_t CTRL0;
	uint32_t RXNDF;
	uint32_t SSIENR;
	const uint32_t RESERVED;
	uint32_t SER;
	uint32_t BAUDR;
	uint32_t TXFTLR;
	uint32_t RXFTLR;
	uint32_t TXFLR;
	uint32_t RXFLR;
	uint32_t SR;
	uint32_t IMR;
	uint32_t ISR;
	uint32_t RISR;
	uint32_t TXOICR;
	uint32_t RXOICR;
	uint32_t RXUICR;
	uint32_t MSTICR;
	uint32_t ICR;
	const uint32_t RESERVED1[5];
	union {
		uint8_t BYTE;
		uint16_t HALF;
		uint32_t WORD;
	} DR;
	const uint32_t RESERVED2[43];
	uint32_t CTRLR2;
	uint32_t FBAUD;
	uint32_t USERLENGTH;
	const uint32_t RESERVED3[3];
	uint32_t FLUSH;
	const uint32_t RESERVED4;
	uint32_t TXNDF;
};

/* CTRL0 */
#define SPIC_CTRL0_SIPOL_Pos           (0UL)
#define SPIC_CTRL0_SIPOL_Msk           GENMASK(4, 0)
#define SPIC_CTRL0_SCPH                BIT(6UL)
#define SPIC_CTRL0_SCPOL               BIT(7UL)
#define SPIC_CTRL0_TMOD_Pos            (8UL)
#define SPIC_CTRL0_TMOD_Msk            GENMASK(9, 8)
#define SPIC_CTRL0_ADDRCH_Pos          (16UL)
#define SPIC_CTRL0_ADDRCH_Msk          GENMASK(17, 16)
#define SPIC_CTRL0_DATACH_Pos          (18UL)
#define SPIC_CTRL0_DATACH_Msk          GENMASK(19, 18)
#define SPIC_CTRL0_CMDCH_Pos           (20UL)
#define SPIC_CTRL0_CMDCH_Msk           GENMASK(21, 20)
#define SPIC_CTRL0_CK_MTIMES_POS       (23UL)
#define SPIC_CTRL0_CK_MTIMES_Msk       GENMASK(27, 23)
#define SPIC_CTRL0_USERMD              BIT(31UL)
/* RXNDF */
#define SPIC_RXNDF_NUM_Pos             (0UL)
#define SPIC_RXNDF_NUM_Msk             GENMASK(23, 0)
/* SSIENR */
#define SPIC_SSIENR_SPICEN             BIT(0UL)
#define SPIC_SSIENR_ATCKCMD            BIT(1UL)
/* SER */
#define SPIC_SER_SEL                   BIT(0UL)
/* BAUDR */
#define SPIC_BAUDR_SCKDV_Pos           (0UL)
#define SPIC_BAUDR_SCKDV_Msk           GENMASK(11, 0)
/* SR */
#define SPIC_SR_BUSY                   BIT(0UL)
#define SPIC_SR_TFNF                   BIT(1UL)
#define SPIC_SR_TFE                    BIT(2UL)
#define SPIC_SR_RFNE                   BIT(3UL)
#define SPIC_SR_RFF                    BIT(4UL)
#define SPIC_SR_TXE                    BIT(5UL)
/* IMR */
#define SPIC_IMR_TXEIM                 BIT(0UL)
#define SPIC_IMR_TXOIM                 BIT(1UL)
#define SPIC_IMR_RXUIM                 BIT(2UL)
#define SPIC_IMR_RXOIM                 BIT(3UL)
#define SPIC_IMR_RXFIM                 BIT(4UL)
#define SPIC_IMR_FSEIM                 BIT(5UL)
#define SPIC_IMR_USSIM                 BIT(9UL)
#define SPIC_IMR_TFSIM                 BIT(10UL)
/* ISR */
#define SPIC_ISR_TXEIS                 BIT(0UL)
#define SPIC_ISR_TXOIS                 BIT(1UL)
#define SPIC_ISR_RXUIS                 BIT(2UL)
#define SPIC_ISR_RXOIS                 BIT(3UL)
#define SPIC_ISR_RXFIS                 BIT(4UL)
#define SPIC_ISR_FSEIS                 BIT(5UL)
#define SPIC_ISR_USEIS                 BIT(9UL)
#define SPIC_ISR_TFSIS                 BIT(10UL)
/* RISR */
#define SPIC_RISR_TXEIR                BIT(0UL)
#define SPIC_RISR_TXOIR                BIT(1UL)
#define SPIC_RISR_RXUIR                BIT(2UL)
#define SPIC_RISR_RXOIR                BIT(3UL)
#define SPIC_RISR_RXFIR                BIT(4UL)
#define SPIC_RISR_FSEIR                BIT(5UL)
#define SPIC_RISR_USEIR                BIT(9UL)
#define SPIC_RISR_TFSIR                BIT(10UL)
/* CTRLR2 */
#define SPIC_CTRLR2_WPN_SET            BIT(1UL)
/* USERLENGTH */
#define SPIC_USERLENGTH_RDDUMMYLEN_Pos (0UL)
#define SPIC_USERLENGTH_RDDUMMYLEN_Msk GENMASK(11, 0)
#define SPIC_USERLENGTH_CMDLEN_Pos     (12UL)
#define SPIC_USERLENGTH_CMDLEN_Msk     GENMASK(13, 12)
#define SPIC_USERLENGTH_ADDRLEN_Pos    (16UL)
#define SPIC_USERLENGTH_ADDRLEN_Msk    GENMASK(19, 16)
/* FLUSH */
#define SPIC_FLUSH_ALL                 BIT(0UL)
#define SPIC_FLUSH_DRFIFO              BIT(1UL)
#define SPIC_FLUSH_STFIFO              BIT(2UL)
/* TXNDF */
#define SPIC_TXNDF_NUM_Pos             (0UL)
#define SPIC_TXNDF_NUM_Msk             GENMASK(23, 0)

#endif /* _REALTEK_RTS5912_REG_SPIC_H */
