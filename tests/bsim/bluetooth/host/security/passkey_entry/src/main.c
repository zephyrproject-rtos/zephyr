/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>

#include "babblekit/flags.h"
#include "babblekit/testcase.h"

/* Both devices declare KeyboardOnly, which selects Passkey Entry with input on
 * both sides (Core Spec 6.3, Vol 3, Part H, Table 2.8). That models a user
 * typing the same passkey into both devices, and lets the two agree on one
 * without a display or a backchannel.
 */
#define TEST_PASSKEY 123456U

static DEFINE_FLAG(flag_connected);
static DEFINE_FLAG(flag_pairing_complete);
static DEFINE_FLAG(flag_pairing_failed);
static DEFINE_FLAG(flag_encrypted);

static struct bt_conn *default_conn;
static bt_security_t reached_level;

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err != 0U) {
		TEST_FAIL("Connection failed (0x%02x)", err);
		return;
	}

	default_conn = bt_conn_ref(conn);
	SET_FLAG(flag_connected);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	bt_conn_drop(&default_conn);
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	if (err != BT_SECURITY_ERR_SUCCESS) {
		return;
	}

	reached_level = level;

	if (level >= BT_SECURITY_L4) {
		SET_FLAG(flag_encrypted);
	}
}

BT_CONN_CB_DEFINE(conn_cb) = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,
};

static void auth_passkey_entry(struct bt_conn *conn)
{
	int err = bt_conn_auth_passkey_entry(conn, TEST_PASSKEY);

	if (err != 0) {
		TEST_FAIL("Failed to enter passkey (err %d)", err);
	}
}

static void auth_cancel(struct bt_conn *conn)
{
	TEST_FAIL("Pairing cancelled");
}

/* Only passkey_entry is set, so get_io_capa() reports KeyboardOnly. */
static struct bt_conn_auth_cb auth_cb = {
	.passkey_entry = auth_passkey_entry,
	.cancel = auth_cancel,
};

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	SET_FLAG(flag_pairing_complete);
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	SET_FLAG(flag_pairing_failed);
}

static struct bt_conn_auth_info_cb auth_info_cb = {
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed,
};

static void test_init(void)
{
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		TEST_FAIL("bt_enable failed (err %d)", err);
	}

	err = bt_conn_auth_cb_register(&auth_cb);
	if (err != 0) {
		TEST_FAIL("Failed to register auth callbacks (err %d)", err);
	}

	err = bt_conn_auth_info_cb_register(&auth_info_cb);
	if (err != 0) {
		TEST_FAIL("Failed to register auth info callbacks (err %d)", err);
	}
}

static void test_check_paired(void)
{
	WAIT_FOR_FLAG(flag_pairing_complete);
	WAIT_FOR_FLAG(flag_encrypted);

	if (IS_FLAG_SET(flag_pairing_failed)) {
		TEST_FAIL("Pairing reported as failed");
	}

	if (reached_level < BT_SECURITY_L4) {
		TEST_FAIL("Expected LE SC authenticated pairing, got level %d", reached_level);
	}
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	int err;

	if (type != BT_GAP_ADV_TYPE_ADV_IND || rssi < -70) {
		return;
	}

	err = bt_le_scan_stop();
	if (err != 0) {
		return;
	}

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT,
				&default_conn);
	if (err != 0) {
		TEST_FAIL("Failed to create connection (err %d)", err);
	}

	/* connected() takes its own reference */
	bt_conn_drop(&default_conn);
}

static void central(void)
{
	int err;

	TEST_START("central");

	test_init();

	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, device_found);
	if (err != 0) {
		TEST_FAIL("Scanning failed to start (err %d)", err);
	}

	WAIT_FOR_FLAG(flag_connected);

	err = bt_conn_set_security(default_conn, BT_SECURITY_L4);
	if (err != 0) {
		TEST_FAIL("Failed to set security (err %d)", err);
	}

	test_check_paired();

	TEST_PASS("Passkey Entry completed at level %d", reached_level);
}

static void peripheral(void)
{
	int err;

	TEST_START("peripheral");

	test_init();

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, NULL, 0, NULL, 0);
	if (err != 0) {
		TEST_FAIL("Advertising failed to start (err %d)", err);
	}

	WAIT_FOR_FLAG(flag_connected);

	test_check_paired();

	TEST_PASS("Passkey Entry completed at level %d", reached_level);
}

static const struct bst_test_instance test_to_add[] = {
	{
		.test_id = "central",
		.test_main_f = central,
	},
	{
		.test_id = "peripheral",
		.test_main_f = peripheral,
	},
	BSTEST_END_MARKER,
};

static struct bst_test_list *install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_to_add);
};

bst_test_install_t test_installers[] = {install, NULL};

int main(void)
{
	bst_main();
	return 0;
}
