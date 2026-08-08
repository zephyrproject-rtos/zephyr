/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BNEP_INTERNAL_H_
#define BNEP_INTERNAL_H_

#include <zephyr/bluetooth/l2cap.h>

#define BT_L2CAP_PSM_BNEP 0x000f

#define BNEP_TYPE_GENERAL_ETHERNET 0x00
#define BNEP_TYPE_CONTROL          0x01
#define BNEP_TYPE_COMPRESSED_ETH   0x02
#define BNEP_TYPE_COMPRESSED_SRC   0x03
#define BNEP_TYPE_COMPRESSED_DST   0x04

#define BNEP_CONTROL_SETUP_CONN_REQ        0x01
#define BNEP_CONTROL_SETUP_CONN_RSP        0x02
#define BNEP_CONTROL_FILTER_MULTI_ADDR_REQ 0x03
#define BNEP_CONTROL_FILTER_MULTI_ADDR_RSP 0x04
#define BNEP_CONTROL_FILTER_NET_TYPE_REQ   0x05
#define BNEP_CONTROL_FILTER_NET_TYPE_RSP   0x06

#define BNEP_SETUP_SUCCESS 0x0000

#define BNEP_SVC_NONE 0x00
#define BNEP_SVC_PANU 0x02
#define BNEP_SVC_NAP  0x04
#define BNEP_SVC_GN   0x10

#define BNEP_EXT_HEADER 0x80

struct bt_bnep;

enum bt_bnep_state {
	BT_BNEP_STATE_DISCONNECTED,
	BT_BNEP_STATE_CONNECTING,
	BT_BNEP_STATE_CONNECTED,
};

struct bt_bnep_cb {
	void (*connected)(struct bt_bnep *bnep);
	void (*disconnected)(struct bt_bnep *bnep);
	void (*recv)(struct bt_bnep *bnep, struct net_buf *buf);
};

struct bt_bnep {
	struct bt_l2cap_br_chan chan;
	struct bt_conn *conn;
	enum bt_bnep_state state;
	uint8_t local_service;
	uint8_t remote_service;
	bool initiator;
	const struct bt_bnep_cb *cb;
};

void bt_bnep_init(void);

int bt_bnep_register_cb(const struct bt_bnep_cb *cb);

int bt_bnep_connect(struct bt_conn *conn, struct bt_bnep *bnep, uint8_t local_service,
		    uint8_t remote_service);

int bt_bnep_accept(struct bt_conn *conn, struct bt_bnep *bnep, uint8_t local_service);

int bt_bnep_disconnect(struct bt_bnep *bnep);

int bt_bnep_send(struct bt_bnep *bnep, struct net_buf *buf);

struct net_buf *bt_bnep_alloc_buf(size_t len);

typedef int (*bt_bnep_accept_fn)(struct bt_conn *conn, struct bt_bnep **bnep);

int bt_bnep_register_accept(bt_bnep_accept_fn accept);

int bt_bnep_l2cap_accept(struct bt_conn *conn, struct bt_l2cap_server *server,
			 struct bt_l2cap_chan **chan);

#endif /* BNEP_INTERNAL_H_ */
