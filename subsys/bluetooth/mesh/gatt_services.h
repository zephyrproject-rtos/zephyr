/*  Bluetooth Mesh */

/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2020 Lingao Meng
 *
 * SPDX-License-Identifier: Apache-2.0
 */


int bt_mesh_gatt_prov_enable(void);
int bt_mesh_gatt_prov_disable(void);
int bt_mesh_gatt_proxy_enable(void);
int bt_mesh_gatt_proxy_disable(void);
int bt_mesh_gatt_send(struct bt_conn *conn,
		      struct bt_gatt_notify_params *params);
int bt_mesh_gatt_init(void);
