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

typedef struct {
	volatile uint32_t CTRL0;
	volatile uint32_t RXNDF;
	volatile uint32_t SSIENR;
	volatile const uint32_t RESERVED;
	volatile uint32_t SER;
	volatile uint32_t BAUDR;
	volatile uint32_t TXFTLR;
	volatile uint32_t RXFTLR;
	volatile uint32_t TXFLR;
	volatile uint32_t RXFLR;
	volatile uint32_t SR;
	volatile uint32_t IMR;
	volatile uint32_t ISR;
	volatile uint32_t RISR;
	volatile uint32_t TXOICR;
	volatile uint32_t RXOICR;
	volatile uint32_t RXUICR;
	volatile uint32_t MSTICR;
	volatile uint32_t ICR;
	volatile const uint32_t RESERVED1[5];
	union {
		volatile uint8_t BYTE;
		volatile uint16_t HALF;
		volatile uint32_t WORD;
	} DR;
	volatile const uint32_t RESERVED2[44];
	volatile uint32_t FBAUD;
	volatile uint32_t USERLENGTH;
	volatile const uint32_t RESERVED3[3];
	volatile uint32_t FLUSH;
	volatile const uint32_t RESERVED4;
	volatile uint32_t TXNDF;
} SPIC_Type;

/* CTRL0 */
#define SPIC_CTRL0_SCPH_Pos            (6UL)
#define SPIC_CTRL0_SCPH_Msk            BIT(SPIC_CTRL0_SCPH_Pos)
#define SPIC_CTRL0_SCPOL_Pos           (7UL)
#define SPIC_CTRL0_SCPOL_Msk           BIT(SPIC_CTRL0_SCPOL_Pos)
#define SPIC_CTRL0_TMOD_Pos            (8UL)
#define SPIC_CTRL0_TMOD_Msk            GENMASK(9, 8)
#define SPIC_CTRL0_ADDRCH_Pos          (16UL)
#define SPIC_CTRL0_ADDRCH_Msk          GENMASK(17, 16)
#define SPIC_CTRL0_DATACH_Pos          (18UL)
#define SPIC_CTRL0_DATACH_Msk          GENMASK(19, 18)
#define SPIC_CTRL0_CMDCH_Pos           (20UL)
#define SPIC_CTRL0_CMDCH_Msk           GENMASK(21, 20)
#define SPIC_CTRL0_USERMD_Pos          (31UL)
#define SPIC_CTRL0_USERMD_Msk          BIT(SPIC_CTRL0_USERMD_Pos)
/* RXNDF */
#define SPIC_RXNDF_NUM_Pos             (0UL)
#define SPIC_RXNDF_NUM_Msk             GENMASK(23, 0)
/* SSIENR */
#define SPIC_SSIENR_SPICEN_Pos         (0UL)
#define SPIC_SSIENR_SPICEN_Msk         BIT(SPIC_SSIENR_SPICEN_Pos)
#define SPIC_SSIENR_ATCKCMD_Pos        (1UL)
#define SPIC_SSIENR_ATCKCMD_Msk        BIT(SPIC_SSIENR_ATCKCMD_Pos)
/* SER */
#define SPIC_SER_SEL_Pos               (0UL)
#define SPIC_SER_SEL_Msk               BIT(SPIC_SER_SEL_Pos)
/* BAUDR */
#define SPIC_BAUDR_SCKDV_Pos           (0UL)
#define SPIC_BAUDR_SCKDV_Msk           GENMASK(11, 0)
/* SR */
#define SPIC_SR_BUSY_Pos               (0UL)
#define SPIC_SR_BUSY_Msk               BIT(SPIC_SR_BUSY_Pos)
#define SPIC_SR_TFNF_Pos               (1UL)
#define SPIC_SR_TFNF_Msk               BIT(SPIC_SR_TFNF_Pos)
#define SPIC_SR_TFE_Pos                (2UL)
#define SPIC_SR_TFE_Msk                BIT(SPIC_SR_TFE_Pos)
#define SPIC_SR_RFNE_Pos               (3UL)
#define SPIC_SR_RFNE_Msk               BIT(SPIC_SR_RFNE_Pos)
#define SPIC_SR_RFF_Pos                (4UL)
#define SPIC_SR_RFF_Msk                BIT(SPIC_SR_RFF_Pos)
#define SPIC_SR_TXE_Pos                (5UL)
#define SPIC_SR_TXE_Msk                BIT(SPIC_SR_TXE_Pos)
/* IMR */
#define SPIC_IMR_TXEIM_Pos             (0UL)
#define SPIC_IMR_TXEIM_Msk             BIT(SPIC_IMR_TXEIM_Pos)
#define SPIC_IMR_TXOIM_Pos             (1UL)
#define SPIC_IMR_TXOIM_Msk             BIT(SPIC_IMR_TXOIM_Pos)
#define SPIC_IMR_RXUIM_Pos             (2UL)
#define SPIC_IMR_RXUIM_Msk             BIT(SPIC_IMR_RXUIM_Pos)
#define SPIC_IMR_RXOIM_Pos             (3UL)
#define SPIC_IMR_RXOIM_Msk             BIT(SPIC_IMR_RXOIM_Pos)
#define SPIC_IMR_RXFIM_Pos             (4UL)
#define SPIC_IMR_RXFIM_Msk             BIT(SPIC_IMR_RXFIM_Pos)
#define SPIC_IMR_FSEIM_Pos             (5UL)
#define SPIC_IMR_FSEIM_Msk             BIT(SPIC_IMR_FSEIM_Pos)
#define SPIC_IMR_USSIM_Pos             (9UL)
#define SPIC_IMR_USSIM_Msk             BIT(SPIC_IMR_USSIM_Pos)
#define SPIC_IMR_TFSIM_Pos             (10UL)
#define SPIC_IMR_TFSIM_Msk             BIT(SPIC_IMR_TFSIM_Pos)
/* ISR */
#define SPIC_ISR_TXEIS_Pos             (0UL)
#define SPIC_ISR_TXEIS_Msk             BIT(SPIC_ISR_TXEIS_Pos)
#define SPIC_ISR_TXOIS_Pos             (1UL)
#define SPIC_ISR_TXOIS_Msk             BIT(SPIC_ISR_TXOIS_Pos)
#define SPIC_ISR_RXUIS_Pos             (2UL)
#define SPIC_ISR_RXUIS_Msk             BIT(SPIC_ISR_RXUIS_Pos)
#define SPIC_ISR_RXOIS_Pos             (3UL)
#define SPIC_ISR_RXOIS_Msk             BIT(SPIC_ISR_RXOIS_Pos)
#define SPIC_ISR_RXFIS_Pos             (4UL)
#define SPIC_ISR_RXFIS_Msk             BIT(SPIC_ISR_RXFIS_Pos)
#define SPIC_ISR_FSEIS_Pos             (5UL)
#define SPIC_ISR_FSEIS_Msk             BIT(SPIC_ISR_FSEIS_Pos)
#define SPIC_ISR_USEIS_Pos             (9UL)
#define SPIC_ISR_USEIS_Msk             BIT(SPIC_ISR_USEIS_Pos)
#define SPIC_ISR_TFSIS_Pos             (10UL)
#define SPIC_ISR_TFSIS_Msk             BIT(SPIC_ISR_TFSIS_Pos)
/* RISR */
#define SPIC_RISR_TXEIR_Pos            (0UL)
#define SPIC_RISR_TXEIR_Msk            BIT(SPIC_RISR_TXEIR_Pos)
#define SPIC_RISR_TXOIR_Pos            (1UL)
#define SPIC_RISR_TXOIR_Msk            BIT(SPIC_RISR_TXOIR_Pos)
#define SPIC_RISR_RXUIR_Pos            (2UL)
#define SPIC_RISR_RXUIR_Msk            BIT(SPIC_RISR_RXUIR_Pos)
#define SPIC_RISR_RXOIR_Pos            (3UL)
#define SPIC_RISR_RXOIR_Msk            BIT(SPIC_RISR_RXOIR_Pos)
#define SPIC_RISR_RXFIR_Pos            (4UL)
#define SPIC_RISR_RXFIR_Msk            BIT(SPIC_RISR_RXFIR_Pos)
#define SPIC_RISR_FSEIR_Pos            (5UL)
#define SPIC_RISR_FSEIR_Msk            BIT(SPIC_RISR_FSEIR_Pos)
#define SPIC_RISR_USEIR_Pos            (9UL)
#define SPIC_RISR_USEIR_Msk            BIT(SPIC_RISR_USEIR_Pos)
#define SPIC_RISR_TFSIR_Pos            (10UL)
#define SPIC_RISR_TFSIR_Msk            BIT(SPIC_RISR_TFSIR_Pos)
/* USERLENGTH */
#define SPIC_USERLENGTH_RDDUMMYLEN_Pos (0UL)
#define SPIC_USERLENGTH_RDDUMMYLEN_Msk GENMASK(11, 0)
#define SPIC_USERLENGTH_CMDLEN_Pos     (12UL)
#define SPIC_USERLENGTH_CMDLEN_Msk     GENMASK(13, 12)
#define SPIC_USERLENGTH_ADDRLEN_Pos    (16UL)
#define SPIC_USERLENGTH_ADDRLEN_Msk    GENMASK(19, 16)
/* FLUSH */
#define SPIC_FLUSH_ALL_Pos             (0UL)
#define SPIC_FLUSH_ALL_Msk             BIT(SPIC_FLUSH_ALL_Pos)
#define SPIC_FLUSH_DRFIFO_Pos          (1UL)
#define SPIC_FLUSH_DRFIFO_Msk          BIT(SPIC_FLUSH_DRFIFO_Pos)
#define SPIC_FLUSH_STFIFO_Pos          (2UL)
#define SPIC_FLUSH_STFIFO_Msk          BIT(SPIC_FLUSH_STFIFO_Pos)
/* TXNDF */
#define SPIC_TXNDF_NUM_Pos             (0UL)
#define SPIC_TXNDF_NUM_Msk             GENMASK(23, 0)

#endif /* _REALTEK_RTS5912_REG_SPIC_H */
