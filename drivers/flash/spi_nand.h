/*
 * Copyright (c) 2022 Macronix International Co., Ltd.
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
#define SPI_NAND_FEA_ADDR_CONF_B0    0xB0
#define SPI_NAND_FEA_ADDR_STATUS     0xC0
#define SPI_NAND_FEA_ADDR_RECOVERY   0x70

/* Status Register Bits 0xC0 */
#define SPINAND_STATUS_BIT_WIP             0x1  /* Write In Progress */
#define SPINAND_STATUS_BIT_WEL             0x2  /* Write Enable Latch */
#define SPINAND_STATUS_BIT_ERASE_FAIL      0x4  /* Erase failed */
#define SPINAND_STATUS_BIT_PROGRAM_FAIL    0x8  /* Program failed */
#define SPINAND_STATUS_BIT_ECC_STATUS_MASK 0x30 /* ECC status */
#define SPINAND_STATUS_ECC_STATUS_NO_ERR     0x00
#define SPINAND_STATUS_ECC_STATUS_ERR_COR    0x10
#define SPINAND_STATUS_ECC_STATUS_ERR_NO_COR 0x20

/* Secure OTP Register Bits  0xB0 */
#define SPINAND_SECURE_BIT_QE          0x01  /* Quad enable */
#define SPINAND_SECURE_BIT_CONT        0x04  /* continuous read enable */
#define SPINAND_SECURE_BIT_ECC_EN      0x10  /* On-die ECC enable */
#define SPINAND_SECURE_BIT_OTP_EN      0x40
#define SPINAND_SECURE_BIT_OTP_PROT    0x80

/* Block Protection Register Bits 0xA0*/
#define  SPINAND_BLOCK_PROT_BIT_SP      0x01
#define  SPINAND_BLOCK_PROT_BIT_COMPLE  0x02
#define  SPINAND_BLOCK_PROT_BIT_INVERT  0x04
#define  SPINAND_BLOCK_PROT_BIT_BP0     0x08
#define  SPINAND_BLOCK_PROT_BIT_BP1     0x10
#define  SPINAND_BLOCK_PROT_BIT_BP2     0x20
#define  SPINAND_BLOCK_PROT_BIT_BPRWD   0x80
#define  SPINAND_BLOCK_PROT_BIT_BP_MASK 0x38

#define  SPINAND_BLOCK_PROT_BP_OFFSET     3
#define  SPINAND_BLOCK_PROT_COMPLE_OFFSET 1

/* Special read for data recovery 0x70 */
#define  SPINAND_RECOVERY_DISABLE      0x00
#define  SPINAND_RECOVERY_MODE1        0x01
#define  SPINAND_RECOVERY_MODE2        0x02
#define  SPINAND_RECOVERY_MODE3        0x03
#define  SPINAND_RECOVERY_MODE4        0x04
#define  SPINAND_RECOVERY_MODE5        0x05

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
#define SPI_NAND_CMD_PP_LOAD         0x02    /* Load data to cache*/
#define SPI_NAND_CMD_PP_RAND_LOAD    0x84    /* Load random data to cache */
#define SPI_NAND_CMD_4PP_LOAD        0x32    /* Load data to cache with 4 IO*/
#define SPI_NAND_CMD_4PP_RAND_LOAD   0x34    /* Load random data to cache with 4 IO*/
#define SPI_NAND_CMD_PROGRAM_EXEC    0x10    /* Execute program */
#define SPI_NAND_CMD_BE              0xD8    /* Block erase */

#define SPI_NAND_CMD_GET_FEATURE     0x0F    
#define SPI_NAND_CMD_SET_FEATURE     0x1F  
#define SPI_NAND_CMD_RESET           0xFF    
#define SPI_NAND_CMD_ECC_STAT_READ   0x7C  

/* Page, sector, and block size are standard, not configurable. */
#define SPI_NAND_PAGE_SIZE    2048
//#define SPI_NAND_SECTOR_SIZE  0x1000U
//#define SPI_NAND_BLOCK_SIZE   0x20000U
#define SPI_NAND_NOP           4
#define SPI_NAND_SUB_PAGE_SIZE (SPI_NAND_PAGE_SIZE / SPI_NAND_NOP)

#define SPI_NAND_BLOCK_OFFSET  0x40000
#define SPI_NAND_PAGE_OFFSET   0x1000
#define SPI_NAND_BLOCK_MASK    0x3FFFF
#define SPI_NAND_PAGE_MASK     0xFFF

#endif /*__SPI_NAND_H__*/
