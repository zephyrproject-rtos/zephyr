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
	/* JEDEC id from devicetree */
	uint8_t id[SPI_NOR_MAX_ID_LEN];

	/* Size from devicetree, in bytes */
	uint32_t size;

	/* Page size, in bytes */
	uint16_t page_size;

	/* Indicates if device has chip erase capability */
	bool has_chip_erase;

	/* Erase size exponent and instruction as in JESD216, Basic Flash
	 * Parameter Table Dword 8 and 9
	 */
	const uint8_t erase_size_exp[4];
	const uint8_t erase_cmd[4];
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
#define SPI_NOR_CMD_CE          0xC7    /* Chip erase */
#define SPI_NOR_CMD_RDID        0x9F    /* Read JEDEC ID */
#define SPI_NOR_CMD_ULBPR       0x98    /* Global Block Protection Unlock */
#define SPI_NOR_CMD_DPD         0xB9    /* Deep Power Down */
#define SPI_NOR_CMD_RDPD        0xAB    /* Release from Deep Power Down */

#define SPI_NOR_SECTOR_SIZE  0x1000U
#define SPI_NOR_BLOCK_SIZE   0x10000U

/* SFDP Basic Flash Parameters. See JESD216 for documentation*/
#define SFDP_GET_FIELD(reg, h, l) ((reg & GENMASK(h, l)) >> l)
#define SFDP_B8_ERASE_SIZE_2(dword)	SFDP_GET_FIELD(dword, 23, 16)
#define SFDP_B8_ERASE_SIZE_1(dword)	SFDP_GET_FIELD(dword,  7,  0)
#define SFDP_B8_ERASE_CMD_2(dword)	SFDP_GET_FIELD(dword, 31, 24)
#define SFDP_B8_ERASE_CMD_1(dword)	SFDP_GET_FIELD(dword, 15,  8)
#define SFDP_B9_ERASE_SIZE_4(dword)	SFDP_GET_FIELD(dword, 23, 16)
#define SFDP_B9_ERASE_SIZE_3(dword)	SFDP_GET_FIELD(dword,  7,  0)
#define SFDP_B9_ERASE_CMD_4(dword)	SFDP_GET_FIELD(dword, 31, 24)
#define SFDP_B9_ERASE_CMD_3(dword)	SFDP_GET_FIELD(dword, 15,  8)
#define SFDP_B11_PAGE_SIZE(dword)	SFDP_GET_FIELD(dword,  7,  4)

/* Test whether offset is aligned. */
#define SPI_NOR_IS_ALIGNED(_ofs, _size) (((_ofs) & (_size - 1U)) == 0)
#define SPI_NOR_IS_SECTOR_ALIGNED(_ofs) (((_ofs) & (SPI_NOR_SECTOR_SIZE - 1U)) == 0)


#endif /*__SPI_NOR_H__*/
