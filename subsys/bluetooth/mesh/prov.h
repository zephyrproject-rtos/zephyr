/*  Bluetooth Mesh */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int bt_mesh_pb_adv_open(const u8_t uuid[16], u16_t net_idx, u16_t addr,
			u8_t attention_duration);

void bt_mesh_pb_adv_recv(struct net_buf_simple *buf);

bool bt_prov_active(void);

int bt_mesh_pb_gatt_open(struct bt_conn *conn);
int bt_mesh_pb_gatt_close(struct bt_conn *conn);
int bt_mesh_pb_gatt_recv(struct bt_conn *conn, struct net_buf_simple *buf);

const struct bt_mesh_prov *bt_mesh_prov_get(void);

int bt_mesh_prov_init(const struct bt_mesh_prov *prov);

void bt_mesh_prov_complete(u16_t net_idx, u16_t addr);
void bt_mesh_prov_node_added(u16_t net_idx, u16_t addr, u8_t num_elem);
void bt_mesh_prov_reset(void);
