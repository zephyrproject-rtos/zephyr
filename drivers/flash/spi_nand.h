/*
 * Copyright (c) 2022-2025 Macronix International Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SPI_NAND_H__
#define __SPI_NAND_H__

#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>

#define SPI_NAND_ID_LEN 2

/* Status register bits */
#define SPI_NAND_WIP_BIT BIT(0) /* Write in progress */

/* Get/Set Feature Address Definition */
#define SPI_NAND_FEA_ADDR_BLOCK_PROT 0xA0
#define SPI_NAND_FEA_ADDR_CONF_B0    0xB0
#define SPI_NAND_FEA_ADDR_STATUS     0xC0
#define SPI_NAND_FEA_ADDR_RECOVERY   0x70

/* Secure OTP Register Bits  0xB0 */
#define SPINAND_SECURE_BIT_CONT   0x04 /* continuous read enable */
#define SPINAND_SECURE_BIT_ECC_EN 0x10 /* On-die ECC enable */
#define SPINAND_SECURE_BIT_OTP_EN 0x40

/* Block Protection Register Bits 0xA0 */
#define SPINAND_BLOCK_PROT_BIT_BP_MASK 0x38

/* Flash opcodes */
#define SPI_NAND_CMD_RDID 0x9F /* Read ID */

#define SPI_NAND_CMD_PAGE_READ  0x13 /* Read data from array to cache */
#define SPI_NAND_CMD_READ_CACHE 0x03 /* Read data from cache*/

#define SPI_NAND_CMD_WREN         0x06 /* Write enable */
#define SPI_NAND_CMD_PP_LOAD      0x02 /* Load data to cache*/
#define SPI_NAND_CMD_PROGRAM_EXEC 0x10 /* Execute program */
#define SPI_NAND_CMD_BE           0xD8 /* Block erase */

#define SPI_NAND_CMD_GET_FEATURE   0x0F
#define SPI_NAND_CMD_SET_FEATURE   0x1F
#define SPI_NAND_CMD_RESET         0xFF
#define SPI_NAND_CMD_ECC_STAT_READ 0x7C

#define SPI_NAND_BLOCK_OFFSET 0x40000
#define SPI_NAND_PAGE_OFFSET  0x1000
#define SPI_NAND_BLOCK_MASK   0x3FFFF
#define SPI_NAND_PAGE_MASK    0xFFF

#define ONFI_SIG_0         0
#define ONFI_SIG_1         1
#define ONFI_SIG_2         2
#define ONFI_SIG_3         3
#define ONFI_PAGE_SIZE_80  80
#define ONFI_PAGE_SIZE_81  81
#define ONFI_PAGE_SIZE_82  82
#define ONFI_OOB_SIZE_84   84
#define ONFI_OOB_SIZE_85   85
#define ONFI_PAGE_NUM_92   92
#define ONFI_PAGE_NUM_93   93
#define ONFI_BLK_NUM_96    96
#define ONFI_BLK_NUM_97    97
#define ONFI_ECC_NUM_112   112
#define ONFI_BE_TIME_135   135
#define ONFI_BE_TIME_136   136
#define ONFI_CONT_READ_168 168

#endif /*__SPI_NAND_H__*/
