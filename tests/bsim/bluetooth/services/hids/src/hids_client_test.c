/*
 * Copyright (c) 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Peer of the HID Service server test, in the HID Host role. It is a plain
 * GATT client: it discovers the HID Service, checks the static values and the
 * descriptors, and drives the Report and HID Control Point characteristics
 * the way a Host would.
 *
 * It never initiates pairing, so the server has to request security itself.
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
#include <zephyr/net_buf.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>

#include "testlib/att.h"
#include "testlib/att_read.h"
#include "testlib/att_write.h"
#include "testlib/conn.h"
#include "testlib/scan.h"

#include "babblekit/flags.h"
#include "babblekit/sync.h"
#include "babblekit/testcase.h"
#include "bstests.h"

#include "common.h"

/* One Report characteristic of each type */
#define MAX_REPORTS 4U
/* Every characteristic of the service: the Reports above, Report Map, HID
 * Information and HID Control Point.
 */
#define MAX_CHRCS (MAX_REPORTS + 3U)

DEFINE_FLAG_STATIC(flag_security_updated);
DEFINE_FLAG_STATIC(flag_discover_complete);
DEFINE_FLAG_STATIC(flag_subscribed);
DEFINE_FLAG_STATIC(flag_notified);

struct report_chrc {
	uint16_t value_handle;
	uint16_t ccc_handle;
	uint16_t ref_handle;
	uint8_t id;
	uint8_t type;
};

/* Every discovered characteristic, in handle order, so that a descriptor can be
 * attributed to the characteristic that owns it: the one with the greatest value
 * handle below the descriptor.
 */
struct chrc_entry {
	uint16_t value_handle;
	uint16_t uuid_val;
};

static struct bt_conn *client_conn;

static uint16_t hids_start_handle;
static uint16_t hids_end_handle;
static uint16_t report_map_handle;
static uint16_t hid_info_handle;
static uint16_t ctrl_point_handle;
static struct report_chrc reports[MAX_REPORTS];
static uint8_t reports_cnt;
static struct chrc_entry chrcs[MAX_CHRCS];
static uint8_t chrcs_cnt;

/* Value of the last read */
NET_BUF_SIMPLE_DEFINE_STATIC(read_buf, MAX(sizeof(test_report_map), TEST_LONG_REPORT_LEN));
static uint8_t notified_data[TEST_INPUT_REPORT_LEN];
static uint16_t notified_len;

static const struct report_chrc *report_by_id(uint8_t id)
{
	for (uint8_t i = 0U; i < reports_cnt; i++) {
		if (reports[i].id == id) {
			return &reports[i];
		}
	}

	return NULL;
}

static const struct report_chrc *report_by_type(uint8_t type)
{
	for (uint8_t i = 0U; i < reports_cnt; i++) {
		if (reports[i].type == type) {
			return &reports[i];
		}
	}

	return NULL;
}

/* Connection handling */

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	ARG_UNUSED(conn);

	TEST_ASSERT(err == BT_SECURITY_ERR_SUCCESS, "Security failed (err %d)", err);
	TEST_ASSERT(level >= BT_SECURITY_L2, "Security level is %u", level);

	SET_FLAG(flag_security_updated);
}

static struct bt_conn_cb client_conn_cb = {
	.security_changed = security_changed,
};

static void connect_to_server(void)
{
	bt_addr_le_t addr;
	int err;

	UNSET_FLAG(flag_security_updated);

	err = bt_testlib_scan_find_name(&addr, CONFIG_BT_DEVICE_NAME);
	TEST_ASSERT(err == 0, "Failed to find the server: %d", err);

	err = bt_testlib_connect(&addr, &client_conn);
	TEST_ASSERT(err == 0, "Failed to connect: %d", err);

	/* The HID Device requests security itself, so all the Host has to do is
	 * wait for it.
	 */
	WAIT_FOR_FLAG(flag_security_updated);
}

