/** @file
 *  @brief Audio Video Control Transport Protocol internal header.
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright (C) 2024 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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

struct bt_avctp_header_common {
	uint8_t byte0;  /** [7:4]: Transaction label, [3:2]: Packet_type, [1]: C/R, [0]: IPID */
} __packed;

struct bt_avctp_header_single {
	struct bt_avctp_header_common common;
	uint16_t pid;
} __packed;

struct bt_avctp_header_start {
	struct bt_avctp_header_common common;
	uint8_t number_packet;
	uint16_t pid;
} __packed;

struct bt_avctp_header_continue_end {
	struct bt_avctp_header_common common;
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
/** Set all AVCTP header fields into the first octet. Packs the four fields (IPID, CR, TYPE, TID)
 *  into a single 8-bit header according to the AVRCP bit layout.
 */
#define BT_AVCTP_HDR_SET(hdr, ipid, cr, type, tid)                                                 \
	(hdr)->byte0 = (FIELD_PREP(BIT(0), (ipid)) | FIELD_PREP(BIT(1), (cr)) |                    \
			FIELD_PREP(GENMASK(3, 2), (type)) | FIELD_PREP(GENMASK(7, 4), (tid)))

struct bt_avctp;

struct bt_avctp_ops_cb {
	void (*connected)(struct bt_avctp *session);
	void (*disconnected)(struct bt_avctp *session);
	int (*recv)(struct bt_avctp *session, struct net_buf *buf, bt_avctp_cr_t cr, uint8_t tid);
};

struct bt_avctp {
	struct bt_l2cap_br_chan br_chan;
	const struct bt_avctp_ops_cb *ops;
	uint16_t pid;  /** Profile Identifier */
	uint16_t max_tx_payload_size;
	struct net_buf_pool *tx_pool;
	struct net_buf_pool *rx_pool;
	struct net_buf *reassembly_buf;
};

/**
 * @brief AVCTP L2CAP Server structure
 *
 * This structure defines the L2CAP server used for AVCTP over L2CAP transport.
 */
struct bt_avctp_server {
	/**
	 * @brief L2CAP server parameters
	 *
	 * This field is used to register the L2CAP server. The `psm` field can be set
	 * to a specific value (not recommended), or set to 0 to allow automatic PSM
	 * allocation during registration via @ref bt_avctp_server_register.
	 *
	 * The `sec_level` field specifies the minimum required security level.
	 *
	 * @note The `struct bt_l2cap_server::accept` callback of `l2cap` can not be used
	 * by AVCTP applications. Instead, use the `struct bt_avctp_server::accept`
	 * callback defined in this structure.
	 */
	struct bt_l2cap_server l2cap;

	/**
	 * @brief Accept callback for incoming AVCTP connections
	 *
	 * This callback is invoked when a new incoming AVCTP connection is received.
	 * The application is responsible for authorizing the connection and allocating
	 * a new AVCTP session object.
	 *
	 * @warning The caller must ensure that the parent object of the AVCTP session
	 * is properly zero-initialized before use.
	 *
	 * @param conn   The Bluetooth connection requesting authorization.
	 * @param session Pointer to receive the allocated AVCTP session object.
	 *
	 * @retval 0        Success.
	 * @retval -ENOMEM  No available space for a new session.
	 * @retval -EACCES  Connection not authorized by the application.
	 * @retval -EPERM   Encryption key size is insufficient.
	 */
	int (*accept)(struct bt_conn *conn, struct bt_avctp **session);

	/** @brief Internal node for list management */
	sys_snode_t node;
};

struct bt_avctp_event_cb {
	int (*accept)(struct bt_conn *conn, struct bt_avctp **session);
};

/* Initialize AVCTP layer*/
int bt_avctp_init(void);

/* Application register with AVCTP layer */
int bt_avctp_server_register(struct bt_avctp_server *server);

/* AVCTP connect */
int bt_avctp_connect(struct bt_conn *conn, uint16_t psm, struct bt_avctp *session);

/* AVCTP disconnect */
int bt_avctp_disconnect(struct bt_avctp *session);

/* Create AVCTP PDU */
struct net_buf *bt_avctp_create_pdu(struct net_buf_pool *pool);

/* Send AVCTP PDU */
int bt_avctp_send(struct bt_avctp *session, struct net_buf *buf, bt_avctp_cr_t cr, uint8_t tid);
