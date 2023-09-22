/*
 * Copyright (c) 2023 Demant A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/audio/csip.h>

#include "common.h"
#include "common/bt_str.h"

extern enum bst_result_t bst_result;

CREATE_FLAG(flag_csip_set_lock_discovered);
CREATE_FLAG(flag_all_notifications_received);

static void csip_discover_cb(struct bt_conn *conn,
			     const struct bt_csip_set_coordinator_set_member *member,
			     int err, size_t set_count)
{
	if (err != 0) {
		printk("CSIP Lock Discover failed (err = %d)\n", err);
		return;
	}

	if (member->insts->info.lockable) {
		SET_FLAG(flag_csip_set_lock_discovered);
	}
}

static void csip_lock_changed(struct bt_csip_set_coordinator_csis_inst *inst, bool locked)
{
	SET_FLAG(flag_all_notifications_received);
}

static struct bt_csip_set_coordinator_cb cbs = {
	.lock_changed = csip_lock_changed,
	.discover = csip_discover_cb,
};

static void test_main(void)
{
	int err;

	printk("Enabling Bluetooth\n");
	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth enable failed (err %d)\n", err);
		return;
	}

	bt_le_scan_cb_register(&common_scan_cb);
	bt_csip_set_coordinator_register_cb(&cbs);

	printk("Starting scan\n");
	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL);
	if (err != 0) {
		FAIL("Could not start scanning (err %d)\n", err);
		return;
	}

	printk("Waiting for connect\n");
	WAIT_FOR_FLAG(flag_connected);

	printk("Raising security\n");
	err = bt_conn_set_security(default_conn, BT_SECURITY_L2);
	if (err) {
		FAIL("Failed to ser security level %d (err %d)\n", BT_SECURITY_L2, err);
		return;
	}

	printk("Starting Discovery\n");
	bt_csip_set_coordinator_discover(default_conn);
	WAIT_FOR_FLAG(flag_csip_set_lock_discovered);

	printk("Waiting for all notifications to be received\n");

	WAIT_FOR_FLAG(flag_all_notifications_received);

	/* Disconnect and wait for server to advertise again (after notifications are triggered) */
	bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	UNSET_FLAG(flag_all_notifications_received);
	UNSET_FLAG(flag_csip_set_lock_discovered);

	printk("Waiting for disconnect\n");
	WAIT_FOR_UNSET_FLAG(flag_connected);

	printk("Starting scan\n");
	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL);
	if (err != 0) {
		FAIL("Could not start scanning (err %d)\n", err);
		return;
	}

	printk("Waiting for reconnect\n");
	WAIT_FOR_FLAG(flag_connected);

	printk("Raising security\n");
	err = bt_conn_set_security(default_conn, BT_SECURITY_L2);
	if (err) {
		FAIL("Failed to ser security level %d (err %d)\n", BT_SECURITY_L2, err);
		return;
	}

	printk("Starting Discovery\n");
	bt_csip_set_coordinator_discover(default_conn);
	WAIT_FOR_FLAG(flag_csip_set_lock_discovered);

	printk("Waiting for all notifications to be received\n");
	WAIT_FOR_FLAG(flag_all_notifications_received);

	bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	WAIT_FOR_UNSET_FLAG(flag_connected);

	PASS("CSIP Notify client Passed\n");
}

static const struct bst_test_instance test_csip_notify_client[] = {
	{
		.test_id = "csip_notify_client",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_csip_notify_client_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_csip_notify_client);
}
