/*
 * avdtp_internal.h - avdtp handling

 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright 2021,2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/classic/avdtp.h>

/* @brief A2DP ROLE's */
#define A2DP_SRC_ROLE 0x00
#define A2DP_SNK_ROLE 0x01

/* @brief AVDTP Role */
#define BT_AVDTP_INT 0x00
#define BT_AVDTP_ACP 0x01

#define BT_L2CAP_PSM_AVDTP 0x0019

/* AVDTP SIGNAL HEADER - Packet Type*/
#define BT_AVDTP_PACKET_TYPE_SINGLE   0x00
#define BT_AVDTP_PACKET_TYPE_START    0x01
#define BT_AVDTP_PACKET_TYPE_CONTINUE 0x02
#define BT_AVDTP_PACKET_TYPE_END      0x03

/* AVDTP SIGNAL HEADER - MESSAGE TYPE */
#define BT_AVDTP_CMD        0x00
#define BT_AVDTP_GEN_REJECT 0x01
#define BT_AVDTP_ACCEPT     0x02
#define BT_AVDTP_REJECT     0x03

/* @brief AVDTP SIGNAL HEADER - Signal Identifier */
#define BT_AVDTP_DISCOVER             0x01
#define BT_AVDTP_GET_CAPABILITIES     0x02
#define BT_AVDTP_SET_CONFIGURATION    0x03
#define BT_AVDTP_GET_CONFIGURATION    0x04
#define BT_AVDTP_RECONFIGURE          0x05
#define BT_AVDTP_OPEN                 0x06
#define BT_AVDTP_START                0x07
#define BT_AVDTP_CLOSE                0x08
#define BT_AVDTP_SUSPEND              0x09
#define BT_AVDTP_ABORT                0x0a
#define BT_AVDTP_SECURITY_CONTROL     0x0b
#define BT_AVDTP_GET_ALL_CAPABILITIES 0x0c
#define BT_AVDTP_DELAYREPORT          0x0d

/* @brief AVDTP STREAM STATE */
#define BT_AVDTP_STREAM_STATE_IDLE       0x01
#define BT_AVDTP_STREAM_STATE_CONFIGURED 0x02
#define BT_AVDTP_STREAM_STATE_OPEN       0x03
#define BT_AVDTP_STREAM_STATE_STREAMING  0x04
#define BT_AVDTP_STREAM_STATE_CLOSING    0x05

/* @brief AVDTP Media TYPE */
#define BT_AVDTP_SERVICE_CAT_MEDIA_TRANSPORT    0x01
#define BT_AVDTP_SERVICE_CAT_REPORTING          0x02
#define BT_AVDTP_SERVICE_CAT_RECOVERY           0x03
#define BT_AVDTP_SERVICE_CAT_CONTENT_PROTECTION 0x04
#define BT_AVDTP_SERVICE_CAT_HDR_COMPRESSION    0x05
#define BT_AVDTP_SERVICE_CAT_MULTIPLEXING       0x06
#define BT_AVDTP_SERVICE_CAT_MEDIA_CODEC        0x07
#define BT_AVDTP_SERVICE_CAT_DELAYREPORTING     0x08

/* AVDTP Error Codes */
#define BT_AVDTP_SUCCESS                        0x00
#define BT_AVDTP_ERR_BAD_HDR_FORMAT             0x01
#define BT_AVDTP_ERR_BAD_LENGTH                 0x11
#define BT_AVDTP_ERR_BAD_ACP_SEID               0x12
#define BT_AVDTP_ERR_SEP_IN_USE                 0x13
#define BT_AVDTP_ERR_SEP_NOT_IN_USE             0x14
#define BT_AVDTP_ERR_BAD_SERV_CATEGORY          0x17
#define BT_AVDTP_ERR_BAD_PAYLOAD_FORMAT         0x18
#define BT_AVDTP_ERR_NOT_SUPPORTED_COMMAND      0x19
#define BT_AVDTP_ERR_INVALID_CAPABILITIES       0x1a
#define BT_AVDTP_ERR_BAD_RECOVERY_TYPE          0x22
#define BT_AVDTP_ERR_BAD_MEDIA_TRANSPORT_FORMAT 0x23
#define BT_AVDTP_ERR_BAD_RECOVERY_FORMAT        0x25
#define BT_AVDTP_ERR_BAD_ROHC_FORMAT            0x26
#define BT_AVDTP_ERR_BAD_CP_FORMAT              0x27
#define BT_AVDTP_ERR_BAD_MULTIPLEXING_FORMAT    0x28
#define BT_AVDTP_ERR_UNSUPPORTED_CONFIGURAION   0x29
#define BT_AVDTP_ERR_BAD_STATE                  0x31

