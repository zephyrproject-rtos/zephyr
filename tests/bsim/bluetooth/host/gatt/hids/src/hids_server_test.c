/*
 * Copyright (c) 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* HID Service server, in the HID Device role of the HID over GATT Profile.
 * The composition of the role belongs to the application, so this test does
 * what the peripheral_hogp sample does: it registers the HID Service and
 * requests security on every connection.
 *
 * Every step is acknowledged over the sync channel before the client moves on,
 * so that a Host operation cannot land while its flag is being cleared.
 */

#include <errno.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/services/hids.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>

#include "babblekit/flags.h"
#include "babblekit/sync.h"
#include "babblekit/testcase.h"
#include "bstests.h"

#include "common.h"

DEFINE_FLAG_STATIC(flag_connected);
DEFINE_FLAG_STATIC(flag_disconnected);
DEFINE_FLAG_STATIC(flag_ccc_enabled);
DEFINE_FLAG_STATIC(flag_boot_ccc_enabled);
DEFINE_FLAG_STATIC(flag_set_report);
DEFINE_FLAG_STATIC(flag_set_boot_report);
DEFINE_FLAG_STATIC(flag_protocol_mode_changed);
DEFINE_FLAG_STATIC(flag_ctrl_point);

static struct bt_conn *server_conn;

/* Last SET_REPORT, and the Feature Report value returned on GET_REPORT */
static uint8_t set_report_type;
static uint8_t set_report_id;
static uint8_t set_report_data[CONFIG_BT_HIDS_MAX_REPORT_LEN];
static uint16_t set_report_len;
static uint8_t feature_value[TEST_FEATURE_REPORT_LEN];

/* Last SET_REPORT of the Boot Keyboard Output Report, the only writable Boot
 * Report
 */
static uint8_t set_boot_report_data[BT_HIDS_BOOT_KB_OUT_LEN];
static uint16_t set_boot_report_len;

/* The long Feature Report is filled with a sequence number the server changes
 * between the client's reads, so the client can tell one value from the next.
 */
static uint8_t long_report_seq;

static void set_long_report(void)
{
	uint8_t value[TEST_LONG_REPORT_LEN];
	int err;

	long_report_seq++;
	(void)memset(value, long_report_seq, sizeof(value));

	err = bt_hids_report_set(BT_HID_REPORT_TYPE_FEATURE, TEST_REPORT_ID_FEATURE_LONG, value,
				 sizeof(value));
	TEST_ASSERT(err == 0, "Failed to set the long Feature Report: %d", err);
}

static enum bt_hid_protocol_mode last_protocol_mode;
static enum bt_hids_ctrl_point last_ctrl_point;

/* The HID Device informs the Host of its security requirements with the
 * Peripheral Security Request procedure, instead of
 * leaving the Host with a failing read of a HID Service characteristic. The
 * client side of this test relies on it and never initiates pairing itself, so
 * building with CONFIG_TEST_HIDS_SECURITY_REQUEST=n has to make the test fail.
 */
static void request_security(struct bt_conn *conn)
{
	int err;

	if (!IS_ENABLED(CONFIG_TEST_HIDS_SECURITY_REQUEST)) {
		return;
	}

	if (bt_conn_get_security(conn) >= BT_SECURITY_L2) {
		return;
	}

	err = bt_conn_set_security(conn, BT_SECURITY_L2);
	TEST_ASSERT(err == 0, "Failed to request security: %d", err);
}

static void server_connected(struct bt_conn *conn, uint8_t err)
{
	TEST_ASSERT(err == 0U, "Failed to connect (err 0x%02x)", err);

	server_conn = bt_conn_ref(conn);
	SET_FLAG(flag_connected);

	request_security(conn);
}

static void server_disconnected(struct bt_conn *conn, uint8_t reason)
{
	ARG_UNUSED(reason);

	if (conn != server_conn) {
		return;
	}

	bt_conn_unref(server_conn);
	server_conn = NULL;

	SET_FLAG(flag_disconnected);
}

