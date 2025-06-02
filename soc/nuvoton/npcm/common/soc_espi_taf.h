/*
 * Copyright (c) 2023 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NUVOTON_NPCM_SOC_ESPI_TAF_H_
#define _NUVOTON_NPCM_SOC_ESPI_TAF_H_

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Transmit buffer for eSPI TAF transaction on NPCM              */
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
 * NPCM_TAF_CMP_HEADER_LEN is the preamble length of Type, Length
 * and Tag (i.e. byte 1~byte 3) for flash access completion packet
 * on NPCM
 */
#define NPCM_TAF_CMP_HEADER_LEN                    3

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

/* Timeout for checking transmit buffer available and no completion was sent */
#define NPCM_FLASH_CHK_TIMEOUT                     10000

/* Clear RSTBUFHEADS, FLASH_ACC_TX_AVAIL, and FLASH_ACC_NP_FREE */
#define NPCM_FLASHCTL_ACCESS_MASK     (~(BIT(NPCM_FLASHCTL_RSTBUFHEADS) |   \
					 BIT(NPCM_FLASHCTL_FLASH_NP_FREE) | \
					 BIT(NPCM_FLASHCTL_FLASH_TX_AVAIL)))

/* Flash Sharing Capability Support */
#define NPCM_FLASH_SHARING_CAP_SUPP_CAF            0
#define NPCM_FLASH_SHARING_CAP_SUPP_TAF            2
#define NPCM_FLASH_SHARING_CAP_SUPP_TAF_AND_CAF    3

enum NPCM_ESPI_TAF_REQ {
	NPCM_ESPI_TAF_REQ_READ,
	NPCM_ESPI_TAF_REQ_WRITE,
	NPCM_ESPI_TAF_REQ_ERASE,
	NPCM_ESPI_TAF_REQ_RPMC_OP1,
	NPCM_ESPI_TAF_REQ_RPMC_OP2,
	NPCM_ESPI_TAF_REQ_UNKNOWN,
};

/* NPCM_ESPI_TAF_ERASE_BLOCK_SIZE_4KB is default */
enum NPCM_ESPI_TAF_ERASE_BLOCK_SIZE {
	NPCM_ESPI_TAF_ERASE_BLOCK_SIZE_1KB,
	NPCM_ESPI_TAF_ERASE_BLOCK_SIZE_2KB,
	NPCM_ESPI_TAF_ERASE_BLOCK_SIZE_4KB,
	NPCM_ESPI_TAF_ERASE_BLOCK_SIZE_8KB,
	NPCM_ESPI_TAF_ERASE_BLOCK_SIZE_16KB,
	NPCM_ESPI_TAF_ERASE_BLOCK_SIZE_32KB,
	NPCM_ESPI_TAF_ERASE_BLOCK_SIZE_64KB,
	NPCM_ESPI_TAF_ERASE_BLOCK_SIZE_128KB,
};

/* NPCM_ESPI_TAF_MAX_READ_REQ_64B is default */
enum NPCM_ESPI_TAF_MAX_READ_REQ {
	NPCM_ESPI_TAF_MAX_READ_REQ_64B            = 1,
	NPCM_ESPI_TAF_MAX_READ_REQ_128B,
	NPCM_ESPI_TAF_MAX_READ_REQ_256B,
	NPCM_ESPI_TAF_MAX_READ_REQ_512B,
	NPCM_ESPI_TAF_MAX_READ_REQ_1024B,
	NPCM_ESPI_TAF_MAX_READ_REQ_2048B,
	NPCM_ESPI_TAF_MAX_READ_REQ_4096B,
};

/*
 * The configurations of SPI flash are set in FIU module.
 * Thus, eSPI TAF driver of NPCM does not need additional hardware configuarations.
 * Therefore, define an empty structure here to comply with espi_saf.h
 */
struct espi_saf_hw_cfg {
};
struct espi_saf_flash_cfg {
};

/*
 * 16 flash protection regions
 * Each region is described by:
 * SPI start address. 17-bits = bits[28:12] of SPI address
 * SPI end address. 17-bits = bits[28:12] of last SPI address
 * 1-bit bit map of eSPI master write-erase permission
 * 1-bit bit map of eSPI maste read permission
 *
 * eSPI Host maps threads by a tag number to master numbers.
 * Thread numbers are 4-bit
 * Master numbers are 3-bit
 * Master number    Thread numbers    Description
 *     0                0h, 1h        Host PCH HW init
 *     1                2h, 3h        Host CPU access(HW/BIOS/SMM/SW)
 *     2                4h, 5h        Host PCH ME
 *     3                6h            Host PCH LAN
 *     4                N/A           Not defined/used
 *     5                N/A           EC Firmware portal access
 *     6                9h, Dh        Host PCH IE
 *     7                N/A           Not defined/used
 *
 * NOTE: eSPI SAF specification allows master 0 (Host PCH HW) full
 * access to all protection regions.
 *
 */
#define NPCM_TAF_PR_FLAG_UPDATE_ADDR_RANGE	0x01U

#define NPCM_TAF_MSTR_HOST_PCH		0U
#define NPCM_TAF_MSTR_HOST_CPU		1U
#define NPCM_TAF_MSTR_HOST_PCH_ME	2U
#define NPCM_TAF_MSTR_HOST_PCH_LAN	3U
#define NPCM_TAF_MSTR_RSVD4			4U
#define NPCM_TAF_MSTR_EC			5U
#define NPCM_TAF_MSTR_HOST_PCH_IE	6U

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

struct espi_taf_npcm_pckt {
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

struct npcm_taf_head {
	uint8_t pkt_len;
	uint8_t type;
	uint8_t tag_hlen;
	uint8_t llen;
};

#ifdef __cplusplus
}
#endif

#endif
