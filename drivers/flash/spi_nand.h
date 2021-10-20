/*
 * Copyright (c) 2021 Macronix.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SPI_NAND_H__
#define __SPI_NAND_H__

#include <sys/util.h>
#include <sys/byteorder.h>

#define SPI_NAND_ID_LEN	2

/* Status register bits */
#define SPI_NAND_WIP_BIT         BIT(0)  /* Write in progress */
#define SPI_NAND_WEL_BIT         BIT(1)  /* Write enable latch */

/* Get/Set Feature Address Definition */
#define SPI_NAND_FEA_ADDR_BLOCK_PROT 0xA0
#define SPI_NAND_FEA_ADDR_SECURE_OTP 0xB0
#define SPI_NAND_FEA_ADDR_STATUS     0xC0

/* Flash opcodes */
#define SPI_NAND_CMD_RDSR            0x05    /* Read status register */
#define SPI_NAND_CMD_RDID            0x9F    /* Read ID */

#define SPI_NAND_CMD_PAGE_READ       0x13   /* Read data from array to cache */
#define SPI_NAND_CMD_READ_CACHE      0x03   /* Read data from cache*/
#define SPI_NAND_CMD_READ_CACHE2     0x3B
#define SPI_NAND_CMD_READ_CACHE4     0x6B
#define SPI_NAND_CMD_READ_CACHE_SEQ  0x31
#define SPI_NAND_CMD_READ_CACHE_END  0x3F

#define SPI_NAND_CMD_WREN            0x06    /* Write enable */
#define SPI_NAND_CMD_WRDI            0x04    /* Write disable */
#define SPI_NAND_CMD_PP_LOAD         0x02    /* Page program */
#define SPI_NAND_CMD_PP_RAND_LOAD    0x84    /* Page program */
#define SPI_NAND_CMD_4PP_LOAD        0x32    /* Page program */
#define SPI_NAND_CMD_4PP_RAND_LOAD   0x34    /* Page program */
#define SPI_NAND_CMD_PROGRAM_EXEC    0x10    /*   */
#define SPI_NAND_CMD_BE              0xD8    /* Block erase */

#define SPI_NAND_CMD_GET_FEATURE     0x0F    
#define SPI_NAND_CMD_SET_FEATURE     0x1F  
#define SPI_NAND_CMD_RESET           0xFF    
#define SPI_NAND_CMD_ECC_STAT_READ   0x7C  

/* Page, sector, and block size are standard, not configurable. */
#define SPI_NAND_PAGE_SIZE    0x0100U
#define SPI_NAND_SECTOR_SIZE  0x1000U
#define SPI_NAND_BLOCK_SIZE   0x10000U

#define SPI_NAND_BLOCK_OFFSET  0x40000
#define SPI_NAND_PAGE_OFFSET   0x1000
#define SPI_NAND_PAGE_MASK     0xFFF

/* Test whether offset is aligned to a given number of bits. */
#define SPI_NAND_IS_ALIGNED(_ofs, _bits) (((_ofs) & BIT_MASK(_bits)) == 0)
#define SPI_NAND_IS_SECTOR_ALIGNED(_ofs) SPI_NAND_IS_ALIGNED(_ofs, 12)

#endif /*__SPI_NAND_H__*/
