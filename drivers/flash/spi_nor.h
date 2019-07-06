/*
 * Copyright (c) 2018 Savoir-Faire Linux.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SPI_NOR_H__
#define __SPI_NOR_H__

#include <sys/util.h>

#define SPI_NOR_MAX_ID_LEN	3

struct spi_nor_config {
	u8_t id[SPI_NOR_MAX_ID_LEN];
	u32_t page_size;
	u32_t sector_size;
	u32_t n_sectors;
};

/* Status register bits */
#define SPI_NOR_WIP_BIT         BIT(0)  /* Write in progress */
#define SPI_NOR_WEL_BIT         BIT(1)  /* Write enable latch */

/* Flash opcodes */
#define SPI_NOR_CMD_WRSR        0x01    /* Write status register */
#define SPI_NOR_CMD_RDSR        0x05    /* Read status register */
#define SPI_NOR_CMD_READ        0x03    /* Read data */
#define SPI_NOR_CMD_WREN        0x06    /* Write enable */
#define SPI_NOR_CMD_WRDI        0x04    /* Write disable */
#define SPI_NOR_CMD_PP          0x02    /* Page program */
#define SPI_NOR_CMD_SE          0x20    /* Sector erase */
#define SPI_NOR_CMD_BE_32K      0x52    /* Block erase 32KB */
#define SPI_NOR_CMD_BE          0xD8    /* Block erase */
#define SPI_NOR_CMD_CE          0xC7    /* Chip erase */
#define SPI_NOR_CMD_RDID        0x9F    /* Read JEDEC ID */
#define SPI_NOR_CMD_MCHP_UNLOCK 0x98    /* Microchip: Global unblock */

#endif /*__SPI_NOR_H__*/
