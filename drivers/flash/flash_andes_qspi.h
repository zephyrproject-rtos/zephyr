/*
 * Copyright (c) 2023 Andes Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Flash opcodes */
#define FLASH_ANDES_CMD_WRSR	0x01    /* Write status register */
#define FLASH_ANDES_CMD_RDSR	0x05    /* Read status register */
#define FLASH_ANDES_CMD_READ	0x03    /* Read data */
#define FLASH_ANDES_CMD_4READ	0xEB    /* Quad mode Read data*/
#define FLASH_ANDES_CMD_WREN	0x06    /* Write enable */
#define FLASH_ANDES_CMD_WRDI	0x04    /* Write disable */
#define FLASH_ANDES_CMD_PP	0x02    /* Page program */
#define FLASH_ANDES_CMD_4PP	0x38    /* Quad mode page program*/
#define FLASH_ANDES_CMD_SE	0x20    /* Sector erase */
#define FLASH_ANDES_CMD_BE_32K	0x52    /* Block erase 32KB */
#define FLASH_ANDES_CMD_BE	0xD8    /* Block erase */
#define FLASH_ANDES_CMD_CE	0xC7    /* Chip erase */
#define FLASH_ANDES_CMD_RDID	0x9F    /* Read JEDEC ID */
#define FLASH_ANDES_CMD_ULBPR	0x98    /* Global Block Protection Unlock */
#define FLASH_ANDES_CMD_DPD	0xB9    /* Deep Power Down */
#define FLASH_ANDES_CMD_RDPD	0xAB    /* Release from Deep Power Down */

/* Status register bits */
#define FLASH_ANDES_WIP_BIT	BIT(0)  /* Write in progress */
#define FLASH_ANDES_WEL_BIT	BIT(1)  /* Write enable latch */
#define FLASH_ANDES_QE_BIT	BIT(6)

#define QSPI_TFMAT(base)	(base + 0x10)
#define QSPI_TCTRL(base)	(base + 0x20)
#define QSPI_CMD(base)		(base + 0x24)
#define QSPI_ADDR(base)		(base + 0x28)
#define QSPI_DATA(base)		(base + 0x2c)
#define QSPI_CTRL(base)		(base + 0x30)
#define QSPI_STAT(base)		(base + 0x34)
#define QSPI_INTEN(base)	(base + 0x38)
#define QSPI_INTST(base)	(base + 0x3c)
#define QSPI_TIMIN(base)	(base + 0x40)
#define QSPI_CONFIG(base)	(base + 0x7c)

/* Field mask of SPI transfer format register */
#define TFMAT_DATA_LEN_OFFSET		(8)
#define TFMAT_ADDR_LEN_OFFSET		(16)

#define TFMAT_SLVMODE_MSK		BIT(2)
#define TFMAT_DATA_MERGE_MSK		BIT(7)
#define TFMAT_DATA_LEN_MSK		GENMASK(12, 8)

/* Field mask of SPI transfer control register */
#define TCTRL_RD_TCNT_OFFSET		(0)
#define TCTRL_DUMMY_CNT_OFFSET		(9)
#define TCTRL_WR_TCNT_OFFSET		(12)
#define TCTRL_DUAL_MODE_OFFSET		(22)
#define TCTRL_TRNS_MODE_OFFSET		(24)

#define TCTRL_TRNS_MODE_MSK		GENMASK(27, 24)
#define TCTRL_ADDR_FMT_MSK		BIT(28)
#define TCTRL_ADDR_EN_MSK		BIT(29)
#define TCTRL_CMD_EN_MSK		BIT(30)

/* Transfer mode */
#define TRNS_MODE_WRITE_READ		(0 << TCTRL_TRNS_MODE_OFFSET)
#define TRNS_MODE_WRITE_ONLY		(1 << TCTRL_TRNS_MODE_OFFSET)
#define TRNS_MODE_READ_ONLY		(2 << TCTRL_TRNS_MODE_OFFSET)
#define TRNS_MODE_NONE_DATA		(7 << TCTRL_TRNS_MODE_OFFSET)
#define TRNS_MODE_DUMMY_READ		(9 << TCTRL_TRNS_MODE_OFFSET)

/* Dual/Qual mode */
#define DUAL_IO_MODE			(2 << TCTRL_DUAL_MODE_OFFSET)

/* Dummy count */
/* In Qual mode, dummy count 3 implies 6 dummy cycles */
#define DUMMY_CNT_3			(0x2 << TCTRL_DUMMY_CNT_OFFSET)

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
#define STAT_RX_NUM_MSK			GENMASK(13, 8)
#define STAT_TX_NUM_MSK			GENMASK(21, 16)

/* Field mask of SPI control register */
#define CTRL_RX_THRES_OFFSET		(8)
#define CTRL_TX_THRES_OFFSET		(16)

#define CTRL_RX_THRES_MSK		GENMASK(15, 8)
#define CTRL_TX_THRES_MSK		GENMASK(23, 16)

/* Field mask of SPI status register */
#define TIMIN_SCLK_DIV_MSK		GENMASK(7, 0)

#define TX_FIFO_THRESHOLD		(1 << CTRL_TX_THRES_OFFSET)
#define RX_FIFO_THRESHOLD		(1 << CTRL_RX_THRES_OFFSET)
#define MAX_TRANSFER_CNT		(512)

#define TX_FIFO_SIZE_SETTING(base)				\
	(sys_read32(QSPI_CONFIG(base)) & CFG_TX_FIFO_SIZE_MSK)
#define TX_FIFO_SIZE(base)					\
	(2 << (TX_FIFO_SIZE_SETTING(base) >> 4))

#define RX_FIFO_SIZE_SETTING(base)				\
	(sys_read32(QSPI_CONFIG(base)) & CFG_RX_FIFO_SIZE_MSK)
#define RX_FIFO_SIZE(base)					\
	(2 << (RX_FIFO_SIZE_SETTING(base) >> 0))

#define TX_NUM_STAT(base)	(sys_read32(QSPI_STAT(base)) & STAT_TX_NUM_MSK)
#define RX_NUM_STAT(base)	(sys_read32(QSPI_STAT(base)) & STAT_RX_NUM_MSK)
#define GET_TX_NUM(base)	(TX_NUM_STAT(base) >> 16)
#define GET_RX_NUM(base)	(RX_NUM_STAT(base) >> 8)
