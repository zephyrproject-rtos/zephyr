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

CREATE_FLAG(flag_connected);
CREATE_FLAG(flag_conn_recycled);

static void common_init(void)
{
	int err;

	err = bt_enable(NULL);

	if (err) {
		FAIL("Bluetooth init failed: %d\n", err);
		return;
	}
	printk("Bluetooth initialized\n");
}

static void create_ext_adv_set(struct bt_le_ext_adv **adv, bool connectable)
{
	int err;

	printk("Creating extended advertising set...");

	const struct bt_le_adv_param *adv_param = connectable ?
		BT_LE_EXT_ADV_CONN_NAME : BT_LE_EXT_ADV_NCONN_NAME;

	err = bt_le_ext_adv_create(adv_param, NULL, adv);
	if (err) {
		printk("Failed to create advertising set: %d\n", err);
		return;
	}
	printk("done.\n");
}

static void start_ext_adv_set(struct bt_le_ext_adv *adv)
{
	int err;

	printk("Starting Extended Advertising...");
	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err) {
		printk("Failed to start extended advertising: %d\n", err);
		return;
	}
	printk("done.\n");
}

static void stop_ext_adv_set(struct bt_le_ext_adv *adv)
{
	int err;

	printk("Stopping Extended Advertising...");
	err = bt_le_ext_adv_stop(adv);
	if (err) {
		printk("Failed to stop extended advertising: %d\n",
		       err);
		return;
	}
	printk("done.\n");
}

static void delete_adv_set(struct bt_le_ext_adv *adv)
{
	int err;

	printk("Delete extended advertising set...");
	err = bt_le_ext_adv_delete(adv);
	if (err) {
		printk("Failed Delete extended advertising set: %d\n", err);
		return;
	}
	printk("done.\n");
}

static void disconnect_from_target(void)
{
	int err;

	printk("Disconnecting...\n");

	err = bt_conn_disconnect(g_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (err) {
		FAIL("BT Disconnect failed: %d\n", err);
		return;
	}
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err != BT_HCI_ERR_SUCCESS) {
		FAIL("Failed to connect to %s: %u\n", addr, err);
		return;
	}

	printk("Connected to %s\n", addr);
	if (g_conn != NULL) {
		FAIL("Attempt to override connection object without clean-up\n");
		return;
	}
	g_conn = bt_conn_ref(conn);
	SET_FLAG(flag_connected);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s (reason %u)\n", addr, reason);

	bt_conn_unref(g_conn);
	g_conn = NULL;
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

static void main_ext_adv_advertiser(void)
{
	struct bt_le_ext_adv *ext_adv;

	common_init();

	create_ext_adv_set(&ext_adv, false);
	start_ext_adv_set(ext_adv);

	/* Advertise for a bit */
	k_sleep(K_SECONDS(5));

	stop_ext_adv_set(ext_adv);
	delete_adv_set(ext_adv);

	ext_adv = NULL;

	PASS("Extended advertiser passed\n");
}

static void main_ext_conn_adv_advertiser(void)
{
	struct bt_le_ext_adv *ext_adv;

	common_init();

	bt_conn_cb_register(&conn_cbs);

	create_ext_adv_set(&ext_adv, true);
	start_ext_adv_set(ext_adv);

	printk("Waiting for connection...\n");
	WAIT_FOR_FLAG(flag_connected);

	disconnect_from_target();

	printk("Waiting for Connection object to be recycled...\n");
	WAIT_FOR_FLAG(flag_conn_recycled);

	stop_ext_adv_set(ext_adv);
	delete_adv_set(ext_adv);

	create_ext_adv_set(&ext_adv, false);
	start_ext_adv_set(ext_adv);

	/* Advertise for a bit */
	k_sleep(K_SECONDS(5));

	stop_ext_adv_set(ext_adv);
	delete_adv_set(ext_adv);

	ext_adv = NULL;

	PASS("Extended advertiser passed\n");
}

static const struct bst_test_instance ext_adv_advertiser[] = {
	{
		.test_id = "ext_adv_advertiser",
		.test_descr = "Basic extended advertising test. "
			      "Will just start extended advertising.",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = main_ext_adv_advertiser
	},
	{
		.test_id = "ext_adv_conn_advertiser",
		.test_descr = "Basic connectable extended advertising test. "
			      "Starts extended advertising, and restarts it after disconnecting",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = main_ext_conn_adv_advertiser
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_ext_adv_advertiser(struct bst_test_list *tests)
{
	return bst_add_tests(tests, ext_adv_advertiser);
}
