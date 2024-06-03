/*
 * Copyright (c) 2024 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bstests.h"
#include "babblekit/testcase.h"
#include <zephyr/bluetooth/conn.h>

static K_SEM_DEFINE(sem_connected, 0, 1);

static void connected_cb(struct bt_conn *conn, uint8_t err)
{
	TEST_ASSERT(conn);
	TEST_ASSERT(err == BT_HCI_ERR_UNKNOWN_CONN_ID, "Expected connection timeout");

	k_sem_give(&sem_connected);
	bt_conn_unref(conn);
}

static struct bt_conn_cb conn_cb = {
	.connected = connected_cb,
};

static void test_central_connect_timeout_with_timeout(uint32_t timeout_ms)
{
	int err;
	struct bt_conn *conn;

	/* A zero value for `bt_conn_le_create_param.timeout` shall be
	 * interpreted as `CONFIG_BT_CREATE_CONN_TIMEOUT`.
	 */
	uint32_t expected_conn_timeout_ms =
		timeout_ms ? timeout_ms : CONFIG_BT_CREATE_CONN_TIMEOUT * MSEC_PER_SEC;

	bt_addr_le_t peer = {.a.val = {0x01}};
	const struct bt_conn_le_create_param create_param = {
		.options = BT_CONN_LE_OPT_NONE,
		.interval = BT_GAP_SCAN_FAST_INTERVAL,
		.window = BT_GAP_SCAN_FAST_WINDOW,
		.interval_coded = 0,
		.window_coded = 0,
		.timeout = timeout_ms / 10,
	};

	k_sem_reset(&sem_connected);

	const uint64_t conn_create_start = k_uptime_get();

	err = bt_conn_le_create(&peer, &create_param, BT_LE_CONN_PARAM_DEFAULT, &conn);
	TEST_ASSERT(err == 0, "Failed starting initiator (err %d)", err);

	err = k_sem_take(&sem_connected, K_MSEC(2 * expected_conn_timeout_ms));
	TEST_ASSERT(err == 0, "Failed getting connected timeout within %d s (err %d)",
		    2 * expected_conn_timeout_ms, err);

	const uint64_t conn_create_end = k_uptime_get();

	const int64_t time_diff_ms = conn_create_end - conn_create_start;
	const int64_t diff_to_expected_ms = abs(time_diff_ms - expected_conn_timeout_ms);

	TEST_PRINT("Connection timeout after %d ms", time_diff_ms);
	TEST_ASSERT(diff_to_expected_ms < 0.1 * expected_conn_timeout_ms,
		    "Connection timeout not within 10%% of expected timeout. "
		    "Actual timeout: %d", time_diff_ms);
}

static void test_central_connect_timeout(void)
{
	int err;

	bt_conn_cb_register(&conn_cb);

	/* Initialize Bluetooth */
	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Can't enable Bluetooth (err %d)", err);

	test_central_connect_timeout_with_timeout(0);
	test_central_connect_timeout_with_timeout(1000);

	TEST_PASS("Correct timeout");
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "central_connect_timeout",
		.test_descr = "Verifies that the default connection timeout is used correctly",
		.test_tick_f = bst_tick,
		.test_main_f = test_central_connect_timeout
	},
	BSTEST_END_MARKER
};

static struct bst_test_list *test_central_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}

bst_test_install_t test_installers[] = {
	test_central_install,
	NULL
};

int main(void)
{
	bst_main();
	return 0;
}
