/*
 * Copyright 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/services/hogp_host.h>

#include "babblekit/flags.h"
#include "babblekit/sync.h"
#include "babblekit/testcase.h"
#include "bstests.h"

#include "common.h"

DEFINE_FLAG(flag_connected);
DEFINE_FLAG(flag_hogp_ready);
DEFINE_FLAG(flag_report_map_received);
DEFINE_FLAG(flag_input_report_received);
DEFINE_FLAG(flag_hid_info_received);
DEFINE_FLAG(flag_pnp_id_received);
DEFINE_FLAG(flag_battery_received);
DEFINE_FLAG(flag_disconnected);
DEFINE_FLAG(flag_get_report_done);

static struct bt_conn *host_conn;
static uint8_t received_report_data[32];
static uint16_t received_report_len;

static void hogp_connected_cb(struct bt_conn *conn, int status,
			       uint8_t num_instances, uint8_t protocol_mode)
{
	TEST_ASSERT(!status, "HOGP connect status: %d", status);
	TEST_ASSERT(num_instances >= 1, "No HID instances found");
	SET_FLAG(flag_hogp_ready);
}

static void hogp_disconnected_cb(struct bt_conn *conn, int reason)
{
	SET_FLAG(flag_disconnected);
}

static void hogp_report_map_cb(struct bt_conn *conn, uint8_t service_index,
				const uint8_t *data, uint16_t len)
{
	/* TC2: Verify Report Map matches */
	TEST_ASSERT(len == sizeof(test_report_map),
		    "Report map length mismatch: %u vs %u",
		    len, (unsigned int)sizeof(test_report_map));
	TEST_ASSERT(!memcmp(data, test_report_map, len),
		    "Report map data mismatch");
	SET_FLAG(flag_report_map_received);
}

static void hogp_input_report_cb(struct bt_conn *conn, uint8_t service_index,
				  uint8_t report_id, const uint8_t *data,
				  uint16_t len)
{
	/* TC3: Verify Input Report data */
	TEST_ASSERT(report_id == TEST_REPORT_ID_INPUT,
		    "Wrong report ID: %u", report_id);
	TEST_ASSERT(len == TEST_INPUT_REPORT_LEN,
		    "Wrong report len: %u", len);
	if (len <= sizeof(received_report_data)) {
		memcpy(received_report_data, data, len);
		received_report_len = len;
	}
	SET_FLAG(flag_input_report_received);
}

static void hogp_get_report_result_cb(struct bt_conn *conn,
				       uint8_t report_id,
				       uint8_t report_type,
				       const uint8_t *data, uint16_t len,
				       int status)
{
	/* TC4: Get Report result */
	SET_FLAG(flag_get_report_done);
}

static void hogp_pnp_id_cb(struct bt_conn *conn,
			     const struct bt_hogp_host_pnp_id *id)
{
	TEST_ASSERT(id->vid == 0x2717, "Wrong VID: 0x%04x", id->vid);
	SET_FLAG(flag_pnp_id_received);
}

static void hogp_hid_info_cb(struct bt_conn *conn, uint8_t service_index,
			      uint16_t bcd_hid, uint8_t b_country_code,
			      uint8_t flags)
{
	TEST_ASSERT(bcd_hid == TEST_BCD_HID,
		    "Wrong bcdHID: 0x%04x", bcd_hid);
	TEST_ASSERT(b_country_code == TEST_COUNTRY_CODE,
		    "Wrong country: %u", b_country_code);
	TEST_ASSERT(flags == TEST_HID_FLAGS,
		    "Wrong flags: 0x%02x", flags);
	SET_FLAG(flag_hid_info_received);
}

static void hogp_battery_cb(struct bt_conn *conn, uint8_t bat_index,
			     uint8_t level)
{
	SET_FLAG(flag_battery_received);
}

static const struct bt_hogp_host_cb hogp_cb = {
	.connected = hogp_connected_cb,
	.disconnected = hogp_disconnected_cb,
	.report_map = hogp_report_map_cb,
	.input_report = hogp_input_report_cb,
	.get_report_result = hogp_get_report_result_cb,
	.pnp_id = hogp_pnp_id_cb,
	.hid_info = hogp_hid_info_cb,
	.battery_level = hogp_battery_cb,
};

static void scan_cb(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
		    struct net_buf_simple *ad)
{
	int err;

	if (host_conn) {
		return;
	}

	err = bt_le_scan_stop();
	if (err) {
		return;
	}

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
				BT_LE_CONN_PARAM_DEFAULT, &host_conn);
	TEST_ASSERT(!err, "Create conn failed: %d", err);
}

static void ble_connected(struct bt_conn *conn, uint8_t err)
{
	TEST_ASSERT(!err, "BLE connect failed: %d", err);
	SET_FLAG(flag_connected);
}

static void ble_disconnected(struct bt_conn *conn, uint8_t reason)
{
	if (host_conn) {
		bt_conn_unref(host_conn);
		host_conn = NULL;
	}
	SET_FLAG(flag_disconnected);
}

BT_CONN_CB_DEFINE(conn_cbs) = {
	.connected = ble_connected,
	.disconnected = ble_disconnected,
};

