/*
 * Copyright (c) 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/types.h>

#include "babblekit/flags.h"
#include "babblekit/sync.h"
#include "babblekit/testcase.h"
#include "bstests.h"

#include "common.h"

#define MAX_REPORTS 4

DEFINE_FLAG_STATIC(flag_connected);
DEFINE_FLAG_STATIC(flag_disconnected);
DEFINE_FLAG_STATIC(flag_security_updated);
DEFINE_FLAG_STATIC(flag_discover_complete);
DEFINE_FLAG_STATIC(flag_mtu_exchanged);
DEFINE_FLAG_STATIC(flag_write_complete);
DEFINE_FLAG_STATIC(flag_read_complete);
DEFINE_FLAG_STATIC(flag_subscribed);
DEFINE_FLAG_STATIC(flag_notified);

struct report_chrc {
	uint16_t value_handle;
	uint16_t ccc_handle;
	uint16_t ref_handle;
	uint8_t id;
	uint8_t type;
};

static struct bt_conn *host_conn;

static uint16_t hids_start_handle;
static uint16_t hids_end_handle;
static uint16_t protocol_mode_handle;
static uint16_t report_map_handle;
static uint16_t hid_info_handle;
static uint16_t ctrl_point_handle;
static struct report_chrc reports[MAX_REPORTS];
static uint8_t reports_cnt;

static uint8_t att_err;
static uint8_t read_data[sizeof(test_report_map)];
static uint16_t read_len;
static uint8_t notified_data[TEST_INPUT_REPORT_LEN];
static uint16_t notified_len;

static struct report_chrc *report_by_type(uint8_t type)
{
	for (uint8_t i = 0U; i < reports_cnt; i++) {
		if (reports[i].type == type) {
			return &reports[i];
		}
	}

	return NULL;
}

/* Connection handling */

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err != 0U) {
		TEST_FAIL("Failed to connect (err 0x%02x)", err);
		return;
	}

	SET_FLAG(flag_connected);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	if (conn != host_conn) {
		return;
	}

	bt_conn_unref(host_conn);
	host_conn = NULL;

	SET_FLAG(flag_disconnected);
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	if (err != BT_SECURITY_ERR_SUCCESS) {
		TEST_FAIL("Security failed (err %d)", err);
		return;
	}

	SET_FLAG(flag_security_updated);
}

static struct bt_conn_cb host_conn_cb = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,
};

static bool ad_has_hids(struct net_buf_simple *ad)
{
	struct net_buf_simple copy;

	net_buf_simple_clone(ad, &copy);

	while (copy.len > 1U) {
		uint8_t len = net_buf_simple_pull_u8(&copy);
		uint8_t type;

		if (len == 0U || len > copy.len) {
			return false;
		}

		type = net_buf_simple_pull_u8(&copy);
		if (type == BT_DATA_UUID16_ALL || type == BT_DATA_UUID16_SOME) {
			for (uint8_t i = 0U; (i + 2U) <= (len - 1U); i += 2U) {
				if (sys_get_le16(&copy.data[i]) == BT_UUID_HIDS_VAL) {
					return true;
				}
			}
		}

		(void)net_buf_simple_pull_mem(&copy, len - 1U);
	}

	return false;
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	int err;

	if (host_conn != NULL) {
		return;
	}

	if (type != BT_HCI_ADV_IND && type != BT_HCI_ADV_DIRECT_IND) {
		return;
	}

	if (!ad_has_hids(ad)) {
		return;
	}

	err = bt_le_scan_stop();
	if (err != 0) {
		TEST_FAIL("Failed to stop scan (err %d)", err);
		return;
	}

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
				BT_LE_CONN_PARAM_DEFAULT, &host_conn);
	if (err != 0) {
		TEST_FAIL("Failed to create connection (err %d)", err);
	}
}

static void connect_to_device(void)
{
	int err;

	UNSET_FLAG(flag_connected);
	UNSET_FLAG(flag_security_updated);

	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, device_found);
	TEST_ASSERT(err == 0, "Failed to start scan: %d", err);

	WAIT_FOR_FLAG(flag_connected);
}

/* HOGP v1.1, Section 7.1: the HID Device uses the Peripheral Security Request
 * procedure to inform the Host of its security requirements, so the Host does
 * not have to initiate the procedure itself.
 */
