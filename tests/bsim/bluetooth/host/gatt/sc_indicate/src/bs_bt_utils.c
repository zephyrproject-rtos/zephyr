/**
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci_types.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test_utils, LOG_LEVEL_DBG);

#include "bs_bt_utils.h"

struct bt_conn *g_conn;
DEFINE_FLAG(flag_is_connected);

void wait_connected(void)
{
	LOG_DBG("Wait for connection...");
	WAIT_FOR_FLAG(flag_is_connected);
}

void wait_disconnected(void)
{
	LOG_DBG("Wait for disconnection...");
	WAIT_FOR_FLAG_UNSET(flag_is_connected);
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	LOG_DBG("security changed");
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	UNSET_FLAG(flag_is_connected);
}

struct bt_conn *get_g_conn(void)
{
	return g_conn;
}

void clear_g_conn(void)
{
	struct bt_conn *conn;

	conn = g_conn;
	g_conn = NULL;
	BSIM_ASSERT(conn, "Test error: no g_conn!\n");
	bt_conn_unref(conn);
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	BSIM_ASSERT((!g_conn || (conn == g_conn)), "Unexpected new connection.");

	if (!g_conn) {
		g_conn = bt_conn_ref(conn);
	}

	if (err != 0) {
		clear_g_conn();
		return;
	}

	SET_FLAG(flag_is_connected);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,
};

static void stop_scan_and_connect(const bt_addr_le_t *addr,
				  int8_t rssi,
				  uint8_t type,
				  struct net_buf_simple *ad)
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	int err;

	if (g_conn != NULL) {
		return;
	}

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
	printk("Got scan result, connecting.. dst %s, RSSI %d\n", addr_str, rssi);

	err = bt_le_scan_stop();
	BSIM_ASSERT(!err, "Err bt_le_scan_stop %d", err);

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT, &g_conn);
	BSIM_ASSERT(!err, "Err bt_conn_le_create %d", err);
}

void scan_connect_to_first_result(void)
{
	int err;

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, stop_scan_and_connect);
	BSIM_ASSERT(!err, "Err bt_le_scan_start %d", err);
}

void disconnect(void)
{
	int err;

	err = bt_conn_disconnect(g_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	BSIM_ASSERT(!err, "bt_conn_disconnect failed (%d)\n", err);
}

void set_security(bt_security_t sec)
{
	int err;

	err = bt_conn_set_security(g_conn, sec);
	BSIM_ASSERT(!err, "Err bt_conn_set_security %d", err);
}

void create_adv(struct bt_le_ext_adv **adv)
{
	int err;
	struct bt_le_adv_param params = {};

	params.options |= BT_LE_ADV_OPT_CONNECTABLE;
	params.options |= BT_LE_ADV_OPT_EXT_ADV;

	params.id = BT_ID_DEFAULT;
	params.sid = 0;
	params.interval_min = BT_GAP_ADV_FAST_INT_MIN_2;
	params.interval_max = BT_GAP_ADV_FAST_INT_MAX_2;

	err = bt_le_ext_adv_create(&params, NULL, adv);
	BSIM_ASSERT(!err, "bt_le_ext_adv_create failed (%d)\n", err);
}

void start_adv(struct bt_le_ext_adv *adv)
{
	int err;

	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	BSIM_ASSERT(!err, "bt_le_ext_adv_start failed (%d)\n", err);
}

void stop_adv(struct bt_le_ext_adv *adv)
{
	int err;

	err = bt_le_ext_adv_stop(adv);
	BSIM_ASSERT(!err, "bt_le_ext_adv_stop failed (%d)\n", err);
}

/* The following flags are raised by events and lowered by test code. */
DEFINE_FLAG(flag_pairing_complete);
DEFINE_FLAG(flag_bonded);
DEFINE_FLAG(flag_not_bonded);

void pairing_complete(struct bt_conn *conn, bool bonded)
{
	LOG_DBG("pairing complete");
	SET_FLAG(flag_pairing_complete);

	if (bonded) {
		SET_FLAG(flag_bonded);
		LOG_DBG("Bonded status: true");
	} else {
		SET_FLAG(flag_not_bonded);
		LOG_DBG("Bonded status: false");
	}
}

void pairing_failed(struct bt_conn *conn, enum bt_security_err err)
{
	FAIL("Pairing failed\n");
}
