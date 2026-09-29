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
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/services/nus.h>

#include "testlib/att_read.h"
#include "testlib/conn.h"
#include "testlib/scan.h"

#include "babblekit/testcase.h"
#include "babblekit/flags.h"
#include "babblekit/sync.h"
#include "common.h"

DEFINE_FLAG_STATIC(flag_subscribed);

static struct bt_conn *g_conn;
static struct bt_gatt_subscribe_params subscribe_params;

#define RX_BUF_SIZE (TX_FIFO_SIZE + sizeof(SUBSCRIBED_MSG))

static char rx_buf[RX_BUF_SIZE];
static atomic_t rx_len;

static uint8_t notify_cb(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
			 const void *data, uint16_t length)
{
	size_t len = (size_t)atomic_get(&rx_len);

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
	static struct bt_gatt_discover_params ccc_discover_params;
	bt_addr_le_t dut_addr;
	uint16_t tx_chrc_value_handle;
	int err;

	UNSET_FLAG(flag_subscribed);

	err = bt_testlib_scan_find_name(&dut_addr, CONFIG_BT_DEVICE_NAME);
	TEST_ASSERT(err == 0, "Failed to find the DUT (err %d)", err);

	err = bt_testlib_connect(&dut_addr, &g_conn);
	TEST_ASSERT(err == 0, "Failed to connect to the DUT (err %d)", err);

	err = bt_testlib_gatt_discover_characteristic(&tx_chrc_value_handle, NULL, NULL, g_conn,
						      BT_UUID_DECLARE_128(BT_UUID_NUS_TX_CHAR_VAL),
						      BT_ATT_FIRST_ATTRIBUTE_HANDLE,
						      BT_ATT_LAST_ATTRIBUTE_HANDLE);
	TEST_ASSERT(err == 0, "NUS TX characteristic not found (err %d)", err);

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

	while ((size_t)atomic_get(&rx_len) < expected_len) {
		(void)k_sleep(K_MSEC(10));
	}

	/* Leave room for anything the DUT should not have sent. */
	(void)k_sleep(K_MSEC(500));

	TEST_ASSERT((size_t)atomic_get(&rx_len) == expected_len &&
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
	char expected[RX_BUF_SIZE];

	(void)memcpy(expected, UNSUBSCRIBED_MSG, TX_FIFO_SIZE);
	(void)memcpy(&expected[TX_FIFO_SIZE], SUBSCRIBED_MSG, sizeof(SUBSCRIBED_MSG));

	peer_init();
	connect_and_subscribe();
	bk_sync_send();

	expect_rx(expected);
	bk_sync_send();

	TEST_PASS("Peer passed");
}

static const struct bst_test_instance test_peer[] = {
	{
		.test_id = "uart_bt_peer",
		.test_main_f = test_main,
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_peer_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_peer);
}
