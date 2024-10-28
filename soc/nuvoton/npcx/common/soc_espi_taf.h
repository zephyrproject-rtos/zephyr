/*
 * Copyright (c) 2023 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NUVOTON_NPCX_SOC_ESPI_TAF_H_
#define _NUVOTON_NPCX_SOC_ESPI_TAF_H_

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Transmit buffer for eSPI TAF transaction on NPCX              */
/* +-------------+--------------+--------------+---------------+ */
/* |   Byte 3    |    Byte 2    |    Byte 1    |    Byte 0     | */
/* +-------------+--------------+--------------+---------------+ */
/* |   Length    |  Tag |Length |     Type     |    PKT_LEN    | */
/* |   [7:0]     |      |[11:8] |              |               | */
/* +-------------+--------------+--------------+---------------+ */
/* |   Data 3    |     Data 2   |    Data 1    |    Data 0     | */
/* +-------------+--------------+--------------+---------------+ */
/* |   Data 7    |     Data 6   |    Data 5    |    Data 4     | */
/* +-------------+--------------+--------------+---------------+ */
/* |    ...      |     ...      |    ...       |      ...      | */
/* +-------------+--------------+--------------+---------------+ */
/* |  Data 63    |    Data 62   |    Data 61   |    Data 60    | */
/* +-------------+--------------+--------------+---------------+ */
/* PKT_LEN holds the sum of header (Type, Tag and Length) length */
/* and data length                                               */

/*
 * NPCX_TAF_CMP_HEADER_LEN is the preamble length of Type, Length
 * and Tag (i.e. byte 1~byte 3) for flash access completion packet
 * on NPCX
 */
#define NPCX_TAF_CMP_HEADER_LEN                    3

/* Successful Completion Without Data     */
#define CYC_SCS_CMP_WITHOUT_DATA                   0x06
/* Successful middle Completion With Data */
#define CYC_SCS_CMP_WITH_DATA_MIDDLE               0x09
/* Successful first Completion With Data  */
#define CYC_SCS_CMP_WITH_DATA_FIRST                0x0B
/* Successful last Completion With Data   */
#define CYC_SCS_CMP_WITH_DATA_LAST                 0x0D
/* Successful only Completion With Data   */
#define CYC_SCS_CMP_WITH_DATA_ONLY                 0x0F
/* Unsuccessful Completion Without Data   */
#define CYC_UNSCS_CMP_WITHOUT_DATA                 0x08
/* Unsuccessful Last Completion Without Data */
#define CYC_UNSCS_CMP_WITHOUT_DATA_LAST            0x0C
/* Unsuccessful Only Completion Without Data */
#define CYC_UNSCS_CMP_WITHOUT_DATA_ONLY            0x0E

/* ESPI TAF RPMC OP1 instruction */
#define ESPI_TAF_RPMC_OP1_CMD                      0x9B
/* ESPI TAF RPMC OP2 instruction */
#define ESPI_TAF_RPMC_OP2_CMD                      0x96

/* Timeout for checking transmit buffer available and no completion was sent */
#define NPCX_FLASH_CHK_TIMEOUT                     10000

/* Clear RSTBUFHEADS, FLASH_ACC_TX_AVAIL, and FLASH_ACC_NP_FREE */
#define NPCX_FLASHCTL_ACCESS_MASK     (~(BIT(NPCX_FLASHCTL_RSTBUFHEADS) |   \
					 BIT(NPCX_FLASHCTL_FLASH_NP_FREE) | \
					 BIT(NPCX_FLASHCTL_FLASH_TX_AVAIL)))

/* Flash Sharing Capability Support */
#define NPCX_FLASH_SHARING_CAP_SUPP_CAF            0
#define NPCX_FLASH_SHARING_CAP_SUPP_TAF            2
#define NPCX_FLASH_SHARING_CAP_SUPP_TAF_AND_CAF    3

enum NPCX_ESPI_TAF_REQ {
	NPCX_ESPI_TAF_REQ_READ,
	NPCX_ESPI_TAF_REQ_WRITE,
	NPCX_ESPI_TAF_REQ_ERASE,
	NPCX_ESPI_TAF_REQ_RPMC_OP1,
	NPCX_ESPI_TAF_REQ_RPMC_OP2,
	NPCX_ESPI_TAF_REQ_UNKNOWN,
};

/* NPCX_ESPI_TAF_ERASE_BLOCK_SIZE_4KB is default */
enum NPCX_ESPI_TAF_ERASE_BLOCK_SIZE {
	NPCX_ESPI_TAF_ERASE_BLOCK_SIZE_1KB,
	NPCX_ESPI_TAF_ERASE_BLOCK_SIZE_2KB,
	NPCX_ESPI_TAF_ERASE_BLOCK_SIZE_4KB,
	NPCX_ESPI_TAF_ERASE_BLOCK_SIZE_8KB,
	NPCX_ESPI_TAF_ERASE_BLOCK_SIZE_16KB,
	NPCX_ESPI_TAF_ERASE_BLOCK_SIZE_32KB,
	NPCX_ESPI_TAF_ERASE_BLOCK_SIZE_64KB,
	NPCX_ESPI_TAF_ERASE_BLOCK_SIZE_128KB,
};

/* NPCX_ESPI_TAF_MAX_READ_REQ_64B is default */
enum NPCX_ESPI_TAF_MAX_READ_REQ {
	NPCX_ESPI_TAF_MAX_READ_REQ_64B            = 1,
	NPCX_ESPI_TAF_MAX_READ_REQ_128B,
	NPCX_ESPI_TAF_MAX_READ_REQ_256B,
	NPCX_ESPI_TAF_MAX_READ_REQ_512B,
	NPCX_ESPI_TAF_MAX_READ_REQ_1024B,
	NPCX_ESPI_TAF_MAX_READ_REQ_2048B,
	NPCX_ESPI_TAF_MAX_READ_REQ_4096B,
};

/*
 * The configurations of SPI flash are set in FIU module.
 * Thus, eSPI TAF driver of NPCX does not need additional hardware configuarations.
 * Therefore, define an empty structure here to comply with espi_saf.h
 */
struct espi_saf_hw_cfg {
};

struct espi_saf_pr {
	uint32_t start;
	uint32_t end;
	uint16_t override_r;
	uint16_t override_w;
	uint8_t  master_bm_we;
	uint8_t  master_bm_rd;
	uint8_t  pr_num;
	uint8_t  flags;
};

struct espi_saf_protection {
	size_t nregions;
	const struct espi_saf_pr *pregions;
};

struct espi_taf_npcx_pckt {
	uint8_t tag;
	uint8_t *data;
};

struct espi_taf_pckt {
	uint8_t  type;
	uint8_t  tag;
	uint32_t addr;
	uint16_t len;
	uint32_t src[16];
};

struct npcx_taf_head {
	uint8_t pkt_len;
	uint8_t type;
	uint8_t tag_hlen;
	uint8_t llen;
};

int npcx_init_taf(const struct device *dev, sys_slist_t *callbacks);

#ifdef __cplusplus
}
#endif

#endif
