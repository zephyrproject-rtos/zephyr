/*
 * Copyright (c) 2023 Demant A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stddef.h>

#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/csip.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include "bstests.h"
#include "common.h"

extern enum bst_result_t bst_result;

static struct bt_csip_set_member_svc_inst *svc_inst;

static bool is_peer_subscribed(struct bt_conn *conn)
{
	struct bt_gatt_attr *attr;

	attr = bt_gatt_find_by_uuid(NULL, 0, BT_UUID_CSIS_SET_LOCK);
	if (!attr) {
		printk("No BT_UUID_PACS_SNK attribute found\n");
		return false;
	}

	return bt_gatt_is_subscribed(conn, attr, BT_GATT_CCC_NOTIFY);
}

static void csip_set_member_lock_changed_cb(struct bt_conn *conn,
					    struct bt_csip_set_member_svc_inst *svc_inst,
					    bool locked)
{
	printk("Client %p %s the lock\n", conn, locked ? "locked" : "released");
}

static struct bt_csip_set_member_cb csip_cb = {
	.lock_changed  = csip_set_member_lock_changed_cb,
};

static void test_main(void)
{
	int err;
	const struct bt_data ad[] = {
		BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	};
	struct bt_csip_set_member_register_param csip_params = {
		.set_size = 1,
		.rank = 1,
		.lockable = true,
		.cb       = &csip_cb,
	};

	printk("Enabling Bluetooth\n");
	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth enable failed (err %d)\n", err);
		return;
	}

	printk("Registering CSIP Set Member\n");

	err = bt_cap_acceptor_register(&csip_params, &svc_inst);
	if (err != 0) {
		printk("Failed to register csip\n");
		return;
	}

	printk("Start Advertising\n");
	err = bt_le_adv_start(BT_LE_ADV_CONN_ONE_TIME, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err != 0) {
		FAIL("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Waiting to be connected\n");
	WAIT_FOR_FLAG(flag_connected);
	printk("Connected\n");
	printk("Waiting to be subscribed\n");

	while (!is_peer_subscribed(default_conn)) {
		(void)k_sleep(K_MSEC(10));
	}
	printk("Subscribed\n");

	err = bt_csip_set_member_lock(svc_inst, true, false);
	if (err != 0) {
		FAIL("Failed to set lock (err %d)\n", err);
		return;
	}

	/* Now wait for client to disconnect, then stop adv so it does not reconnect */
	printk("Wait for client disconnect\n");
	WAIT_FOR_UNSET_FLAG(flag_connected);
	printk("Client disconnected\n");

	err = bt_le_adv_stop();
	if (err != 0) {
		FAIL("Advertising failed to stop (err %d)\n", err);
		return;
	}

	/* Trigger changes while device is disconnected */
	err = bt_csip_set_member_lock(svc_inst, false, false);
	if (err != 0) {
		FAIL("Failed to set lock (err %d)\n", err);
		return;
	}

	printk("Start Advertising\n");
	err = bt_le_adv_start(BT_LE_ADV_CONN_ONE_TIME, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err != 0) {
		FAIL("Advertising failed to start (err %d)\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_connected);
	WAIT_FOR_UNSET_FLAG(flag_connected);

	PASS("CSIP Notify Server passed\n");
}

static const struct bst_test_instance test_csip_notify_server[] = {
	{
		.test_id = "csip_notify_server",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_csip_notify_server_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_csip_notify_server);
}