static struct bt_conn_cb server_conn_cb = {
	.connected = server_connected,
	.disconnected = server_disconnected,
};

static void set_report(struct bt_conn *conn, uint8_t report_type, uint8_t report_id,
		       const uint8_t *data, uint16_t len)
{
	ARG_UNUSED(conn);

	TEST_ASSERT(len <= sizeof(set_report_data), "SET_REPORT of %u bytes", len);

	set_report_type = report_type;
	set_report_id = report_id;
	(void)memcpy(set_report_data, data, len);
	set_report_len = len;

	if (report_type == BT_HID_REPORT_TYPE_FEATURE && len == sizeof(feature_value)) {
		(void)memcpy(feature_value, data, len);
	}

	SET_FLAG(flag_set_report);
}

static void protocol_mode_changed(struct bt_conn *conn, uint8_t protocol)
{
	ARG_UNUSED(conn);

	last_protocol_mode = protocol;
	SET_FLAG(flag_protocol_mode_changed);
}

static void ctrl_point(struct bt_conn *conn, enum bt_hids_ctrl_point cmd)
{
	ARG_UNUSED(conn);

	last_ctrl_point = cmd;
	SET_FLAG(flag_ctrl_point);
}

static void verify_suspend_state(bool expected)
{
	bool suspended;
	int err;

	err = bt_hids_get_suspend_state(server_conn, &suspended);
	TEST_ASSERT(err == 0, "Failed to get the Suspend state: %d", err);
	TEST_ASSERT(suspended == expected, "Suspend state is %d", suspended);
}

static void ccc_changed(struct bt_conn *conn, uint8_t report_id, uint8_t report_type, bool enabled)
{
	ARG_UNUSED(conn);

	/* Only the Input Report has a CCC descriptor */
	TEST_ASSERT(report_type == BT_HID_REPORT_TYPE_INPUT, "CCC of Report Type %u changed",
		    report_type);
	TEST_ASSERT(report_id == TEST_REPORT_ID_INPUT, "CCC of Report ID %u changed", report_id);

	if (enabled) {
		SET_FLAG(flag_ccc_enabled);
	}
}

/* The Boot Keyboard Output Report is the only writable Boot Report, and the
 * service refuses a write of any length but its own, so the application only
 * ever sees the fixed length.
 */
static void set_boot_report(struct bt_conn *conn, enum bt_hids_boot_report report,
			    const uint8_t *data, uint16_t len)
{
	ARG_UNUSED(conn);

	TEST_ASSERT(report == BT_HIDS_BOOT_REPORT_KEYBOARD_OUTPUT, "SET_REPORT of Boot Report %u",
		    (unsigned int)report);
	TEST_ASSERT(len == sizeof(set_boot_report_data), "SET_REPORT of %u octets, expected %zu",
		    len, sizeof(set_boot_report_data));

	(void)memcpy(set_boot_report_data, data, len);
	set_boot_report_len = len;

	SET_FLAG(flag_set_boot_report);
}

static void boot_ccc_changed(struct bt_conn *conn, enum bt_hids_boot_report report, bool enabled)
{
	ARG_UNUSED(conn);

	/* Only the Boot Input Reports have a CCC descriptor, and this test
	 * subscribes to the Boot Keyboard Input Report
	 */
	TEST_ASSERT(report == BT_HIDS_BOOT_REPORT_KEYBOARD_INPUT, "CCC of Boot Report %u changed",
		    (unsigned int)report);

	if (enabled) {
		SET_FLAG(flag_boot_ccc_enabled);
	}
}

static const struct bt_hids_cb hids_cb = {
	.set_report = set_report,
	.set_boot_report = set_boot_report,
	.protocol_mode_changed = protocol_mode_changed,
	.ctrl_point = ctrl_point,
	.ccc_changed = ccc_changed,
	.boot_ccc_changed = boot_ccc_changed,
};

