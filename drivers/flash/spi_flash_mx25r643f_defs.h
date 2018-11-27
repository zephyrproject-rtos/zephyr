/*
 * Copyright (c) 2018 Sigma Connectivity.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief This file has the MXSMIO SPI flash private definitions
 */

#ifndef ZEPHYR_DRIVERS_FLASH_SPI_FLASH_MX25R643F_DEFS_H_
#define ZEPHYR_DRIVERS_FLASH_SPI_FLASH_MX25R6435F_DEFS_H_

#define MX25R6435F_RDID_VALUE  (0x00c22817)
#define MX25R6435F_RES_VALUE   (0x17)
#define MX25R6435F_REMS_VALUE  (0xc217)

/* relevant status register bits */
#define MX25R6435F_WIP_BIT         (0x1 << 0)
#define MX25R6435F_WEL_BIT         (0x1 << 1)
#define MX25R6435F_SRWD_BIT        (0x1 << 7)

/* supported erase size */
#define MX25R6435F_SECTOR_SIZE     (0x1000)
#define MX25R6435F_BLOCK32K_SIZE   (0x8000)
#define MX25R6435F_BLOCK_SIZE      (0x10000)

#define MX25R6435F_SECTOR_MASK     (0xFFF)

/* ID commands */
#define MX25R6435F_CMD_RDID        0x9F
#define MX25R6435F_CMD_RES         0xAB
#define MX25R6435F_CMD_REMS        0x90

/*Register commands */
#define MX25R6435F_CMD_WRSR        0x01
#define MX25R6435F_CMD_RDSR        0x05
#define MX25R6435F_CMD_RDCR        0x15
#define MX25R6435F_CMD_WRSCUR      0x2F
#define MX25R6435F_CMD_RDSCUR      0x2B

/* READ commands */
#define MX25R6435F_CMD_READ        0x03
#define MX25R6435F_CMD_2READ       0xBB
#define MX25R6435F_CMD_4READ       0xEB
#define MX25R6435F_CMD_FAST_READ   0x0B
#define MX25R6435F_CMD_DREAD       0x3B
#define MX25R6435F_CMD_QREAD       0x6B
#define MX25R6435F_CMD_RDSFDP      0x5A

/* Program commands */
#define MX25R6435F_CMD_WREN        0x06
#define MX25R6435F_CMD_WRDI        0x04
#define MX25R6435F_CMD_PP          0x02
#define MX25R6435F_CMD_4PP         0x38

/* Erase commands */
#define MX25R6435F_CMD_SE          0x20
#define MX25R6435F_CMD_BE32K       0x52
#define MX25R6435F_CMD_BE          0xD8
#define MX25R6435F_CMD_CE          0x60

/* Mode setting commands */
#define MX25R6435F_CMD_DP          0xB9

/* Reset commands */
#define MX25R6435F_CMD_RSTEN       0x66
#define MX25R6435F_CMD_RST         0x99

/* Suspend/Resume commands */
#define MX25R6435F_CMD_RESUME      0x7A

#define MX25R6435F_CMD_NOP         0x00

#endif /*ZEPHYR_DRIVERS_FLASH_SPI_FLASH_MX25R6435F_DEFS_H_*/
