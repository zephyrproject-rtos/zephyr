/*
 * Copyright (c) 2022 Macronix.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __OSPI_NOR_H__
#define __OSPI_NOR_H__

#include <sys/util.h>

#define OSPI_NOR_MAX_ID_LEN	3

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
#define SPI_NOR_CMD_ULBPR       0x98    /* Global Block Protection Unlock */
#define SPI_NOR_CMD_DPD         0xB9    /* Deep Power Down */
#define SPI_NOR_CMD_RDPD        0xAB    /* Release from Deep Power Down */
#define SPI_NOR_CMD_RSTEN	0x66
#define SPI_NOR_CMD_RST		0x99

#define SPI_NOR_CMD_READ4B      0x13    /* Read data with 4 byte address*/
#define SPI_NOR_CMD_PP4B        0x12    /* Page program with 4 byte address*/
#define SPI_NOR_CMD_SE4B        0x21    /* Sector erase with 4 byte address*/
#define SPI_NOR_CMD_BE4B        0xDC    /* Block erase with 4 byte address*/
#define SPI_NOR_CMD_8READ       0xEC    /* Read data in 8IO mode*/
#define SPI_NOR_CMD_8DTRD       0xEE    /* Read data in 8IO DTR mode*/

#define SPI_NOR_CMD_WRCR2	0x72
#define SPI_NOR_CMD_RDCR2	0x71

/* Page, sector, and block size are standard, not configurable. */
#define SPI_NOR_PAGE_SIZE    0x0100U
#define SPI_NOR_SECTOR_SIZE  0x1000U
#define SPI_NOR_BLOCK_SIZE   0x10000U

/* Test whether offset is aligned to a given number of bits. */
#define SPI_NOR_IS_ALIGNED(_ofs, _bits) (((_ofs) & BIT_MASK(_bits)) == 0)
#define SPI_NOR_IS_SECTOR_ALIGNED(_ofs) SPI_NOR_IS_ALIGNED(_ofs, 12)

/**
 * @name OSPI definition for the OctoSPI peripherals
 * Note that the possible combination is
 *  SPI mode in STR transfer rate
 *  OPI mode in STR transfer rate
 *  OPI mode in DTR transfer rate
 */

/* OSPI mode operating on 1 line, 2 lines, 4 lines or 8 lines */
/* 1 Cmd Line, 1 Address Line and 1 Data Line    */
#define OSPI_SPI_MODE                     1
/* 2 Cmd Lines, 2 Address Lines and 2 Data Lines */
#define OSPI_DUAL_MODE                    2
/* 4 Cmd Lines, 4 Address Lines and 4 Data Lines */
#define OSPI_QUAD_MODE                    4
/* 8 Cmd Lines, 8 Address Lines and 8 Data Lines */
#define OSPI_OPI_MODE                     8

/* OSPI mode operating on Single or Double Transfer Rate */
/* Single Transfer Rate */
#define OSPI_STR_TRANSFER                 1
/* Double Transfer Rate */
#define OSPI_DTR_TRANSFER                 2

#define STM32_OSPI_RESET_MAX_TIME         100U

#endif /*__OSPI_NOR_H__*/
