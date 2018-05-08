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

#ifndef __SPI_FLASH_MX25_DEFS_H__
#define __SPI_FLASH_MX25_DEFS_H__

/* Status Registers
 *    S7     S6     S5     S4     S3     S2     S1     S0
 *  +-------------------------------------------------------+
 *  | SRP0 | SEC  |  TB  | BP2  | BP1  | BP0  | WEL  | BUSY |
 *  +-------------------------------------------------------+
 *
 * BUSY - Erase/Write In Progress - 1 device is executing a command, 0 ready
 * for command WEL  - Write Enable Latch - 1 write enable is received, 0
 * completeion of a Write Disable, Page Program, Erase, Write Status Register
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

#define MX25_RDID_VALUE		(0x00C2803A)
#define MX25_MAX_LEN_REG_CMD      (6)
#define MX25_OPCODE_LEN           (1)
#define MX25_ADDRESS_WIDTH        (3)
#define MX25_LEN_CMD_ADDRESS      (4)
#define MX25_LEN_CMD_AND_ID       (4)

/* relevant status register bits */
#define MX25_WIP_BIT         (0x1 << 0)
#define MX25_WEL_BIT         (0x1 << 1)
#define MX25_SRWD_BIT        (0x1 << 7)
#define MX25_TB_BIT          (0x1 << 3)
#define MX25_SR_BP_OFFSET    (2)

/* relevant security register bits */
#define MX25_SECR_WPSEL_BIT  (0x1 << 7)
#define MX25_SECR_EFAIL_BIT  (0x1 << 6)
#define MX25_SECR_PFAIL_BIT  (0x1 << 5)

/* supported erase size */
#define MX25_SECTOR_SIZE     (0x1000)
#define MX25_BLOCK32K_SIZE   (0x8000)
#define MX25_BLOCK_SIZE      (0x10000)

#define MX25_SECTOR_MASK     (0xFFF)

/* ID comands */
#define MX25_CMD_RDID        0x9F
#define MX25_CMD_RES         0xAB
#define MX25_CMD_REMS        0x90
#define MX25_CMD_QPIID       0xAF
#define MX25_CMD_UNID        0x4B

/*Register comands */
#define MX25_CMD_WRSR        0x01
#define MX25_CMD_RDSR        0x05
#define MX25_CMD_RDSR2       0x35
#define MX25_CMD_WRSCUR      0x2F
#define MX25_CMD_RDSCUR      0x48

/* READ comands */
#define MX25_CMD_READ        0x03
#define MX25_CMD_2READ       0xBB
#define MX25_CMD_4READ       0xEB
#define MX25_CMD_FASTREAD    0x0B
#define MX25_CMD_DREAD       0x3B
#define MX25_CMD_QREAD       0x6B
#define MX25_CMD_RDSFDP      0x5A

/* Program comands */
#define MX25_CMD_WREN        0x06
#define MX25_CMD_WRDI        0x04
#define MX25_CMD_PP          0x02
#define MX25_CMD_4PP         0x32
#define MX25_CMD_WRENVSR     0x50

/* Erase comands */
#define MX25_CMD_SE          0x20
#define MX25_CMD_BE32K       0x52
#define MX25_CMD_BE          0xD8
#define MX25_CMD_CE          0x60

/* Mode setting comands */
#define MX25_CMD_DP          0xB9
#define MX25_CMD_RDP         0xAB

/* Reset comands */
#define MX25_CMD_RSTEN       0x66
#define MX25_CMD_RST         0x99
#define MX25_CMD_RSTQIO      0xF5

/* Security comands */
#define MX25_CMD_ERSR        0x44
#define MX25_CMD_PRSR        0x42

/* Suspend/Resume comands */
#define MX25_CMD_PGM_ERS_S   0x75
#define MX25_CMD_PGM_ERS_R   0x7A

#define MX25_CMD_NOP         0x00

#endif /*__SPI_FLASH_MX25_DEFS_H__*/
