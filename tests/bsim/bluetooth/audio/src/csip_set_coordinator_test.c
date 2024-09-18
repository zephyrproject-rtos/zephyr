/*
 * Copyright (c) 2019 Bose Corporation
 * Copyright (c) 2020-2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/csip.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include "bstests.h"
#include "common.h"

#ifdef CONFIG_BT_CSIP_SET_COORDINATOR

static bool expect_rank = true;
static bool expect_set_size = true;
static bool expect_lockable = true;

extern enum bst_result_t bst_result;
static volatile bool discovered;
static volatile bool members_discovered;
static volatile bool discover_timed_out;
static volatile bool set_locked;
static volatile bool set_unlocked;
static volatile bool ordered_access_locked;
static volatile bool ordered_access_unlocked;
static const struct bt_csip_set_coordinator_csis_inst *primary_inst;
CREATE_FLAG(flag_sirk_changed);

static uint8_t connected_member_count;
static uint8_t members_found;
static struct k_work_delayable discover_members_timer;
static bt_addr_le_t addr_found[CONFIG_BT_MAX_CONN];
static struct bt_conn *conns[CONFIG_BT_MAX_CONN];
static const struct bt_csip_set_coordinator_set_member *set_members[CONFIG_BT_MAX_CONN];

static void csip_set_coordinator_lock_set_cb(int err);

static void csip_set_coordinator_lock_release_cb(int err)
{
	printk("%s\n", __func__);

	if (err != 0) {
		FAIL("Release sets failed (%d)\n", err);
		return;
	}

	set_unlocked = true;
}

static void csip_set_coordinator_lock_set_cb(int err)
{
	printk("%s\n", __func__);

	if (err != 0) {
		FAIL("Lock sets failed (%d)\n", err);
		return;
	}

	set_locked = true;
}

static void csip_discover_cb(struct bt_conn *conn,
			     const struct bt_csip_set_coordinator_set_member *member,
			     int err, size_t set_count)
{
	uint8_t conn_index;

	printk("%s\n", __func__);

	if (err != 0 || set_count == 0U) {
		FAIL("Discover failed (%d)\n", err);
		return;
	}

	conn_index = bt_conn_index(conn);

	for (size_t i = 0U; i < set_count; i++) {
		const uint8_t rank = member->insts[i].info.rank;
		const uint8_t set_size = member->insts[i].info.set_size;
		const uint8_t lockable = member->insts[i].info.lockable;

		printk("CSIS[%zu]: %p\n", i, &member->insts[i]);
		printk("\tRank: %u\n", rank);
		printk("\tSet Size: %u\n", set_size);
		printk("\tLockable: %u\n", lockable);

		if ((expect_rank && rank == 0U) || (!expect_rank && rank != 0U)) {
			FAIL("Unexpected rank: %u %u", expect_rank, rank);

			return;
		}

		if ((expect_set_size && set_size == 0U) || (!expect_set_size && set_size != 0U)) {
			FAIL("Unexpected set_size: %u %u", expect_set_size, set_size);

			return;
		}

		if (expect_lockable != lockable) {
			FAIL("Unexpected lockable: %u %u", expect_lockable, lockable);

			return;
		}
	}

	if (primary_inst == NULL) {
		primary_inst = &member->insts[0];
	}

	set_members[conn_index] = member;
	discovered = true;
}

static void csip_lock_changed_cb(struct bt_csip_set_coordinator_csis_inst *inst,
				 bool locked)
{
	printk("inst %p %s\n", inst, locked ? "locked" : "released");
}

static void csip_sirk_changed_cb(struct bt_csip_set_coordinator_csis_inst *inst)
{
	printk("Inst %p SIRK changed\n", inst);

	SET_FLAG(flag_sirk_changed);
}

static void csip_set_coordinator_ordered_access_cb(
	const struct bt_csip_set_coordinator_set_info *set_info, int err,
	bool locked,  struct bt_csip_set_coordinator_set_member *member)
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

static struct bt_csip_set_coordinator_cb cbs = {
	.lock_set = csip_set_coordinator_lock_set_cb,
	.release_set = csip_set_coordinator_lock_release_cb,
	.discover = csip_discover_cb,
	.lock_changed = csip_lock_changed_cb,
	.sirk_changed = csip_sirk_changed_cb,
	.ordered_access = csip_set_coordinator_ordered_access_cb,
};

static bool csip_set_coordinator_oap_cb(const struct bt_csip_set_coordinator_set_info *set_info,
					struct bt_csip_set_coordinator_set_member *members[],
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
		if (bt_addr_le_eq(addr, &addr_found[i])) {
			return true;
		}
	}
	return false;
}

static bool csip_found(struct bt_data *data, void *user_data)
{
	if (bt_csip_set_coordinator_is_set_member(primary_inst->info.sirk, data)) {
		const bt_addr_le_t *addr = user_data;
		char addr_str[BT_ADDR_LE_STR_LEN];

		bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
		printk("Found CSIP advertiser with address %s\n", addr_str);

		if (is_discovered(addr)) {
			printk("Set member already found\n");
			/* Stop parsing */
			return false;
		}

		bt_addr_le_copy(&addr_found[members_found++], addr);

		if (primary_inst->info.set_size == 0) {
			printk("Found member %u\n", members_found);
		} else {
			printk("Found member (%u / %u)\n", members_found,
			       primary_inst->info.set_size);
		}

		/* Stop parsing */
		return false;
	}
	/* Continue parsing */
	return true;
}

