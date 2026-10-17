/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 * Copyright (c) 2026 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_LORAWAN_PACKETS_H_
#define ZEPHYR_INCLUDE_LORAWAN_PACKETS_H_

/**
 * @file
 * @brief Header file for the LoRaWAN packet definitions.
 * @ingroup lorawan_api
 */

#include <stdint.h>

#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

/* LoRaWAN packet field sizes. */
#define LORAWAN_PKT_MIC_SIZE    4U
#define LORAWAN_PKT_MHDR_SIZE   1U
#define LORAWAN_PKT_FPORT_SIZE  1U
#define LORAWAN_PKT_CFLIST_SIZE 16U

/* MHDR message types and bit fields. */
#define LORAWAN_PKT_MHDR_JOIN_REQUEST     0x00U
#define LORAWAN_PKT_MHDR_JOIN_ACCEPT      0x20U
#define LORAWAN_PKT_MHDR_UNCONF_DATA_UP   0x40U
#define LORAWAN_PKT_MHDR_UNCONF_DATA_DOWN 0x60U
#define LORAWAN_PKT_MHDR_CONF_DATA_UP     0x80U
#define LORAWAN_PKT_MHDR_CONF_DATA_DOWN   0xA0U
#define LORAWAN_PKT_MHDR_TYPE_MASK        0xE0U
#define LORAWAN_PKT_MHDR_MAJOR_MASK       0x03U
#define LORAWAN_PKT_MHDR_MAJOR_R1         0x00U

/* Join-accept DLSettings bit fields. */
#define LORAWAN_PKT_DL_RX1_DR_OFFSET_MASK 0x70U
#define LORAWAN_PKT_DL_RX2_DATARATE_MASK  0x0FU

/* Data-frame direction and FCtrl fields. */
#define LORAWAN_PKT_DIR_UPLINK           0U
#define LORAWAN_PKT_DIR_DOWNLINK         1U
#define LORAWAN_PKT_FCTRL_ADR            0x80U
#define LORAWAN_PKT_FCTRL_ACK            0x20U
#define LORAWAN_PKT_FCTRL_FPENDING       0x10U
#define LORAWAN_PKT_FCTRL_FOPTS_LEN_MASK 0x0FU

/* B0 block type used when calculating a data-frame MIC. */
#define LORAWAN_PKT_DATA_MIC_BLOCK_TYPE 0x49U

/**
 * LoRaWAN OTAA join request (23 bytes, unencrypted):
 *
 *       +--------------------+
 *    0  | MHDR (1)           |  0x00 (Join Request)
 *       +--------------------+
 *    1  | JoinEUI (8)        |  Big-endian application identifier
 *       +--------------------+
 *    9  | DevEUI (8)         |  Big-endian device identifier
 *       +--------------------+
 *   17  | DevNonce (2)       |  Replay protection counter
 *       +--------------------+
 *   19  | MIC (4)            |  aes128_cmac(NwkKey, MHDR..DevNonce)[0:3]
 *       +--------------------+
 */
struct lorawan_pkt_join_request {
	uint8_t mhdr;         /**< MAC header. */
	uint8_t join_eui[8];  /**< Join server identifier, big-endian. */
	uint8_t dev_eui[8];   /**< End-device identifier, big-endian. */
	uint8_t dev_nonce[2]; /**< Device nonce, little-endian. */
	uint8_t mic[4];       /**< Truncated AES-CMAC message integrity code. */
} __packed;

/**
 * LoRaWAN OTAA join accept (raw frame: 17 or 33 bytes):
 *
 *       +--------------------+
 *    0  | MHDR (1)           |  0x20 (Join Accept)
 *       +--------------------+
 *    1  | encrypted payload  |  aes128_encrypt(NwkKey) -- not decrypt
 *       | (16 or 32)         |
 *       +--------------------+
 *
 * Decrypted payload (offsets relative to payload start):
 *
 *       +--------------------+
 *    0  | JoinNonce (3)      |  Server-side nonce
 *       +--------------------+
 *    3  | NetID (3)          |  Network identifier
 *       +--------------------+
 *    6  | DevAddr (4)        |  Assigned device address
 *       +--------------------+
 *   10  | DLSettings (1)     |  RX1DRoffset[6:4] | RX2DataRate[3:0]
 *       +--------------------+
 *   11  | RxDelay (1)        |  Seconds (0 means 1)
 *       +--------------------+
 *   12  | [CFList] (0/16)    |  Optional channel list
 *       +--------------------+
 *       | MIC (4)            |  aes128_cmac(NwkKey, MHDR..last field)[0:3]
 *       +--------------------+
 */
