/* csip.c - CAP Commander specific VOCS mocks */

/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/audio/vocs.h>

static struct bt_vocs {
	bool active;
	struct bt_conn *conn;
	struct bt_vocs_cb *cb;
} vocs_clients[CONFIG_BT_MAX_CONN * CONFIG_BT_VOCS_CLIENT_MAX_INSTANCE_COUNT];

int bt_vocs_client_conn_get(const struct bt_vocs *vocs, struct bt_conn **conn)
{
	*conn = vocs->conn;

	return 0;
}

int bt_vocs_state_set(struct bt_vocs *vocs, int16_t offset)
{
	if (vocs != NULL && vocs->cb != NULL && vocs->cb->set_offset != NULL) {
		vocs->cb->set_offset(vocs, 0);
	}

	return 0;
}

void bt_vocs_client_cb_register(struct bt_vocs *vocs, struct bt_vocs_cb *cb)
{
	vocs->cb = cb;
}

struct bt_vocs *bt_vocs_client_free_instance_get(void)
{
	for (size_t i = 0U; i < ARRAY_SIZE(vocs_clients); i++) {
		if (!vocs_clients[i].active) {
			vocs_clients[i].active = true;

			return &vocs_clients[i];
		}
	}

	return NULL;
}

int bt_vocs_discover(struct bt_conn *conn, struct bt_vocs *vocs,
		     const struct bt_vocs_discover_param *param)
{
	vocs->conn = conn;

	if (vocs != NULL && vocs->cb != NULL && vocs->cb->discover != NULL) {
		vocs->cb->discover(vocs, 0);
	}

	return 0;
}

void mock_bt_vocs_init(void)
{
}

void mock_bt_vocs_cleanup(void)
{
	/* Reset everything but the callbacks, as they will not be registered again between each
	 * test
	 */
	for (size_t i = 0U; i < ARRAY_SIZE(vocs_clients); i++) {
		vocs_clients[i].active = false;
		vocs_clients[i].conn = NULL;
	}
}
