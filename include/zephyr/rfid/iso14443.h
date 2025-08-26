/*
 * Copyright (c) 2023 Basalte bv
 * Copyright (c) 2025 Conny Marco Menebröcker
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for RFID ISO 14443 Type A
 */

#ifndef ZEPHYR_INCLUDE_RFID_ISO14443A_H_
#define ZEPHYR_INCLUDE_RFID_ISO14443A_H_

#include <errno.h>

#include <zephyr/drivers/rfid.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief TODO
 */
#define RFID_ISO14443A_CMD_SENS_REQ 0x26U

/**
 * @brief TODO
 */
#define RFID_ISO14443A_CMD_ALL_REQ 0x52U

/**
 * @brief TODO
 */
#define RFID_ISO14443A_CMD_HALT 0x50U

/**
 * @brief TODO
 */
#define RFID_ISO14443A_CMD_SDD_SEL_CL1 0x93U

/**
 * @brief TODO
 */
#define RFID_ISO14443A_CMD_SDD_SEL_CL2 0x95U

/**
 * @brief TODO
 */
#define RFID_ISO14443A_CMD_SDD_SEL_CL3 0x97U

/**
 * @brief TODO
 */
#define RFID_ISO14443A_CMD_RATS 0xE0U

/**
 * @brief TODO
 */
#define RFID_ISO14443A_CMD_PPSS 0xD0U

/**
 * @brief TODO
 */
#define RFID_ISO14443A_PPS_PPS0 0x01U

/**
 * @brief TODO
 */
#define RFID_ISO14443A_PPS_PPS1 0x00U

/**
 * @brief TODO
 */
#define RFID_ISO14443A_CASCADE_TAG 0x88U

#define RFID_ISO14443_PCB_BLOCK_MASK 0xC0U
#define RFID_ISO14443_PCB_BLOCK_NUM  0x01U
#define RFID_ISO14443_PCB_BLOCK_NAD  0x04U
#define RFID_ISO14443_PCB_BLOCK_CID  0x08U

#define RFID_ISO14443_PCB_IBLOCK     0x00U
#define RFID_ISO14443_PCB_IBLOCK_FXD 0x02U

#define RFID_ISO14443_PCB_IBLOCK_CHAINING 0x10U

#define RFID_ISO14443_PCB_RBLOCK     0x80U
#define RFID_ISO14443_PCB_RBLOCK_FXD 0x22U

#define RFID_ISO14443_PCB_RBLOCK_NAK 0x10U

#define RFID_ISO14443_PCB_SBLOCK     0xC0U
#define RFID_ISO14443_PCB_SBLOCK_FXD 0x02U

#define RFID_ISO14443_PCB_SBLOCK_MASK     0x30U
#define RFID_ISO14443_PCB_SBLOCK_WTX      0x30U
#define RFID_ISO14443_PCB_SBLOCK_DESELECT 0x00U

/**
 * @brief Maximum data length code for ISO/IEC 14443 A ATQA.
 */
#define RFID_ISO14443A_MAX_ATQA_LEN    2U
/**
 * @brief Maximum data length code for ISO/IEC 14443 A UID.
 */
#define RFID_ISO14443A_MAX_UID_LEN     10U
/**
 * @brief Maximum data length code for ISO/IEC 14443 A ATS.
 */
#define RFID_ISO14443A_MAX_ATS_LEN     254U
/**
 * @brief Maximum data length code for ISO/IEC 14443 A history in ATS.
 */
#define RFID_ISO14443A_MAX_HISTORY_LEN 249U

/**
 * @brief ATQA UID single size type (4 bytes).
 */
#define RFID_ISO14443A_ATQA_UID_SINGLE 0x0U
/**
 * @brief ATQA UID double size type (7 bytes).
 */
#define RFID_ISO14443A_ATQA_UID_DOUBLE 0x1U
/**
 * @brief ATQA UID triple size type (10 bytes).
 */
#define RFID_ISO14443A_ATQA_UID_TRIPLE 0x2U

/**
 * @brief TODO
 */
#define RFID_ISO14443A_SAK_CASCADE 0x04U

/**
 * @brief TODO
 */
#define RFID_ISO14443A_SAK_ATS_SUPPORTED 0x20U

#define RFID_ISO14443A_ATS_TA_PRESENT 0x10U
#define RFID_ISO14443A_ATS_TB_PRESENT 0x20U
#define RFID_ISO14443A_ATS_TC_PRESENT 0x40U

/**
 * @brief TODO
 */
#define RFID_ISO14443A_CRC16_SEED 0x6363
/**
 * @brief TODO
 */
#define RFID_ISO14443A_CRC16_POLY 0x8408

struct rfid_iso14443a_info {
	uint8_t atqa[RFID_ISO14443A_MAX_ATQA_LEN];
	uint8_t uid[RFID_ISO14443A_MAX_UID_LEN];
	uint8_t uid_len;
	uint8_t sak;
	union {
		uint8_t fsci;
		uint8_t fsdi;
	};
	uint8_t cid;
	bool cid_supported;
	bool nad_supported;
	rfid_mode_t modes;
	uint8_t sfgi;
	uint8_t fwi;
	uint8_t history[RFID_ISO14443A_MAX_HISTORY_LEN];
	uint8_t history_len;
	uint8_t block_num;
};

int rfid_iso14443a_request(const struct device *dev, uint8_t *atqa, bool sens);

/* Single Device Detect */
int rfid_iso14443a_sdd(const struct device *dev, struct rfid_iso14443a_info *info);

/* Request for Answer To Select */
int rfid_iso14443a_rats(const struct device *dev, struct rfid_iso14443a_info *info, uint8_t cid);

/* Protocol and Parameter Selection */
int rfid_iso14443a_pps(const struct device *dev, struct rfid_iso14443a_info *info,
		       rfid_mode_t modes);

int rfid_iso14443a_halt(const struct device *dev);

int rfid_iso14443a_exchange(const struct device *dev, struct rfid_iso14443a_info *info,
			    const uint8_t *tx_data, uint16_t tx_len, uint8_t *rx_data,
			    uint16_t *rx_len, uint8_t nad);

/* Target mode handler */
int rfid_iso14443a_listen(const struct device *dev, struct rfid_iso14443a_info *info);

int rfid_iso14443a_receive(const struct device *dev, struct rfid_iso14443a_info *info,
			   uint8_t *rx_data, uint16_t *rx_len, uint8_t *nad);

int rfid_iso14443a_transmit(const struct device *dev, struct rfid_iso14443a_info *info,
			    const uint8_t *tx_data, uint16_t tx_len, uint8_t nad);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_RFID_ISO14443A_H_ */
