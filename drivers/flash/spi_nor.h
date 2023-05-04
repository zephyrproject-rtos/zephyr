/*
 * Copyright (c) 2018 Savoir-Faire Linux.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SPI_NOR_H__
#define __SPI_NOR_H__

#include <zephyr/sys/util.h>

#define SPI_NOR_MAX_ID_LEN	3

/* Status register bits */
#define SPI_NOR_WIP_BIT         BIT(0)  /* Write in progress */
#define SPI_NOR_WEL_BIT         BIT(1)  /* Write enable latch */

/* Flash opcodes */
#define SPI_NOR_CMD_WRSR        0x01    /* Write status register */
#define SPI_NOR_CMD_RDSR        0x05    /* Read status register */
#define SPI_NOR_CMD_WRSR2       0x31    /* Write status register 2 */
#define SPI_NOR_CMD_RDSR2       0x35    /* Read status register 2 */
#define SPI_NOR_CMD_RDSR3       0x15    /* Read status register 3 */
#define SPI_NOR_CMD_WRSR3       0x11    /* Write status register 3 */
#define SPI_NOR_CMD_READ        0x03    /* Read data */
#define SPI_NOR_CMD_READ_FAST   0x0B    /* Read data */
#define SPI_NOR_CMD_DREAD       0x3B    /* Read data (1-1-2) */
#define SPI_NOR_CMD_2READ       0xBB    /* Read data (1-2-2) */
#define SPI_NOR_CMD_QREAD       0x6B    /* Read data (1-1-4) */
#define SPI_NOR_CMD_4READ       0xEB    /* Read data (1-4-4) */
#define SPI_NOR_CMD_WREN        0x06    /* Write enable */
#define SPI_NOR_CMD_WRDI        0x04    /* Write disable */
#define SPI_NOR_CMD_PP          0x02    /* Page program */
#define SPI_NOR_CMD_PP_1_1_2    0xA2    /* Dual Page program (1-1-2) */
#define SPI_NOR_CMD_PP_1_1_4    0x32    /* Quad Page program (1-1-4) */
#define SPI_NOR_CMD_PP_1_4_4    0x38    /* Quad Page program (1-4-4) */
#define SPI_NOR_CMD_RDCR        0x15    /* Read control register */
#define SPI_NOR_CMD_SE          0x20    /* Sector erase */
#define SPI_NOR_CMD_SE_4B       0x21    /* Sector erase 4 byte address*/
#define SPI_NOR_CMD_BE_32K      0x52    /* Block erase 32KB */
#define SPI_NOR_CMD_BE          0xD8    /* Block erase */
#define SPI_NOR_CMD_CE          0xC7    /* Chip erase */
#define SPI_NOR_CMD_RDID        0x9F    /* Read JEDEC ID */
#define SPI_NOR_CMD_ULBPR       0x98    /* Global Block Protection Unlock */
#define SPI_NOR_CMD_4BA         0xB7    /* Enter 4-Byte Address Mode */
#define SPI_NOR_CMD_DPD         0xB9    /* Deep Power Down */
#define SPI_NOR_CMD_RDPD        0xAB    /* Release from Deep Power Down */
#define SPI_NOR_CMD_WR_CFGREG2  0x72    /* Write config register 2 */
#define SPI_NOR_CMD_RD_CFGREG2  0x71    /* Read config register 2 */
#define SPI_NOR_CMD_RESET_EN    0x66    /* Reset Enable */
#define SPI_NOR_CMD_RESET_MEM   0x99    /* Reset Memory */
#define SPI_NOR_CMD_BULKE       0x60    /* Bulk Erase */
#define SPI_NOR_CMD_READ_4B      0x13  /* Read data 4 Byte Address */
#define SPI_NOR_CMD_READ_FAST_4B 0x0C  /* Fast Read 4 Byte Address */
#define SPI_NOR_CMD_DREAD_4B     0x3C  /* Read data (1-1-2) 4 Byte Address */
#define SPI_NOR_CMD_2READ_4B     0xBC  /* Read data (1-2-2) 4 Byte Address */
#define SPI_NOR_CMD_QREAD_4B     0x6C  /* Read data (1-1-4) 4 Byte Address */
#define SPI_NOR_CMD_4READ_4B     0xEC  /* Read data (1-4-4) 4 Byte Address */
#define SPI_NOR_CMD_PP_4B        0x12  /* Page Program 4 Byte Address */
#define SPI_NOR_CMD_PP_1_1_4_4B  0x34  /* Quad Page program (1-1-4) 4 Byte Address */
#define SPI_NOR_CMD_PP_1_4_4_4B  0x3e  /* Quad Page program (1-4-4) 4 Byte Address */