static void disconnect_from_server(void)
{
	int err;

	err = bt_testlib_disconnect(&client_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	TEST_ASSERT(err == 0, "Failed to disconnect: %d", err);
}

static bool is_connected(void)
{
	struct bt_conn_info info;

	return bt_conn_get_info(client_conn, &info) == 0 && info.state == BT_CONN_STATE_CONNECTED;
}

/* The Report Map is read in a single ATT read, so the MTU has to fit it */
static void exchange_mtu(void)
{
	int err;

	err = bt_testlib_att_exchange_mtu(client_conn);
	TEST_ASSERT(err == 0, "Failed to exchange MTU: %d", err);

	TEST_ASSERT(bt_gatt_get_mtu(client_conn) > sizeof(test_report_map),
		    "ATT MTU %u is too small for the Report Map", bt_gatt_get_mtu(client_conn));
}

/* Discovery */

static void store_report_chrc(uint16_t value_handle)
{
	TEST_ASSERT(reports_cnt < ARRAY_SIZE(reports), "Too many Report characteristics");

	reports[reports_cnt].value_handle = value_handle;
	reports_cnt++;
}

/* The characteristic a descriptor belongs to */
static const struct chrc_entry *chrc_before(uint16_t handle)
{
	const struct chrc_entry *match = NULL;

	for (uint8_t i = 0U; i < chrcs_cnt; i++) {
		if (chrcs[i].value_handle < handle &&
		    (match == NULL || chrcs[i].value_handle > match->value_handle)) {
			match = &chrcs[i];
		}
	}

	return match;
}

/* The Report characteristic a descriptor belongs to, NULL when the descriptor
 * belongs to a characteristic that is not a Report.
 */
static struct report_chrc *report_owning(uint16_t handle)
{
	const struct chrc_entry *owner = chrc_before(handle);

	if (owner == NULL || owner->uuid_val != BT_UUID_HIDS_REPORT_VAL) {
		return NULL;
	}

	for (uint8_t i = 0U; i < reports_cnt; i++) {
		if (reports[i].value_handle == owner->value_handle) {
			return &reports[i];
		}
	}

	return NULL;
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

		TEST_ASSERT(chrcs_cnt < ARRAY_SIZE(chrcs), "Too many characteristics");
		TEST_ASSERT(chrc->uuid->type == BT_UUID_TYPE_16,
			    "Characteristic with a UUID that is not 16 bit");
		chrcs[chrcs_cnt].value_handle = chrc->value_handle;
		chrcs[chrcs_cnt].uuid_val = BT_UUID_16(chrc->uuid)->val;
		chrcs_cnt++;

		if (bt_uuid_cmp(chrc->uuid, BT_UUID_HIDS_REPORT_MAP) == 0) {
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
		struct report_chrc *report = report_owning(attr->handle);

		if (report != NULL) {
			if (bt_uuid_cmp(attr->uuid, BT_UUID_GATT_CCC) == 0) {
				report->ccc_handle = attr->handle;
			} else if (bt_uuid_cmp(attr->uuid, BT_UUID_HIDS_REPORT_REF) == 0) {
				report->ref_handle = attr->handle;
			}
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
	chrcs_cnt = 0U;
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

	err = bt_gatt_discover(client_conn, &params);
	TEST_ASSERT(err == 0, "Service discovery failed: %d", err);

	WAIT_FOR_FLAG(flag_discover_complete);

	TEST_ASSERT(report_map_handle != 0U, "Report Map not found");
	TEST_ASSERT(hid_info_handle != 0U, "HID Information not found");
	TEST_ASSERT(ctrl_point_handle != 0U, "HID Control Point not found");
	TEST_ASSERT(reports_cnt == MAX_REPORTS, "Expected %u Report characteristics, got %u",
		    MAX_REPORTS, reports_cnt);
}

/* Reads and writes. Each returns the ATT error, 0 on success. */

static uint8_t gatt_read_offset(uint16_t handle, uint16_t offset, bool long_read)
{
	int err;

	net_buf_simple_reset(&read_buf);

	if (long_read) {
		err = bt_testlib_gatt_long_read(&read_buf, NULL, NULL, client_conn,
						BT_ATT_CHAN_OPT_NONE, handle, offset);
	} else {
		err = bt_testlib_att_read_by_handle_sync(&read_buf, NULL, NULL, client_conn,
							 BT_ATT_CHAN_OPT_NONE, handle, offset);
	}

	TEST_ASSERT(err >= 0, "Read of handle %u failed: %d", handle, err);

	return (uint8_t)err;
}

static void gatt_read(uint16_t handle)
{
	uint8_t att_err = gatt_read_offset(handle, 0U, false);

	TEST_ASSERT(att_err == 0U, "Read of handle %u failed (att err 0x%02x)", handle, att_err);
}

/* Read the whole value with the Read Long Characteristic Value sub-procedure */
static void gatt_read_long(uint16_t handle)
{
	uint8_t att_err = gatt_read_offset(handle, 0U, true);

	TEST_ASSERT(att_err == 0U, "Long read of handle %u failed (att err 0x%02x)", handle,
		    att_err);
}

/* A single ATT Read Blob Request, without the read that would precede it in a
 * long read
 */
static void gatt_read_blob(uint16_t handle, uint16_t offset)
{
	uint8_t att_err = gatt_read_offset(handle, offset, false);

	TEST_ASSERT(att_err == 0U, "Read Blob of handle %u at offset %u failed (att err 0x%02x)",
		    handle, offset, att_err);
}

static void gatt_read_blob_expect_err(uint16_t handle, uint16_t offset, uint8_t expected)
{
	uint8_t att_err = gatt_read_offset(handle, offset, false);

	TEST_ASSERT(att_err == expected,
		    "Read Blob of handle %u at offset %u: att err 0x%02x, expected 0x%02x", handle,
		    offset, att_err, expected);
}

static uint8_t gatt_write_raw(uint16_t handle, const uint8_t *data, uint16_t len)
{
	int err;

	err = bt_testlib_att_write(client_conn, BT_ATT_CHAN_OPT_NONE, handle, data, len);
	TEST_ASSERT(err >= 0, "Write to handle %u failed: %d", handle, err);

	return (uint8_t)err;
}

static void gatt_write(uint16_t handle, const uint8_t *data, uint16_t len)
{
	uint8_t att_err = gatt_write_raw(handle, data, len);

	TEST_ASSERT(att_err == 0U, "Write to handle %u failed (att err 0x%02x)", handle, att_err);
}

static void gatt_write_expect_err(uint16_t handle, const uint8_t *data, uint16_t len,
				  uint8_t expected)
{
	uint8_t att_err = gatt_write_raw(handle, data, len);

	TEST_ASSERT(att_err == expected, "Write to handle %u: att err 0x%02x, expected 0x%02x",
		    handle, att_err, expected);
}

/* The HID Control Point is written without a response */
static void gatt_write_cmd(uint16_t handle, uint8_t value)
{
	int err;

	err = bt_gatt_write_without_response(client_conn, handle, &value, sizeof(value), false);
	TEST_ASSERT(err == 0, "Write command to handle %u failed: %d", handle, err);
}

/* Notifications */

static uint8_t notify_cb(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
			 const void *data, uint16_t length)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(params);

	if (data == NULL) {
		return BT_GATT_ITER_STOP;
	}

	TEST_ASSERT(length <= sizeof(notified_data), "Notification of %u bytes", length);

	(void)memcpy(notified_data, data, length);
	notified_len = length;

	SET_FLAG(flag_notified);

	return BT_GATT_ITER_CONTINUE;
}

static void subscribe_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_subscribe_params *params)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(params);

	TEST_ASSERT(err == 0U, "Subscribe failed (att err 0x%02x)", err);
	SET_FLAG(flag_subscribed);
}

