/*
 * Copyright 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/services/hogp_device.h>
#include <zephyr/bluetooth/services/bas.h>

#include "babblekit/flags.h"
#include "babblekit/sync.h"
#include "babblekit/testcase.h"
#include "bstests.h"

#include "common.h"

DEFINE_FLAG(flag_connected);
DEFINE_FLAG(flag_disconnected);
DEFINE_FLAG(flag_ccc_enabled);
DEFINE_FLAG(flag_ccc_disabled);
DEFINE_FLAG(flag_set_report_received);
DEFINE_FLAG(flag_ctrl_point_received);
DEFINE_FLAG(flag_set_protocol_received);
DEFINE_FLAG(flag_get_report_received);

static struct bt_conn *device_conn;
static uint8_t last_ctrl_point_val;
static uint8_t last_set_report_data[32];
static uint16_t last_set_report_len;

static const struct bt_hogp_device_report test_reports[] = {
	{ .id = TEST_REPORT_ID_INPUT, .type = BT_HID_REPORT_TYPE_INPUT },
	{ .id = TEST_REPORT_ID_OUTPUT, .type = BT_HID_REPORT_TYPE_OUTPUT },
	{ .id = TEST_REPORT_ID_FEATURE, .type = BT_HID_REPORT_TYPE_FEATURE },
};

static void dev_connected_cb(struct bt_conn *conn)
{
	device_conn = bt_conn_ref(conn);
	SET_FLAG(flag_connected);
}

static void dev_disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
	if (device_conn) {
		bt_conn_unref(device_conn);
		device_conn = NULL;
	}
	SET_FLAG(flag_disconnected);
}

static void dev_get_report_cb(struct bt_conn *conn, uint8_t report_type,
			      uint8_t report_id, uint16_t buf_size)
{
	SET_FLAG(flag_get_report_received);
}

static void dev_set_report_cb(struct bt_conn *conn, uint8_t report_type,
			      uint8_t report_id, const uint8_t *data,
			      uint16_t len)
{
	if (len <= sizeof(last_set_report_data)) {
		memcpy(last_set_report_data, data, len);
		last_set_report_len = len;
	}
	SET_FLAG(flag_set_report_received);
}

static void dev_set_protocol_cb(struct bt_conn *conn, uint8_t protocol)
{
	SET_FLAG(flag_set_protocol_received);
}

static void dev_ctrl_point_cb(struct bt_conn *conn, uint8_t value)
{
	last_ctrl_point_val = value;
	SET_FLAG(flag_ctrl_point_received);
}

static void dev_ccc_changed_cb(struct bt_conn *conn, uint8_t report_id,
			       uint8_t report_type, bool enabled)
{
	if (enabled) {
		SET_FLAG(flag_ccc_enabled);
	} else {
		SET_FLAG(flag_ccc_disabled);
	}
}

static const struct bt_hogp_device_cb dev_cb = {
	.connected = dev_connected_cb,
	.disconnected = dev_disconnected_cb,
	.get_report = dev_get_report_cb,
	.set_report = dev_set_report_cb,
	.set_protocol = dev_set_protocol_cb,
	.ctrl_point = dev_ctrl_point_cb,
	.ccc_changed = dev_ccc_changed_cb,
};

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_HIDS_VAL)),
};

static int start_adv(void)
{
	return bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad),
			       NULL, 0);
}

static void test_hogp_device_basic(void)
{
	int err;

	TEST_START("HOGP Device basic test");

	bk_sync_init();

	err = bt_enable(NULL);
	TEST_ASSERT(!err, "bt_enable failed: %d", err);

	struct bt_hogp_device_init_param param = {
		.info = {
			.bcd_hid = TEST_BCD_HID,
			.b_country_code = TEST_COUNTRY_CODE,
			.flags = TEST_HID_FLAGS,
		},
		.report_map = test_report_map,
		.report_map_len = sizeof(test_report_map),
		.reports = test_reports,
		.report_count = ARRAY_SIZE(test_reports),
		.cb = &dev_cb,
	};

	err = bt_hogp_device_register(&param);
	TEST_ASSERT(!err, "register failed: %d", err);

	err = start_adv();
	TEST_ASSERT(!err, "adv start failed: %d", err);

	/* TC1: Wait for Host to connect */
	WAIT_FOR_FLAG(flag_connected);

	/* TC12: Wait for Host to subscribe (CCC enabled) */
	WAIT_FOR_FLAG(flag_ccc_enabled);

	/* Sync: tell host we're ready to send reports */
	bk_sync_send();

	/* TC3: Send Input Report */
	err = bt_hogp_device_send_report(device_conn, TEST_REPORT_ID_INPUT,
					 test_input_data,
					 TEST_INPUT_REPORT_LEN, NULL, NULL);
	TEST_ASSERT(!err, "send_report failed: %d", err);

	/* TC5: Wait for host to write set_report (Output) */
	bk_sync_wait();
	WAIT_FOR_FLAG(flag_set_report_received);

	/* TC7: Wait for host to write suspend */
	bk_sync_wait();
	WAIT_FOR_FLAG(flag_ctrl_point_received);
	TEST_ASSERT(last_ctrl_point_val == 0, "Expected Suspend (0), got %u",
		    last_ctrl_point_val);

	/* TC7: Wait for host to write exit suspend */
	UNSET_FLAG(flag_ctrl_point_received);
	bk_sync_wait();
	WAIT_FOR_FLAG(flag_ctrl_point_received);
	TEST_ASSERT(last_ctrl_point_val == 1, "Expected Exit Suspend (1), got %u",
		    last_ctrl_point_val);

	/* TC6: Wait for host to write protocol mode */
	bk_sync_wait();
	WAIT_FOR_FLAG(flag_set_protocol_received);

	/* TC4: Wait for host to do get_report */
	bk_sync_wait();
	WAIT_FOR_FLAG(flag_get_report_received);

	/* Signal basic tests complete */
	bk_sync_send();

	/* TC8: Reconnection test - disconnect then reconnect */
	WAIT_FOR_FLAG(flag_disconnected);
	UNSET_FLAG(flag_connected);
	UNSET_FLAG(flag_disconnected);
	UNSET_FLAG(flag_ccc_enabled);

	/* Re-advertise for reconnection */
	err = start_adv();
	TEST_ASSERT(!err, "re-adv start failed: %d", err);

	/* Wait for reconnection */
	WAIT_FOR_FLAG(flag_connected);
	WAIT_FOR_FLAG(flag_ccc_enabled);

	/* Send report after reconnection */
	bk_sync_send();
	err = bt_hogp_device_send_report(device_conn, TEST_REPORT_ID_INPUT,
					 test_input_data,
					 TEST_INPUT_REPORT_LEN, NULL, NULL);
	TEST_ASSERT(!err, "send_report after reconnect failed: %d", err);

	/* Wait for final disconnect */
	bk_sync_wait();
	WAIT_FOR_FLAG(flag_disconnected);

	TEST_PASS("HOGP Device basic test passed");
}

static struct bst_test_instance test_hogp_device[] = {
	{
		.test_id = "hogp_device_basic",
		.test_main_f = test_hogp_device_basic,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_hogp_device_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_hogp_device);
}
