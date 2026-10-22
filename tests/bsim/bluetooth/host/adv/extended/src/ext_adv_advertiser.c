/**
 * Copyright (c) 2024 Croxel, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>

#include "babblekit/testcase.h"
#include "babblekit/flags.h"

#include <zephyr/types.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>

extern enum bst_result_t bst_result;

static struct bt_conn *g_conn;

DEFINE_FLAG_STATIC(flag_connected);
DEFINE_FLAG_STATIC(flag_conn_recycled);

static void common_init(void)
{
	int err;

	err = bt_enable(NULL);

	if (err) {
		TEST_FAIL("Bluetooth init failed: %d", err);
		return;
	}
	printk("Bluetooth initialized\n");
}

static void create_ext_adv_set(struct bt_le_ext_adv **adv, bool connectable)
{
	int err;

	printk("Creating extended advertising set...");

	const struct bt_le_adv_param *adv_param = connectable ?
		BT_LE_EXT_ADV_CONN : BT_LE_EXT_ADV_NCONN;

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
		TEST_FAIL("BT Disconnect failed: %d", err);
		return;
	}
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err != BT_HCI_ERR_SUCCESS) {
		TEST_FAIL("Failed to connect to %s: %u", bt_conn_dst_str(conn), err);
		return;
	}

	printk("Connected to %s\n", bt_conn_dst_str(conn));
	if (g_conn != NULL) {
		TEST_FAIL("Attempt to override connection object without clean-up");
		return;
	}
	g_conn = bt_conn_ref(conn);
	SET_FLAG(flag_connected);
}

static void free_conn_object_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);

	bt_conn_drop(&g_conn);
}

static K_WORK_DELAYABLE_DEFINE(free_conn_object_work, free_conn_object_work_fn);

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected: %s (reason %u)\n", bt_conn_dst_str(conn), reason);

	/* Schedule to cause de-sync between disconnected and recycled events,
	 * in order to prove the test is relying properly on it.
	 */
	k_work_schedule(&free_conn_object_work, K_MSEC(100));

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

	TEST_PASS("Extended advertiser passed");
}

static void adv_connect_and_disconnect_cycle(void)
{
	struct bt_le_ext_adv *ext_adv;

	create_ext_adv_set(&ext_adv, true);
	start_ext_adv_set(ext_adv);

	printk("Waiting for connection...\n");
	WAIT_FOR_FLAG(flag_connected);

	disconnect_from_target();
	WAIT_FOR_FLAG_UNSET(flag_connected);

	printk("Waiting for Connection object to be recycled...\n");
	WAIT_FOR_FLAG(flag_conn_recycled);

	/* Iteration Cleanup */
	UNSET_FLAG(flag_conn_recycled);
	stop_ext_adv_set(ext_adv);
	delete_adv_set(ext_adv);
}

static void main_ext_conn_adv_advertiser(void)
{
	struct bt_le_ext_adv *ext_adv;

	common_init();

	bt_conn_cb_register(&conn_cbs);

	adv_connect_and_disconnect_cycle();

	create_ext_adv_set(&ext_adv, false);
	start_ext_adv_set(ext_adv);

	/* Advertise for a bit */
	k_sleep(K_SECONDS(5));

	stop_ext_adv_set(ext_adv);
	delete_adv_set(ext_adv);

	ext_adv = NULL;

	TEST_PASS("Extended advertiser passed");
}

static void main_ext_conn_adv_advertiser_x5(void)
{
	struct bt_le_ext_adv *ext_adv;

	common_init();

	bt_conn_cb_register(&conn_cbs);

	for (size_t i = 0 ; i < 5 ; i++) {
		printk("Iteration %d...\n", i);
		adv_connect_and_disconnect_cycle();
	}

	/* Advertise for a bit */
	create_ext_adv_set(&ext_adv, false);
	start_ext_adv_set(ext_adv);
	k_sleep(K_SECONDS(5));
	stop_ext_adv_set(ext_adv);
	delete_adv_set(ext_adv);
	ext_adv = NULL;

	TEST_PASS("Extended advertiser passed");
}

