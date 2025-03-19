/* csip.c - CAP Commander specific AICS mocks */

/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>
#include <stddef.h>

#include <zephyr/bluetooth/audio/aics.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/sys/util.h>

static struct bt_aics {
	bool active;
	struct bt_conn *conn;
	struct bt_aics_cb *cb;
} aics_clients[CONFIG_BT_MAX_CONN * CONFIG_BT_AICS_CLIENT_MAX_INSTANCE_COUNT];

int bt_aics_client_conn_get(const struct bt_aics *aics, struct bt_conn **conn)
{
	*conn = aics->conn;

	return 0;
}

int bt_aics_gain_set(struct bt_aics *aics, int8_t gain)
{
	if (aics != NULL && aics->cb != NULL && aics->cb->set_gain != NULL) {
		aics->cb->set_gain(aics, 0);
	}

	return 0;
}

void bt_aics_client_cb_register(struct bt_aics *aics, struct bt_aics_cb *cb)
{
	aics->cb = cb;
}

struct bt_aics *bt_aics_client_free_instance_get(void)
{
	for (size_t i = 0U; i < ARRAY_SIZE(aics_clients); i++) {
		if (!aics_clients[i].active) {
			aics_clients[i].active = true;

			return &aics_clients[i];
		}
	}

	return NULL;
}

int bt_aics_discover(struct bt_conn *conn, struct bt_aics *aics,
		     const struct bt_aics_discover_param *param)
{
	aics->conn = conn;

	if (aics != NULL && aics->cb != NULL && aics->cb->discover != NULL) {
		aics->cb->discover(aics, 0);
	}

	return 0;
}

void mock_bt_aics_init(void)
{
}

void mock_bt_aics_cleanup(void)
{
	/* Reset everything but the callbacks, as they will not be registered again between each
	 * test
	 */
	for (size_t i = 0U; i < ARRAY_SIZE(aics_clients); i++) {
		aics_clients[i].active = false;
		aics_clients[i].conn = NULL;
	}
}