static void wait_for_security(void)
{
	if (bt_conn_get_security(host_conn) >= BT_SECURITY_L2) {
		return;
	}

	WAIT_FOR_FLAG(flag_security_updated);
}

static void mtu_exchanged(struct bt_conn *conn, uint8_t err,
			  struct bt_gatt_exchange_params *params)
{
	ARG_UNUSED(params);

	TEST_ASSERT(err == 0U, "MTU exchange failed (att err 0x%02x)", err);
	SET_FLAG(flag_mtu_exchanged);
}

static void exchange_mtu(void)
{
	static struct bt_gatt_exchange_params params = {
		.func = mtu_exchanged,
	};
	int err;

	UNSET_FLAG(flag_mtu_exchanged);

	err = bt_gatt_exchange_mtu(host_conn, &params);
	TEST_ASSERT(err == 0, "Failed to exchange MTU: %d", err);

	WAIT_FOR_FLAG(flag_mtu_exchanged);

	TEST_ASSERT(bt_gatt_get_mtu(host_conn) > sizeof(test_report_map),
		    "ATT MTU %u too small for the Report Map",
		    bt_gatt_get_mtu(host_conn));
}

/* Discovery */

static void store_report_chrc(uint16_t value_handle)
{
	TEST_ASSERT(reports_cnt < ARRAY_SIZE(reports), "Too many Report characteristics");

	reports[reports_cnt].value_handle = value_handle;
	reports_cnt++;
}

static struct report_chrc *report_before(uint16_t handle)
{
	struct report_chrc *match = NULL;

	for (uint8_t i = 0U; i < reports_cnt; i++) {
		if (reports[i].value_handle < handle &&
		    (match == NULL || reports[i].value_handle > match->value_handle)) {
			match = &reports[i];
		}
	}

	return match;
}

static uint8_t discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	int err;

	if (attr == NULL) {
		if (params->type == BT_GATT_DISCOVER_CHARACTERISTIC) {
			/* Characteristics done, continue with the descriptors */
			params->type = BT_GATT_DISCOVER_DESCRIPTOR;
			params->uuid = NULL;
			params->start_handle = hids_start_handle + 1U;
			params->end_handle = hids_end_handle;

			err = bt_gatt_discover(conn, params);
			TEST_ASSERT(err == 0, "Descriptor discovery failed: %d", err);

			return BT_GATT_ITER_STOP;
		}

		SET_FLAG(flag_discover_complete);

		return BT_GATT_ITER_STOP;
	}

	switch (params->type) {
	case BT_GATT_DISCOVER_PRIMARY: {
		const struct bt_gatt_service_val *svc = attr->user_data;

		hids_start_handle = attr->handle;
		hids_end_handle = svc->end_handle;

		params->type = BT_GATT_DISCOVER_CHARACTERISTIC;
		params->uuid = NULL;
		params->start_handle = hids_start_handle + 1U;
		params->end_handle = hids_end_handle;

		err = bt_gatt_discover(conn, params);
		TEST_ASSERT(err == 0, "Characteristic discovery failed: %d", err);

		return BT_GATT_ITER_STOP;
	}
	case BT_GATT_DISCOVER_CHARACTERISTIC: {
		const struct bt_gatt_chrc *chrc = attr->user_data;

		if (bt_uuid_cmp(chrc->uuid, BT_UUID_HIDS_PROTOCOL_MODE) == 0) {
			protocol_mode_handle = chrc->value_handle;
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_HIDS_REPORT_MAP) == 0) {
			report_map_handle = chrc->value_handle;
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_HIDS_INFO) == 0) {
			hid_info_handle = chrc->value_handle;
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_HIDS_CTRL_POINT) == 0) {
			ctrl_point_handle = chrc->value_handle;
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_HIDS_REPORT) == 0) {
			store_report_chrc(chrc->value_handle);
		}

		return BT_GATT_ITER_CONTINUE;
	}
	case BT_GATT_DISCOVER_DESCRIPTOR: {
		struct report_chrc *report = report_before(attr->handle);

		if (report == NULL) {
			return BT_GATT_ITER_CONTINUE;
		}

		if (bt_uuid_cmp(attr->uuid, BT_UUID_GATT_CCC) == 0) {
			report->ccc_handle = attr->handle;
		} else if (bt_uuid_cmp(attr->uuid, BT_UUID_HIDS_REPORT_REF) == 0) {
			report->ref_handle = attr->handle;
		}

		return BT_GATT_ITER_CONTINUE;
	}
	default:
		return BT_GATT_ITER_CONTINUE;
	}
}