static const struct bt_hids_register_param hids_param = {
	/* clang-format off */
	.info = {
		.bcd_hid = TEST_BCD_HID,
		.b_country_code = TEST_COUNTRY_CODE,
		.flags = TEST_HID_FLAGS,
	},
	/* clang-format on */
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
};

static void start_adv(void)
{
	int err;

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), NULL, 0);
	TEST_ASSERT(err == 0, "Failed to start advertising: %d", err);
}

static void send_input_report(void)
{
	int err;

	err = bt_hids_send_report(server_conn, TEST_REPORT_ID_INPUT, test_input_data,
				  TEST_INPUT_REPORT_LEN, NULL, NULL);
	TEST_ASSERT(err == 0, "Failed to notify the Input Report: %d", err);
}

static void send_boot_kb_in_report(void)
{
	int err;

	err = bt_hids_boot_report_send(server_conn, BT_HIDS_BOOT_REPORT_KEYBOARD_INPUT,
				       test_boot_kb_in_data, BT_HIDS_BOOT_KB_IN_LEN, NULL, NULL);
	TEST_ASSERT(err == 0, "Failed to notify the Boot Keyboard Input Report: %d", err);
}

static void test_send_report_validation(void)
{
	static uint8_t too_long[CONFIG_BT_HIDS_MAX_REPORT_LEN + 1];
	int err;

	err = bt_hids_send_report(server_conn, TEST_REPORT_ID_INPUT, NULL, TEST_INPUT_REPORT_LEN,
				  NULL, NULL);
	TEST_ASSERT(err == -EINVAL, "Notified without a payload: %d", err);

	err = bt_hids_send_report(server_conn, TEST_REPORT_ID_INPUT, test_input_data, 0U, NULL,
				  NULL);
	TEST_ASSERT(err == -EINVAL, "Notified an empty Report: %d", err);

	/* CONFIG_BT_HIDS_MAX_REPORT_LEN bounds the payload in both directions */
	err = bt_hids_send_report(server_conn, TEST_REPORT_ID_INPUT, too_long, sizeof(too_long),
				  NULL, NULL);
	TEST_ASSERT(err == -EINVAL, "Notified a Report of %zu octets: %d", sizeof(too_long), err);

	/* Only Input Reports are notified */
	err = bt_hids_send_report(server_conn, TEST_REPORT_ID_OUTPUT, test_output_data,
				  TEST_OUTPUT_REPORT_LEN, NULL, NULL);
	TEST_ASSERT(err == -ENOENT, "Notified an Output Report: %d", err);
}

/* Every Boot Report has a fixed length of its own, so a payload of another Boot
 * Report's length is refused. That is what tells the three Boot Report contexts
 * apart on this side: a read tells them apart on the Host side.
 */
static void test_boot_report_validation(void)
{
	uint8_t value[BT_HIDS_BOOT_KB_IN_LEN];
	uint16_t len;
	int err;

	/* Only the Boot Input Reports are notifiable */
	err = bt_hids_boot_report_send(server_conn, BT_HIDS_BOOT_REPORT_KEYBOARD_OUTPUT,
				       test_boot_kb_out_data, BT_HIDS_BOOT_KB_OUT_LEN, NULL, NULL);
	TEST_ASSERT(err == -EINVAL, "Notified the Boot Keyboard Output Report: %d", err);

	err = bt_hids_boot_report_send(server_conn, BT_HIDS_BOOT_REPORT_MOUSE_INPUT,
				       test_boot_kb_in_data, BT_HIDS_BOOT_KB_IN_LEN, NULL, NULL);
	TEST_ASSERT(err == -EINVAL, "Notified a Boot Mouse Input Report of %u octets: %d",
		    BT_HIDS_BOOT_KB_IN_LEN, err);

	err = bt_hids_boot_report_set(BT_HIDS_BOOT_REPORT_MOUSE_INPUT, test_boot_kb_in_data,
				      BT_HIDS_BOOT_KB_IN_LEN);
	TEST_ASSERT(err == -EINVAL, "Set a Boot Mouse Input Report of %u octets: %d",
		    BT_HIDS_BOOT_KB_IN_LEN, err);

	len = BT_HIDS_BOOT_MOUSE_IN_LEN - 1;
	err = bt_hids_boot_report_get(BT_HIDS_BOOT_REPORT_MOUSE_INPUT, value, &len);
	TEST_ASSERT(err == -ENOMEM, "Read a Boot Mouse Input Report into %u octets: %d", len, err);

	len = sizeof(value);
	err = bt_hids_boot_report_get(BT_HIDS_BOOT_REPORT_MOUSE_INPUT, value, &len);
	TEST_ASSERT(err == 0, "Failed to read the Boot Mouse Input Report: %d", err);
	TEST_ASSERT(len == BT_HIDS_BOOT_MOUSE_IN_LEN,
		    "Boot Mouse Input Report of %u octets, expected %u", len,
		    BT_HIDS_BOOT_MOUSE_IN_LEN);
}

