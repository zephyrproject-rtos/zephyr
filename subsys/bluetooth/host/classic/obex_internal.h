/* obex.c - Internal for IrDA Oject Exchange Protocol handling */

/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define BT_OBEX_VERSION 0x10

#define BT_OBEX_MIN_MTU 255

#define BT_OBEX_OPCODE_CONNECT  0x80
#define BT_OBEX_OPCODE_DISCONN  0x81
#define BT_OBEX_OPCODE_PUT      0x02
#define BT_OBEX_OPCODE_PUT_F    0x82
#define BT_OBEX_OPCODE_GET      0x03
#define BT_OBEX_OPCODE_GET_F    0x83
#define BT_OBEX_OPCODE_SETPATH  0x85
#define BT_OBEX_OPCODE_ACTION   0x06
#define BT_OBEX_OPCODE_ACTION_F 0x86
#define BT_OBEX_OPCODE_SESSION  0x87
#define BT_OBEX_OPCODE_ABORT    0xFF

struct bt_obex_req_hdr {
	uint8_t code;
	uint16_t len;
} __packed;

struct bt_obex_rsp_hdr {
	uint8_t code;
	uint16_t len;
} __packed;

struct bt_obex_conn_req_hdr {
	uint8_t version;
	uint8_t flags;
	uint16_t mopl;
} __packed;

struct bt_obex_conn_rsp_hdr {
	uint8_t version;
	uint8_t flags;
	uint16_t mopl;
} __packed;

struct bt_obex_setpath_req_hdr {
	uint8_t flags;
	uint8_t constants;
} __packed;

/* OBEX initialization */
int bt_obex_reg_transport(struct bt_obex *obex, const struct bt_obex_transport_ops *ops);

/* Process the received OBEX packet */
int bt_obex_recv(struct bt_obex *obex, struct net_buf *buf);

/* Notify OBEX that the transport has been connected */
int bt_obex_transport_connected(struct bt_obex *obex);

/* Notify OBEX that the transport has been disconnected */
int bt_obex_transport_disconnected(struct bt_obex *obex);
