/*
 * Copyright (c) 2026 Realtek Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SPI_SPI_RTS5817_DW_REG_H_
#define ZEPHYR_DRIVERS_SPI_SPI_RTS5817_DW_REG_H_

#define R_SSI_CTRLR0        0x00
#define R_SSI_CTRLR1        0x04
#define R_SSI_SSIENR        0x08
#define R_SSI_MWCR          0x0C
#define R_SSI_SER           0x10
#define R_SSI_BAUDR         0x14
#define R_SSI_TXFTLR        0x18
#define R_SSI_RXFTLR        0x1C
#define R_SSI_TXFLR         0x20
#define R_SSI_RXFLR         0x24
#define R_SSI_SR            0x28
#define R_SSI_IMR           0x2C
#define R_SSI_ISR           0x30
#define R_SSI_RISR          0x34
#define R_SSI_TXOICR        0x38
#define R_SSI_RXOICR        0x3C
#define R_SSI_RXUICR        0x40
#define R_SSI_MSTICR        0x44
#define R_SSI_ICR           0x48
#define R_SSI_DMACR         0x4C
#define R_SSI_DMATDLR       0x50
#define R_SSI_DMARDLR       0x54
#define R_SSI_IDR           0x58
#define R_SSI_COMP_VERSION  0x5C
#define R_SSI_DR            0x60
#define R_SSI_RX_SAMPLE_DLY 0xF0

/* Bits of SSI_CTRLR0 (0x00) */

#define DFS_OFFSET 0
#define DFS_MASK   GENMASK(3, 0)

#define FRF_OFFSET 4
#define FRF_MASK   GENMASK(5, 4)

#define SCPH_OFFSET 6
#define SCPH_MASK   BIT(6)

#define SCPOL_OFFSET 7
#define SCPOL_MASK   BIT(7)

#define TMOD_OFFSET 8
#define TMOD_MASK   GENMASK(9, 8)

#define SLV_OE_OFFSET 10
#define SLV_OE_MASK   BIT(10)

#define SRL_OFFSET 11
#define SRL_MASK   BIT(11)

#define CFS_OFFSET 12
#define CFS_MASK   GENMASK(15, 12)

#define DFS_32_OFFSET 16
#define DFS_32_MASK   GENMASK(20, 16)

#define SPI_FRF_OFFSET 21
#define SPI_FRF_MASK   GENMASK(22, 21)

#define SSTE_OFFSET 24
#define SSTE_MASK   BIT(24)

#define SECONV_OFFSET 25
#define SECONV_MASK   BIT(25)

/* Bits of R_SSI_SSIENR (0x008) */

#define SSI_EN_OFFSET 0
#define SSI_EN_MASK   BIT(0)

/* Bits of R_SSI_SER (0x10) */

#define SER_OFFSET 0
#define SER_MASK   BIT(0)

#endif /* ZEPHYR_DRIVERS_SPI_SPI_RTS5817_DW_REG_H_ */
