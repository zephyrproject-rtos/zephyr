/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>

#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/services/nus.h>

#include "babblekit/testcase.h"
#include "babblekit/flags.h"
#include "babblekit/sync.h"
#include "common.h"

DEFINE_FLAG_STATIC(flag_connected);
DEFINE_FLAG_STATIC(flag_discovered);
DEFINE_FLAG_STATIC(flag_subscribed);

static struct bt_conn *g_conn;
static uint16_t tx_chrc_value_handle;

static uint8_t rx_buf[128];
static atomic_t rx_len;

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err != 0) {
		TEST_FAIL("Failed to connect to %s (%u)", bt_conn_dst_str(conn), err);
		return;
	}

	__ASSERT_NO_MSG(g_conn == conn);

	SET_FLAG(flag_connected);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	if (conn != g_conn) {
		return;
	}

	bt_conn_drop(&g_conn);
	UNSET_FLAG(flag_connected);
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	int err;

	if (g_conn != NULL) {
		return;
	}

	if (type != BT_HCI_ADV_IND && type != BT_HCI_ADV_DIRECT_IND) {
		return;
	}

	err = bt_le_scan_stop();
	if (err != 0) {
		TEST_FAIL("Could not stop scan: %d", err);
		return;
	}

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
				BT_LE_CONN_PARAM_DEFAULT, &g_conn);
	if (err != 0) {
		TEST_FAIL("Could not connect to peer: %d", err);
	}
}

static uint8_t chrc_discover_func(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr,
				  struct bt_gatt_discover_params *params)
{
	const struct bt_gatt_chrc *chrc;

	if (attr == NULL) {
		(void)memset(params, 0, sizeof(*params));
		SET_FLAG(flag_discovered);

		return BT_GATT_ITER_STOP;
	}

	chrc = attr->user_data;
	tx_chrc_value_handle = chrc->value_handle;

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t notify_cb(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
			 const void *data, uint16_t length)
{
	size_t len = atomic_get(&rx_len);

	if (data == NULL) {
		return BT_GATT_ITER_STOP;
	}

	TEST_ASSERT(len + length <= sizeof(rx_buf), "Received %zu bytes, more than expected",
		    len + length);

	(void)memcpy(&rx_buf[len], data, length);
	(void)atomic_add(&rx_len, length);

	return BT_GATT_ITER_CONTINUE;
}

static void subscribed_cb(struct bt_conn *conn, uint8_t err,
			  struct bt_gatt_subscribe_params *params)
{
	TEST_ASSERT(err == 0, "Subscribe failed (err %u)", err);

	SET_FLAG(flag_subscribed);
}

static void connect_and_subscribe(void)
{
	static struct bt_gatt_discover_params discover_params;
	static struct bt_gatt_discover_params ccc_discover_params;
	static struct bt_gatt_subscribe_params subscribe_params;
	int err;

	UNSET_FLAG(flag_discovered);
	UNSET_FLAG(flag_subscribed);
	tx_chrc_value_handle = 0;

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	TEST_ASSERT(err == 0, "Scanning failed to start (err %d)", err);

	WAIT_FOR_FLAG(flag_connected);

	discover_params.uuid = BT_UUID_DECLARE_128(BT_UUID_NUS_TX_CHAR_VAL);
	discover_params.func = chrc_discover_func;
	discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

	err = bt_gatt_discover(g_conn, &discover_params);
	TEST_ASSERT(err == 0, "Discover failed (err %d)", err);

	WAIT_FOR_FLAG(flag_discovered);
	TEST_ASSERT(tx_chrc_value_handle != 0, "NUS TX characteristic not found");

	(void)memset(&subscribe_params, 0, sizeof(subscribe_params));
	subscribe_params.value_handle = tx_chrc_value_handle;
	subscribe_params.ccc_handle = BT_GATT_AUTO_DISCOVER_CCC_HANDLE;
	subscribe_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	subscribe_params.disc_params = &ccc_discover_params;
	subscribe_params.value = BT_GATT_CCC_NOTIFY;
	subscribe_params.notify = notify_cb;
	subscribe_params.subscribe = subscribed_cb;

	err = bt_gatt_subscribe(g_conn, &subscribe_params);
	TEST_ASSERT(err == 0, "Subscribe failed (err %d)", err);

	WAIT_FOR_FLAG(flag_subscribed);
}

static void expect_rx(const char *msg)
{
	const size_t expected_len = strlen(msg);

	while (atomic_get(&rx_len) < expected_len) {
		k_sleep(K_MSEC(10));
	}

	/* Leave room for anything the DUT should not have sent. */
	k_sleep(K_MSEC(500));

	TEST_ASSERT(atomic_get(&rx_len) == expected_len &&
		    memcmp(rx_buf, msg, expected_len) == 0,
		    "Received \"%.*s\", expected \"%s\"",
		    (int)atomic_get(&rx_len), rx_buf, msg);

	(void)atomic_set(&rx_len, 0);
}

static void peer_init(void)
{
	int err;

	TEST_ASSERT(bk_sync_init() == 0, "Failed to open backchannel");

	bt_conn_cb_register(&conn_callbacks);

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);
}

static void test_main(void)
{
	peer_init();
	connect_and_subscribe();
	bk_sync_send();

	expect_rx(SUBSCRIBED_MSG);
	bk_sync_send();

	TEST_PASS("Peer passed");
}

static void test_reconnect_main(void)
{
	int err;

	peer_init();
	connect_and_subscribe();
	bk_sync_send();

	expect_rx(FIRST_SESSION_MSG);
	bk_sync_send();

	/* The DUT holds HELD_MSG in its TX FIFO. */
	bk_sync_wait();

	err = bt_conn_disconnect(g_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	TEST_ASSERT(err == 0, "Disconnect failed (err %d)", err);

	WAIT_FOR_FLAG_UNSET(flag_connected);

	connect_and_subscribe();
	bk_sync_send();

	expect_rx(SECOND_SESSION_MSG);
	bk_sync_send();

	TEST_PASS("Peer passed");
}

static const struct bst_test_instance test_peer[] = {
	{
		.test_id = "uart_bt_peer",
		.test_main_f = test_main,
	},
	{
		.test_id = "uart_bt_reconnect_peer",
		.test_main_f = test_reconnect_main,
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_peer_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_peer);
}