static void discover_hids(void)
{
	static struct bt_gatt_discover_params params;
	int err;

	reports_cnt = 0U;
	protocol_mode_handle = 0U;
	report_map_handle = 0U;
	hid_info_handle = 0U;
	ctrl_point_handle = 0U;
	(void)memset(reports, 0, sizeof(reports));

	params.uuid = BT_UUID_HIDS;
	params.func = discover_func;
	params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	params.type = BT_GATT_DISCOVER_PRIMARY;

	UNSET_FLAG(flag_discover_complete);

	err = bt_gatt_discover(host_conn, &params);
	TEST_ASSERT(err == 0, "Service discovery failed: %d", err);

	WAIT_FOR_FLAG(flag_discover_complete);

	TEST_ASSERT(protocol_mode_handle != 0U, "Protocol Mode not found");
	TEST_ASSERT(report_map_handle != 0U, "Report Map not found");
	TEST_ASSERT(hid_info_handle != 0U, "HID Information not found");
	TEST_ASSERT(ctrl_point_handle != 0U, "HID Control Point not found");
	TEST_ASSERT(reports_cnt == 3U, "Expected 3 Report characteristics, got %u", reports_cnt);
}

/* Reads and writes */

static uint8_t read_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_read_params *params,
		       const void *data, uint16_t length)
{
	att_err = err;

	if (data != NULL) {
		TEST_ASSERT(length <= sizeof(read_data), "Read value too long: %u", length);
		(void)memcpy(read_data, data, length);
		read_len = length;
	}

	SET_FLAG(flag_read_complete);

	return BT_GATT_ITER_STOP;
}

static void gatt_read(uint16_t handle)
{
	static struct bt_gatt_read_params params;
	int err;

	params.func = read_cb;
	params.handle_count = 1U;
	params.single.handle = handle;
	params.single.offset = 0U;

	read_len = 0U;
	att_err = 0U;
	UNSET_FLAG(flag_read_complete);

	err = bt_gatt_read(host_conn, &params);
	TEST_ASSERT(err == 0, "Read of handle %u failed: %d", handle, err);

	WAIT_FOR_FLAG(flag_read_complete);
	TEST_ASSERT(att_err == 0U, "Read of handle %u failed (att err 0x%02x)", handle, att_err);
}

static void write_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_write_params *params)
{
	ARG_UNUSED(params);

	att_err = err;
	SET_FLAG(flag_write_complete);
}

static void gatt_write(uint16_t handle, const uint8_t *data, uint16_t len)
{
	static struct bt_gatt_write_params params;
	int err;

	params.func = write_cb;
	params.handle = handle;
	params.offset = 0U;
	params.data = data;
	params.length = len;

	att_err = 0U;
	UNSET_FLAG(flag_write_complete);

	err = bt_gatt_write(host_conn, &params);
	TEST_ASSERT(err == 0, "Write to handle %u failed: %d", handle, err);

	WAIT_FOR_FLAG(flag_write_complete);
	TEST_ASSERT(att_err == 0U, "Write to handle %u failed (att err 0x%02x)", handle, att_err);
}

/* Notifications */

static uint8_t notify_cb(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
			 const void *data, uint16_t length)
{
	if (data == NULL) {
		return BT_GATT_ITER_STOP;
	}

	TEST_ASSERT(length <= sizeof(notified_data), "Notification too long: %u", length);

	(void)memcpy(notified_data, data, length);
	notified_len = length;

	SET_FLAG(flag_notified);

	return BT_GATT_ITER_CONTINUE;
}

static void subscribe_cb(struct bt_conn *conn, uint8_t err,
			 struct bt_gatt_subscribe_params *params)
{
	TEST_ASSERT(err == 0U, "Subscribe failed (att err 0x%02x)", err);
	SET_FLAG(flag_subscribed);
}

static struct bt_gatt_subscribe_params sub_params;

