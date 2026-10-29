/*
 * Copyright (c) 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* HID Device for the HOGP Host test: the HID Service, registered the way a
 * HOGP Device application registers it, with a Security Request on every
 * connection.
 */

#include <errno.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/services/hids.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/util.h>

#include "babblekit/flags.h"
#include "babblekit/sync.h"
#include "babblekit/testcase.h"
#include "bstests.h"

#include "common.h"

DEFINE_FLAG_STATIC(flag_connected);
DEFINE_FLAG_STATIC(flag_disconnected);
DEFINE_FLAG_STATIC(flag_set_report);

static struct bt_conn *device_conn;

static enum bt_hids_report_type set_report_type;
static uint8_t set_report_id;
static uint8_t set_report_data[CONFIG_BT_HIDS_MAX_REPORT_LEN];
static uint16_t set_report_len;

static void device_connected(struct bt_conn *conn, uint8_t err)
{
	TEST_ASSERT(err == 0U, "Failed to connect (err 0x%02x)", err);

	device_conn = bt_conn_ref(conn);
	SET_FLAG(flag_connected);

	err = bt_conn_set_security(conn, BT_SECURITY_L2);
	TEST_ASSERT(err == 0, "Failed to request security: %d", err);
}

static void device_disconnected(struct bt_conn *conn, uint8_t reason)
{
	ARG_UNUSED(reason);

	if (conn != device_conn) {
		return;
	}

	bt_conn_unref(device_conn);
	device_conn = NULL;
	SET_FLAG(flag_disconnected);
}

static struct bt_conn_cb device_conn_cb = {
	.connected = device_connected,
	.disconnected = device_disconnected,
};

static void set_report(struct bt_conn *conn, enum bt_hids_report_type report_type,
		       uint8_t report_id, const uint8_t *data, uint16_t len)
{
	ARG_UNUSED(conn);

	TEST_ASSERT(len <= sizeof(set_report_data), "SET_REPORT of %u bytes", len);

	set_report_type = report_type;
	set_report_id = report_id;
	(void)memcpy(set_report_data, data, len);
	set_report_len = len;
	SET_FLAG(flag_set_report);
}

static const struct bt_hids_cb hids_cb = {
	.set_report = set_report,
};

static const struct bt_hids_register_param hids_param = {
	.bcd_hid = TEST_BCD_HID,
	.country_code = TEST_COUNTRY_CODE,
	.flags = TEST_HID_FLAGS,
	.report_map = test_report_map,
	.report_map_len = sizeof(test_report_map),
	.input_report_ids = {TEST_REPORT_ID_INPUT},
	.output_report_ids = {TEST_REPORT_ID_OUTPUT},
	.feature_report_ids = {TEST_REPORT_ID_FEATURE, TEST_REPORT_ID_FEATURE_LONG},
	.cb = &hids_cb,
};

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_HIDS_VAL)),
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static void connect_host(void)
{
	int err;

	UNSET_FLAG(flag_connected);
	UNSET_FLAG(flag_disconnected);

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), NULL, 0);
	TEST_ASSERT(err == 0, "Failed to start advertising: %d", err);

	WAIT_FOR_FLAG(flag_connected);
}

static void send_input_report(void)
{
	int err;

	err = bt_hids_send_report(device_conn, TEST_REPORT_ID_INPUT, test_input_data,
				  TEST_INPUT_REPORT_LEN, NULL, NULL);
	TEST_ASSERT(err == 0, "Failed to notify the Input Report: %d", err);
}

static void verify_set_report(enum bt_hids_report_type type, uint8_t id, const uint8_t *data,
			      uint16_t len)
{
	WAIT_FOR_FLAG(flag_set_report);
	UNSET_FLAG(flag_set_report);

	TEST_ASSERT(set_report_type == type && set_report_id == id,
		    "SET_REPORT of type %u ID %u, expected type %u ID %u", set_report_type,
		    set_report_id, type, id);
	TEST_ASSERT(set_report_len == len && memcmp(set_report_data, data, len) == 0,
		    "SET_REPORT with the wrong payload");
}

static void set_long_report(uint16_t len)
{
	uint8_t value[TEST_LONG_REPORT_LEN + 1U];
	int err;

	(void)memset(value, TEST_LONG_REPORT_FILL, len);
	err = bt_hids_report_set(BT_HIDS_REPORT_TYPE_FEATURE, TEST_REPORT_ID_FEATURE_LONG, value,
				 len);
	TEST_ASSERT(err == 0, "Failed to set the long Feature Report: %d", err);
}

static void set_reports(void)
{
	int err;

	err = bt_hids_report_set(BT_HIDS_REPORT_TYPE_FEATURE, TEST_REPORT_ID_FEATURE,
				 test_feature_data, sizeof(test_feature_data));
	TEST_ASSERT(err == 0, "Failed to set the Feature Report: %d", err);

	set_long_report(TEST_LONG_REPORT_LEN);
}

static void test_hogp_device(void)
{
	int err;

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed: %d", err);

	err = settings_load();
	TEST_ASSERT(err == 0, "Failed to load the settings: %d", err);

	TEST_ASSERT(bk_sync_init() == 0, "Failed to open the sync channel");

	bt_conn_cb_register(&device_conn_cb);

	err = bt_hids_register(&hids_param);
	TEST_ASSERT(err == 0, "Failed to register the HID Service: %d", err);

	set_reports();
	connect_host();

	/* The Host has discovered the service and subscribed */
	bk_sync_wait();
	send_input_report();

	/* SET_REPORT on the Output Report, then on the Feature Report */
	verify_set_report(BT_HIDS_REPORT_TYPE_OUTPUT, TEST_REPORT_ID_OUTPUT, test_output_data,
			  sizeof(test_output_data));
	verify_set_report(BT_HIDS_REPORT_TYPE_FEATURE, TEST_REPORT_ID_FEATURE,
			  test_feature_data_new, sizeof(test_feature_data_new));

	/* One octet more than the Host reads */
	set_long_report(TEST_LONG_REPORT_LEN + 1U);
	bk_sync_send();

	/* The Host reconnects to the bonded Device and discovers it again */
	WAIT_FOR_FLAG(flag_disconnected);
	connect_host();

	bk_sync_wait();
	send_input_report();

	WAIT_FOR_FLAG(flag_disconnected);

	TEST_PASS("HOGP Device test passed");
}

static const struct bst_test_instance test_hogp_device_defs[] = {
	{
		.test_id = "hogp_device",
		.test_descr = "HID Device built on the HID Service",
		.test_main_f = test_hogp_device,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_hogp_device_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_hogp_device_defs);
}
