/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BNEP_INTERNAL_H_
#define BNEP_INTERNAL_H_

#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/bnep.h>

#define BNEP_ETH_ADDR_LEN 6
#define BNEP_ETH_HDR_LEN  14

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

#define BNEP_SETUP_SUCCESS           0x0000
#define BNEP_SETUP_INVALID_DST_UUID  0x0001
#define BNEP_SETUP_INVALID_SRC_UUID  0x0002
#define BNEP_SETUP_INVALID_UUID_SIZE 0x0003
#define BNEP_SETUP_NOT_ALLOWED       0x0004

/* PAN service class UUIDs carried in the BNEP setup connection messages. */
#define BNEP_SVC_NONE 0x0000
#define BNEP_SVC_PANU BT_BNEP_SVC_PANU
#define BNEP_SVC_NAP  BT_BNEP_SVC_NAP
#define BNEP_SVC_GN   BT_BNEP_SVC_GN

#define BNEP_EXT_HEADER 0x80

/*
 * BNEP endpoint MAC addresses are the Bluetooth device addresses in Ethernet
 * byte order, which is the reverse of the bt_addr_t byte order.
 */
static inline void bnep_addr_to_mac(const bt_addr_t *addr, uint8_t mac[BNEP_ETH_ADDR_LEN])
{
	sys_memcpy_swap(mac, addr->val, BNEP_ETH_ADDR_LEN);
}

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
	uint16_t local_service;
	uint16_t remote_service;
	bool initiator;
	const struct bt_bnep_cb *cb;
};

void bt_bnep_init(void);

int bt_bnep_register_cb(const struct bt_bnep_cb *cb);

int bt_bnep_connect(struct bt_conn *conn, struct bt_bnep *bnep, uint16_t local_service,
		    uint16_t remote_service);

int bt_bnep_accept(struct bt_conn *conn, struct bt_bnep *bnep, uint16_t local_service);

int bt_bnep_disconnect(struct bt_bnep *bnep);

int bt_bnep_send(struct bt_bnep *bnep, struct net_buf *buf);

struct net_buf *bt_bnep_alloc_buf(size_t len);

typedef int (*bt_bnep_accept_fn)(struct bt_conn *conn, struct bt_bnep **bnep);

int bt_bnep_register_accept(bt_bnep_accept_fn accept);

int bt_bnep_l2cap_accept(struct bt_conn *conn, struct bt_l2cap_server *server,
			 struct bt_l2cap_chan **chan);

#endif /* BNEP_INTERNAL_H_ */