static void verify_set_report(uint8_t report_type, uint8_t report_id, const uint8_t *data,
			      uint16_t len)
{
	TEST_ASSERT(set_report_type == report_type, "SET_REPORT of Report Type %u",
		    set_report_type);
	TEST_ASSERT(set_report_id == report_id, "SET_REPORT of Report ID %u", set_report_id);
	TEST_ASSERT(set_report_len == len, "SET_REPORT of %u bytes", set_report_len);
	TEST_ASSERT(memcmp(set_report_data, data, len) == 0, "SET_REPORT payload mismatch");
}

/* A Report Map longer than 512 octets would need several HID Service
 * instances, which is not supported. The number of Report
 * characteristics of each type is a build time configuration, so a mismatching
 * Report set cannot be passed at all.
 */
static void test_register_validation(void)
{
	struct bt_hids_register_param invalid;
	int err;

	err = bt_hids_register(NULL);
	TEST_ASSERT(err == -EINVAL, "Registered without parameters: %d", err);

	invalid = hids_param;
	invalid.report_map_len = BT_HIDS_REPORT_MAP_MAX_LEN + 1;
	err = bt_hids_register(&invalid);
	TEST_ASSERT(err == -EINVAL, "Oversized Report Map accepted: %d", err);

	invalid = hids_param;
	invalid.cb = NULL;
	err = bt_hids_register(&invalid);
	TEST_ASSERT(err == -EINVAL, "Registered without callbacks: %d", err);

	/* The SCI Supported flag makes the HID SCI Information and HID SCI Mode
	 * characteristics mandatory, which the service does not have.
	 */
	invalid = hids_param;
	invalid.info.flags = BIT(2);
	err = bt_hids_register(&invalid);
	TEST_ASSERT(err == -EINVAL, "SCI Supported flag accepted: %d", err);
}

/* The Protocol Mode and the Suspend state are per connection and reset to
 * their default values every time a Host connects.
 */
static void verify_per_connection_defaults(void)
{
	enum bt_hid_protocol_mode mode;
	int err;

	err = bt_hids_get_protocol_mode(server_conn, &mode);
	TEST_ASSERT(err == 0, "Failed to get the Protocol Mode: %d", err);
	TEST_ASSERT(mode == BT_HID_PROTOCOL_REPORT, "Protocol Mode is %u", mode);

	verify_suspend_state(false);
}