static void subscribe_input_report(const struct report_chrc *report)
{
	int err;

	sub_params.value_handle = report->value_handle;
	sub_params.ccc_handle = report->ccc_handle;
	sub_params.value = BT_GATT_CCC_NOTIFY;
	sub_params.notify = notify_cb;
	sub_params.subscribe = subscribe_cb;

	UNSET_FLAG(flag_subscribed);
	UNSET_FLAG(flag_notified);

	err = bt_gatt_subscribe(host_conn, &sub_params);
	TEST_ASSERT(err == 0, "Failed to subscribe: %d", err);

	WAIT_FOR_FLAG(flag_subscribed);
}

static void unsubscribe_input_report(void)
{
	int err;

	err = bt_gatt_unsubscribe(host_conn, &sub_params);
	TEST_ASSERT(err == 0, "Failed to unsubscribe: %d", err);
}

static void wait_for_input_report(void)
{
	WAIT_FOR_FLAG(flag_notified);
	UNSET_FLAG(flag_notified);

	TEST_ASSERT(notified_len == TEST_INPUT_REPORT_LEN,
		    "Expected a %u byte Input Report, got %u", TEST_INPUT_REPORT_LEN,
		    notified_len);
	TEST_ASSERT(memcmp(notified_data, test_input_data, TEST_INPUT_REPORT_LEN) == 0,
		    "Input Report payload mismatch");
}

/* Identify the reports through their Report Reference descriptors */
static void resolve_report_types(void)
{
	for (uint8_t i = 0U; i < reports_cnt; i++) {
		TEST_ASSERT(reports[i].ref_handle != 0U,
			    "Report at handle %u has no Report Reference descriptor",
			    reports[i].value_handle);

		gatt_read(reports[i].ref_handle);
		TEST_ASSERT(read_len == 2U, "Report Reference is %u bytes, expected 2", read_len);

		reports[i].id = read_data[0];
		reports[i].type = read_data[1];
	}

	for (uint8_t i = 0U; i < reports_cnt; i++) {
		const struct report_chrc *report = &reports[i];

		switch (report->type) {
		case BT_HID_REPORT_TYPE_INPUT:
			TEST_ASSERT(report->id == TEST_REPORT_ID_INPUT,
				    "Input Report ID %u, expected %u", report->id,
				    TEST_REPORT_ID_INPUT);
			TEST_ASSERT(report->ccc_handle != 0U,
				    "Input Report has no CCC descriptor");
			break;
		case BT_HID_REPORT_TYPE_OUTPUT:
			TEST_ASSERT(report->id == TEST_REPORT_ID_OUTPUT,
				    "Output Report ID %u, expected %u", report->id,
				    TEST_REPORT_ID_OUTPUT);
			break;
		case BT_HID_REPORT_TYPE_FEATURE:
			TEST_ASSERT(report->id == TEST_REPORT_ID_FEATURE,
				    "Feature Report ID %u, expected %u", report->id,
				    TEST_REPORT_ID_FEATURE);
			break;
		default:
			TEST_FAIL("Unexpected Report Type %u", report->type);
		}
	}
}

static void verify_static_values(void)
{
	const uint8_t expected_info[] = {
		TEST_BCD_HID & 0xFFU,
		(TEST_BCD_HID >> 8) & 0xFFU,
		TEST_COUNTRY_CODE,
		TEST_HID_FLAGS,
	};

	gatt_read(report_map_handle);
	TEST_ASSERT(read_len == sizeof(test_report_map),
		    "Report Map is %u bytes, expected %zu", read_len, sizeof(test_report_map));
	TEST_ASSERT(memcmp(read_data, test_report_map, sizeof(test_report_map)) == 0,
		    "Report Map content mismatch");

	gatt_read(hid_info_handle);
	TEST_ASSERT(read_len == sizeof(expected_info),
		    "HID Information is %u bytes, expected %zu", read_len, sizeof(expected_info));
	TEST_ASSERT(memcmp(read_data, expected_info, sizeof(expected_info)) == 0,
		    "HID Information content mismatch");

	gatt_read(protocol_mode_handle);
	TEST_ASSERT(read_len == 1U, "Protocol Mode is %u bytes, expected 1", read_len);
	TEST_ASSERT(read_data[0] == BT_HID_PROTOCOL_REPORT,
		    "Protocol Mode is %u, expected Report Protocol Mode", read_data[0]);
}

