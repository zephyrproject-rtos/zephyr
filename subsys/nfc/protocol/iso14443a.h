/*
 * Copyright (c) 2023 Basalte bv
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_NFC_PROTOCOL_ISO14443A_H_
#define ZEPHYR_SUBSYS_NFC_PROTOCOL_ISO14443A_H_

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/nfc/iso_dep.h>
#include <zephyr/nfc/poller.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>

#define NFC_ISO14443A_CMD_SENS_REQ    0x26U
#define NFC_ISO14443A_CMD_ALL_REQ     0x52U
#define NFC_ISO14443A_CMD_HALT        0x50U
#define NFC_ISO14443A_CMD_SDD_SEL_CL1 0x93U
#define NFC_ISO14443A_CMD_SDD_SEL_CL2 0x95U
#define NFC_ISO14443A_CMD_SDD_SEL_CL3 0x97U
#define NFC_ISO14443A_CMD_RATS        0xE0U
#define NFC_ISO14443A_CMD_PPSS        0xD0U
#define NFC_ISO14443A_PPS_PPS0        0x01U
#define NFC_ISO14443A_PPS_PPS1        0x00U
#define NFC_ISO14443A_CASCADE_TAG     0x88U

#define NFC_ISO14443_PCB_BLOCK_MASK      0xC0U
#define NFC_ISO14443_PCB_BLOCK_NUM       0x01U
#define NFC_ISO14443_PCB_BLOCK_NAD       0x04U
#define NFC_ISO14443_PCB_BLOCK_CID       0x08U
#define NFC_ISO14443_PCB_IBLOCK          0x00U
#define NFC_ISO14443_PCB_IBLOCK_FXD      0x02U
#define NFC_ISO14443_PCB_IBLOCK_CHAINING 0x10U
#define NFC_ISO14443_PCB_RBLOCK          0x80U
#define NFC_ISO14443_PCB_RBLOCK_FXD      0x22U
#define NFC_ISO14443_PCB_RBLOCK_NAK      0x10U
#define NFC_ISO14443_PCB_SBLOCK          0xC0U
#define NFC_ISO14443_PCB_SBLOCK_FXD      0x02U
#define NFC_ISO14443_PCB_SBLOCK_WTX      0x30U

/* NFC Forum Digital t(gt,NFC-A): a target must be powered this long before REQA. */
#define NFC_ISO14443A_GUARD_TIME_US  5100U
/* ISO/IEC 14443-4 (256 * 16 / fc), the unit both FWT and SFGT scale by 2^I. */
#define NFC_ISO14443_FGT_UNIT_US     302U
#define NFC_ISO14443A_MAX_ATS_LEN    254U
#define NFC_ISO14443A_SAK_CASCADE    0x04U
#define NFC_ISO14443A_ATS_TA_PRESENT 0x10U
#define NFC_ISO14443A_ATS_TB_PRESENT 0x20U
#define NFC_ISO14443A_ATS_TC_PRESENT 0x40U
#define NFC_ISO14443A_CRC16_SEED     0x6363
#define NFC_ISO14443A_CRC16_POLY     0x8408

#define NFC_ISO14443_EXCHANGE_MAX_RETRY 3

static inline uint8_t nfc_iso14443a_bcc(const uint8_t *data, size_t len)
{
	uint8_t bcc = 0;

	for (size_t i = 0; i < len; ++i) {
		bcc ^= data[i];
	}
	return bcc;
}

static inline uint16_t nfc_iso14443a_crc(const uint8_t *data, size_t len)
{
	return crc16_reflect(NFC_ISO14443A_CRC16_POLY, NFC_ISO14443A_CRC16_SEED, data, len);
}

static inline void nfc_iso14443a_crc_append(uint8_t *data, size_t len)
{
	sys_put_le16(nfc_iso14443a_crc(data, len), &data[len]);
}

/**
 * @brief Remaining time until @p deadline, in microseconds, capped at @p cap_us.
 *
 * Lets a multi-step operation share one overall deadline: each sub-exchange is
 * bounded by the smaller of its protocol cap and the time left. Returns 0 when
 * the deadline has passed (caller should abort), @p cap_us for K_FOREVER.
 */
static inline uint32_t z_nfc_timeout_us_cap(k_timepoint_t deadline, uint32_t cap_us)
{
	k_timeout_t rem = sys_timepoint_timeout(deadline);

	if (K_TIMEOUT_EQ(rem, K_NO_WAIT)) {
		return 0U;
	}
	if (K_TIMEOUT_EQ(rem, K_FOREVER)) {
		return cap_us;
	}

	return MIN(cap_us, MAX(1U, k_ticks_to_us_ceil32((uint32_t)rem.ticks)));
}

/** @brief How one NFC-A exchange is framed and bounded. */
struct z_nfca_xfer {
	/** Append the NFC-A CRC to the command. */
	bool tx_crc;
	/** Expect and verify the NFC-A CRC on the answer. */
	bool rx_crc;
	/** Deadline for the answer, in microseconds. */
	uint32_t timeout_us;
};

/**
 * @brief Raw NFC-A command exchange.
 *
 * Used by tag-type layers (for example Type 2) that speak NFC-A commands
 * directly. The caller must already hold the poller session.
 *
 * On a frontend the command is framed here, with a software CRC when the
 * hardware has none. An offloading controller does both itself, so the CRC
 * fields of @p xfer are ignored there and @p target selects the activated tag.
 */
int z_nfca_transceive(struct nfc_poller *poller, const struct nfc_target *target, const uint8_t *tx,
		      uint16_t tx_len, uint8_t *rx, uint16_t *rx_len,
		      const struct z_nfca_xfer *xfer);

/**
 * @brief Discover a single NFC-A target.
 *
 * Runs SENS_REQ and anticollision on a frontend, or delegates to the firmware
 * of an offloading controller. Reached through nfc_discover().
 *
 * @retval 0 on success (@p out holds @ref NFC_TECH_A activation data).
 * @retval -EAGAIN if no target answered before the deadline.
 */
int z_nfca_discover(struct nfc_poller *poller, k_timeout_t timeout, struct nfc_target *out);

/**
 * @brief Move a selected NFC-A target to the HALT state.
 *
 * Valid on a target still at layer 3; one that reached ISO-DEP is halted by
 * z_nfc_iso_dep_deselect() instead. Reached through nfc_target_release() and
 * nfc_tag_close(). The caller must already hold the poller session.
 *
 * @retval 0 on success, including the silence a target answers HALT with.
 */
int z_nfca_halt(struct nfc_poller *poller, const struct nfc_target *target, k_timeout_t timeout);

/**
 * @brief Release an ISO-DEP connection with S(DESELECT).
 *
 * Leaves the target in the HALT state. Reached through nfc_tag_close(). The
 * caller must already hold the poller session.
 *
 * @retval 0 on success.
 * @retval -EBADMSG if the target answered with something other than S(DESELECT).
 */
int z_nfc_iso_dep_deselect(struct nfc_iso_dep_tag *tag, k_timeout_t timeout);

#endif /* ZEPHYR_SUBSYS_NFC_PROTOCOL_ISO14443A_H_ */
