/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_BLUETOOTH_MESH_PROXY_H_
#define ZEPHYR_SUBSYS_BLUETOOTH_MESH_PROXY_H_

#include <bluetooth/gatt.h>

#define BT_MESH_PROXY_NET_PDU   0x00
#define BT_MESH_PROXY_BEACON    0x01
#define BT_MESH_PROXY_CONFIG    0x02
#define BT_MESH_PROXY_PROV      0x03

int bt_mesh_pb_gatt_send(struct bt_conn *conn, struct net_buf_simple *buf,
			 bt_gatt_complete_func_t end, void *user_data);

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
