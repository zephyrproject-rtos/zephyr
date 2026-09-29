/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>

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
static struct bt_gatt_subscribe_params subscribe_params;

static uint8_t rx_buf[128];
static atomic_t rx_len;

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (conn != g_conn) {
		return;
	}

	if (err != 0) {
		TEST_FAIL("Failed to connect to %s (%u)", bt_conn_dst_str(conn), err);
		return;
	}

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

BT_CONN_CB_DEFINE(conn_callbacks) = {
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

static void connect_and_subscribe(bt_gatt_notify_func_t notify)
{
	static struct bt_gatt_discover_params discover_params;
	static struct bt_gatt_discover_params ccc_discover_params;
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
	subscribe_params.notify = notify;
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

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);
}

static void test_main(void)
{
	char expected[TX_FIFO_SIZE + sizeof(SUBSCRIBED_MSG)];

	(void)memcpy(expected, UNSUBSCRIBED_MSG, TX_FIFO_SIZE);
	(void)memcpy(&expected[TX_FIFO_SIZE], SUBSCRIBED_MSG, sizeof(SUBSCRIBED_MSG));

	peer_init();
	connect_and_subscribe(notify_cb);
	bk_sync_send();

	expect_rx(expected);
	bk_sync_send();

	TEST_PASS("Peer passed");
}

static void test_reconnect_main(void)
{
	int err;

	peer_init();
	connect_and_subscribe(notify_cb);
	bk_sync_send();

	expect_rx(FIRST_SESSION_MSG);
	bk_sync_send();

	/* The DUT holds HELD_MSG in its TX FIFO. */
	bk_sync_wait();

	err = bt_conn_disconnect(g_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	TEST_ASSERT(err == 0, "Disconnect failed (err %d)", err);

	WAIT_FOR_FLAG_UNSET(flag_connected);

	connect_and_subscribe(notify_cb);
	bk_sync_send();

	expect_rx(DISCONNECTED_MSG SECOND_SESSION_MSG);
	bk_sync_send();

	TEST_PASS("Peer passed");
}

static K_SEM_DEFINE(rx_release_sem, 0, 1);
static atomic_t rx_resubscribed_len;

static uint8_t notify_hold_cb(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
			      const void *data, uint16_t length)
{
	static bool held;
	const uint8_t *bytes = data;

	if (data == NULL) {
		return BT_GATT_ITER_STOP;
	}

	for (uint16_t i = 0; i < length; i++) {
		if (bytes[i] == RESUBSCRIBED_BYTE) {
			(void)atomic_inc(&rx_resubscribed_len);
		} else {
			TEST_ASSERT(bytes[i] == STALLED_BYTE &&
					    atomic_get(&rx_resubscribed_len) == 0,
				    "Received 0x%02x after %ld resubscribed bytes", bytes[i],
				    atomic_get(&rx_resubscribed_len));
		}
	}

	/* Blocking here stops this host from reading, so the DUT runs out of
	 * buffers and its next notification waits for one.
	 */
	if (!held) {
		held = true;
		k_sem_take(&rx_release_sem, K_FOREVER);
	}

	return BT_GATT_ITER_CONTINUE;
}

static void write_ccc(uint16_t value)
{
	uint8_t buf[sizeof(value)];
	int err;

	sys_put_le16(value, buf);

	err = bt_gatt_write_without_response(g_conn, subscribe_params.ccc_handle, buf, sizeof(buf),
					     false);
	TEST_ASSERT(err == 0, "CCC write failed (err %d)", err);
}

static void test_resubscribe_main(void)
{
	peer_init();
	connect_and_subscribe(notify_hold_cb);
	bk_sync_send();

	/* The DUT's TX FIFO is full and stays full. */
	bk_sync_wait();

	/* Write the CCC directly, so this host keeps delivering notifications
	 * to notify_hold_cb() in between.
	 */
	write_ccc(0);
	write_ccc(BT_GATT_CCC_NOTIFY);
	k_sleep(K_MSEC(500));
	bk_sync_send();

	/* The DUT has written the new output. */
	bk_sync_wait();

	k_sem_give(&rx_release_sem);

	while (atomic_get(&rx_resubscribed_len) < RESUBSCRIBED_LEN) {
		k_sleep(K_MSEC(10));
	}

	/* Leave room for anything the DUT should not have sent. */
	k_sleep(K_MSEC(500));

	TEST_ASSERT(atomic_get(&rx_resubscribed_len) == RESUBSCRIBED_LEN,
		    "Received %ld resubscribed bytes, expected %d",
		    atomic_get(&rx_resubscribed_len), RESUBSCRIBED_LEN);

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
	{
		.test_id = "uart_bt_resubscribe_peer",
		.test_main_f = test_resubscribe_main,
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_peer_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_peer);
}
