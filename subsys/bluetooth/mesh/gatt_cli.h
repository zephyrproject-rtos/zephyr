/*
 * Copyright (c) 2021 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


struct bt_mesh_gatt_cli {
	struct bt_uuid_16 srv_uuid;
	struct bt_uuid_16 data_in_uuid;
	struct bt_uuid_16 data_out_uuid;
	struct bt_uuid_16 data_out_cccd_uuid;

	void (*connected)(struct bt_conn *conn, void *user_data);
	void (*link_open)(struct bt_conn *conn);
	void (*disconnected)(struct bt_conn *conn);
};

int bt_mesh_gatt_cli_connect(const bt_addr_le_t *addr,
			     const struct bt_mesh_gatt_cli *gatt,
			     void *user_data);

int bt_mesh_gatt_send(struct bt_conn *conn,
		      const void *data, uint16_t len,
		      bt_gatt_complete_func_t end, void *user_data);

void bt_mesh_gatt_client_init(void);

void bt_mesh_gatt_client_deinit(void);
