/*
 * Copyright (c) 2025 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ESPI_NCT_H_
#define ZEPHYR_INCLUDE_DRIVERS_ESPI_NCT_H_

struct espi_comm_hdr {
	uint8_t cyc;
	uint8_t len_h : 4;
	uint8_t tag : 4;
	uint8_t len_l;
} __packed;

struct espi_flash_rwe {
	uint8_t cyc;
	uint8_t len_h : 4;
	uint8_t tag : 4;
	uint8_t len_l;
	uint32_t addr_be;
	uint8_t data[];
} __packed;

struct espi_flash_cmplt {
	uint8_t cyc;
	uint8_t len_h : 4;
	uint8_t tag : 4;
	uint8_t len_l;
	uint8_t data[];
} __packed;

#define NCT_FLASH_RX_CYC                FIELD(8, 8)
#define NCT_FLASH_RX_LEN_H              FIELD(16, 4)
#define NCT_FLASH_RX_TAG                FIELD(20, 4)
#define NCT_FLASH_RX_LEN_L              FIELD(24, 8)
#define NCT_FLASH_RX_LEN_H_SHIFT        8

#define ESPI_FLASH_READ_CYCLE_TYPE   0x00
#define ESPI_FLASH_WRITE_CYCLE_TYPE  0x01
#define ESPI_FLASH_ERASE_CYCLE_TYPE  0x02

#define ESPI_FLASH_SUC_CMPLT                0x06
#define ESPI_FLASH_UNSUC_CMPLT              0x0c
#define ESPI_FLASH_SUC_CMPLT_D_ONLY         0x0f

#define ESPI_FLASH_BUF_SIZE                 192
#define ESPI_PLD_LEN_MAX                    (1UL << 12)

#define ESPI_FLASH_RESP_LEN	3

#endif
