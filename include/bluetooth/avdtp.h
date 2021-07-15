/** @file
 * @brief Audio/Video Distribution Transport Protocol header.
 */

/*
 * Copyright (c) 2021 NXP
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AVDTP_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_AVDTP_H_

#ifdef __cplusplus
extern "C" {
#endif

#define BAD_HEADER_FORMAT 0x01
#define BAD_ACP_SEID 0x12
#define INVALID_CAPABILITIES 0x1A
#define BAD_STATE 0x31

/** @brief AVDTP SEID Information */
struct bt_avdtp_seid_info {
	/** Reserved */
	uint8_t rfa0:1;
	/** End Point usage status */
	uint8_t inuse:1;
	/** Stream End Point ID */
	uint8_t id:6;
	/** Reserved */
	uint8_t rfa1:3;
	/** TSEP of the End Point */
	uint8_t tsep:1;
	/** Media-type of the End Point */
	uint8_t media_type:4;
} __packed;

/** @brief service category Type */
enum SERVICE_CATEGORY {
	/** Media Transport */
	BT_AVDTP_SERVICE_MEDIA_TRANSPORT = 0x01,
	/** Reporting */
	BT_AVDTP_SERVICE_REPORTING = 0x02,
	/** Recovery */
	BT_AVDTP_SERVICE_MEDIA_RECOVERY = 0x03,
	/** Content Protection */
	BT_AVDTP_SERVICE_CONTENT_PROTECTION = 0x04,
	/** Header Compression */
	BT_AVDTP_SERVICE_HEADER_COMPRESSION = 0x05,
	/** Multiplexing */
	BT_AVDTP_SERVICE_MULTIPLEXING = 0x06,
	/** Media Codec */
	BT_AVDTP_SERVICE_MEDIA_CODEC = 0x07,
	/** Delay Reporting */
	BT_AVDTP_SERVICE_DELAY_REPORTING = 0x08,
};

/** @brief AVDTP Local SEP*/
struct bt_avdtp_seid_lsep {
	/** Stream End Point information */
	struct bt_avdtp_seid_info sep;
	/** Pointer to next local Stream End Point structure */
	struct bt_avdtp_seid_lsep *next;
	/** Transport Channel*/
	struct bt_l2cap_br_chan media_br_chan;
	/** the endpoint media data */
	void (*media_data_cb)(struct bt_avdtp_seid_lsep *lsep,
				struct net_buf *buf);
	/** avdtp session */
	struct bt_avdtp *session;
	/** SEP state */
	uint8_t seid_state;
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AVDTP_H_ */