#define BT_AVDTP_MIN_SEID 0x01
#define BT_AVDTP_MAX_SEID 0x3E

#define BT_AVDTP_RTP_VERSION 2

struct bt_avdtp;
struct bt_avdtp_req;
struct bt_avdtp_sep_info;

/** @brief AVDTP SEID Information AVDTP_SPEC V13 Table 8.8 */
struct bt_avdtp_sep_data {
#ifdef CONFIG_LITTLE_ENDIAN
	uint8_t rfa0: 1;
	uint8_t inuse: 1;
	uint8_t id: 6;
	uint8_t rfa1: 3;
	uint8_t tsep: 1;
	uint8_t media_type: 4;
#else
	uint8_t id: 6;
	uint8_t inuse: 1;
	uint8_t rfa0: 1;
	uint8_t media_type: 4;
	uint8_t tsep: 1;
	uint8_t rfa1: 3;
#endif
} __packed;

typedef int (*bt_avdtp_func_t)(struct bt_avdtp_req *req);

struct bt_avdtp_req {
	uint8_t sig;
	uint8_t tid;
	uint8_t status;
	bt_avdtp_func_t func;
};

struct bt_avdtp_single_sig_hdr {
	uint8_t hdr;
	uint8_t signal_id;
} __packed;

struct bt_avdtp_media_hdr {
#ifdef CONFIG_LITTLE_ENDIAN
	uint8_t CSRC_count: 4;
	uint8_t header_extension: 1;
	uint8_t padding: 1;
	uint8_t RTP_version: 2;
	uint8_t playload_type: 7;
	uint8_t marker: 1;
#else
	uint8_t RTP_version: 2;
	uint8_t padding: 1;
	uint8_t header_extension: 1;
	uint8_t CSRC_count: 4;
	uint8_t marker: 1;
	uint8_t playload_type: 7;
#endif
	uint16_t sequence_number;
	uint32_t time_stamp;
	uint32_t synchronization_source;
} __packed;

struct bt_avdtp_discover_params {
	struct bt_avdtp_req req;
	struct net_buf *buf;
};

struct bt_avdtp_get_capabilities_params {
	struct bt_avdtp_req req;
	uint8_t stream_endpoint_id;
	struct net_buf *buf;
};

struct bt_avdtp_set_configuration_params {
	struct bt_avdtp_req req;
	struct bt_avdtp_sep *sep;
	uint8_t acp_stream_ep_id;
	uint8_t int_stream_endpoint_id;
	uint8_t media_type;
	uint8_t media_codec_type;
	uint8_t codec_specific_ie_len;
	uint8_t *codec_specific_ie;
};

/* avdtp_open, avdtp_close, avdtp_start, avdtp_suspend */
struct bt_avdtp_ctrl_params {
	struct bt_avdtp_req req;
	struct bt_avdtp_sep *sep;
	uint8_t acp_stream_ep_id;
};

struct bt_avdtp_ops_cb {
	void (*connected)(struct bt_avdtp *session);

	void (*disconnected)(struct bt_avdtp *session);

	struct net_buf *(*alloc_buf)(struct bt_avdtp *session);

	int (*discovery_ind)(struct bt_avdtp *session, uint8_t *errcode);

	int (*get_capabilities_ind)(struct bt_avdtp *session, struct bt_avdtp_sep *sep,
				    struct net_buf *rsp_buf, uint8_t *errcode);

	int (*set_configuration_ind)(struct bt_avdtp *session, struct bt_avdtp_sep *sep,
				     uint8_t int_seid, struct net_buf *buf, uint8_t *errcode);

