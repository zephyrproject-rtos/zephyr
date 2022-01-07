/*
 * Copyright (c) 2019 Bose Corporation
 * Copyright (c) 2020-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifdef CONFIG_BT_CSIS_CLIENT
#include <bluetooth/audio/csis.h>
#include "common.h"

extern enum bst_result_t bst_result;
static volatile bool is_connected;
static volatile bool discovered;
static volatile bool sets_discovered;
static volatile bool members_discovered;
static volatile bool set_locked;
static volatile bool set_unlocked;
static struct bt_csis_client_set *set;

static uint8_t members_found;
static struct k_work_delayable discover_members_timer;
static struct bt_csis_client_set_member set_members[CONFIG_BT_MAX_CONN];

static void csis_client_lock_set_cb(int err);

static void csis_client_lock_release_cb(int err)
{
	printk("%s\n", __func__);

	if (err != 0) {
		FAIL("Release sets failed (%d)\n", err);
		return;
	}

	set_unlocked = true;
}

static void csis_client_lock_set_cb(int err)
{
	printk("%s\n", __func__);

	if (err != 0) {
		FAIL("Lock sets failed (%d)\n", err);
		return;
	}

	set_locked = true;
}

static void csis_client_discover_members_cb(int err, uint8_t set_size,
					       uint8_t members_found)
{
	printk("Discovered %u/%u set members\n", members_found, set_size);

	if (err != 0) {
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

static void csis_client_discover_sets_cb(struct bt_conn *conn, int err,
					 uint8_t set_count,
					 struct bt_csis_client_set *sets)
{
	printk("%s\n", __func__);

	if (err != 0) {
		FAIL("Discover sets failed (%d)\n", err);
		return;
	}

	for (uint8_t i = 0; i < set_count; i++) {
		printk("Set %u: size %d\n", i, sets[i].set_size);
		memcpy(&set_members[bt_conn_index(conn)].sets[i], &sets[i],
		       sizeof(sets[i]));
	}

	set = sets;
	sets_discovered = true;
}

static void csis_discover_cb(struct bt_conn *conn, int err, uint8_t set_count)
{
	printk("%s\n", __func__);

	if (err != 0) {
		FAIL("Init failed (%d)\n", err);
		return;
	}

	discovered = true;
}

static void csis_lock_changed_cb(struct bt_conn *conn,
				 struct bt_csis_client_set *set,
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

	if (err != 0) {
		FAIL("Failed to connect to %s (%u)\n", addr, err);
		return;
	}

	printk("Connected to %s\n", addr);
	is_connected = true;
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
};

static struct bt_csis_client_cb cbs = {
	.lock_set = csis_client_lock_set_cb,
	.release_set = csis_client_lock_release_cb,
	.members = csis_client_discover_members_cb,
	.sets = csis_client_discover_sets_cb,
	.discover = csis_discover_cb,
	.lock_changed = csis_lock_changed_cb,
};

static bool is_discovered(const bt_addr_le_t *addr)
{
	for (int i = 0; i < members_found; i++) {
		if (bt_addr_le_cmp(addr, &set_members[i].addr) == 0) {
			return true;
		}
	}
	return false;
}

static bool csis_found(struct bt_data *data, void *user_data)
{
	if (bt_csis_client_is_set_member(set->set_sirk.value, data)) {
		const bt_addr_le_t *addr = user_data;
		char addr_str[BT_ADDR_LE_STR_LEN];

		bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
		printk("Found CSIS advertiser with address %s\n", addr_str);

		if (is_discovered(addr)) {
			printk("Set member already found\n");
			/* Stop parsing */
			return false;
		}

		bt_addr_le_copy(&set_members[members_found++].addr, addr);

		printk("Found member (%u / %u)\n",
		       members_found, set->set_size);

		/* Stop parsing */
		return false;
	}
	/* Continue parsing */
	return true;
}

static void csis_client_scan_recv(const struct bt_le_scan_recv_info *info,
				  struct net_buf_simple *ad)
{
	/* We're only interested in connectable events */
	if (info->adv_props & BT_GAP_ADV_PROP_CONNECTABLE) {
		if (set == NULL) {
			/* Scanning for the first device */
			if (members_found == 0) {
				bt_addr_le_copy(&set_members[members_found++].addr,
						info->addr);
			}
		} else { /* Scanning for set members */
			bt_data_parse(ad, csis_found, (void *)info->addr);
		}
	}
}

static struct bt_le_scan_cb csis_client_scan_callbacks = {
	.recv = csis_client_scan_recv
};

static void discover_members_timer_handler(struct k_work *work)
{
	FAIL("Could not find all members (%u / %u)\n",
	     members_found, set->set_size);
}

