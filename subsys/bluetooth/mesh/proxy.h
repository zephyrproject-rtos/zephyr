/*  Bluetooth Mesh */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_BLUETOOTH_MESH_PROXY_H_
#define ZEPHYR_SUBSYS_BLUETOOTH_MESH_PROXY_H_

#include <bluetooth/gatt.h>

int bt_mesh_pb_gatt_send(struct bt_conn *conn, struct net_buf_simple *buf,
			 bt_gatt_complete_func_t end, void *user_data);

#define PDU_TYPE(data)     (data[0] & BIT_MASK(6))
#define CFG_FILTER_SET     0x00
#define CFG_FILTER_ADD     0x01
#define CFG_FILTER_REMOVE  0x02
#define CFG_FILTER_STATUS  0x03

#define PDU_HDR(sar, type) (sar << 6 | (type & BIT_MASK(6)))

typedef int (*proxy_send_cb_t)(struct bt_conn *conn,
			       const void *data, uint16_t len,
			       bt_gatt_complete_func_t end, void *user_data);

typedef void (*proxy_recv_cb_t)(struct bt_conn *conn,
				struct bt_mesh_net_rx *rx,
				struct net_buf_simple *buf);

struct bt_mesh_proxy_role {
	struct bt_conn *conn;
	uint8_t msg_type;

	struct {
		proxy_send_cb_t send;
		proxy_recv_cb_t recv;
	} cb;

	struct k_work_delayable sar_timer;
	struct net_buf_simple buf;
};

#define BT_MESH_PROXY_NET_PDU   0x00
#define BT_MESH_PROXY_BEACON    0x01
#define BT_MESH_PROXY_CONFIG    0x02
#define BT_MESH_PROXY_PROV      0x03

ssize_t bt_mesh_proxy_msg_recv(struct bt_mesh_proxy_role *role,
			       const void *buf, uint16_t len);
int bt_mesh_proxy_msg_send(struct bt_mesh_proxy_role *role, uint8_t type,
			   struct net_buf_simple *msg,
			   bt_gatt_complete_func_t end, void *user_data);
void bt_mesh_proxy_msg_init(struct bt_mesh_proxy_role *role);

int bt_mesh_proxy_prov_enable(void);
int bt_mesh_proxy_prov_disable(bool disconnect);

int bt_mesh_proxy_gatt_enable(void);
int bt_mesh_proxy_gatt_disable(void);
void bt_mesh_proxy_gatt_disconnect(void);

void bt_mesh_proxy_beacon_send(struct bt_mesh_subnet *sub);

struct net_buf_simple *bt_mesh_proxy_get_buf(void);

int bt_mesh_proxy_adv_start(void);

void bt_mesh_proxy_identity_start(struct bt_mesh_subnet *sub);
void bt_mesh_proxy_identity_stop(struct bt_mesh_subnet *sub);

bool bt_mesh_proxy_relay(struct net_buf *buf, uint16_t dst);
void bt_mesh_proxy_addr_add(struct net_buf_simple *buf, uint16_t addr);

int bt_mesh_proxy_init(void);

#endif /* ZEPHYR_SUBSYS_BLUETOOTH_MESH_PROXY_H_ */
