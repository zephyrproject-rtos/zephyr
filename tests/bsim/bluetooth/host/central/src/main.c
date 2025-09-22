/*
 * Copyright (c) 2024 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bstests.h"
#include "babblekit/testcase.h"
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>

/* Include hci_common_internal for the purpose of checking HCI Command counts. */
#include "common/hci_common_internal.h"

/* Include conn_internal for the purpose of checking reference counts. */
#include "host/conn_internal.h"

struct bst_test_list *test_peripheral_install(struct bst_test_list *tests);

static K_SEM_DEFINE(sem_failed_to_connect, 0, 1);
static K_SEM_DEFINE(sem_connected, 0, 1);

static void connected_cb_expect_fail(struct bt_conn *conn, uint8_t err)
{
	TEST_ASSERT(conn);
	TEST_ASSERT(err == BT_HCI_ERR_UNKNOWN_CONN_ID, "Expected connection timeout");

	k_sem_give(&sem_failed_to_connect);
	bt_conn_unref(conn);
}

static void connected_cb(struct bt_conn *conn, uint8_t err)
{
	TEST_ASSERT(conn);
	TEST_ASSERT(err == BT_HCI_ERR_SUCCESS, "Expected connection establishment");

	k_sem_give(&sem_connected);
	bt_conn_unref(conn);
}

static struct bt_conn_cb conn_cb_expect_fail = {
	.connected = connected_cb_expect_fail,
};

static struct bt_conn_cb conn_cb = {
	.connected = connected_cb,
};

static void test_central_connect_timeout_with_timeout(uint32_t timeout_ms, bool stack_load)
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
	struct net_buf *bufs[BT_BUF_CMD_TX_COUNT];

	k_sem_reset(&sem_failed_to_connect);

	const uint64_t conn_create_start = k_uptime_get();

	err = bt_conn_le_create(&peer, &create_param, BT_LE_CONN_PARAM_DEFAULT, &conn);
	TEST_ASSERT(err == 0, "Failed starting initiator (err %d)", err);

	if (stack_load) {
		/* Claim all the buffers so that the stack cannot handle the timeout */
		for (int i = 0; i < BT_BUF_CMD_TX_COUNT; i++) {
			bufs[i] = bt_hci_cmd_create(BT_HCI_LE_ADV_ENABLE, 0);
			TEST_ASSERT(bufs[i] != NULL, "Failed to claim all command buffers");
		}
		/* Hold all the buffers until after we expect the connection to timeout */
		err = k_sem_take(&sem_failed_to_connect, K_MSEC(expected_conn_timeout_ms + 50));
		TEST_ASSERT(err == -EAGAIN, "Callback ran with no buffers available", err);
		/* Release all the buffers back to the stack */
		for (int i = 0; i < BT_BUF_CMD_TX_COUNT; i++) {
			net_buf_unref(bufs[i]);
		}
	}

	err = k_sem_take(&sem_failed_to_connect, K_MSEC(2 * expected_conn_timeout_ms));
	TEST_ASSERT(err == 0, "Failed getting connected timeout within %d s (err %d)",
		    2 * expected_conn_timeout_ms, err);

	const uint64_t conn_create_end = k_uptime_get();

	const int64_t time_diff_ms = conn_create_end - conn_create_start;
	const int64_t diff_to_expected_ms = abs(time_diff_ms - expected_conn_timeout_ms);

	TEST_PRINT("Connection timeout after %d ms", time_diff_ms);
	TEST_ASSERT(diff_to_expected_ms < 0.1 * expected_conn_timeout_ms,
		    "Connection timeout not within 10%% of expected timeout. "
		    "Actual timeout: %d",
		    time_diff_ms);
}

static void test_central_connect_timeout(void)
{
	int err;

	bt_conn_cb_register(&conn_cb_expect_fail);

	/* Initialize Bluetooth */
	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Can't enable Bluetooth (err %d)", err);

	test_central_connect_timeout_with_timeout(0, false);
	test_central_connect_timeout_with_timeout(1000, false);
	test_central_connect_timeout_with_timeout(2000, true);

	TEST_PASS("Correct timeout");
}