	int (*re_configuration_ind)(struct bt_avdtp *session, struct bt_avdtp_sep *sep,
				    uint8_t int_seid, struct net_buf *buf, uint8_t *errcode);

	int (*open_ind)(struct bt_avdtp *session, struct bt_avdtp_sep *sep, uint8_t *errcode);

	int (*close_ind)(struct bt_avdtp *session, struct bt_avdtp_sep *sep, uint8_t *errcode);

	int (*start_ind)(struct bt_avdtp *session, struct bt_avdtp_sep *sep, uint8_t *errcode);

	int (*suspend_ind)(struct bt_avdtp *session, struct bt_avdtp_sep *sep, uint8_t *errcode);

	int (*abort_ind)(struct bt_avdtp *session, struct bt_avdtp_sep *sep, uint8_t *errcode);

	/* stream l2cap is closed */
	int (*stream_l2cap_disconnected)(struct bt_avdtp *session, struct bt_avdtp_sep *sep);
};

/** @brief Global AVDTP session structure. */
struct bt_avdtp {
	struct bt_l2cap_br_chan br_chan;
	struct bt_avdtp_req *req;
	const struct bt_avdtp_ops_cb *ops;
	struct bt_avdtp_sep *current_sep;
	struct k_work_delayable timeout_work;
	/* semaphore for lock/unlock */
	struct k_sem sem_lock;
};

struct bt_avdtp_event_cb {
	int (*accept)(struct bt_conn *conn, struct bt_avdtp **session);
};

/* Initialize AVDTP layer*/
int bt_avdtp_init(void);

/* Application register with AVDTP layer */
int bt_avdtp_register(struct bt_avdtp_event_cb *cb);

/* AVDTP connect */
int bt_avdtp_connect(struct bt_conn *conn, struct bt_avdtp *session);

/* AVDTP disconnect */
int bt_avdtp_disconnect(struct bt_avdtp *session);

/* AVDTP SEP register function */
int bt_avdtp_register_sep(uint8_t media_type, uint8_t sep_type, struct bt_avdtp_sep *sep);

/* AVDTP Discover Request */
int bt_avdtp_discover(struct bt_avdtp *session, struct bt_avdtp_discover_params *param);

/* Parse the sep of discovered result */
int bt_avdtp_parse_sep(struct net_buf *buf, struct bt_avdtp_sep_info *sep_info);

/* AVDTP Get Capabilities */
int bt_avdtp_get_capabilities(struct bt_avdtp *session,
			      struct bt_avdtp_get_capabilities_params *param);

/* Parse the codec type of capabilities */
int bt_avdtp_parse_capability_codec(struct net_buf *buf, uint8_t *codec_type,
				    uint8_t **codec_info_element, uint16_t *codec_info_element_len);

/* AVDTP Set Configuration */
int bt_avdtp_set_configuration(struct bt_avdtp *session,
			       struct bt_avdtp_set_configuration_params *param);

/* AVDTP reconfigure */
int bt_avdtp_reconfigure(struct bt_avdtp *session, struct bt_avdtp_set_configuration_params *param);

/* AVDTP OPEN */
int bt_avdtp_open(struct bt_avdtp *session, struct bt_avdtp_ctrl_params *param);

/* AVDTP CLOSE */
int bt_avdtp_close(struct bt_avdtp *session, struct bt_avdtp_ctrl_params *param);

/* AVDTP START */
int bt_avdtp_start(struct bt_avdtp *session, struct bt_avdtp_ctrl_params *param);

/* AVDTP SUSPEND */
int bt_avdtp_suspend(struct bt_avdtp *session, struct bt_avdtp_ctrl_params *param);

/* AVDTP ABORT */
int bt_avdtp_abort(struct bt_avdtp *session, struct bt_avdtp_ctrl_params *param);

/* AVDTP send data */
int bt_avdtp_send_media_data(struct bt_avdtp_sep *sep, struct net_buf *buf);

/* get media l2cap connection MTU */
uint32_t bt_avdtp_get_media_mtu(struct bt_avdtp_sep *sep);