static void test_hogp_host_basic(void)
{
	const uint8_t output_report[] = {0xAA, 0xBB};
	const uint8_t ctrl_suspend = BT_HIDS_CTRL_SUSPEND;
	const uint8_t ctrl_exit_suspend = BT_HIDS_CTRL_EXIT_SUSPEND;
	const uint8_t protocol_report = BT_HID_PROTOCOL_REPORT;
	const struct report_chrc *input;
	const struct report_chrc *output;
	int err;

	TEST_START("HOGP Host basic test");

	TEST_ASSERT(bk_sync_init() == 0, "Failed to open sync channel");

	err = bt_conn_cb_register(&host_conn_cb);
	TEST_ASSERT(err == 0, "Failed to register connection callbacks: %d", err);

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "bt_enable failed: %d", err);

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		(void)settings_load();
	}

	connect_to_device();
	wait_for_security();
	exchange_mtu();

	discover_hids();
	resolve_report_types();
	verify_static_values();

	input = report_by_type(BT_HID_REPORT_TYPE_INPUT);
	output = report_by_type(BT_HID_REPORT_TYPE_OUTPUT);
	TEST_ASSERT(input != NULL && output != NULL, "Input or Output Report missing");

	/* The Device sends its first Input Report once we have subscribed */
	subscribe_input_report(input);
	bk_sync_wait();
	wait_for_input_report();

	/* SET_REPORT on the Output Report */
	gatt_write(output->value_handle, output_report, sizeof(output_report));
	bk_sync_send();

	/* Suspend and Exit Suspend through the HID Control Point */
	gatt_write(ctrl_point_handle, &ctrl_suspend, sizeof(ctrl_suspend));
	bk_sync_send();

	gatt_write(ctrl_point_handle, &ctrl_exit_suspend, sizeof(ctrl_exit_suspend));
	bk_sync_send();

	/* Protocol Mode write */
	gatt_write(protocol_mode_handle, &protocol_report, sizeof(protocol_report));
	bk_sync_send();

	/* GET_REPORT on the Input Report */
	gatt_read(input->value_handle);
	TEST_ASSERT(read_len == TEST_INPUT_REPORT_LEN,
		    "GET_REPORT returned %u bytes, expected %u", read_len,
		    TEST_INPUT_REPORT_LEN);
	TEST_ASSERT(memcmp(read_data, test_input_data, TEST_INPUT_REPORT_LEN) == 0,
		    "GET_REPORT payload mismatch");
	bk_sync_send();

	/* Wait for the Device to be done with the basic sequence */
	bk_sync_wait();

	/* Reconnection: the Device re-advertises and must serve the same HID
	 * Service again, with the Protocol Mode back at its default.
	 */
	unsubscribe_input_report();

	UNSET_FLAG(flag_disconnected);
	err = bt_conn_disconnect(host_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	TEST_ASSERT(err == 0, "Failed to disconnect: %d", err);
	WAIT_FOR_FLAG(flag_disconnected);

	connect_to_device();
	wait_for_security();
	exchange_mtu();

	discover_hids();
	resolve_report_types();

	gatt_read(protocol_mode_handle);
	TEST_ASSERT(read_data[0] == BT_HID_PROTOCOL_REPORT,
		    "Protocol Mode after reconnection is %u, expected Report Protocol Mode",
		    read_data[0]);

	input = report_by_type(BT_HID_REPORT_TYPE_INPUT);
	TEST_ASSERT(input != NULL, "Input Report missing after reconnection");

	subscribe_input_report(input);
	bk_sync_wait();
	wait_for_input_report();

	UNSET_FLAG(flag_disconnected);
	bk_sync_send();

	/* The Device unregisters the HID Service while we are connected */
	bk_sync_wait();
	TEST_ASSERT(!IS_FLAG_SET(flag_disconnected),
		    "The Device disconnected us when it unregistered the HID Service");

	err = bt_conn_disconnect(host_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	TEST_ASSERT(err == 0, "Failed to disconnect: %d", err);
	WAIT_FOR_FLAG(flag_disconnected);

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
