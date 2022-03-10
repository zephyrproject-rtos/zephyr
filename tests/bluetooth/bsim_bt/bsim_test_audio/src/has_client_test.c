/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_BT_HAS_CLIENT
#include "bluetooth/audio/has.h"
#include "common.h"

extern enum bst_result_t bst_result;

CREATE_FLAG(g_is_connected);
CREATE_FLAG(g_mtu_exchanged);
CREATE_FLAG(g_has_discovered);
CREATE_FLAG(g_active_preset_notified);
CREATE_FLAG(g_preset_read);
CREATE_FLAG(g_preset_updated);

static struct bt_conn *g_conn;
static struct bt_has *g_has;
static volatile uint8_t g_active_preset_id;

static void discover_cb(struct bt_conn *conn, struct bt_has *has, enum bt_has_hearing_aid_type type)
{
	if (!has) {
		FAIL("Failed to discover HAS\n");
		return;
	}

	printk("HAS discovered (type %d)\n", type);

	g_has = has;
	SET_FLAG(g_has_discovered);
}

static void active_preset_cb(struct bt_has *has, int err, uint8_t id)
{
	if (err != 0) {
		FAIL("Failed to get active preset (err %d)\n", err);
		return;
	}

	g_active_preset_id = id;
	SET_FLAG(g_active_preset_notified);
}

static void preset_cb(struct bt_has *has, int err, uint8_t id, uint8_t properties, const char *name)
{
	printk("id %d properties 0x%02x name %s\n", id, properties, name);

	SET_FLAG(g_preset_updated);
}

static void preset_read_complete_cb(struct bt_has *has, int err)
{
	SET_FLAG(g_preset_read);
}

static struct bt_has_client_cb has_cb = {
	.discover = discover_cb,
	.active_preset = active_preset_cb,
	.preset = preset_cb,
	.preset_read_complete = preset_read_complete_cb,
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err > 0) {
		char addr[BT_ADDR_LE_STR_LEN];

		bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

		FAIL("Failed to connect to %s (err %u)\n", addr, err);
		return;
	}
	
	g_conn = conn;
	SET_FLAG(g_is_connected);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static void mtu_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_exchange_params *params)
{
	if (err > 0) {
		FAIL("Failed to exchange MTU (err %u)\n", err);
		return;
	}

	SET_FLAG(g_mtu_exchanged);
}

static void test_main(void)
{
	int err;
	uint8_t id_prev, id_curr;

	static struct bt_gatt_exchange_params mtu_params = {
		.func = mtu_cb,
	};

	err = bt_enable(NULL);
	if (err < 0) {
		FAIL("Bluetooth discover failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	err = bt_has_client_cb_register(&has_cb);
	if (err < 0) {
		FAIL("Callback registration (err %d)\n", err);
		return;
	}

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err < 0) {
		FAIL("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning succesfuly started\n");

	WAIT_FOR(g_is_connected);

	err = bt_gatt_exchange_mtu(g_conn, &mtu_params);
	if (err < 0) {
		FAIL("Failed to exchange MTU (err %d)\n", err);
		return;
	}

	WAIT_FOR(g_mtu_exchanged);

	err = bt_has_discover(g_conn);
	if (err < 0) {
		FAIL("Failed to discover HAS (err %d)\n", err);
		return;
	}

	WAIT_FOR(g_has_discovered);

	err = bt_has_preset_active_get(g_has);
	if (err < 0) {
		FAIL("Failed to get active preset id (err %d)\n", err);
		return;
	}

	WAIT_FOR(g_active_preset_notified);

	printk("Got active preset id %d\n", g_active_preset_id);

	id_prev = g_active_preset_id;

	/* Reset the notification indicator */
	UNSET_FLAG(g_active_preset_notified);

	err = bt_has_preset_active_set_next(g_has);
	if (err < 0) {
		FAIL("Failed to set next (err %d)\n", err);
		return;
	}

	WAIT_FOR(g_active_preset_notified);

	id_curr = g_active_preset_id;

	if (id_curr == id_prev) {
		FAIL("Preset not changed");
		return;
	}

	printk("Got active preset id %d\n", g_active_preset_id);

	id_prev = g_active_preset_id;

	/* Reset the notification indicator */
	UNSET_FLAG(g_active_preset_notified);

	err = bt_has_preset_active_set_next(g_has);
	if (err < 0) {
		FAIL("Failed to set next (err %d)\n", err);
		return;
	}

	WAIT_FOR(g_active_preset_notified);

	id_curr = g_active_preset_id;

	if (id_curr == id_prev) {
		FAIL("Preset not changed");
		return;
	}

	printk("Got active preset id %d\n", g_active_preset_id);

	/* Reset the notification indicator */
	UNSET_FLAG(g_active_preset_notified);

	err = bt_has_preset_active_set_prev(g_has);
	if (err < 0) {
		FAIL("Failed to set prev (err %d)\n", err);
		return;
	}

	WAIT_FOR(g_active_preset_notified);

	id_curr = g_active_preset_id;

	if (id_curr != id_prev) {
		FAIL("Failed to set to previous %d != %d\n", id_curr, id_prev);
		return;
	}

	printk("Got active preset id %d\n", g_active_preset_id);

	err = bt_has_preset_active_clear(g_has);
	if (err == 0) {
		FAIL("Client cannot perform Active Preset clear operation\n");
		return;
	}

	err = bt_has_preset_read_multiple(g_has, 0x01, 0xff);
	if (err < 0) {
		FAIL("Failed to read all presets (err %d)\n", err);
		return;
	}

	WAIT_FOR(g_preset_read);

	UNSET_FLAG(g_preset_updated);

	err = bt_has_preset_name_set(g_has, id_curr, "Custom");
	if (err < 0) {
		FAIL("Failed to set name (err %d)\n", err);
		return;
	}

	WAIT_FOR(g_preset_updated);

	UNSET_FLAG(g_preset_read);

	err = bt_has_preset_read_multiple(g_has, 0x01, 0xff);
	if (err < 0) {
		FAIL("Failed to read all presets (err %d)\n", err);
		return;
	}

	WAIT_FOR(g_preset_read);

	PASS("HAS main PASS\n");
}

static const struct bst_test_instance test_has[] = {
	{
		.test_id = "has_client",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main,
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_has_client_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_has);
}
#else
struct bst_test_list *test_has_client_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_HAS_CLIENT */