static void csip_set_coordinator_scan_recv(const struct bt_le_scan_recv_info *info,
					   struct net_buf_simple *ad)
{
	/* We're only interested in connectable events */
	if (info->adv_props & BT_GAP_ADV_PROP_CONNECTABLE) {
		if (primary_inst == NULL) {
			/* Scanning for the first device */
			if (members_found == 0) {
				bt_addr_le_copy(&addr_found[members_found++],
						info->addr);
			}
		} else { /* Scanning for set members */
			bt_data_parse(ad, csip_found, (void *)info->addr);
		}
	}
}

static struct bt_le_scan_cb csip_set_coordinator_scan_callbacks = {
	.recv = csip_set_coordinator_scan_recv
};

static void discover_members_timer_handler(struct k_work *work)
{
	if (primary_inst->info.set_size > 0) {
		FAIL("Could not find all members (%u / %u)\n", members_found,
		     primary_inst->info.set_size);
	} else {
		discover_timed_out = true;
	}
}

static void ordered_access(const struct bt_csip_set_coordinator_set_member **members,
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

	err = bt_csip_set_coordinator_ordered_access(members, count, &primary_inst->info,
						     csip_set_coordinator_oap_cb);
	if (err != 0) {
		FAIL("Failed to do CSIP set coordinator ordered access (%d)",
		      err);
		return;
	}

	if (expect_locked) {
		WAIT_FOR_COND(ordered_access_locked);
	} else {
		WAIT_FOR_COND(ordered_access_unlocked);
	}
}

static void discover_csis(struct bt_conn *conn)
{
	int err;

	discovered = false;

	err = bt_csip_set_coordinator_discover(conns[bt_conn_index(conn)]);
	if (err != 0) {
		FAIL("Failed to initialize set coordinator for connection %d\n", err);
		return;
	}

	WAIT_FOR_COND(discovered);
}

static void init(void)
{
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Audio Client: Bluetooth initialized\n");

	bt_csip_set_coordinator_register_cb(&cbs);
	k_work_init_delayable(&discover_members_timer,
			      discover_members_timer_handler);
	bt_le_scan_cb_register(&csip_set_coordinator_scan_callbacks);
}

static void connect_set(void)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	connected_member_count = 0U;

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL);
	if (err != 0) {
		FAIL("Scanning failed to start (err %d)\n", err);

		return;
	}

	printk("Scanning successfully started\n");

	WAIT_FOR_COND(members_found == 1U);

	printk("Stopping scan\n");
	err = bt_le_scan_stop();
	if (err != 0) {
		FAIL("Could not stop scan");

		return;
	}

	bt_addr_le_to_str(&addr_found[0], addr, sizeof(addr));
	err = bt_conn_le_create(&addr_found[0], BT_CONN_LE_CREATE_CONN,
				BT_LE_CONN_PARAM_DEFAULT, &conns[0]);
	if (err != 0) {
		FAIL("Failed to connect to %s: %d\n", err);

		return;
	}
	printk("Connecting to %s\n", addr);

	WAIT_FOR_FLAG(flag_connected);
	connected_member_count++;

	discover_csis(conns[0]);
	discover_csis(conns[0]); /* test that we can discover twice */

	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, NULL);
	if (err != 0) {
		FAIL("Could not start scan: %d", err);

		return;
	}

	err = k_work_reschedule(&discover_members_timer,
				BT_CSIP_SET_COORDINATOR_DISCOVER_TIMER_VALUE);
	if (err < 0) { /* Can return 0, 1 and 2 for success */
		FAIL("Could not schedule discover_members_timer %d", err);

		return;
	}

	if (primary_inst->info.set_size > 0U) {
		WAIT_FOR_COND(members_found == primary_inst->info.set_size);

		(void)k_work_cancel_delayable(&discover_members_timer);
	} else {
		WAIT_FOR_COND(discover_timed_out);
	}

	err = bt_le_scan_stop();
	if (err != 0) {
		FAIL("Scanning failed to stop (err %d)\n", err);

		return;
	}

	for (uint8_t i = 1; i < members_found; i++) {
		bt_addr_le_to_str(&addr_found[i], addr, sizeof(addr));

		UNSET_FLAG(flag_connected);
		printk("Connecting to member[%d] (%s)", i, addr);
		err = bt_conn_le_create(&addr_found[i], BT_CONN_LE_CREATE_CONN,
					BT_LE_CONN_PARAM_DEFAULT, &conns[i]);
		if (err != 0) {
			FAIL("Failed to connect to %s: %d\n", addr, err);

			return;
		}

		printk("Connected to %s\n", addr);
		WAIT_FOR_FLAG(flag_connected);
		connected_member_count++;

		printk("Doing discovery on member[%u]", i);
		discover_csis(conns[i]);
	}
}

