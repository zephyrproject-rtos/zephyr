/*
 * Copyright (c) 2025 EXALT Technologies.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_MSPI_MSPI_NOR_H_
#define ZEPHYR_DRIVERS_MSPI_MSPI_NOR_H_

/* Flash opcodes */
#define MSPI_NOR_CMD_WRSR         0x01    /* Write status register */
#define MSPI_NOR_CMD_RDSR         0x05    /* Read status register */
#define MSPI_NOR_CMD_WRSR2        0x31    /* Write status register 2 */
#define MSPI_NOR_CMD_RDSR2        0x35    /* Read status register 2 */
#define MSPI_NOR_CMD_RDSR3        0x15    /* Read status register 3 */
#define MSPI_NOR_CMD_WRSR3        0x11    /* Write status register 3 */
#define MSPI_NOR_CMD_READ         0x03    /* Read data */
#define MSPI_NOR_CMD_READ_FAST    0x0B    /* Read data */
#define MSPI_NOR_CMD_DREAD        0x3B    /* Read data (1-1-2) */
#define MSPI_NOR_CMD_2READ        0xBB    /* Read data (1-2-2) */
#define MSPI_NOR_CMD_QREAD        0x6B    /* Read data (1-1-4) */
#define MSPI_NOR_CMD_4READ        0xEB    /* Read data (1-4-4) */
#define MSPI_NOR_CMD_WREN         0x06    /* Write enable */
#define MSPI_NOR_CMD_WRDI         0x04    /* Write disable */
#define MSPI_NOR_CMD_PP           0x02    /* Page program */
#define MSPI_NOR_CMD_PP_1_1_2     0xA2    /* Dual Page program (1-1-2) */
#define MSPI_NOR_CMD_PP_1_1_4     0x32    /* Quad Page program (1-1-4) */
#define MSPI_NOR_CMD_PP_1_4_4     0x38    /* Quad Page program (1-4-4) */
#define MSPI_NOR_CMD_RDCR         0x15    /* Read control register */
#define MSPI_NOR_CMD_SE           0x20    /* Sector erase */
#define MSPI_NOR_CMD_SE_4B        0x21    /* Sector erase 4 byte address*/
#define MSPI_NOR_CMD_BE_32K       0x52    /* Block erase 32KB */
#define MSPI_NOR_CMD_BE_32K_4B    0x5C    /* Block erase 32KB 4 byte address*/
#define MSPI_NOR_CMD_BE           0xD8    /* Block erase */
#define MSPI_NOR_CMD_BE_4B        0xDC    /* Block erase 4 byte address*/
#define MSPI_NOR_CMD_CE           0xC7    /* Chip erase */
#define MSPI_NOR_CMD_RDID         0x9F    /* Read JEDEC ID */
#define MSPI_NOR_CMD_ULBPR        0x98    /* Global Block Protection Unlock */
#define MSPI_NOR_CMD_4BA          0xB7    /* Enter 4-Byte Address Mode */
#define MSPI_NOR_CMD_DPD          0xB9    /* Deep Power Down */
#define MSPI_NOR_CMD_RDPD         0xAB    /* Release from Deep Power Down */
#define MSPI_NOR_CMD_WR_CFGREG2   0x72    /* Write config register 2 */
#define MSPI_NOR_CMD_RD_CFGREG2   0x71    /* Read config register 2 */
#define MSPI_NOR_CMD_RESET_EN     0x66    /* Reset Enable */
#define MSPI_NOR_CMD_RESET_MEM    0x99    /* Reset Memory */
#define MSPI_NOR_CMD_BULKE        0x60    /* Bulk Erase */
#define MSPI_NOR_CMD_READ_4B      0x13  /* Read data 4 Byte Address */
#define MSPI_NOR_CMD_READ_FAST_4B 0x0C  /* Fast Read 4 Byte Address */
#define MSPI_NOR_CMD_DREAD_4B     0x3C  /* Read data (1-1-2) 4 Byte Address */
#define MSPI_NOR_CMD_2READ_4B     0xBC  /* Read data (1-2-2) 4 Byte Address */
#define MSPI_NOR_CMD_QREAD_4B     0x6C  /* Read data (1-1-4) 4 Byte Address */
#define MSPI_NOR_CMD_4READ_4B     0xEC  /* Read data (1-4-4) 4 Byte Address */
#define MSPI_NOR_CMD_PP_4B        0x12  /* Page Program 4 Byte Address */
#define MSPI_NOR_CMD_PP_1_1_4_4B  0x34  /* Quad Page program (1-1-4) 4 Byte Address */
#define MSPI_NOR_CMD_PP_1_4_4_4B  0x3e  /* Quad Page program (1-4-4) 4 Byte Address */
#define MSPI_NOR_CMD_RDFLSR       0x70  /* Read Flag Status Register */
#define MSPI_NOR_CMD_CLRFLSR      0x50  /* Clear Flag Status Register */
#define MSPI_NOR_CMD_WR_VCFGREG   0x81  /* Octal Write volatile configuration Register */

/* Flash octal opcodes */
#define MSPI_NOR_OCMD_READ        0xFD  /* Octal IO read command */
#define MSPI_NOR_OCMD_SE          0x21DE  /* Octal Sector erase */
#define MSPI_NOR_OCMD_CE          0xC738  /* Octal Chip erase */
#define MSPI_NOR_OCMD_RDSR        0x05FA  /* Octal Read status register */
#define MSPI_NOR_OCMD_DTR_RD      0xEE11  /* Octal IO DTR read command */
#define MSPI_NOR_OCMD_RD          0xEC13  /* Octal IO read command */
#define MSPI_NOR_OCMD_PAGE_PRG    0x12ED  /* Octal Page Prog */
#define MSPI_NOR_OCMD_WREN        0x06F9  /* Octal Write enable */
#define MSPI_NOR_OCMD_NOP         0x00FF  /* Octal No operation */
#define MSPI_NOR_OCMD_RESET_EN    0x6699  /* Octal Reset Enable */
#define MSPI_NOR_OCMD_RESET_MEM   0x9966  /* Octal Reset Memory */
#define MSPI_NOR_OCMD_WR_CFGREG2  0x728D  /* Octal Write configuration Register2 */
#define MSPI_NOR_OCMD_RD_CFGREG2  0x718E  /* Octal Read configuration Register2 */
#define MSPI_NOR_OCMD_BULKE       0x609F  /* Octa Bulk Erase */

/* Flash Auto-polling values */
#define MSPI_NOR_WREN_MATCH    0x02
#define MSPI_NOR_WREN_MASK     0x02
#define MSPI_NOR_MEM_RDY_MATCH 0x00
#define MSPI_NOR_MEM_RDY_MASK  0x01
#define MSPI_NOR_AUTO_POLLING_INTERVAL   0x10

/* Flash Dummy Cycles values */
#define MSPI_NOR_DUMMY_REG_OCTAL         4U
#define MSPI_NOR_DUMMY_REG_OCTAL_DTR     5U

/* Memory registers address */
#define MSPI_NOR_REG2_ADDR1              0x0000000
#define MSPI_NOR_CR2_STR_OPI_EN          0x01
#define MSPI_NOR_CR2_DTR_OPI_EN          0x02
#define MSPI_NOR_REG2_ADDR3              0x00000300
#define MSPI_NOR_CR2_DUMMY_CYCLES_66MHZ  0x07

#endif /* ZEPHYR_DRIVERS_MSPI_MSPI_NOR_H_ */
