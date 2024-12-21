/**
 * Copyright (c) 2024 Croxel, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>

#include "common.h"

#include "bs_types.h"
#include "bs_tracing.h"
#include "time_machine.h"
#include "bstests.h"

#include <zephyr/types.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>

extern enum bst_result_t bst_result;

static struct bt_conn *g_conn;

CREATE_FLAG(flag_ext_adv_seen);
CREATE_FLAG(flag_connected);
CREATE_FLAG(flag_conn_recycled);

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err != BT_HCI_ERR_SUCCESS) {
		FAIL("Failed to connect to %s: %u\n", addr, err);
		bt_conn_unref(g_conn);
		g_conn = NULL;
		return;
	}

	printk("Connected to %s\n", addr);
	SET_FLAG(flag_connected);
}

static void free_conn_object_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);

	bt_conn_unref(g_conn);
	g_conn = NULL;
}

static K_WORK_DELAYABLE_DEFINE(free_conn_object_work, free_conn_object_work_fn);

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s (reason %u)\n", addr, reason);

	/* Schedule to cause de-sync between disconnected and recycled events,
	 * in order to prove the test is relying properly on it.
	 */
	k_work_schedule(&free_conn_object_work, K_MSEC(500));

	UNSET_FLAG(flag_connected);
}

static void recycled(void)
{
	SET_FLAG(flag_conn_recycled);
}

static struct bt_conn_cb conn_cbs = {
	.connected = connected,
	.disconnected = disconnected,
	.recycled = recycled,
};


static void scan_recv(const struct bt_le_scan_recv_info *info,
		      struct net_buf_simple *buf)
{
	printk("Found advertisement. Adv-type: 0x%02x, Adv-prop: 0x%02x\n",
		info->adv_type, info->adv_props);

	if (info->adv_type == BT_GAP_ADV_TYPE_EXT_ADV &&
	    info->adv_props & BT_GAP_ADV_PROP_EXT_ADV) {
		printk("Found extended advertisement!\n");
		SET_FLAG(flag_ext_adv_seen);
	}

	if (!TEST_FLAG(flag_connected) &&
	    info->adv_props & BT_GAP_ADV_PROP_CONNECTABLE) {
		int err;

		printk("Stopping scan\n");
		err = bt_le_scan_stop();
		if (err) {
			FAIL("Failed to stop scan: %d", err);
			return;
		}

		err = bt_conn_le_create(info->addr, BT_CONN_LE_CREATE_CONN,
					BT_LE_CONN_PARAM_DEFAULT, &g_conn);
		if (err) {
			FAIL("Could not connect to peer: %d", err);
			return;
		}
	}
}

static struct bt_le_scan_cb scan_callbacks = {
	.recv = scan_recv,
};

static void common_init(void)
{
	int err = 0;

	err = bt_enable(NULL);

	if (err) {
		FAIL("Bluetooth init failed: %d\n", err);
		return;
	}

	bt_conn_cb_register(&conn_cbs);
	bt_le_scan_cb_register(&scan_callbacks);

	printk("Bluetooth initialized\n");
}

static void start_scan(void)
{
	int err;

	printk("Start scanning...");
	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL);
	if (err) {
		FAIL("Failed to start scan: %d\n", err);
		return;
	}
	printk("done.\n");
}

static void main_ext_adv_scanner(void)
{
	common_init();
	start_scan();

	printk("Waiting for extended advertisements...\n");

	WAIT_FOR_FLAG(flag_ext_adv_seen);

	PASS("Extended adv scanner passed\n");
}

static void scan_connect_and_disconnect_cycle(void)
{
	start_scan();

	printk("Waiting for extended advertisements...\n");
	WAIT_FOR_FLAG(flag_ext_adv_seen);

	printk("Waiting for connection with device...\n");
	WAIT_FOR_FLAG(flag_connected);

	printk("Waiting for device disconnection...\n");
	WAIT_FOR_FLAG_UNSET(flag_connected);

	printk("Waiting for Connection object to be recycled...\n");
	WAIT_FOR_FLAG(flag_conn_recycled);

	/* Iteration cleanup */
	printk("Clearing flag for seen extended advertisements...\n");
	UNSET_FLAG(flag_ext_adv_seen);
	UNSET_FLAG(flag_conn_recycled);
}

static void main_ext_adv_conn_scanner(void)
{
	common_init();

	scan_connect_and_disconnect_cycle();

	start_scan();
	printk("Waiting to extended advertisements (again)...\n");
	WAIT_FOR_FLAG(flag_ext_adv_seen);

	PASS("Extended adv scanner passed\n");
}

static void main_ext_adv_conn_scanner_x5(void)
{
	common_init();

	for (size_t i = 0 ; i < 5 ; i++) {
		printk("Iteration %d...\n", i);
		scan_connect_and_disconnect_cycle();
	}

	start_scan();
	printk("Waiting to extended advertisements (again)...\n");
	WAIT_FOR_FLAG(flag_ext_adv_seen);

	PASS("Extended adv scanner x5 passed\n");
}

static const struct bst_test_instance ext_adv_scanner[] = {
	{
		.test_id = "ext_adv_scanner",
		.test_descr = "Basic extended advertising scanning test. "
			      "Will just scan an extended advertiser.",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = main_ext_adv_scanner
	},
	{
		.test_id = "ext_adv_conn_scanner",
		.test_descr = "Basic extended advertising scanning test. "
			      "Will scan an extended advertiser, connect "
			      "and verify it's detected after disconnection",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = main_ext_adv_conn_scanner
	},
	{
		.test_id = "ext_adv_conn_scanner_x5",
		.test_descr = "Basic extended advertising scanning test. "
			      "Will scan an extended advertiser, connect "
			      "and verify it's detected after disconnection,"
			      "repeated over 5 times",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = main_ext_adv_conn_scanner_x5
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_ext_adv_scanner(struct bst_test_list *tests)
{
	return bst_add_tests(tests, ext_adv_scanner);
}

bst_test_install_t test_installers[] = {
	test_ext_adv_scanner,
	NULL
};

int main(void)
{
	bst_main();
	return 0;
}
