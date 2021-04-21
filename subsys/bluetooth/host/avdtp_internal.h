/*
 * avdtp_internal.h - avdtp handling

 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bluetooth/avdtp.h>

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
#define BT_AVDTP_STREAM_STATE_IDLE        0x01
#define BT_AVDTP_STREAM_STATE_CONFIGURED  0x02
#define BT_AVDTP_STREAM_STATE_OPEN        0x03
#define BT_AVDTP_STREAM_STATE_STREAMING   0x04
#define BT_AVDTP_STREAM_STATE_CLOSING     0x05

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

#define BT_AVDTP_MAX_MTU CONFIG_BT_L2CAP_RX_MTU

#define BT_AVDTP_MIN_SEID 0x01
#define BT_AVDTP_MAX_SEID 0x3E

struct bt_avdtp;
struct bt_avdtp_req;

typedef int (*bt_avdtp_func_t)(struct bt_avdtp *session,
			       struct bt_avdtp_req *req);

struct bt_avdtp_req {
	uint8_t sig;
	uint8_t tid;
	bt_avdtp_func_t func;
	struct k_delayed_work timeout_work;
};

struct bt_avdtp_single_sig_hdr {
	uint8_t hdr;
	uint8_t signal_id;
} __packed;

#define BT_AVDTP_SIG_HDR_LEN sizeof(struct bt_avdtp_single_sig_hdr)

struct bt_avdtp_ind_cb {
	/*
	 * discovery_ind;
	 * get_capabilities_ind;
	 * set_configuration_ind;
	 * open_ind;
	 * start_ind;
	 * suspend_ind;
	 * close_ind;
	 */
};

struct bt_avdtp_cap {
	uint8_t cat;
	uint8_t len;
	uint8_t data[0];
};

struct bt_avdtp_sep {
	uint8_t seid;
	uint8_t len;
	struct bt_avdtp_cap caps[0];
};

struct bt_avdtp_discover_params {
	struct bt_avdtp_req req;
	uint8_t status;
	struct bt_avdtp_sep *caps;
};

/** @brief Global AVDTP session structure. */
struct bt_avdtp {
	struct bt_l2cap_br_chan br_chan;
	struct bt_avdtp_stream *streams; /* List of AV streams */
	struct bt_avdtp_req *req;
};

struct bt_avdtp_event_cb {
	struct bt_avdtp_ind_cb *ind;
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
int bt_avdtp_register_sep(uint8_t media_type, uint8_t role,
				struct bt_avdtp_seid_lsep *sep);

/* AVDTP Discover Request */
int bt_avdtp_discover(struct bt_avdtp *session,
		      struct bt_avdtp_discover_params *param);
