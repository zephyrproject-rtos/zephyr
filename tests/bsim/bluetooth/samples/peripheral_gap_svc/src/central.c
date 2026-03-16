/*
 * Copyright (c) 2026 Koppel Electronic
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include <zephyr/logging/log.h>

#include "testlib/att.h"
#include "testlib/att_read.h"
#include "testlib/att_write.h"
#include "testlib/conn.h"

#include "babblekit/testcase.h"

LOG_MODULE_REGISTER(central, LOG_LEVEL_DBG);

/* Wait time in microseconds for the test to be finished */
#define WAIT_TIME 10e6

static struct bt_conn *default_conn;
static struct bt_conn *connected_conn;
struct k_sem connected_sem;

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char dev[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(addr, dev, sizeof(dev));
	printk("[DEVICE]: %s, AD evt type %u, AD data len %u, RSSI %i\n",
	       dev, type, ad->len, rssi);

	/* We're only interested in connectable events */
	if (type == BT_GAP_ADV_TYPE_ADV_IND ||
	    type == BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
		int err = bt_le_scan_stop();

		if (err != 0) {
			TEST_FAIL("Stop LE scan failed (err %d)", err);
			return;
		}

		err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
					BT_LE_CONN_PARAM_DEFAULT, &default_conn);
		if (err != 0) {
			TEST_FAIL("Create conn failed (err %d)", err);
		}
	}
}

static void start_scan(void)
{
	int err;

	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, device_found);
	if (err != 0) {
		printk("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning successfully started\n");
}