static void test_central_connect_when_connecting(void)
{
	int err;

	bt_conn_cb_register(&conn_cb_expect_fail);

	/* Initialize Bluetooth */
	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Can't enable Bluetooth (err %d)", err);

	struct bt_conn *conn;

	bt_addr_le_t peer = {.a.val = {0x01}};

	const struct bt_conn_le_create_param create_param = {
		.options = BT_CONN_LE_OPT_NONE,
		.interval = BT_GAP_SCAN_FAST_INTERVAL,
		.window = BT_GAP_SCAN_FAST_WINDOW,
	};

	k_sem_reset(&sem_failed_to_connect);

	err = bt_conn_le_create(&peer, &create_param, BT_LE_CONN_PARAM_DEFAULT, &conn);
	TEST_ASSERT(err == 0, "Failed starting initiator (err %d)", err);

	/* Now we have a valid connection reference */
	atomic_val_t initial_refs = atomic_get(&conn->ref);

	TEST_ASSERT(initial_refs >= 1, "Expect to have at least once reference");

	err = bt_conn_le_create(&peer, &create_param, BT_LE_CONN_PARAM_DEFAULT, &conn);
	TEST_ASSERT(err == -EALREADY, "Expected to fail to create connection (err %d)", err);

	/* Expect the number of refs to be unchanged. */
	TEST_ASSERT(atomic_get(&conn->ref) == initial_refs,
		    "Expect number of references to be unchanged");

	err = k_sem_take(&sem_failed_to_connect, K_FOREVER);
	TEST_ASSERT(err == 0, "Failed getting connected timeout", err);

	TEST_ASSERT(atomic_get(&conn->ref) == 0, "Expect no more references");

	TEST_PASS("Passed");
}

static void test_central_connect_to_existing(void)
{
	int err;

	bt_conn_cb_register(&conn_cb);

	/* Initialize Bluetooth */
	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Can't enable Bluetooth (err %d)", err);

	struct bt_conn *conn;

	bt_addr_le_t peer = {.type = BT_ADDR_LE_RANDOM,
			     .a.val = {0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0}};

	const struct bt_conn_le_create_param create_param = {
		.options = BT_CONN_LE_OPT_NONE,
		.interval = BT_GAP_SCAN_FAST_INTERVAL,
		.window = BT_GAP_SCAN_FAST_WINDOW,
	};

	k_sem_reset(&sem_connected);

	err = bt_conn_le_create(&peer, &create_param, BT_LE_CONN_PARAM_DEFAULT, &conn);
	TEST_ASSERT(err == 0, "Failed starting initiator (err %d)", err);

	err = k_sem_take(&sem_connected, K_FOREVER);
	TEST_ASSERT(err == 0, "Failed establishing connection", err);

	/* Now we have a valid connection reference */
	atomic_val_t initial_refs = atomic_get(&conn->ref);

	TEST_ASSERT(initial_refs >= 1, "Expect to have at least once reference");

	err = bt_conn_le_create(&peer, &create_param, BT_LE_CONN_PARAM_DEFAULT, &conn);
	TEST_ASSERT(err == -EINVAL, "Expected to fail to create a connection (err %d)", err);

	/* Expect the number of refs to be unchanged. */
	TEST_ASSERT(atomic_get(&conn->ref) == initial_refs,
		    "Expect number of references to be unchanged");

	TEST_PASS("Passed");
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "central_connect_timeout",
		.test_descr = "Verifies that the default connection timeout is used correctly",
		.test_main_f = test_central_connect_timeout,
	},
	{
		.test_id = "central_connect_when_connecting",
		.test_descr = "Verifies that the stack returns an error code when trying to connect"
			      " while already connecting",
		.test_main_f = test_central_connect_when_connecting,
	},
	{
		.test_id = "central_connect_to_existing",
		.test_descr =
			"Verifies that the stack returns an error code when trying to connect"
			" to an existing device and does not unref the existing connection object.",
		.test_main_f = test_central_connect_to_existing,
	},
	BSTEST_END_MARKER,
};

static struct bst_test_list *test_central_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}

bst_test_install_t test_installers[] = {test_central_install, test_peripheral_install, NULL};

int main(void)
{
	bst_main();
	return 0;
}
