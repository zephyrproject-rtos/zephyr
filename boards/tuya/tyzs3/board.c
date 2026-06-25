/*
 * Copyright (c) 2025 sevenlab engineering GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_CONN) && defined(CONFIG_SILABS_GECKO_POWER_MANAGER)
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <sl_power_manager.h>

static bool em_acquired;
static size_t num_connections;

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err == 0) {
		num_connections += 1;
	}

	if (!em_acquired && num_connections > 0) {
		sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);
		em_acquired = true;
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	num_connections -= 1;

	if (em_acquired && num_connections == 0) {
		sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
		em_acquired = false;
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};
#endif
