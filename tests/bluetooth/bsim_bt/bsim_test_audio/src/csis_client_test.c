/*
 * Copyright (c) 2019 Bose Corporation
 * Copyright (c) 2020-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifdef CONFIG_BT_CSIS_CLIENT
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/audio/csis.h>
#include "common.h"

extern enum bst_result_t bst_result;
static volatile bool is_connected;
static volatile bool discovered;
static volatile bool members_discovered;
static volatile bool set_locked;
static volatile bool set_unlocked;
static volatile bool ordered_access_locked;
static volatile bool ordered_access_unlocked;
static struct bt_csis_client_csis_inst *inst;

static uint8_t members_found;
static struct k_work_delayable discover_members_timer;
static bt_addr_le_t addr_found[CONFIG_BT_MAX_CONN];
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

static void csis_discover_cb(struct bt_csis_client_set_member *member, int err,
			     uint8_t set_count)
{
	printk("%s\n", __func__);

	if (err != 0) {
		FAIL("Init failed (%d)\n", err);
		return;
	}

	inst = &member->insts[0];
	discovered = true;
}

static void csis_lock_changed_cb(struct bt_csis_client_csis_inst *inst,
				 bool locked)
{
	printk("Inst %p %s\n", inst, locked ? "locked" : "released");
}

static void csis_client_ordered_access_cb(const struct bt_csis_client_set_info *set_info,
					  int err, bool locked,
					  struct bt_csis_client_set_member *member)
{
	if (err) {
		FAIL("Ordered access failed with err %d\n", err);
	} else if (locked) {
		printk("Ordered access procedure locked member %p\n", member);
		ordered_access_locked = true;
	} else {
		printk("Ordered access procedure finished\n");
		ordered_access_unlocked = true;
	}
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (is_connected) {
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err != 0) {
		bt_conn_unref(default_conn);
		default_conn = NULL;
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
	.discover = csis_discover_cb,
	.lock_changed = csis_lock_changed_cb,
	.ordered_access = csis_client_ordered_access_cb
};

static bool csis_client_oap_cb(const struct bt_csis_client_set_info *set_info,
			       struct bt_csis_client_set_member *members[],
			       size_t count)
{
	for (size_t i = 0; i < count; i++) {
		printk("Ordered access for members[%zu]: %p\n", i, members[i]);
	}

	return true;
}

static bool is_discovered(const bt_addr_le_t *addr)
{
	for (int i = 0; i < members_found; i++) {
		if (bt_addr_le_cmp(addr, &addr_found[i]) == 0) {
			return true;
		}
	}
	return false;
}

static bool csis_found(struct bt_data *data, void *user_data)
{
	if (bt_csis_client_is_set_member(inst->info.set_sirk, data)) {
		const bt_addr_le_t *addr = user_data;
		char addr_str[BT_ADDR_LE_STR_LEN];

		bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
		printk("Found CSIS advertiser with address %s\n", addr_str);

		if (is_discovered(addr)) {
			printk("Set member already found\n");
			/* Stop parsing */
			return false;
		}

		bt_addr_le_copy(&addr_found[members_found++], addr);

		printk("Found member (%u / %u)\n",
		       members_found, inst->info.set_size);

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
		if (inst == NULL) {
			/* Scanning for the first device */
			if (members_found == 0) {
				bt_addr_le_copy(&addr_found[members_found++],
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
	     members_found, inst->info.set_size);
}

static void ordered_access(struct bt_csis_client_set_member **members,
			   size_t count, bool expect_locked)
{
	int err;

	printk("Performing ordered access, expecting %s\n",
	       expect_locked ? "locked" : "unlocked");

	if (expect_locked) {
		ordered_access_locked = false;
	} else {
		ordered_access_unlocked = false;
	}

	err = bt_csis_client_ordered_access(members, count, &inst->info,
					    csis_client_oap_cb);
	if (err != 0) {
		FAIL("Failed to do CSIS client ordered access (%d)", err);
		return;
	}

	if (expect_locked) {
		WAIT_FOR_COND(ordered_access_locked);
	} else {
		WAIT_FOR_COND(ordered_access_unlocked);
	}
}

static void test_main(void)
{
	int err;
	char addr[BT_ADDR_LE_STR_LEN];
	struct bt_csis_client_set_member *locked_members[CONFIG_BT_MAX_CONN];
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

	WAIT_FOR_COND(members_found == 1);

	printk("Stopping scan\n");
	err = bt_le_scan_stop();
	if (err != 0) {
		FAIL("Could not stop scan");
		return;
	}

	bt_addr_le_to_str(&addr_found[0], addr, sizeof(addr));
	err = bt_conn_le_create(&addr_found[0], BT_CONN_LE_CREATE_CONN,
				BT_LE_CONN_PARAM_DEFAULT, &set_members[0].conn);
	if (err != 0) {
		FAIL("Failed to connect to %s: %d\n", err);
		return;
	}
	printk("Connecting to %s\n", addr);

	WAIT_FOR_COND(is_connected);
	connected_member_count++;

	err = bt_csis_client_discover(&set_members[0]);
	if (err != 0) {
		FAIL("Failed to initialize CSIS client for connection %d\n",
		     err);
		return;
	}

	WAIT_FOR_COND(discovered);

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

	WAIT_FOR_COND(members_found == inst->info.set_size);

	(void)k_work_cancel_delayable(&discover_members_timer);
	err = bt_le_scan_stop();
	if (err != 0) {
		FAIL("Scanning failed to stop (err %d)\n", err);
		return;
	}

	for (uint8_t i = 1; i < members_found; i++) {
		bt_addr_le_to_str(&addr_found[i], addr, sizeof(addr));

		is_connected = false;
		printk("Connecting to member[%d] (%s)", i, addr);
		err = bt_conn_le_create(&addr_found[i],
					BT_CONN_LE_CREATE_CONN,
					BT_LE_CONN_PARAM_DEFAULT,
					&set_members[i].conn);
		if (err != 0) {
			FAIL("Failed to connect to %s: %d\n", addr, err);
			return;
		}

		printk("Connected to %s\n", addr);
		WAIT_FOR_COND(is_connected);
		connected_member_count++;

		discovered = false;
		printk("Doing discovery on member[%u]", i);
		err = bt_csis_client_discover(&set_members[i]);
		if (err != 0) {
			FAIL("Failed to initialize CSIS client for connection %d\n",
			      err);
			return;
		}

		WAIT_FOR_COND(discovered);
	}

	for (size_t i = 0; i < ARRAY_SIZE(locked_members); i++) {
		locked_members[i] = &set_members[i];
	}

	ordered_access(locked_members, connected_member_count, false);

	printk("Locking set\n");
	err = bt_csis_client_lock(locked_members, connected_member_count,
				  &inst->info);
	if (err != 0) {
		FAIL("Failed to do CSIS client lock (%d)", err);
		return;
	}

	WAIT_FOR_COND(set_locked);

	ordered_access(locked_members, connected_member_count, true);

	k_sleep(K_MSEC(1000)); /* Simulate doing stuff */

	printk("Releasing set\n");
	err = bt_csis_client_release(locked_members, connected_member_count,
				     &inst->info);
	if (err != 0) {
		FAIL("Failed to do CSIS client release (%d)", err);
		return;
	}

	WAIT_FOR_COND(set_unlocked);

	ordered_access(locked_members, connected_member_count, false);

	/* Lock and unlock again */
	set_locked = false;
	set_unlocked = false;

	printk("Locking set\n");
	err = bt_csis_client_lock(locked_members, connected_member_count,
				  &inst->info);
	if (err != 0) {
		FAIL("Failed to do CSIS client lock (%d)", err);
		return;
	}

	WAIT_FOR_COND(set_locked);

	k_sleep(K_MSEC(1000)); /* Simulate doing stuff */

	printk("Releasing set\n");
	err = bt_csis_client_release(locked_members, connected_member_count,
				     &inst->info);
	if (err != 0) {
		FAIL("Failed to do CSIS client release (%d)", err);
		return;
	}

	WAIT_FOR_COND(set_unlocked);

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
