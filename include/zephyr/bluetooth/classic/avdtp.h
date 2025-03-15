/** @file
 * @brief Audio/Video Distribution Transport Protocol header.
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AVDTP_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_AVDTP_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AVDTP error code
 */
enum bt_avdtp_err_code {
	/** The response is success, it is not from avdtp spec. */
	BT_AVDTP_SUCCESS = 0x00,
	/** The request is time out without response, it is not from avdtp spec. */
	BT_AVDTP_TIME_OUT = 0xFF,
	/** The request packet header format error */
	BT_AVDTP_BAD_HEADER_FORMAT = 0x01,
	/** The request packet length is not match the assumed length */
	BT_AVDTP_BAD_LENGTH = 0x11,
	/** The requested command indicates an invalid ACP SEID (not addressable) */
	BT_AVDTP_BAD_ACP_SEID = 0x12,
	/** The SEP is in use */
	BT_AVDTP_SEP_IN_USE = 0x13,
	/** The SEP is not in use */
	BT_AVDTP_SEP_NOT_IN_USE = 0x14,
	/** The value of Service Category in the request packet is not defined in AVDTP */
	BT_AVDTP_BAD_SERV_CATEGORY = 0x17,
	/** The requested command has an incorrect payload format */
	BT_AVDTP_BAD_PAYLOAD_FORMAT = 0x18,
	/** The requested command is not supported by the device */
	BT_AVDTP_NOT_SUPPORTED_COMMAND = 0x19,
	/** The reconfigure command is an attempt to reconfigure a transport service codec_cap
	 * of the SEP. Reconfigure is only permitted for application service codec_cap
	 */
	BT_AVDTP_INVALID_CAPABILITIES = 0x1A,
	/** The requested Recovery Type is not defined in AVDTP */
	BT_AVDTP_BAD_RECOVERY_TYPE = 0x22,
	/** The format of Media Transport Capability is not correct */
	BT_AVDTP_BAD_MEDIA_TRANSPORT_FORMAT = 0x23,
	/** The format of Recovery Service Capability is not correct */
	BT_AVDTP_BAD_RECOVERY_FORMAT = 0x25,
	/** The format of Header Compression Service Capability is not correct */
	BT_AVDTP_BAD_ROHC_FORMAT = 0x26,
	/** The format of Content Protection Service Capability is not correct */
	BT_AVDTP_BAD_CP_FORMAT = 0x27,
	/** The format of Multiplexing Service Capability is not correct */
	BT_AVDTP_BAD_MULTIPLEXING_FORMAT = 0x28,
	/** Configuration not supported */
	BT_AVDTP_UNSUPPORTED_CONFIGURAION = 0x29,
	/** Indicates that the ACP state machine is in an invalid state in order to process the
	 * signal. This also includes the situation when an INT receives a request for the
	 * same command that it is currently expecting a response
	 */
	BT_AVDTP_BAD_STATE = 0x31,
};

/** @brief Stream End Point Type */
enum bt_avdtp_sep_type {
	/** Source Role */
	BT_AVDTP_SOURCE = 0,
	/** Sink Role */
	BT_AVDTP_SINK = 1
};

/** @brief Stream End Point Media Type */
enum bt_avdtp_media_type {
	/** Audio Media Type */
	BT_AVDTP_AUDIO = 0x00,
	/** Video Media Type */
	BT_AVDTP_VIDEO = 0x01,
	/** Multimedia Media Type */
	BT_AVDTP_MULTIMEDIA = 0x02
};

/** @brief AVDTP stream endpoint information.
 *  Don't need to care endianness because it is not used for data parsing.
 */
struct bt_avdtp_sep_info {
	/** End Point usage status */
	uint8_t inuse: 1;
	/** Stream End Point ID that is the identifier of the stream endpoint */
	uint8_t id: 6;
	/** Reserved */
	uint8_t reserved: 1;
	/** Stream End-point Type that indicates if the stream end-point is SNK or SRC */
	enum bt_avdtp_sep_type tsep;
	/** Media-type of the End Point
	 * Only @ref BT_AVDTP_AUDIO is supported now.
	 */
	enum bt_avdtp_media_type media_type;
};

/** @brief service category Type */
enum bt_avdtp_service_category {
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

/** @brief AVDTP Stream End Point */
struct bt_avdtp_sep {
	/** Stream End Point information */
	struct bt_avdtp_sep_info sep_info;
	/** Media Transport Channel*/
	struct bt_l2cap_br_chan chan;
	/** the endpoint media data */
	void (*media_data_cb)(struct bt_avdtp_sep *sep, struct net_buf *buf);
	/* semaphore for lock/unlock */
	struct k_sem sem_lock;
	/** avdtp session */
	struct bt_avdtp *session;
	/** SEP state */
	uint8_t state;
	/* Internally used list node */
	sys_snode_t _node;
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AVDTP_H_ */