static struct bt_gatt_subscribe_params sub_params;

/* An Input Report supports notifications only, so its Client Characteristic
 * Configuration descriptor refuses the indication bit. A Reserved for Future
 * Use bit is processed as if it were zero, so a write that only sets one is
 * accepted. Run before subscribing, so that the last write leaves the
 * descriptor disabled.
 */
static void test_ccc_validation(uint16_t ccc_handle)
{
	static const uint8_t indicate[] = {BT_GATT_CCC_INDICATE & 0xFFU,
					   BT_GATT_CCC_INDICATE >> 8};
	/* The indication bit and a Reserved bit */
	static const uint8_t indicate_reserved[] = {0x06U, 0x00U};
	static const uint8_t reserved[] = {0x04U, 0x00U};
	static const uint8_t disabled[] = {0x00U, 0x00U};

	gatt_write_expect_err(ccc_handle, indicate, sizeof(indicate),
			      BT_ATT_ERR_CCC_IMPROPER_CONF);

	/* Masking the Reserved bits does not make the indication bit acceptable */
	gatt_write_expect_err(ccc_handle, indicate_reserved, sizeof(indicate_reserved),
			      BT_ATT_ERR_CCC_IMPROPER_CONF);

	/* A Reserved bit on its own is accepted and leaves the descriptor
	 * disabled
	 */
	gatt_write(ccc_handle, reserved, sizeof(reserved));

	/* Unsubscribing stays allowed */
	gatt_write(ccc_handle, disabled, sizeof(disabled));
}

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

	err = bt_gatt_subscribe(client_conn, &sub_params);
	TEST_ASSERT(err == 0, "Failed to subscribe: %d", err);

	WAIT_FOR_FLAG(flag_subscribed);
}