static void disconnect_set(void)
{
	for (uint8_t i = 0; i < connected_member_count; i++) {
		char addr[BT_ADDR_LE_STR_LEN];
		int err;

		bt_addr_le_to_str(&addr_found[i], addr, sizeof(addr));

		printk("Disconnecting member[%u] (%s)", i, addr);
		err = bt_conn_disconnect(conns[i], BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		(void)memset(&set_members[i], 0, sizeof(set_members[i]));
		if (err != 0) {
			FAIL("Failed to do disconnect\n", err);
			return;
		}
	}
}

static void test_main(void)
{
	const struct bt_csip_set_coordinator_set_member *locked_members[CONFIG_BT_MAX_CONN];
	int err;

	init();
	connect_set();

	for (size_t i = 0; i < ARRAY_SIZE(locked_members); i++) {
		locked_members[i] = set_members[i];
	}

	if (primary_inst->info.rank != 0U) {
		ordered_access(locked_members, connected_member_count, false);
	}

	if (primary_inst->info.lockable) {
		printk("Locking set\n");
		err = bt_csip_set_coordinator_lock(locked_members, connected_member_count,
						   &primary_inst->info);
		if (err != 0) {
			FAIL("Failed to do set coordinator lock (%d)", err);
			return;
		}

		WAIT_FOR_COND(set_locked);
	}

	if (primary_inst->info.rank != 0U) {
		ordered_access(locked_members, connected_member_count, primary_inst->info.lockable);
	}

	k_sleep(K_MSEC(1000)); /* Simulate doing stuff */

	if (primary_inst->info.lockable) {
		printk("Releasing set\n");
		err = bt_csip_set_coordinator_release(locked_members, connected_member_count,
						      &primary_inst->info);
		if (err != 0) {
			FAIL("Failed to do set coordinator release (%d)", err);
			return;
		}

		WAIT_FOR_COND(set_unlocked);
	}

	if (primary_inst->info.rank != 0U) {
		ordered_access(locked_members, connected_member_count, false);
	}

	if (primary_inst->info.lockable) {
		/* Lock and unlock again */
		set_locked = false;
		set_unlocked = false;

		printk("Locking set\n");
		err = bt_csip_set_coordinator_lock(locked_members, connected_member_count,
						   &primary_inst->info);
		if (err != 0) {
			FAIL("Failed to do set coordinator lock (%d)", err);
			return;
		}

		WAIT_FOR_COND(set_locked);
	}

	k_sleep(K_MSEC(1000)); /* Simulate doing stuff */

	if (primary_inst->info.lockable) {
		printk("Releasing set\n");
		err = bt_csip_set_coordinator_release(locked_members, connected_member_count,
						      &primary_inst->info);
		if (err != 0) {
			FAIL("Failed to do set coordinator release (%d)", err);
			return;
		}

		WAIT_FOR_COND(set_unlocked);
	}

	disconnect_set();

	PASS("All members disconnected\n");
}

static void test_new_sirk(void)
{
	init();
	connect_set();

	backchannel_sync_send_all();
	backchannel_sync_wait_all();

	WAIT_FOR_FLAG(flag_sirk_changed);

	disconnect_set();

	PASS("All members disconnected\n");
}

static void test_args(int argc, char *argv[])
{
	for (int argn = 0; argn < argc; argn++) {
		const char *arg = argv[argn];

		if (strcmp(arg, "no-size") == 0) {
			expect_set_size = false;
		} else if (strcmp(arg, "no-rank") == 0) {
			expect_rank = false;
		} else if (strcmp(arg, "no-lock") == 0) {
			expect_lockable = false;
		} else {
			FAIL("Invalid arg: %s", arg);
		}
	}
}

static const struct bst_test_instance test_connect[] = {
	{
		.test_id = "csip_set_coordinator",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main,
		.test_args_f = test_args,
	},
	{
		.test_id = "csip_set_coordinator_new_sirk",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_new_sirk,
		.test_args_f = test_args,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_csip_set_coordinator_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_connect);
}
#else
struct bst_test_list *test_csip_set_coordinator_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_CSIP_SET_COORDINATOR */
