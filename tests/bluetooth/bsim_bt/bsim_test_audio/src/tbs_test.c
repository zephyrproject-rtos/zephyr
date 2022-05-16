/*
 * Copyright (c) 2019 Bose Corporation
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_BT_TBS
#include <zephyr/bluetooth/audio/tbs.h>
#include "common.h"

extern enum bst_result_t bst_result;
static volatile bool is_connected;
static volatile bool call_placed;
static volatile bool call_held;
static volatile bool call_id;

static void tbs_hold_call_cb(struct bt_conn *conn, uint8_t call_index)
{
	if (call_index == call_id) {
		call_held = true;
	}
}

static bool tbs_originate_call_cb(struct bt_conn *conn, uint8_t call_index,
				  const char *caller_id)
{
	printk("Placing call to remote with id %u to %s\n",
	       call_index, caller_id);
	call_id = call_index;
	call_placed = true;
	return true;
}

static bool tbs_authorize_cb(struct bt_conn *conn)
{
	return conn == default_conn;
}

static struct bt_tbs_cb tbs_cbs = {
	.originate_call = tbs_originate_call_cb,
	.terminate_call = NULL,
	.hold_call = tbs_hold_call_cb,
	.accept_call = NULL,
	.retrieve_call = NULL,
	.join_calls = NULL,
	.authorize = tbs_authorize_cb,
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err != 0) {
		FAIL("Failed to connect to %s (%u)\n", addr, err);
		return;
	}

	printk("Connected to %s\n", addr);

	default_conn = bt_conn_ref(conn);
	is_connected = true;
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

static void test_main(void)
{
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Audio Client: Bluetooth initialized\n");

	bt_conn_cb_register(&conn_callbacks);
	bt_tbs_register_cb(&tbs_cbs);

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err != 0) {
		FAIL("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning successfully started\n");

	WAIT_FOR_COND(is_connected);

	WAIT_FOR_COND(call_placed);

	err = bt_tbs_remote_answer(call_id);
	if (err != BT_TBS_RESULT_CODE_SUCCESS) {
		FAIL("Remote could not answer call: %d\n", err);
		return;
	}
	printk("Remote answered %u\n", call_id);

	err = bt_tbs_remote_hold(call_id);
	if (err != BT_TBS_RESULT_CODE_SUCCESS) {
		FAIL("Remote could not hold call: %d\n", err);
	}
	printk("Remote held %u\n", call_id);

	WAIT_FOR_COND(call_held);

	err = bt_tbs_remote_retrieve(call_id);
	if (err != BT_TBS_RESULT_CODE_SUCCESS) {
		FAIL("Remote could not answer call: %d\n", err);
		return;
	}
	printk("Remote retrieved %u\n", call_id);

	PASS("TBS passed\n");
}

static const struct bst_test_instance test_tbs[] = {
	{
		.test_id = "tbs",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_tbs_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_tbs);
}
#else
struct bst_test_list *test_tbs_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_TBS */
