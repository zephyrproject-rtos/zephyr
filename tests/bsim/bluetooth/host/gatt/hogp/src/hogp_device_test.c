/*
 * Copyright (c) 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hogp_device.h>
#include <zephyr/bluetooth/services/bas.h>
#include <zephyr/settings/settings.h>

#include "babblekit/flags.h"
#include "babblekit/sync.h"
#include "babblekit/testcase.h"
#include "bstests.h"

#include "common.h"

DEFINE_FLAG_STATIC(flag_connected);
DEFINE_FLAG_STATIC(flag_disconnected);
DEFINE_FLAG_STATIC(flag_ccc_enabled);
DEFINE_FLAG_STATIC(flag_ccc_disabled);
DEFINE_FLAG_STATIC(flag_set_report_received);
DEFINE_FLAG_STATIC(flag_ctrl_point_received);
DEFINE_FLAG_STATIC(flag_set_protocol_received);
DEFINE_FLAG_STATIC(flag_get_report_received);

static struct bt_conn *device_conn;
static uint8_t last_ctrl_point_val;
static uint8_t last_set_report_data[32];
static uint16_t last_set_report_len;

static void dev_connected(struct bt_conn *conn, uint8_t err)
{
	if (err != 0U) {
		return;
	}

	device_conn = bt_conn_ref(conn);
	SET_FLAG(flag_connected);
}

static void dev_disconnected(struct bt_conn *conn, uint8_t reason)
{
	ARG_UNUSED(reason);

	if (device_conn != NULL) {
		bt_conn_unref(device_conn);
		device_conn = NULL;
	}
	SET_FLAG(flag_disconnected);
}

static struct bt_conn_cb device_conn_cb = {
	.connected = dev_connected,
	.disconnected = dev_disconnected,
};

static ssize_t dev_get_report_cb(struct bt_conn *conn, uint8_t report_type,
				 uint8_t report_id, uint8_t *buf,
				 uint16_t buf_size)
{
	ARG_UNUSED(conn);

	SET_FLAG(flag_get_report_received);

	if (report_id == TEST_REPORT_ID_INPUT &&
	    report_type == BT_HID_REPORT_TYPE_INPUT) {
		uint16_t len = MIN(buf_size, TEST_INPUT_REPORT_LEN);

		(void)memcpy(buf, test_input_data, len);
		return (ssize_t)len;
	}

	return -ENOENT;
}

static void dev_set_report_cb(struct bt_conn *conn, uint8_t report_type,
			      uint8_t report_id, const uint8_t *data,
			      uint16_t len)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(report_type);
	ARG_UNUSED(report_id);

	if (len <= sizeof(last_set_report_data)) {
		(void)memcpy(last_set_report_data, data, len);
		last_set_report_len = len;
	}
	SET_FLAG(flag_set_report_received);
}

static void dev_protocol_mode_changed_cb(struct bt_conn *conn, uint8_t protocol)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(protocol);

	SET_FLAG(flag_set_protocol_received);
}

static void dev_suspend_changed_cb(struct bt_conn *conn, bool suspended)
{
	ARG_UNUSED(conn);

	last_ctrl_point_val = suspended ? BT_HIDS_CTRL_SUSPEND
					: BT_HIDS_CTRL_EXIT_SUSPEND;
	SET_FLAG(flag_ctrl_point_received);
}

static void dev_ccc_changed_cb(struct bt_conn *conn, uint8_t report_id,
			       uint8_t report_type, bool enabled)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(report_id);
	ARG_UNUSED(report_type);

	if (enabled) {
		SET_FLAG(flag_ccc_enabled);
	} else {
		SET_FLAG(flag_ccc_disabled);
	}
}

static const struct bt_hids_cb dev_cb = {
	.get_report = dev_get_report_cb,
	.set_report = dev_set_report_cb,
	.protocol_mode_changed = dev_protocol_mode_changed_cb,
	.suspend_changed = dev_suspend_changed_cb,
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

/* HOGP v1.1, Section 2.5: an oversized Report Map is rejected at registration
 * time. The number of Report characteristics of each type is a build time
 * configuration, so a mismatching report set cannot be passed at all.
 */
static void test_register_validation(const struct bt_hogp_device_register_param *param)
{
	struct bt_hogp_device_register_param invalid;
	int err;

	invalid = *param;
	invalid.hids.report_map_len = BT_HIDS_REPORT_MAP_MAX_LEN + 1;
	err = bt_hogp_device_register(&invalid);
	TEST_ASSERT(err == -EINVAL, "Oversized Report Map accepted: %d", err);
}

static void test_hogp_device_basic(void)
{
	int err;

	TEST_START("HOGP Device basic test");

	TEST_ASSERT(bk_sync_init() == 0, "Failed to open sync channel");

	err = bt_conn_cb_register(&device_conn_cb);
	TEST_ASSERT(!err, "Failed to register connection callbacks: %d", err);

	err = bt_enable(NULL);
	TEST_ASSERT(!err, "bt_enable failed: %d", err);

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		(void)settings_load();
	}

	struct bt_hogp_device_register_param param = {
		.hids = {
			.info = {
				.bcd_hid = TEST_BCD_HID,
				.b_country_code = TEST_COUNTRY_CODE,
				.flags = TEST_HID_FLAGS,
			},
			.report_map = test_report_map,
			.report_map_len = sizeof(test_report_map),
			.input_report_ids = {TEST_REPORT_ID_INPUT},
			.output_report_ids = {TEST_REPORT_ID_OUTPUT},
			.feature_report_ids = {TEST_REPORT_ID_FEATURE},
			.cb = &dev_cb,
		},
	};

	test_register_validation(&param);

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
	TEST_ASSERT(last_ctrl_point_val == BT_HIDS_CTRL_SUSPEND,
		    "Expected Suspend (0), got %u", last_ctrl_point_val);

	/* TC7: Wait for host to write exit suspend */
	UNSET_FLAG(flag_ctrl_point_received);
	bk_sync_wait();
	WAIT_FOR_FLAG(flag_ctrl_point_received);
	TEST_ASSERT(last_ctrl_point_val == BT_HIDS_CTRL_EXIT_SUSPEND,
		    "Expected Exit Suspend (1), got %u", last_ctrl_point_val);

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

	bk_sync_wait();

	/* Unregistering only removes the GATT service: like every other LE
	 * service in the tree, it does not disconnect the Host.
	 */
	err = bt_hogp_device_unregister();
	TEST_ASSERT(!err, "unregister failed: %d", err);

	/* A disconnect would be requested from bt_hogp_device_unregister(),
	 * but completes asynchronously, so give it time to show up.
	 */
	k_sleep(K_MSEC(500));
	TEST_ASSERT(!IS_FLAG_SET(flag_disconnected),
		    "Unregistering disconnected the Host");
	err = bt_hogp_device_send_report(device_conn, TEST_REPORT_ID_INPUT,
					 test_input_data,
					 TEST_INPUT_REPORT_LEN, NULL, NULL);
	TEST_ASSERT(err == -ESRCH, "send_report after unregister: %d", err);
	err = bt_hogp_device_unregister();
	TEST_ASSERT(err == -EALREADY, "second unregister: %d", err);

	/* The Host closes the connection */
	bk_sync_send();
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
