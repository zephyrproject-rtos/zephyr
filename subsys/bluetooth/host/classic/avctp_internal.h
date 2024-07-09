/** @file
 *  @brief Audio Video Control Transport Protocol internal header.
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright (C) 2024 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define BT_L2CAP_PSM_AVCTP 0x0017

typedef enum __packed {
	BT_AVCTP_IPID_NONE = 0b0,
	BT_AVCTP_IPID_INVALID = 0b1,
} bt_avctp_ipid_t;

typedef enum __packed {
	BT_AVCTP_CMD = 0b0,
	BT_AVCTP_RESPONSE = 0b1,
} bt_avctp_cr_t;

typedef enum __packed {
	BT_AVCTP_PKT_TYPE_SINGLE = 0b00,
} bt_avctp_pkt_type_t;

struct bt_avctp_header {
	uint8_t ipid: 1;     /* Invalid Profile Identifier (1), otherwise zero (0) */
	uint8_t cr: 1;       /* Command(0) or Respone(1) */
	uint8_t pkt_type: 2; /* Set to zero (00) for single L2CAP packet */
	uint8_t tid: 4;      /* Transaction label */
	uint16_t pid;        /* Profile Identifier */
} __packed;

struct bt_avctp;

struct bt_avctp_ops_cb {
	void (*connected)(struct bt_avctp *session);
	void (*disconnected)(struct bt_avctp *session);
};

struct bt_avctp {
	struct bt_l2cap_br_chan br_chan;
	const struct bt_avctp_ops_cb *ops;
};

struct bt_avctp_event_cb {
	int (*accept)(struct bt_conn *conn, struct bt_avctp **session);
};

/* Initialize AVCTP layer*/
int bt_avctp_init(void);

/* Application register with AVCTP layer */
int bt_avctp_register(const struct bt_avctp_event_cb *cb);

/* AVCTP connect */
int bt_avctp_connect(struct bt_conn *conn, struct bt_avctp *session);

/* AVCTP disconnect */
int bt_avctp_disconnect(struct bt_avctp *session);

/* Create AVCTP PDU */
struct net_buf *bt_avctp_create_pdu(struct bt_avctp *session, bt_avctp_cr_t cr,
				    bt_avctp_ipid_t ipid, uint8_t *tid, uint16_t pid);

/* Send AVCTP PDU */
int bt_avctp_send(struct bt_avctp *session, struct net_buf *buf);