static void test_main(void)
{
	int err;
	char addr[BT_ADDR_LE_STR_LEN];
	const struct bt_csis_client_set_member *locked_members[CONFIG_BT_MAX_CONN];
	uint8_t connected_member_count = 0;

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Audio Client: Bluetooth initialized\n");

	bt_conn_cb_register(&conn_callbacks);
	bt_csis_client_register_cb(&cbs);
	k_work_init_delayable(&discover_members_timer,
			      discover_members_timer_handler);
	bt_le_scan_cb_register(&csis_client_scan_callbacks);

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL);
	if (err != 0) {
		FAIL("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning successfully started\n");

	WAIT_FOR(members_found == 1);

	printk("Stopping scan\n");
	err = bt_le_scan_stop();
	if (err != 0) {
		FAIL("Could not stop scan");
		return;
	}

	bt_addr_le_to_str(&set_members[0].addr, addr, sizeof(addr));
	err = bt_conn_le_create(&set_members[0].addr, BT_CONN_LE_CREATE_CONN,
				BT_LE_CONN_PARAM_DEFAULT, &set_members[0].conn);
	if (err != 0) {
		FAIL("Failed to connect to %s: %d\n", err);
		return;
	}
	printk("Connecting to %s\n", addr);

	WAIT_FOR(is_connected);
	connected_member_count++;

	err = bt_csis_client_discover(&set_members[0]);
	if (err != 0) {
		FAIL("Failed to initialize CSIS client for connection %d\n",
		     err);
		return;
	}

	WAIT_FOR(discovered);

	err = bt_csis_client_discover_sets(set_members[0].conn);
	if (err != 0) {
		FAIL("Failed to do CSIS client discovery sets (%d)\n", err);
		return;
	}

	WAIT_FOR(sets_discovered);

	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, NULL);
	if (err != 0) {
		FAIL("Could not start scan: %d", err);
		return;
	}

	err = k_work_reschedule(&discover_members_timer,
				CSIS_CLIENT_DISCOVER_TIMER_VALUE);
	if (err < 0) { /* Can return 0, 1 and 2 for success */
		FAIL("Could not schedule discover_members_timer %d", err);
		return;
	}

	WAIT_FOR(members_found == set->set_size);

	(void)k_work_cancel_delayable(&discover_members_timer);
	err = bt_le_scan_stop();
	if (err != 0) {
		FAIL("Scanning failed to stop (err %d)\n", err);
		return;
	}

	for (uint8_t i = 1; i < members_found; i++) {
		bt_addr_le_to_str(&set_members[i].addr, addr, sizeof(addr));

		is_connected = false;
		printk("Connecting to member[%d] (%s)", i, addr);
		err = bt_conn_le_create(&set_members[i].addr,
					BT_CONN_LE_CREATE_CONN,
					BT_LE_CONN_PARAM_DEFAULT,
					&set_members[i].conn);
		if (err != 0) {
			FAIL("Failed to connect to %s: %d\n", addr, err);
			return;
		}

		printk("Connected to %s\n", addr);
		WAIT_FOR(is_connected);
		connected_member_count++;

		discovered = false;
		printk("Doing discovery on member[%u]", i);
		err = bt_csis_client_discover(&set_members[i]);
		if (err != 0) {
			FAIL("Failed to initialize CSIS client for connection %d\n",
			      err);
			return;
		}

		WAIT_FOR(discovered);

		sets_discovered = false;
		printk("Doing sets discovery on member[%u]", i);
		err = bt_csis_client_discover_sets(set_members[i].conn);
		if (err != 0) {
			FAIL("Failed to do CSIS client discovery sets (%d)\n", err);
			return;
		}

		WAIT_FOR(sets_discovered);
	}

	for (size_t i = 0; i < ARRAY_SIZE(locked_members); i++) {
		locked_members[i] = &set_members[i];
	}

	printk("Locking set\n");
	err = bt_csis_client_lock(locked_members, connected_member_count, set);
	if (err != 0) {
		FAIL("Failed to do CSIS client lock (%d)", err);
		return;
	}

	WAIT_FOR(set_locked);

	k_sleep(K_MSEC(1000)); /* Simulate doing stuff */

	printk("Releasing set\n");
	err = bt_csis_client_release(locked_members, connected_member_count,
				     set);
	if (err != 0) {
		FAIL("Failed to do CSIS client release (%d)", err);
		return;
	}

	WAIT_FOR(set_unlocked);

	/* Lock and unlock again */
	set_locked = false;
	set_unlocked = false;

	printk("Locking set\n");
	err = bt_csis_client_lock(locked_members, connected_member_count, set);
	if (err != 0) {
		FAIL("Failed to do CSIS client lock (%d)", err);
		return;
	}

	WAIT_FOR(set_locked);

	k_sleep(K_MSEC(1000)); /* Simulate doing stuff */

	printk("Releasing set\n");
	err = bt_csis_client_release(locked_members, connected_member_count,
				     set);
	if (err != 0) {
		FAIL("Failed to do CSIS client release (%d)", err);
		return;
	}

	WAIT_FOR(set_unlocked);

	for (uint8_t i = 0; i < members_found; i++) {
		printk("Disconnecting member[%u] (%s)", i, addr);
		err = bt_conn_disconnect(set_members[i].conn,
					 BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		(void)memset(&set_members[i], 0, sizeof(set_members[i]));
		if (err != 0) {
			FAIL("Failed to do disconnect\n", err);
			return;
		}
	}

	PASS("All members disconnected\n");
}

static const struct bst_test_instance test_connect[] = {

	{
		.test_id = "csis_client",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main
	},

	BSTEST_END_MARKER
};

struct bst_test_list *test_csis_client_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_connect);
}
#else
struct bst_test_list *test_csis_client_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_CSIS_CLIENT */
