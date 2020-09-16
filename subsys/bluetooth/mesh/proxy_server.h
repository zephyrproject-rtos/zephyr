/*  Bluetooth Mesh */

/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2020 Lingao Meng
 *
 * SPDX-License-Identifier: Apache-2.0
 */


struct bt_mesh_proxy_idle_cb {
	sys_snode_t n;
	void (*cb)(void);
};

int bt_mesh_proxy_send(struct bt_conn *conn, uint8_t type,
		       struct net_buf_simple *msg);

ssize_t bt_mesh_proxy_recv(struct bt_conn *conn,
			  const void *buf, uint16_t len);

void bt_mesh_proxy_connected(struct bt_conn *conn, uint8_t err);
void bt_mesh_proxy_disconnected(struct bt_conn *conn, uint8_t reason);

int bt_mesh_proxy_prov_enable(void);
int bt_mesh_proxy_prov_disable(bool disconnect);

int bt_mesh_proxy_gatt_enable(void);
int bt_mesh_proxy_gatt_disable(void);
void bt_mesh_proxy_gatt_disconnect(void);

void bt_mesh_proxy_beacon_send(struct bt_mesh_subnet *sub);

struct net_buf_simple *bt_mesh_proxy_get_buf(void);

k_timeout_t bt_mesh_gatt_adv_start(void);
void bt_mesh_gatt_adv_stop(void);

void bt_mesh_proxy_identity_start(struct bt_mesh_subnet *sub);
void bt_mesh_proxy_identity_stop(struct bt_mesh_subnet *sub);

bool bt_mesh_proxy_relay(struct net_buf_simple *buf, uint16_t dst);
void bt_mesh_proxy_addr_add(struct net_buf_simple *buf, uint16_t addr);

int bt_mesh_proxy_init(void);
void bt_mesh_proxy_on_idle(struct bt_mesh_proxy_idle_cb *cb);
