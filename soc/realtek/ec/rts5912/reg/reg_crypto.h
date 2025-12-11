/*
 * Copyright (c) 2025 Realtek, SIBG-SD7
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_REALTEK_RTS5912_REG_CRYPTO_H
#define ZEPHYR_SOC_REALTEK_RTS5912_REG_CRYPTO_H

struct sha2_type {
	uint32_t ctrl;
	const uint32_t status;
	const uint32_t reserved[2];
	uint32_t digest[16];
	uint32_t fifo_in;
};

struct sha2dma_type {
	uint32_t sar;
	const uint32_t reserved;
	uint32_t dar;
	const uint32_t reserved1[3];
	uint32_t ctrl_low;
	uint32_t ctrl_high;
	const uint32_t reserved2[9];
	uint32_t config;
	const uint32_t reserved3[178];
	uint32_t msk_transfer;
	const uint32_t reserved4;
	uint32_t msk_block;
	const uint32_t reserved5[7];
	uint32_t clear_transfer;
	const uint32_t reserved6[9];
	const uint32_t interrupt_status;
	const uint32_t reserved7[13];
	uint32_t dma_enable;
	const uint32_t reserved8;
	uint32_t channel_enable;
};

/* SHA_Type Start */
/* CTRL */
#define SHA2_CTRL_RST_Msk      BIT(1)
#define SHA2_CTRL_DMAMD_Msk    BIT(2)
#define SHA2_CTRL_BYTEINV_Msk  BIT(3)
#define SHA2_CTRL_SHAMD_Msk    BIT(4)
#define SHA2_CTRL_ICGEN_Msk    BIT(5)
#define SHA2_CTRL_SLVMD_Msk    BIT(6)
/* STS */
#define SHA2_STS_FIFOSPACE_Msk GENMASK(5, 0)
#define SHA2_STS_BUSY_Msk      BIT(6)
/* SHA_Type End */

/* SHA2DMA_Type Start */
/* SHA2DMA_Type Control */
#define SHA2DMA_CTRL_INT_EN_Msk         BIT(0)
#define SHA2DMA_CTRL_DST_WIDTH_Pos      (1UL)
#define SHA2DMA_CTRL_SRC_WIDTH_Pos      (4UL)
#define SHA2DMA_CTRL_SRC_BURST_Pos      (14UL)
/* SHA2DMA_Type Enable*/
#define SHA2DMA_DMA_ENABLE_Msk          BIT(0)
/* SHA2DMA_Type interrupt clear */
#define SHA2DMA_INTCLR_CLRTFR_Msk       BIT(0)
/* SHA2DMA_Type mask interrupt */
#define SHA2DMA_MSKTFR_INT_EN_Msk       BIT(0)
#define SHA2DMA_MSKTFR_INT_WRITE_EN_Msk BIT(8)
/* SHA2DMA_Type interrupt status */
#define SHA2DMA_INTSTS_TFR_COMPLETE_Msk BIT(0)
#define SHA2DMA_INTSTS_BLK_COMPLETE_Msk BIT(1)
#define SHA2DMA_INTSTS_SCR_COMPLETE_Msk BIT(2)
#define SHA2DMA_INTSTS_DST_COMPLETE_Msk BIT(3)
#define SHA2DMA_INTSTS_BUS_COMPLETE_Msk BIT(4)
/* SHA2DMA_Type Channel enable */
#define SHA2DMA_CHEN_EN_Msk             BIT(0)
#define SHA2DMA_CHEN_WRITE_EN_Msk       BIT(8)
/* SHA2DMA_Type End */

#endif /* ZEPHYR_SOC_REALTEK_RTS5912_REG_CRYPTO_H */
