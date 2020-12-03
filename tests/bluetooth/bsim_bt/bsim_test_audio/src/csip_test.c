/*
 * Copyright (c) 2019 Bose Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifdef CONFIG_BT_CSIP
#include "../../../../../subsys/bluetooth/host/audio/csip.h"
#include "common.h"

extern enum bst_result_t bst_result;
static uint8_t expected_passes = 1;
static uint8_t passes;
static volatile bool is_connected;
static volatile bool discovered;
static volatile bool sets_discovered;
static volatile bool members_discovered;
static volatile bool set_locked;
static volatile bool set_unlocked;
static struct bt_csip_set_t *set;
static struct bt_conn *main_conn;

static void csip_lock_set_cb(int err);

static void csip_lock_release_cb(int err)
{
	printk("csip_lock_release_cb\n");

	if (err) {
		FAIL("Release sets failed (%d)\n", err);
		return;
	}

	set_unlocked = true;
}

static void csip_lock_set_cb(int err)
{
	printk("csip_lock_set_cb\n");

	if (err) {
		FAIL("Lock sets failed (%d)\n", err);
		return;
	}

	set_locked = true;
}

static void bt_csip_discover_members_cb(int err, uint8_t set_size,
					uint8_t members_found)
{
	printk("Discovered %u/%u set members\n", members_found, set_size);

	if (err) {
		FAIL("Discover members failed (%d)\n", err);
		return;
	}

	if (set_size != members_found) {
		FAIL("Discover members only found %u/%u devices\n",
		     members_found, set_size);
		return;
	}

	members_discovered = true;
}

static void csip_discover_sets_cb(struct bt_conn *conn, int err,
				  uint8_t set_count, struct bt_csip_set_t *sets)
{
	printk("csip_discover_sets_cb\n");

	if (err) {
		FAIL("Discover sets failed (%d)\n", err);
		return;
	}

	for (int i = 0; i < set_count; i++) {
		printk("Set %u: size %d\n", i, sets[i].set_size);
	}

	set = sets;
	sets_discovered = true;
}

static void csis_discover_cb(struct bt_conn *conn, int err, uint8_t set_count)
{
	printk("csis_discover_cb\n");

	if (err) {
		FAIL("Init failed (%d)\n", err);
		return;
	}

	discovered = true;
}

static void csis_lock_changed_cb(struct bt_conn *conn,
				 struct bt_csip_set_t *set,
				 bool locked)
{
	printk("Set %p %s\n", set, locked ? "locked" : "released");
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (is_connected) {
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		FAIL("Failed to connect to %s (%u)\n", addr, err);
		return;
	}

	printk("Connected to %s\n", addr);
	main_conn = conn;
	is_connected = true;
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
};

static struct bt_csip_cb_t cbs = {
	.lock_set = csip_lock_set_cb,
	.release_set = csip_lock_release_cb,
	.members = bt_csip_discover_members_cb,
	.sets = csip_discover_sets_cb,
	.discover = csis_discover_cb,
	.lock_changed = csis_lock_changed_cb,
};

static void test_main(void)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Audio Client: Bluetooth initialized\n");

	bt_conn_cb_register(&conn_callbacks);
	bt_csip_register_cb(&cbs);

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err) {
		FAIL("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning successfully started\n");

	WAIT_FOR(is_connected);

	err = bt_csip_discover(main_conn, true);
	if (err) {
		FAIL("Failed to initialize CSIP for connection %d\n", err);
		return;
	}

	WAIT_FOR(discovered);

	err = bt_csip_discover_sets(main_conn);
	if (err) {
		FAIL("Failed to do CSIP discovery sets (%d)\n", err);
		return;
	}

	WAIT_FOR(sets_discovered);

	err = bt_csip_discover_members(set);
	if (err) {
		FAIL("Failed to do CSIP member discovery (%d)\n", err);
		return;
	}

	WAIT_FOR(members_discovered);

	err = bt_csip_lock_set();
	if (err) {
		FAIL("Failed to do CSIP lock (%d)", err);
		return;
	}

	WAIT_FOR(set_locked);

	k_sleep(K_MSEC(1000)); /* Simulate doing stuff */

	err = bt_csip_release_set();
	if (err) {
		FAIL("Failed to do CSIP release (%d)", err);
		return;
	}

	WAIT_FOR(set_unlocked);

	/* Lock and unlock again */
	set_locked = false;
	set_unlocked = false;

	err = bt_csip_lock_set();
	if (err) {
		FAIL("Failed to do CSIP lock (%d)", err);
		return;
	}

	WAIT_FOR(set_locked);

	err = bt_csip_release_set();
	if (err) {
		FAIL("Failed to do CSIP release (%d)", err);
		return;
	}

	WAIT_FOR(set_unlocked);

	err = bt_csip_disconnect();
	if (err) {
		FAIL("Failed to do disconnect\n", err);
		return;
	}

	PASS("All members disconnected\n");
}

static const struct bst_test_instance test_connect[] = {

	{
		.test_id = "csip",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main
	},

	BSTEST_END_MARKER
};

struct bst_test_list *test_csip_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_connect);
}
#else
struct bst_test_list *test_csip_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_CSIP */