struct lorawan_pkt_join_accept {
	uint8_t mhdr;              /**< MAC header. */
	uint8_t join_nonce[3];     /**< Join server nonce, little-endian. */
	uint8_t net_id[3];         /**< Network identifier, little-endian. */
	uint8_t dev_addr[4];       /**< Assigned device address, little-endian. */
	uint8_t dl_settings;       /**< RX1 data-rate offset and RX2 data rate. */
	uint8_t rx_delay;          /**< Delay before the first receive window. */
	uint8_t payload_and_mic[]; /**< Optional CFList followed by the message integrity code. */
} __packed;

/**
 * LoRaWAN MAC data frame (uplink or downlink):
 *
 *       +--------------------+
 *    0  | MHDR (1)           |  Frame type + major version
 *       +--------------------+
 *    1  | DevAddr (4)        |  Little-endian device address
 *       +--------------------+
 *    5  | FCtrl (1)          |  UL: ADR|ADRACKReq|ACK|ClassB|FOptsLen[3:0]
 *       |                    |  DL: ADR|RFU|ACK|FPending|FOptsLen[3:0]
 *       +--------------------+
 *    6  | FCnt (2)           |  Frame counter (low 16 bits)
 *       +--------------------+
 *    8  | [FOpts] (0..15)    |  MAC commands
 *       +--------------------+  --- optional from here ---
 *  8+N  | FPort (1)          |  0 = MAC (FNwkSIntKey), 1-223 = app (AppSKey)
 *       +--------------------+
 *       | FRMPayload (M)     |  Encrypted payload
 *       +--------------------+
 *       | MIC (4)            |  aes128_cmac over B0 + MHDR..FRMPayload
 *       +--------------------+
 */
struct lorawan_pkt_data_header {
	uint8_t mhdr;              /**< MAC header. */
	uint8_t dev_addr[4];       /**< Device address, little-endian. */
	uint8_t fctrl;             /**< Frame control flags and FOpts length. */
	uint8_t fcnt[2];           /**< Least-significant 16 bits of the frame counter. */
	uint8_t payload_and_mic[]; /**< FOpts, optional payload, and message integrity code. */
} __packed;

/**
 * MIC B0 block (prepended to CMAC input, never transmitted):
 *
 *       +--------------------+
 *    0  | 0x49 (1)           |
 *       +--------------------+
 *    1  | 0x00 (4)           |  Reserved
 *       +--------------------+
 *    5  | Dir (1)            |  0 = uplink, 1 = downlink
 *       +--------------------+
 *    6  | DevAddr (4)        |
 *       +--------------------+
 *   10  | FCnt (4)           |  Full 32-bit frame counter
 *       +--------------------+
 *   14  | 0x00 (1)           |
 *       +--------------------+
 *   15  | MsgLen (1)         |  MHDR..FRMPayload length (excl. MIC)
 *       +--------------------+
 */
struct lorawan_pkt_data_mic_block {
	uint8_t type;        /**< Block type, always 0x49. */
	uint8_t reserved[4]; /**< Reserved zero bytes. */
	uint8_t direction;   /**< Frame direction: uplink or downlink. */
	uint8_t dev_addr[4]; /**< Device address, little-endian. */
	uint8_t fcnt[4];     /**< Full 32-bit frame counter, little-endian. */
	uint8_t pad;         /**< Reserved zero byte. */
	uint8_t msg_len;     /**< Length of the data packet excluding its MIC. */
} __packed;

/** @endcond */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_LORAWAN_PACKETS_H_ */