static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err != BT_HCI_ERR_SUCCESS) {
		printk("Failed to connect to %s (%u)\n", addr, conn_err);

		bt_conn_unref(default_conn);
		default_conn = NULL;

		TEST_FAIL("Failed to connect to %s (%u)", addr, conn_err);
		return;
	}

	printk("Connected: %s\n", addr);

	if (conn == default_conn) {
		connected_conn = bt_conn_ref(conn);
		k_sem_give(&connected_sem);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s, reason 0x%02x %s\n", addr, reason, bt_hci_err_to_str(reason));

	if (default_conn != conn) {
		return;
	}

	struct bt_conn *conn_to_unref;

	conn_to_unref = connected_conn;
	connected_conn = NULL;
	bt_conn_unref(conn_to_unref);

	conn_to_unref = default_conn;
	default_conn = NULL;
	bt_conn_unref(conn_to_unref);

	start_scan();
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

/**
 * @brief Test GAP Device Name characteristic write with invalid data
 *
 * This test attempts to write name that expects to be rejected by the server.
 */
static void test_gap_name_write_invalid(uint16_t chrc_handle, struct bt_conn *conn)
{
	int err;
	char invalid_name[] = "smalLetter";

	/* Write new Device name */
	err = bt_testlib_att_write(conn,
				   BT_ATT_CHAN_OPT_UNENHANCED_ONLY,
				   chrc_handle,
				   invalid_name,
				   sizeof(invalid_name));
	TEST_ASSERT(err == BT_ATT_ERR_VALUE_NOT_ALLOWED,
		    "Expected ATT error %d, got: %d",
		    BT_ATT_ERR_VALUE_NOT_ALLOWED, err);

}

/**
 * @brief Test GAP Device Name characteristic write with valid data
 *
 * This test attempts to write name that expects to be accepted by the server.
 */
static void test_gap_name_write_valid(uint16_t chrc_handle, struct bt_conn *conn)
{
	int err;
	char valid_name[] = "ValidName";

	NET_BUF_SIMPLE_DEFINE(attr_value_buf, BT_ATT_MAX_ATTRIBUTE_LEN);

	/* Write new Device name */
	err = bt_testlib_att_write(conn,
				   BT_ATT_CHAN_OPT_UNENHANCED_ONLY,
				   chrc_handle,
				   valid_name,
				   sizeof(valid_name));
	TEST_ASSERT(err == BT_ATT_ERR_SUCCESS, "Got ATT error: %d", err);

	/* Verify new Device name */
	err = bt_testlib_att_read_by_handle_sync(&attr_value_buf, NULL, NULL, conn,
						 BT_ATT_CHAN_OPT_UNENHANCED_ONLY, chrc_handle, 0);
	TEST_ASSERT(err == 0, "Failed to read characteristic (err %d)", err);

	TEST_ASSERT(attr_value_buf.len == strlen(valid_name),
		    "Unexpected Device Name length: %u (!=%u)",
		    attr_value_buf.len, strlen(valid_name));
	TEST_ASSERT(memcmp(attr_value_buf.data, valid_name, attr_value_buf.len) == 0,
		    "Unexpected Device Name value: %.*s",
		    attr_value_buf.len, attr_value_buf.data);

	net_buf_simple_reset(&attr_value_buf);
}


static void test_gap_name(struct bt_conn *conn)
{
	int err;
	uint16_t chrc_handle;

	NET_BUF_SIMPLE_DEFINE(attr_value_buf, BT_ATT_MAX_ATTRIBUTE_LEN);

	err = bt_testlib_gatt_discover_characteristic(&chrc_handle,
						      NULL, NULL, conn,
						      BT_UUID_GAP_DEVICE_NAME,
						      BT_ATT_FIRST_ATTRIBUTE_HANDLE,
						      BT_ATT_LAST_ATTRIBUTE_HANDLE);
	TEST_ASSERT(err == 0, "Device Name characteristic not found (err %d)", err);

	LOG_DBG("Device Name characteristic found at handle %u", chrc_handle);

	/* Read Device name */
	err = bt_testlib_att_read_by_handle_sync(&attr_value_buf, NULL, NULL, conn,
						BT_ATT_CHAN_OPT_UNENHANCED_ONLY, chrc_handle, 0);
	TEST_ASSERT(err == 0, "Failed to read characteristic (err %d)", err);

	LOG_DBG("Device Name of the server: %.*s", attr_value_buf.len, attr_value_buf.data);

	net_buf_simple_reset(&attr_value_buf);

	test_gap_name_write_invalid(chrc_handle, conn);
	test_gap_name_write_valid(chrc_handle, conn);
}

static void test_local_gap_svc_central_main(void)
{
	int err;
	struct bt_conn *conn;

	k_sem_init(&connected_sem, 0, 1);
	bt_conn_cb_register(&conn_callbacks);

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Cannot enable Bluetooth (err %d)", err);

	LOG_INF("Bluetooth initialized");

	start_scan();

	/* Wait for connection */
	k_sem_take(&connected_sem, K_FOREVER);

	conn = bt_conn_ref(connected_conn);

	err = bt_testlib_att_exchange_mtu(conn);
	TEST_ASSERT(err == 0, "Failed to update MTU (err %d)", err);

	test_gap_name(conn);

	bt_conn_unref(conn);

	TEST_PASS("client");
}

static void test_local_gap_svc_central_init(void)
{
	bst_ticker_set_next_tick_absolute(WAIT_TIME);
	TEST_START("test_local_gap_svc_central");
}

static void test_local_gap_svc_central_tick(bs_time_t HW_device_time)
{
	/*
	 * If in WAIT_TIME seconds the testcase did not already pass
	 * (and finish) we consider it failed
	 */
	if (bst_result != Passed) {
		TEST_FAIL("test_local_gap_svc_central failed (not passed after %d seconds)",
			  (int)(WAIT_TIME / 1e6));
	}
}

static const struct bst_test_instance test_central[] = {
	{
		.test_id = "central",
		.test_descr = "peripheral_gap_svc sample - central role tester.",
		.test_main_f = test_local_gap_svc_central_main,
		.test_pre_init_f = test_local_gap_svc_central_init,
		.test_tick_f = test_local_gap_svc_central_tick,
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_local_gap_svc_central_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_central);
	return tests;
}