DEFINE_FLAG_STATIC(flag_adv_sent);
DEFINE_FLAG_STATIC(flag_adv_connected);
static uint8_t adv_num_sent;

static void adv_sent(struct bt_le_ext_adv *adv, struct bt_le_ext_adv_sent_info *info)
{
	printk("Advertising set %p sent %u events\n", adv, info->num_sent);

	adv_num_sent = info->num_sent;
	SET_FLAG(flag_adv_sent);
}

static void adv_connected(struct bt_le_ext_adv *adv, struct bt_le_ext_adv_connected_info *info)
{
	printk("Advertising set %p connected to %s\n", adv, bt_conn_dst_str(info->conn));

	SET_FLAG(flag_adv_connected);
}

static struct bt_le_ext_adv *start_limited_adv_set(const struct bt_le_adv_param *param,
						   uint16_t timeout, uint8_t num_events)
{
	static const struct bt_le_ext_adv_cb adv_cb = {
		.sent = adv_sent,
		.connected = adv_connected,
	};
	struct bt_le_ext_adv *adv;
	int err;

	err = bt_le_ext_adv_create(param, &adv_cb, &adv);
	TEST_ASSERT(err == 0, "Failed to create advertising set: %d", err);

	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_PARAM(timeout, num_events));
	TEST_ASSERT(err == 0, "Failed to start extended advertising: %d", err);

	return adv;
}

static void main_ext_adv_limited_advertiser(void)
{
	struct bt_le_ext_adv *ext_adv;

	common_init();

	bt_conn_cb_register(&conn_cbs);

	printk("Advertising until the event limit...\n");
	ext_adv = start_limited_adv_set(BT_LE_EXT_ADV_NCONN, 0, 3);
	WAIT_FOR_FLAG(flag_adv_sent);
	TEST_ASSERT(adv_num_sent == 3, "Unexpected number of sent events: %u", adv_num_sent);
	UNSET_FLAG(flag_adv_sent);
	delete_adv_set(ext_adv);

	printk("Advertising with a timeout until connected...\n");
	ext_adv = start_limited_adv_set(BT_LE_EXT_ADV_CONN, 500, 0);
	WAIT_FOR_FLAG(flag_adv_connected);
	WAIT_FOR_FLAG(flag_connected);
	disconnect_from_target();
	WAIT_FOR_FLAG_UNSET(flag_connected);
	WAIT_FOR_FLAG(flag_conn_recycled);
	TEST_ASSERT(!IS_FLAG_SET(flag_adv_sent), "Sent callback without reaching a limit");
	delete_adv_set(ext_adv);

	printk("Advertising until the timeout...\n");
	ext_adv = start_limited_adv_set(BT_LE_EXT_ADV_NCONN, 200, 0);
	WAIT_FOR_FLAG(flag_adv_sent);
	delete_adv_set(ext_adv);

	TEST_PASS("Extended advertiser passed");
}

static const struct bst_test_instance ext_adv_advertiser[] = {
	{
		.test_id = "ext_adv_advertiser",
		.test_descr = "Basic extended advertising test. "
			      "Will just start extended advertising.",
		.test_main_f = main_ext_adv_advertiser
	},
	{
		.test_id = "ext_adv_conn_advertiser",
		.test_descr = "Basic connectable extended advertising test. "
			      "Starts extended advertising, and restarts it after disconnecting",
		.test_main_f = main_ext_conn_adv_advertiser
	},
	{
		.test_id = "ext_adv_conn_advertiser_x5",
		.test_descr = "Basic connectable extended advertising test. "
			      "Starts extended advertising, and restarts it after disconnecting, "
			      "repeated over 5 times",
		.test_main_f = main_ext_conn_adv_advertiser_x5
	},
	{
		.test_id = "ext_adv_limited_advertiser",
		.test_descr = "Limited extended advertising test. "
			      "The sent callback reports a reached limit, not a connection",
		.test_main_f = main_ext_adv_limited_advertiser
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_ext_adv_advertiser(struct bst_test_list *tests)
{
	return bst_add_tests(tests, ext_adv_advertiser);
}

bst_test_install_t test_installers[] = {
	test_ext_adv_advertiser,
	NULL
};

int main(void)
{
	bst_main();
	return 0;
}