static void test_hids_server(void)
{
	enum bt_hid_protocol_mode mode;
	int err;

	TEST_START("HID Service server test");

	TEST_ASSERT(bk_sync_init() == 0, "Failed to open the sync channel");

	err = bt_conn_cb_register(&server_conn_cb);
	TEST_ASSERT(err == 0, "Failed to register the connection callbacks: %d", err);

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Failed to enable Bluetooth: %d", err);

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		(void)settings_load();
	}

	test_register_validation();

	err = bt_hids_register(&hids_param);
	TEST_ASSERT(err == 0, "Failed to register the HID Service: %d", err);

	/* The service holds the values a Host reads, so seed them */
	err = bt_hids_report_set(BT_HID_REPORT_TYPE_INPUT, TEST_REPORT_ID_INPUT, test_input_data,
				 TEST_INPUT_REPORT_LEN);
	TEST_ASSERT(err == 0, "Failed to set the Input Report: %d", err);
	err = bt_hids_report_set(BT_HID_REPORT_TYPE_FEATURE, TEST_REPORT_ID_FEATURE, feature_value,
				 sizeof(feature_value));
	TEST_ASSERT(err == 0, "Failed to set the Feature Report: %d", err);
	set_long_report();

	err = bt_hids_boot_report_set(BT_HIDS_BOOT_REPORT_KEYBOARD_INPUT, test_boot_kb_in_stored,
				      BT_HIDS_BOOT_KB_IN_LEN);
	TEST_ASSERT(err == 0, "Failed to set the Boot Keyboard Input Report: %d", err);

	err = bt_hids_register(&hids_param);
	TEST_ASSERT(err == -EALREADY, "Registered the HID Service twice: %d", err);

	start_adv();

	WAIT_FOR_FLAG(flag_connected);
	WAIT_FOR_FLAG(flag_ccc_enabled);
	verify_per_connection_defaults();
	test_send_report_validation();
	test_boot_report_validation();

	/* The client waits for this Input Report notification */
	bk_sync_send();
	send_input_report();

	/* SET_REPORT of the Output Report */
	bk_sync_wait();
	WAIT_FOR_FLAG(flag_set_report);
	verify_set_report(BT_HID_REPORT_TYPE_OUTPUT, TEST_REPORT_ID_OUTPUT, test_output_data,
			  TEST_OUTPUT_REPORT_LEN);
	UNSET_FLAG(flag_set_report);
	bk_sync_send();

	/* SET_REPORT of the Feature Report */
	bk_sync_wait();
	WAIT_FOR_FLAG(flag_set_report);
	verify_set_report(BT_HID_REPORT_TYPE_FEATURE, TEST_REPORT_ID_FEATURE, test_feature_data,
			  TEST_FEATURE_REPORT_LEN);
	UNSET_FLAG(flag_set_report);
	bk_sync_send();

	/* Suspend through the HID Control Point */
	bk_sync_wait();
	WAIT_FOR_FLAG(flag_ctrl_point);
	TEST_ASSERT(last_ctrl_point == BT_HIDS_CTRL_SUSPEND, "HID Control Point command %u",
		    last_ctrl_point);
	verify_suspend_state(true);
	UNSET_FLAG(flag_ctrl_point);
	bk_sync_send();

	/* Exit Suspend through the HID Control Point */
	bk_sync_wait();
	WAIT_FOR_FLAG(flag_ctrl_point);
	TEST_ASSERT(last_ctrl_point == BT_HIDS_CTRL_EXIT_SUSPEND, "HID Control Point command %u",
		    last_ctrl_point);
	verify_suspend_state(false);
	UNSET_FLAG(flag_ctrl_point);
	bk_sync_send();

	/* Switch to Boot Protocol Mode */
	bk_sync_wait();
	WAIT_FOR_FLAG(flag_protocol_mode_changed);
	TEST_ASSERT(last_protocol_mode == BT_HID_PROTOCOL_BOOT, "Protocol Mode is %u",
		    last_protocol_mode);
	err = bt_hids_get_protocol_mode(server_conn, &mode);
	TEST_ASSERT(err == 0, "Failed to get the Protocol Mode: %d", err);
	TEST_ASSERT(mode == BT_HID_PROTOCOL_BOOT, "Protocol Mode is %u", mode);
	UNSET_FLAG(flag_protocol_mode_changed);
	bk_sync_send();

	/* Back to Report Protocol Mode */
	bk_sync_wait();
	WAIT_FOR_FLAG(flag_protocol_mode_changed);
	TEST_ASSERT(last_protocol_mode == BT_HID_PROTOCOL_REPORT, "Protocol Mode is %u",
		    last_protocol_mode);
	UNSET_FLAG(flag_protocol_mode_changed);
	bk_sync_send();

	/* The client has subscribed to the Boot Keyboard Input Report */
	bk_sync_wait();
	WAIT_FOR_FLAG(flag_boot_ccc_enabled);
	send_boot_kb_in_report();
	bk_sync_send();

	/* A write of the wrong length to the Boot Keyboard Output Report is
	 * refused by the service, so only the one of the fixed length arrives
	 */
	bk_sync_wait();
	WAIT_FOR_FLAG(flag_set_boot_report);
	TEST_ASSERT(set_boot_report_len == BT_HIDS_BOOT_KB_OUT_LEN, "SET_REPORT of %u octets",
		    set_boot_report_len);
	TEST_ASSERT(memcmp(set_boot_report_data, test_boot_kb_out_data,
			   BT_HIDS_BOOT_KB_OUT_LEN) == 0,
		    "Boot Keyboard Output Report payload mismatch");
	UNSET_FLAG(flag_set_boot_report);
	bk_sync_send();

	/* GET_REPORT of the Input Report. The service answers a read from the
	 * value it holds, so the application is not called and cannot wait for
	 * anything here.
	 */
	bk_sync_wait();
	bk_sync_send();

	/* GET_REPORT of the Feature Report */
	bk_sync_wait();
	bk_sync_send();

	/* The client reads the long Feature Report, then the server replaces it
	 * and the client reads it again, so that the second read has to return
	 * the new value.
	 */
	bk_sync_wait();
	set_long_report();
	bk_sync_send();

	bk_sync_wait();
	bk_sync_send();

	/* The client disconnects and reconnects */
	WAIT_FOR_FLAG(flag_disconnected);
	UNSET_FLAG(flag_connected);
	UNSET_FLAG(flag_disconnected);
	UNSET_FLAG(flag_ccc_enabled);

	start_adv();

	WAIT_FOR_FLAG(flag_connected);
	WAIT_FOR_FLAG(flag_ccc_enabled);
	verify_per_connection_defaults();

	bk_sync_send();
	send_input_report();

	/* Unregistering only removes the GATT service: like every other LE
	 * service in the tree, it does not disconnect the Host.
	 */
	bk_sync_wait();

	err = bt_hids_unregister();
	TEST_ASSERT(err == 0, "Failed to unregister the HID Service: %d", err);

	/* A disconnection would complete asynchronously, so give it the time
	 * to show up before checking that it did not happen.
	 */
	k_sleep(K_MSEC(500));
	TEST_ASSERT(!IS_FLAG_SET(flag_disconnected), "Unregistering disconnected the Host");

	err = bt_hids_send_report(server_conn, TEST_REPORT_ID_INPUT, test_input_data,
				  TEST_INPUT_REPORT_LEN, NULL, NULL);
	TEST_ASSERT(err == -ESRCH, "Notified after unregistering: %d", err);

	err = bt_hids_boot_report_send(server_conn, BT_HIDS_BOOT_REPORT_KEYBOARD_INPUT,
				       test_boot_kb_in_data, BT_HIDS_BOOT_KB_IN_LEN, NULL, NULL);
	TEST_ASSERT(err == -ESRCH, "Notified a Boot Report after unregistering: %d", err);

	err = bt_hids_unregister();
	TEST_ASSERT(err == -EALREADY, "Unregistered the HID Service twice: %d", err);

	/* Registering while a Host is connected has to track it, as the
	 * connected callback has already run for that Host.
	 */
	err = bt_hids_register(&hids_param);
	TEST_ASSERT(err == 0, "Failed to register the HID Service again: %d", err);

	verify_per_connection_defaults();

	/* The client closes the connection */
	bk_sync_send();
	WAIT_FOR_FLAG(flag_disconnected);

	TEST_PASS("HID Service server test passed");
}

static struct bst_test_instance test_hids_server_defs[] = {
	{
		.test_id = "hids_server",
		.test_descr = "HID Service server side of the two device test",
		.test_main_f = test_hids_server,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_hids_server_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_hids_server_defs);
}
