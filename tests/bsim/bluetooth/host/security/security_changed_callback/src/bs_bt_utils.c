/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bs_bt_utils.h"

#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>

LOG_MODULE_REGISTER(bs_bt_utils, LOG_LEVEL_DBG);

#define BS_SECONDS(dur_sec)    ((bs_time_t)dur_sec * 1000000)
#define TEST_TIMEOUT_SIMULATED BS_SECONDS(60)

void test_tick(bs_time_t HW_device_time)
{
	bs_trace_debug_time(0, "Simulation ends now.\n");
	if (bst_result != Passed) {
		bst_result = Failed;
		bs_trace_error("Test did not pass before simulation ended.\n");
	}
}

void test_init(void)
{
	bst_ticker_set_next_tick_absolute(TEST_TIMEOUT_SIMULATED);
	bst_result = In_progress;
}

DEFINE_FLAG(flag_is_connected);
struct bt_conn *g_conn;

void wait_connected(void)
{
	LOG_DBG("Wait for connection...");
	WAIT_FOR_FLAG(flag_is_connected);
}

void wait_disconnected(void)
{
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

static void connected(struct bt_conn *conn, uint8_t err)
{
	ASSERT((!g_conn || (conn == g_conn)), "Unexpected new connection.");

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

void clear_g_conn(void)
{
	struct bt_conn *conn;

	conn = g_conn;
	g_conn = NULL;
	ASSERT(conn, "Test error: No g_conn!\n");
	bt_conn_unref(conn);
}

/* The following flags are raised by events and lowered by test code. */
DEFINE_FLAG(flag_pairing_complete);
DEFINE_FLAG(flag_bonded);
DEFINE_FLAG(flag_not_bonded);

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	LOG_INF("pairing complete");
	SET_FLAG(flag_pairing_complete);

	if (bonded) {
		SET_FLAG(flag_bonded);
		LOG_DBG("Bonded status: true");
	} else {
		SET_FLAG(flag_not_bonded);
		LOG_DBG("Bonded status: false");
	}
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err err)
{
	LOG_INF("Pairing failed");
}

static struct bt_conn_auth_info_cb bt_conn_auth_info_cb = {
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed,
};

void bs_bt_utils_setup(void)
{
	int err;

	err = bt_enable(NULL);
	ASSERT(!err, "bt_enable failed.\n");
	err = bt_conn_auth_info_cb_register(&bt_conn_auth_info_cb);
	ASSERT(!err, "bt_conn_auth_info_cb_register failed.\n");

	err = settings_load();
	if (err) {
		FAIL("Settings load failed (err %d)\n", err);
	}
}

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
	ASSERT(!err, "Err bt_le_scan_stop %d", err);

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT, &g_conn);
	ASSERT(!err, "Err bt_conn_le_create %d", err);
}

void scan_connect_to_first_result(void)
{
	int err;

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, stop_scan_and_connect);
	ASSERT(!err, "Err bt_le_scan_start %d", err);
}

void set_security(bt_security_t sec)
{
	int err;

	err = bt_conn_set_security(g_conn, sec);
	ASSERT(!err, "Err bt_conn_set_security %d", err);
}

void advertise_connectable(int id, bt_addr_le_t *directed_dst)
{
	int err;
	struct bt_le_adv_param param = {};

	param.id = id;
	param.interval_min = 0x0020;
	param.interval_max = 0x4000;
	param.options |= BT_LE_ADV_OPT_ONE_TIME;
	param.options |= BT_LE_ADV_OPT_CONNECTABLE;

	if (directed_dst) {
		param.options |= BT_LE_ADV_OPT_DIR_ADDR_RPA;
		param.peer = directed_dst;
	}

	err = bt_le_adv_start(&param, NULL, 0, NULL, 0);
	ASSERT(err == 0, "Advertising failed to start (err %d)\n", err);
}
