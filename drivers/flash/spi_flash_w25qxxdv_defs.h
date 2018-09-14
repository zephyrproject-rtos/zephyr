/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief This file has the WinBond SPI flash private definitions
 */

#ifndef ZEPHYR_DRIVERS_FLASH_SPI_FLASH_W25QXXDV_DEFS_H_
#define ZEPHYR_DRIVERS_FLASH_SPI_FLASH_W25QXXDV_DEFS_H_

/* Status Registers
 *    S7     S6     S5     S4     S3     S2     S1     S0
 *  +-------------------------------------------------------+
 *  | SRP0 | SEC  |  TB  | BP2  | BP1  | BP0  | WEL  | BUSY |
 *  +-------------------------------------------------------+
 *
 * BUSY - Erase/Write In Progress - 1 device is executing a command, 0 ready for command
 * WEL  - Write Enable Latch - 1 write enable is received, 0 completeion of
 * a Write Disable, Page Program, Erase, Write Status Register
 *
 *    S15    S14    S13    S12    S11    S10    S9     S8
 *  +-------------------------------------------------------+
 *  | SUS  | CMP  | LB3  | LB2  | LB1  | xxx  | QE   | SRP1 |
 *  +-------------------------------------------------------+
 *
 *    S23        S22    S21    S20    S19    S18    S17    S16
 *  +----------------------------------------------------------+
 *  | HOLD/RST | DRV1 | DRV0 | xxx  | xxx  | WPS  | xxx  | xxx |
 *  +----------------------------------------------------------+
 */

#define W25QXXDV_RDID_VALUE  (0x00ef4015)
#define W25QXXDV_MAX_LEN_REG_CMD      (6)
#define W25QXXDV_OPCODE_LEN           (1)
#define W25QXXDV_ADDRESS_WIDTH        (3)
#define W25QXXDV_LEN_CMD_ADDRESS      (4)
#define W25QXXDV_LEN_CMD_AND_ID       (4)

/* relevant status register bits */
#define W25QXXDV_WIP_BIT         (0x1 << 0)
#define W25QXXDV_WEL_BIT         (0x1 << 1)
#define W25QXXDV_SRWD_BIT        (0x1 << 7)
#define W25QXXDV_TB_BIT          (0x1 << 3)
#define W25QXXDV_SR_BP_OFFSET    (2)

/* relevant security register bits */
#define W25QXXDV_SECR_WPSEL_BIT  (0x1 << 7)
#define W25QXXDV_SECR_EFAIL_BIT  (0x1 << 6)
#define W25QXXDV_SECR_PFAIL_BIT  (0x1 << 5)

/* supported erase size */
#define W25QXXDV_SECTOR_SIZE     (0x1000)
#define W25QXXDV_BLOCK32K_SIZE   (0x8000)
#define W25QXXDV_BLOCK_SIZE      (0x10000)

#define W25QXXDV_SECTOR_MASK     (0xFFF)

/* ID commands */
#define W25QXXDV_CMD_RDID        0x9F
#define W25QXXDV_CMD_RES         0xAB
#define W25QXXDV_CMD_REMS        0x90
#define W25QXXDV_CMD_QPIID       0xAF
#define W25QXXDV_CMD_UNID        0x4B

/*Register commands */
#define W25QXXDV_CMD_WRSR        0x01
#define W25QXXDV_CMD_RDSR        0x05
#define W25QXXDV_CMD_RDSR2       0x35
#define W25QXXDV_CMD_WRSCUR      0x2F
#define W25QXXDV_CMD_RDSCUR      0x48

/* READ commands */
#define W25QXXDV_CMD_READ        0x03
#define W25QXXDV_CMD_2READ       0xBB
#define W25QXXDV_CMD_4READ       0xEB
#define W25QXXDV_CMD_FASTREAD    0x0B
#define W25QXXDV_CMD_DREAD       0x3B
#define W25QXXDV_CMD_QREAD       0x6B
#define W25QXXDV_CMD_RDSFDP      0x5A

/* Program commands */
#define W25QXXDV_CMD_WREN        0x06
#define W25QXXDV_CMD_WRDI        0x04
#define W25QXXDV_CMD_PP          0x02
#define W25QXXDV_CMD_4PP         0x32
#define W25QXXDV_CMD_WRENVSR     0x50

/* Erase commands */
#define W25QXXDV_CMD_SE          0x20
#define W25QXXDV_CMD_BE32K       0x52
#define W25QXXDV_CMD_BE          0xD8
#define W25QXXDV_CMD_CE          0x60

/* Mode setting commands */
#define W25QXXDV_CMD_DP          0xB9
#define W25QXXDV_CMD_RDP         0xAB

/* Reset commands */
#define W25QXXDV_CMD_RSTEN       0x66
#define W25QXXDV_CMD_RST         0x99
#define W25QXXDV_CMD_RSTQIO      0xF5

/* Security commands */
#define W25QXXDV_CMD_ERSR        0x44
#define W25QXXDV_CMD_PRSR        0x42

/* Suspend/Resume commands */
#define W25QXXDV_CMD_PGM_ERS_S   0x75
#define W25QXXDV_CMD_PGM_ERS_R   0x7A

#define W25QXXDV_CMD_NOP         0x00

#endif /*ZEPHYR_DRIVERS_FLASH_SPI_FLASH_W25QXXDV_DEFS_H_*/