/* Flash octal opcodes */
#define SPI_NOR_OCMD_SE         0x21DE  /* Octal Sector erase */
#define SPI_NOR_OCMD_CE         0xC738  /* Octal Chip erase */
#define SPI_NOR_OCMD_RDSR       0x05FA  /* Octal Read status register */
#define SPI_NOR_OCMD_DTR_RD     0xEE11  /* Octal IO DTR read command */
#define SPI_NOR_OCMD_RD         0xEC13  /* Octal IO read command */
#define SPI_NOR_OCMD_PAGE_PRG   0x12ED  /* Octal Page Prog */
#define SPI_NOR_OCMD_WREN       0x06F9  /* Octal Write enable */
#define SPI_NOR_OCMD_NOP        0x00FF  /* Octal No operation */
#define SPI_NOR_OCMD_RESET_EN   0x6699  /* Octal Reset Enable */
#define SPI_NOR_OCMD_RESET_MEM  0x9966  /* Octal Reset Memory */
#define SPI_NOR_OCMD_WR_CFGREG2 0x728D  /* Octal Write configuration Register2 */
#define SPI_NOR_OCMD_RD_CFGREG2 0x718E  /* Octal Read configuration Register2 */
#define SPI_NOR_OCMD_BULKE      0x609F  /* Octa Bulk Erase */

 /* Page, sector, and block size are standard, not configurable. */
 #define SPI_NOR_PAGE_SIZE    0x0100U
 #define SPI_NOR_SECTOR_SIZE  0x1000U
 #define SPI_NOR_BLOCK_SIZE   0x10000U

/* Flash Auto-polling values */
#define SPI_NOR_WREN_MATCH    0x02
#define SPI_NOR_WREN_MASK     0x02

#define SPI_NOR_MEM_RDY_MATCH 0x00
#define SPI_NOR_MEM_RDY_MASK  0x01

#define SPI_NOR_AUTO_POLLING_INTERVAL   0x10

/* Flash Dummy Cycles values */
#define SPI_NOR_DUMMY_RD                8U
#define SPI_NOR_DUMMY_RD_OCTAL          6U
#define SPI_NOR_DUMMY_RD_OCTAL_DTR      6U
#define SPI_NOR_DUMMY_REG_OCTAL         4U
#define SPI_NOR_DUMMY_REG_OCTAL_DTR     5U


/* Memory registers address */
#define SPI_NOR_REG2_ADDR1              0x0000000
#define SPI_NOR_CR2_STR_OPI_EN          0x01
#define SPI_NOR_CR2_DTR_OPI_EN          0x02
#define SPI_NOR_REG2_ADDR3              0x00000300
#define SPI_NOR_CR2_DUMMY_CYCLES_66MHZ  0x07

/* Test whether offset is aligned to a given number of bits. */
#define SPI_NOR_IS_ALIGNED(_ofs, _bits) (((_ofs) & BIT_MASK(_bits)) == 0)
#define SPI_NOR_IS_SECTOR_ALIGNED(_ofs) SPI_NOR_IS_ALIGNED(_ofs, 12)

#endif /*__SPI_NOR_H__*/
