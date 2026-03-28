/*
 * Copyright (c) 2022 Andes Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_atcspi200);

#include "spi_context.h"
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>

#ifdef CONFIG_ANDES_SPI_DMA_MODE
#include <zephyr/drivers/dma.h>
#endif

#define REG_TFMAT		0x10
#define REG_TCTRL		0x20
#define REG_CMD			0x24
#define REG_DATA		0x2c
#define REG_CTRL		0x30
#define REG_STAT		0x34
#define REG_INTEN		0x38
#define REG_INTST		0x3c
#define REG_TIMIN		0x40
#define REG_CONFIG		0x7c

#define SPI_TFMAT(base)		(base + REG_TFMAT)
#define SPI_TCTRL(base)		(base + REG_TCTRL)
#define SPI_CMD(base)		(base + REG_CMD)
#define SPI_DATA(base)		(base + REG_DATA)
#define SPI_CTRL(base)		(base + REG_CTRL)
#define SPI_STAT(base)		(base + REG_STAT)
#define SPI_INTEN(base)		(base + REG_INTEN)
#define SPI_INTST(base)		(base + REG_INTST)
#define SPI_TIMIN(base)		(base + REG_TIMIN)
#define SPI_CONFIG(base)	(base + REG_CONFIG)

/* Field mask of SPI transfer format register */
#define TFMAT_DATA_LEN_OFFSET		(8)

#define TFMAT_CPHA_MSK			BIT(0)
#define TFMAT_CPOL_MSK			BIT(1)
#define TFMAT_SLVMODE_MSK		BIT(2)
#define TFMAT_LSB_MSK			BIT(3)
#define TFMAT_DATA_MERGE_MSK		BIT(7)
#define TFMAT_DATA_LEN_MSK		GENMASK(12, 8)
#define TFMAT_ADDR_LEN_MSK		GENMASK(18, 16)

/* Field mask of SPI transfer control register */
#define TCTRL_RD_TCNT_OFFSET		(0)
#define TCTRL_WR_TCNT_OFFSET		(12)
#define TCTRL_TRNS_MODE_OFFSET		(24)

#define TCTRL_WR_TCNT_MSK		GENMASK(20, 12)
#define TCTRL_TRNS_MODE_MSK		GENMASK(27, 24)

/* Transfer mode */
#define TRNS_MODE_WRITE_READ		(0)
#define TRNS_MODE_WRITE_ONLY		(1)
#define TRNS_MODE_READ_ONLY		(2)

/* Field mask of SPI interrupt enable register */
#define IEN_RX_FIFO_MSK			BIT(2)
#define IEN_TX_FIFO_MSK			BIT(3)
#define IEN_END_MSK			BIT(4)

/* Field mask of SPI interrupt status register */
#define INTST_RX_FIFO_INT_MSK		BIT(2)
#define INTST_TX_FIFO_INT_MSK		BIT(3)
#define INTST_END_INT_MSK		BIT(4)

/* Field mask of SPI config register */
#define CFG_RX_FIFO_SIZE_MSK		GENMASK(3, 0)
#define CFG_TX_FIFO_SIZE_MSK		GENMASK(7, 4)

/* Field mask of SPI status register */
#define STAT_RX_NUM_MSK			GENMASK(12, 8)
#define STAT_TX_NUM_MSK			GENMASK(20, 16)

/* Field mask of SPI control register */
#define CTRL_RX_FIFO_RST_OFFSET		(1)
#define CTRL_TX_FIFO_RST_OFFSET		(2)
#define CTRL_RX_THRES_OFFSET		(8)
#define CTRL_TX_THRES_OFFSET		(16)

#define CTRL_RX_FIFO_RST_MSK		BIT(1)
#define CTRL_TX_FIFO_RST_MSK		BIT(2)
#define CTRL_RX_DMA_EN_MSK		BIT(3)
#define CTRL_TX_DMA_EN_MSK		BIT(4)
#define CTRL_RX_THRES_MSK		GENMASK(12, 8)
#define CTRL_TX_THRES_MSK		GENMASK(20, 16)

/* Field mask of SPI status register */
#define TIMIN_SCLK_DIV_MSK		GENMASK(7, 0)

#define TX_FIFO_THRESHOLD		(1)
#define RX_FIFO_THRESHOLD		(1)
#define MAX_TRANSFER_CNT		(512)
#define	MAX_CHAIN_SIZE			(8)

#define TX_FIFO_SIZE_SETTING(base)	\
	(sys_read32(SPI_CONFIG(base)) & CFG_TX_FIFO_SIZE_MSK)
#define TX_FIFO_SIZE(base)		\
	(2 << (TX_FIFO_SIZE_SETTING(base) >> 4))

#define RX_FIFO_SIZE_SETTING(base)	\
	(sys_read32(SPI_CONFIG(base)) & CFG_RX_FIFO_SIZE_MSK)
#define RX_FIFO_SIZE(base)		\
	(2 << (RX_FIFO_SIZE_SETTING(base) >> 0))

#define TX_NUM_STAT(base)	(sys_read32(SPI_STAT(base)) & STAT_TX_NUM_MSK)
#define RX_NUM_STAT(base)	(sys_read32(SPI_STAT(base)) & STAT_RX_NUM_MSK)
#define GET_TX_NUM(base)	(TX_NUM_STAT(base) >> 16)
#define GET_RX_NUM(base)	(RX_NUM_STAT(base) >> 8)