static int scan_and_connect(void)
{
	int err;

	UNSET_FLAG(flag_connected);
	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, scan_cb);
	if (err) {
		return err;
	}

	WAIT_FOR_FLAG(flag_connected);
	return 0;
}

static int pair_and_discover(void)
{
	int err;

	err = bt_conn_set_security(host_conn, BT_SECURITY_L2);
	TEST_ASSERT(!err, "set security failed: %d", err);
	k_sleep(K_MSEC(500));

	UNSET_FLAG(flag_hogp_ready);
	UNSET_FLAG(flag_report_map_received);
	UNSET_FLAG(flag_hid_info_received);
	UNSET_FLAG(flag_disconnected);

	err = bt_hogp_host_connect(host_conn, BT_HID_PROTOCOL_REPORT,
				   &hogp_cb);
	TEST_ASSERT(!err, "hogp_host_connect failed: %d", err);

	WAIT_FOR_FLAG(flag_hogp_ready);
	return 0;
}

static void test_hogp_host_basic(void)
{
	int err;

	TEST_START("HOGP Host basic test");

	bk_sync_init();

	err = bt_enable(NULL);
	TEST_ASSERT(!err, "bt_enable failed: %d", err);

	bt_hogp_host_init();

	/* --- TC1: Connect and discover --- */
	err = scan_and_connect();
	TEST_ASSERT(!err, "scan_and_connect failed: %d", err);

	err = pair_and_discover();
	TEST_ASSERT(!err, "pair_and_discover failed: %d", err);

	/* TC2: Report Map verified in callback */
	WAIT_FOR_FLAG(flag_report_map_received);

	/* TC hid_info: verified in callback */
	WAIT_FOR_FLAG(flag_hid_info_received);

	/* Wait for device to be ready to send */
	bk_sync_wait();

	/* TC3: Wait for input report notification */
	UNSET_FLAG(flag_input_report_received);
	WAIT_FOR_FLAG(flag_input_report_received);
	TEST_ASSERT(!memcmp(received_report_data, test_input_data,
			    TEST_INPUT_REPORT_LEN),
		    "Input report data mismatch");

	/* TC5: Test set_report (Output) */
	static const uint8_t output_data[] = {0xAA, 0xBB};

	err = bt_hogp_host_set_report(host_conn, TEST_REPORT_ID_OUTPUT,
				      BT_HID_REPORT_TYPE_OUTPUT,
				      output_data, sizeof(output_data));
	TEST_ASSERT(!err, "set_report failed: %d", err);
	k_sleep(K_MSEC(100));
	bk_sync_send();

	/* TC7: Test suspend */
	err = bt_hogp_host_suspend(host_conn, 0);
	TEST_ASSERT(!err, "suspend failed: %d", err);
	k_sleep(K_MSEC(100));
	bk_sync_send();

	/* TC7: Test exit suspend */
	err = bt_hogp_host_exit_suspend(host_conn, 0);
	TEST_ASSERT(!err, "exit_suspend failed: %d", err);
	k_sleep(K_MSEC(100));
	bk_sync_send();

	/* TC6: Test protocol mode switch */
	err = bt_hogp_host_set_protocol_mode(host_conn, 0,
					     BT_HID_PROTOCOL_BOOT);
	TEST_ASSERT(!err, "set_protocol failed: %d", err);
	k_sleep(K_MSEC(100));
	bk_sync_send();

	/* TC4: Test get_report (Feature) */
	err = bt_hogp_host_get_report(host_conn, TEST_REPORT_ID_FEATURE,
				      BT_HID_REPORT_TYPE_FEATURE);
	TEST_ASSERT(!err, "get_report failed: %d", err);
	WAIT_FOR_FLAG(flag_get_report_done);
	bk_sync_send();

	/* Wait for device to signal basic tests complete */
	bk_sync_wait();

	/* TC set_mode returns -ENOTSUP */
	struct bt_hogp_host_mode_param mode_param = {
		.mode = BT_HOGP_MODE_ISO
	};

	err = bt_hogp_host_set_mode(host_conn, &mode_param);
	TEST_ASSERT(err == -ENOTSUP,
		    "set_mode should return -ENOTSUP: %d", err);

	/* TC8: Reconnection test */
	bt_hogp_host_disconnect(host_conn);
	bt_conn_disconnect(host_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	WAIT_FOR_FLAG(flag_disconnected);
	k_sleep(K_MSEC(200));

	/* Reconnect */
	err = scan_and_connect();
	TEST_ASSERT(!err, "reconnect scan failed: %d", err);

	err = pair_and_discover();
	TEST_ASSERT(!err, "reconnect discover failed: %d", err);

	/* Wait for device to send report after reconnection */
	bk_sync_wait();
	UNSET_FLAG(flag_input_report_received);
	WAIT_FOR_FLAG(flag_input_report_received);

	/* Final disconnect */
	bt_hogp_host_disconnect(host_conn);
	bt_conn_disconnect(host_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	bk_sync_send();

	k_sleep(K_MSEC(500));

	TEST_PASS("HOGP Host basic test passed");
}

static struct bst_test_instance test_hogp_host[] = {
	{
		.test_id = "hogp_host_basic",
		.test_main_f = test_hogp_host_basic,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_hogp_host_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_hogp_host);
}
