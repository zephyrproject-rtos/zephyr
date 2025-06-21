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
#define BT_L2CAP_PSM_AVCTP_BROWSING 0x001B

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
	BT_AVCTP_PKT_TYPE_START = 0b01,
	BT_AVCTP_PKT_TYPE_CONTINUE = 0b10,
	BT_AVCTP_PKT_TYPE_END = 0b11,
} bt_avctp_pkt_type_t;

struct bt_avctp_header {
	uint8_t byte0; /** [7:4]: Transaction label, [3:2]: Packet_type, [1]: C/R, [0]: IPID */
	uint16_t pid;  /** Profile Identifier */
} __packed;

/** Transaction label provided by the application and is replicated by the sender of the message in
 *  each packet of the sequence. It isused at the receiver side to identify packets that belong to
 *  the same message.
 */
#define BT_AVCTP_HDR_GET_TRANSACTION_LABLE(hdr) FIELD_GET(GENMASK(7, 4), ((hdr)->byte0))
/** Set to zero (00) to indicate that the command/response message is transmitted in a single L2CAP
 *  packet. Alternatively, set to (01) for start, (10) for continue, or (11) for end packet.
 */
#define BT_AVCTP_HDR_GET_PACKET_TYPE(hdr)       FIELD_GET(GENMASK(3, 2), ((hdr)->byte0))
/** Indicates whether the messageconveys a command frame (0) or a response frame (1). */
#define BT_AVCTP_HDR_GET_CR(hdr)                FIELD_GET(BIT(1), ((hdr)->byte0))
/** The IPID bit is set in a response message to indicate an invalid Profile Identifier received in
 *  the command message of the same transaction; otherwise this bit is set to zero. In command
 *  messages this bit is set to zero. This field is only present in the start packet of the message.
 */
#define BT_AVCTP_HDR_GET_IPID(hdr)              FIELD_GET(BIT(0), ((hdr)->byte0))

/** Transaction label provided by the application and is replicated by the sender of the message in
 *  each packet of the sequence. It isused at the receiver side to identify packets that belong to
 *  the same message.
 */
#define BT_AVCTP_HDR_SET_TRANSACTION_LABLE(hdr, tl)                                                \
	(hdr)->byte0 = (((hdr)->byte0) & ~GENMASK(7, 4)) | FIELD_PREP(GENMASK(7, 4), (tl))
/** Set to zero (00) to indicate that the command/response message is transmitted in a single L2CAP
 *  packet. Alternatively, set to (01) for start, (10) for continue, or (11) for end packet.
 */
#define BT_AVCTP_HDR_SET_PACKET_TYPE(hdr, packet_type)                                             \
	(hdr)->byte0 = (((hdr)->byte0) & ~GENMASK(3, 2)) | FIELD_PREP(GENMASK(3, 2), (packet_type))
/** Indicates whether the messageconveys a command frame (0) or a response frame (1). */
#define BT_AVCTP_HDR_SET_CR(hdr, cr)                                                               \
	(hdr)->byte0 = (((hdr)->byte0) & ~BIT(1)) | FIELD_PREP(BIT(1), (cr))
/** The IPID bit is set in a response message to indicate an invalid Profile Identifier received in
 *  the command message of the same transaction; otherwise this bit is set to zero. In command
 *  messages this bit is set to zero. This field is only present in the start packet of the message.
 */
#define BT_AVCTP_HDR_SET_IPID(hdr, ipid)                                                           \
	(hdr)->byte0 = (((hdr)->byte0) & ~BIT(0)) | FIELD_PREP(BIT(0), (ipid))

struct bt_avctp;
struct bt_avctp_browsing;

struct bt_avctp_ops_cb {
	void (*connected)(struct bt_avctp *session);
	void (*disconnected)(struct bt_avctp *session);
	int (*recv)(struct bt_avctp *session, struct net_buf *buf);
};

struct bt_avctp_browsing_ops_cb {
	void (*connected)(struct bt_avctp_browsing *session);
	void (*disconnected)(struct bt_avctp_browsing *session);
	int (*recv)(struct bt_avctp_browsing *session, struct net_buf *buf);
};

struct bt_avctp {
	struct bt_l2cap_br_chan br_chan;
	const struct bt_avctp_ops_cb *ops;
};

struct bt_avctp_browsing {
	struct bt_l2cap_br_chan br_chan;
	const struct bt_avctp_browsing_ops_cb *ops;
	struct bt_avctp *control_session;
};

struct bt_avctp_event_cb {
	int (*accept)(struct bt_conn *conn, struct bt_avctp **session);
};

struct bt_avctp_browsing_event_cb {
	int (*accept)(struct bt_conn *conn, struct bt_avctp_browsing **session);
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
				    bt_avctp_pkt_type_t pkt_type, bt_avctp_ipid_t ipid,
				    uint8_t tid, uint16_t pid);

/* Send AVCTP PDU */
int bt_avctp_send(struct bt_avctp *session, struct net_buf *buf);

/* Initialize AVCTP Browsing layer */
int bt_avctp_browsing_init(void);

/* Application register with AVCTP Browsing layer */
int bt_avctp_browsing_register(const struct bt_avctp_browsing_event_cb *cb);

/* AVCTP Browsing connect - requires an established control channel */
int bt_avctp_browsing_connect(struct bt_conn *conn, struct bt_avctp_browsing *session,
			      struct bt_avctp *control_session);

/* AVCTP Browsing disconnect */
int bt_avctp_browsing_disconnect(struct bt_avctp_browsing *session);

 /* Create AVCTP Browsing PDU */
struct net_buf *bt_avctp_browsing_create_pdu(struct bt_avctp_browsing *session, bt_avctp_cr_t cr,
					     bt_avctp_pkt_type_t pkt_type, bt_avctp_ipid_t ipid,
					     uint8_t tid, uint16_t pid);

/* Send AVCTP Browsing PDU */
int bt_avctp_browsing_send(struct bt_avctp_browsing *session, struct net_buf *buf);