static void wait_for_input_report(void)
{
	WAIT_FOR_FLAG(flag_notified);
	UNSET_FLAG(flag_notified);

	TEST_ASSERT(notified_len == TEST_INPUT_REPORT_LEN, "Input Report of %u bytes, expected %u",
		    notified_len, TEST_INPUT_REPORT_LEN);
	TEST_ASSERT(memcmp(notified_data, test_input_data, TEST_INPUT_REPORT_LEN) == 0,
		    "Input Report payload mismatch");
}

/* Every Report characteristic has a Report Reference descriptor, and only an
 * Input Report has a Client Characteristic Configuration descriptor.
 */
static void resolve_reports(void)
{
	for (uint8_t i = 0U; i < reports_cnt; i++) {
		TEST_ASSERT(reports[i].ref_handle != 0U,
			    "Report at handle %u has no Report Reference descriptor",
			    reports[i].value_handle);

		gatt_read(reports[i].ref_handle);
		TEST_ASSERT(read_buf.len == 2U, "Report Reference of %u bytes, expected 2",
			    read_buf.len);

		reports[i].id = read_buf.data[0];
		reports[i].type = read_buf.data[1];
	}

	for (uint8_t i = 0U; i < reports_cnt; i++) {
		const struct report_chrc *report = &reports[i];

		switch (report->type) {
		case BT_HIDS_REPORT_TYPE_INPUT:
			TEST_ASSERT(report->id == TEST_REPORT_ID_INPUT,
				    "Input Report ID %u, expected %u", report->id,
				    TEST_REPORT_ID_INPUT);
			TEST_ASSERT(report->ccc_handle != 0U,
				    "The Input Report has no CCC descriptor");
			break;
		case BT_HIDS_REPORT_TYPE_OUTPUT:
			TEST_ASSERT(report->id == TEST_REPORT_ID_OUTPUT,
				    "Output Report ID %u, expected %u", report->id,
				    TEST_REPORT_ID_OUTPUT);
			TEST_ASSERT(report->ccc_handle == 0U,
				    "The Output Report has a CCC descriptor");
			break;
		case BT_HIDS_REPORT_TYPE_FEATURE:
			TEST_ASSERT(report->id == TEST_REPORT_ID_FEATURE ||
					    report->id == TEST_REPORT_ID_FEATURE_LONG,
				    "Feature Report ID %u, expected %u or %u", report->id,
				    TEST_REPORT_ID_FEATURE, TEST_REPORT_ID_FEATURE_LONG);
			TEST_ASSERT(report->ccc_handle == 0U,
				    "The Feature Report has a CCC descriptor");
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
	TEST_ASSERT(read_buf.len == sizeof(test_report_map), "Report Map of %u bytes, expected %zu",
		    read_buf.len, sizeof(test_report_map));
	TEST_ASSERT(memcmp(read_buf.data, test_report_map, sizeof(test_report_map)) == 0,
		    "Report Map content mismatch");

	gatt_read(hid_info_handle);
	TEST_ASSERT(read_buf.len == sizeof(expected_info),
		    "HID Information of %u bytes, expected %zu", read_buf.len,
		    sizeof(expected_info));
	TEST_ASSERT(memcmp(read_buf.data, expected_info, sizeof(expected_info)) == 0,
		    "HID Information content mismatch");
}

/* The HID Service holds the value a Host reads, so a Read Long assembles one
 * value rather than several unrelated ones, and a read that follows returns the
 * value the server set in the meantime.
 */
static void verify_long_report_reads(void)
{
	const struct report_chrc *long_feature = report_by_id(TEST_REPORT_ID_FEATURE_LONG);
	uint8_t first_seq;

	TEST_ASSERT(long_feature != NULL, "The long Feature Report was not discovered");

	/* One logical read, several ATT Read Blob Requests, one value */
	gatt_read_long(long_feature->value_handle);
	TEST_ASSERT(read_buf.len == TEST_LONG_REPORT_LEN,
		    "Long read returned %u bytes, expected %u", read_buf.len, TEST_LONG_REPORT_LEN);

	first_seq = read_buf.data[0];
	for (uint16_t i = 1U; i < read_buf.len; i++) {
		TEST_ASSERT(read_buf.data[i] == first_seq,
			    "Octet %u of the long read is 0x%02x, expected 0x%02x", i,
			    read_buf.data[i], first_seq);
	}

	/* A Read Blob Request inside the value returns the same value */
	gatt_read_blob(long_feature->value_handle, TEST_LONG_REPORT_LEN / 4U);
	TEST_ASSERT(read_buf.len > 0U, "Read Blob returned nothing");
	TEST_ASSERT(read_buf.data[0] == first_seq, "Read Blob returned 0x%02x, expected 0x%02x",
		    read_buf.data[0], first_seq);

	/* Offset at the end of the value: an empty response, not an error */
	gatt_read_blob(long_feature->value_handle, TEST_LONG_REPORT_LEN);
	TEST_ASSERT(read_buf.len == 0U, "Read Blob at the end of the value returned %u bytes",
		    read_buf.len);

	/* Offset past the value */
	gatt_read_blob_expect_err(long_feature->value_handle, TEST_LONG_REPORT_LEN + 1U,
				  BT_ATT_ERR_INVALID_OFFSET);

	/* The server replaces the value, so the next read returns the new one */
	bk_sync_send();
	bk_sync_wait();

	gatt_read_long(long_feature->value_handle);
	TEST_ASSERT(read_buf.len == TEST_LONG_REPORT_LEN,
		    "Long read returned %u bytes, expected %u", read_buf.len, TEST_LONG_REPORT_LEN);
	TEST_ASSERT(read_buf.data[0] != first_seq,
		    "The read returned 0x%02x, the value from before the server replaced it",
		    read_buf.data[0]);
	for (uint16_t i = 1U; i < read_buf.len; i++) {
		TEST_ASSERT(read_buf.data[i] == read_buf.data[0],
			    "Octet %u of the long read is 0x%02x, expected 0x%02x", i,
			    read_buf.data[i], read_buf.data[0]);
	}
}

static void test_hids_client(void)
{
	const struct report_chrc *input;
	const struct report_chrc *output;
	const struct report_chrc *feature;
	int err;

	TEST_START("HID Service client test");

	TEST_ASSERT(bk_sync_init() == 0, "Failed to open the sync channel");

	err = bt_conn_cb_register(&client_conn_cb);
	TEST_ASSERT(err == 0, "Failed to register the connection callbacks: %d", err);

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Failed to enable Bluetooth: %d", err);

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		(void)settings_load();
	}

	connect_to_server();
	exchange_mtu();

	discover_hids();
	resolve_reports();
	verify_static_values();

	input = report_by_type(BT_HIDS_REPORT_TYPE_INPUT);
	output = report_by_type(BT_HIDS_REPORT_TYPE_OUTPUT);
	feature = report_by_id(TEST_REPORT_ID_FEATURE);
	TEST_ASSERT(input != NULL && output != NULL && feature != NULL, "A Report is missing");

	/* The server notifies its first Input Report once we have subscribed */
	test_ccc_validation(input->ccc_handle);
	subscribe_input_report(input);
	bk_sync_send();
	bk_sync_wait();
	wait_for_input_report();

	/* SET_REPORT of the Output Report */
	gatt_write(output->value_handle, test_output_data, TEST_OUTPUT_REPORT_LEN);
	bk_sync_send();
	bk_sync_wait();

	/* SET_REPORT of the Feature Report */
	gatt_write(feature->value_handle, test_feature_data, TEST_FEATURE_REPORT_LEN);
	bk_sync_send();
	bk_sync_wait();

	/* Suspend and Exit Suspend through the HID Control Point */
	gatt_write_cmd(ctrl_point_handle, BT_HIDS_CTRL_SUSPEND);
	bk_sync_send();
	bk_sync_wait();

	gatt_write_cmd(ctrl_point_handle, BT_HIDS_CTRL_EXIT_SUSPEND);
	bk_sync_send();
	bk_sync_wait();

	/* GET_REPORT of the Input Report */
	gatt_read(input->value_handle);
	TEST_ASSERT(read_buf.len == TEST_INPUT_REPORT_LEN,
		    "GET_REPORT returned %u bytes, expected %u", read_buf.len,
		    TEST_INPUT_REPORT_LEN);
	TEST_ASSERT(memcmp(read_buf.data, test_input_data, TEST_INPUT_REPORT_LEN) == 0,
		    "GET_REPORT payload mismatch");
	bk_sync_send();
	bk_sync_wait();

	/* GET_REPORT of the Feature Report returns what we wrote to it */
	gatt_read(feature->value_handle);
	TEST_ASSERT(read_buf.len == TEST_FEATURE_REPORT_LEN,
		    "GET_REPORT returned %u bytes, expected %u", read_buf.len,
		    TEST_FEATURE_REPORT_LEN);
	TEST_ASSERT(memcmp(read_buf.data, test_feature_data, TEST_FEATURE_REPORT_LEN) == 0,
		    "GET_REPORT payload mismatch");
	bk_sync_send();
	bk_sync_wait();

	verify_long_report_reads();
	bk_sync_send();
	bk_sync_wait();

	/* Reconnect: the server serves the same HID Service again */
	err = bt_gatt_unsubscribe(client_conn, &sub_params);
	TEST_ASSERT(err == 0, "Failed to unsubscribe: %d", err);

	disconnect_from_server();

	connect_to_server();
	exchange_mtu();

	discover_hids();
	resolve_reports();

	input = report_by_type(BT_HIDS_REPORT_TYPE_INPUT);
	TEST_ASSERT(input != NULL, "The Input Report is missing after reconnecting");

	subscribe_input_report(input);
	bk_sync_send();
	bk_sync_wait();
	wait_for_input_report();

	/* The server unregisters the HID Service while we are connected */
	bk_sync_send();
	bk_sync_wait();
	TEST_ASSERT(is_connected(),
		    "The server disconnected us when it unregistered the HID Service");

	/* The server registers the HID Service again, which serves it at the
	 * same handles
	 */
	bk_sync_wait();
	verify_static_values();
	bk_sync_send();

	disconnect_from_server();

	TEST_PASS("HID Service client test passed");
}

static struct bst_test_instance test_hids_client_defs[] = {
	{
		.test_id = "hids_client",
		.test_descr = "HID Host side of the two device HID Service test",
		.test_main_f = test_hids_client,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_hids_client_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_hids_client_defs);
}
