/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/att.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_testlib_att, LOG_LEVEL_DBG);

static uint8_t exchange_mtu_err;
static K_MUTEX_DEFINE(exchange_mtu_lock);
static K_CONDVAR_DEFINE(exchange_mtu_done);

static void bt_testlib_att_exchange_mtu_cb(struct bt_conn *conn, uint8_t err,
					   struct bt_gatt_exchange_params *params)
{
	char addr[BT_ADDR_LE_STR_LEN];

	k_mutex_lock(&exchange_mtu_lock, K_FOREVER);

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_DBG("MTU exchange %s (%s)", err == 0U ? "successful" : "failed", addr);

	exchange_mtu_err = err;

	k_condvar_signal(&exchange_mtu_done);

	k_mutex_unlock(&exchange_mtu_lock);
}

int bt_testlib_att_exchange_mtu(struct bt_conn *conn)
{
	int err;
	struct bt_gatt_exchange_params exchange_mtu_params;

	__ASSERT_NO_MSG(conn != NULL);

	k_mutex_lock(&exchange_mtu_lock, K_FOREVER);

	exchange_mtu_err = 0;

	exchange_mtu_params.func = bt_testlib_att_exchange_mtu_cb;

	err = bt_gatt_exchange_mtu(conn, &exchange_mtu_params);
	if (err != 0) {
		LOG_ERR("Failed to exchange MTU (err %d)", err);

		k_mutex_unlock(&exchange_mtu_lock);

		return err;
	}

	k_condvar_wait(&exchange_mtu_done, &exchange_mtu_lock, K_FOREVER);

	err = exchange_mtu_err;

	k_mutex_unlock(&exchange_mtu_lock);

	return err;
}
