/*
 * Copyright (c) 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Peer of the HID Service server test, in the HID Host role. It is a plain
 * GATT client: it discovers the HID Service, checks the static values and the
 * descriptors, and drives the Report, Protocol Mode and HID Control Point
 * characteristics the way a Host would.
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
#include <zephyr/settings/settings.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>

#include "babblekit/flags.h"
#include "babblekit/sync.h"
#include "babblekit/testcase.h"
#include "bstests.h"

#include "common.h"

/* One Report characteristic of each type */
#define MAX_REPORTS 4
/* Every characteristic of the service: the Reports above, Protocol Mode, Report
 * Map, HID Information, HID Control Point and the three Boot Reports.
 */
#define MAX_CHRCS (MAX_REPORTS + 7)

DEFINE_FLAG_STATIC(flag_connected);
DEFINE_FLAG_STATIC(flag_disconnected);
DEFINE_FLAG_STATIC(flag_security_updated);
DEFINE_FLAG_STATIC(flag_discover_complete);
DEFINE_FLAG_STATIC(flag_mtu_exchanged);
DEFINE_FLAG_STATIC(flag_write_complete);
DEFINE_FLAG_STATIC(flag_read_complete);
DEFINE_FLAG_STATIC(flag_subscribed);
DEFINE_FLAG_STATIC(flag_notified);
DEFINE_FLAG_STATIC(flag_boot_notified);

struct report_chrc {
	uint16_t value_handle;
	uint16_t ccc_handle;
	uint16_t ref_handle;
	uint8_t id;
	uint8_t type;
};

/* The three Boot Report characteristics. Only the two Boot Input Reports have a
 * CCC descriptor, and none of them has a Report Reference descriptor: their
 * format is defined by the USB HID Specification rather than by the Report Map.
 */
struct boot_chrc {
	uint16_t value_handle;
	uint16_t ccc_handle;
	uint8_t properties;
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
static uint16_t protocol_mode_handle;
static uint16_t report_map_handle;
static uint16_t hid_info_handle;
static uint16_t ctrl_point_handle;
static struct report_chrc reports[MAX_REPORTS];
static uint8_t reports_cnt;
static struct boot_chrc boot_kb_in;
static struct boot_chrc boot_kb_out;
static struct boot_chrc boot_mouse_in;
static struct chrc_entry chrcs[MAX_CHRCS];
static uint8_t chrcs_cnt;

static uint8_t att_err;
static uint8_t read_data[MAX(sizeof(test_report_map), TEST_LONG_REPORT_LEN)];
static uint16_t read_len;
static uint8_t notified_data[TEST_INPUT_REPORT_LEN];
static uint16_t notified_len;
static uint8_t boot_notified_data[BT_HIDS_BOOT_KB_IN_LEN];
static uint16_t boot_notified_len;

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

static void connected(struct bt_conn *conn, uint8_t err)
{
	TEST_ASSERT(err == 0U, "Failed to connect (err 0x%02x)", err);

	SET_FLAG(flag_connected);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	ARG_UNUSED(reason);

	if (conn != client_conn) {
		return;
	}

	bt_conn_unref(client_conn);
	client_conn = NULL;

	SET_FLAG(flag_disconnected);
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	ARG_UNUSED(conn);

	TEST_ASSERT(err == BT_SECURITY_ERR_SUCCESS, "Security failed (err %d)", err);
	TEST_ASSERT(level >= BT_SECURITY_L2, "Security level is %u", level);

	SET_FLAG(flag_security_updated);
}

static struct bt_conn_cb client_conn_cb = {
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

	ARG_UNUSED(rssi);

	if (client_conn != NULL) {
		return;
	}

	if (type != BT_HCI_ADV_IND && type != BT_HCI_ADV_DIRECT_IND) {
		return;
	}

	if (!ad_has_hids(ad)) {
		return;
	}

	err = bt_le_scan_stop();
	TEST_ASSERT(err == 0, "Failed to stop scanning: %d", err);

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT,
				&client_conn);
	TEST_ASSERT(err == 0, "Failed to create a connection: %d", err);
}

static void connect_to_server(void)
{
	int err;

	UNSET_FLAG(flag_connected);
	UNSET_FLAG(flag_security_updated);

	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, device_found);
	TEST_ASSERT(err == 0, "Failed to start scanning: %d", err);

	WAIT_FOR_FLAG(flag_connected);

	/* The HID Device requests security itself, so all the Host has to do is
	 * wait for it.
	 */
	WAIT_FOR_FLAG(flag_security_updated);
}

static void mtu_exchanged(struct bt_conn *conn, uint8_t err, struct bt_gatt_exchange_params *params)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(params);

	TEST_ASSERT(err == 0U, "MTU exchange failed (att err 0x%02x)", err);
	SET_FLAG(flag_mtu_exchanged);
}

/* The Report Map is read in a single ATT read, so the MTU has to fit it */
static void exchange_mtu(void)
{
	static struct bt_gatt_exchange_params params = {
		.func = mtu_exchanged,
	};
	int err;

	UNSET_FLAG(flag_mtu_exchanged);

	err = bt_gatt_exchange_mtu(client_conn, &params);
	TEST_ASSERT(err == 0, "Failed to exchange MTU: %d", err);

	WAIT_FOR_FLAG(flag_mtu_exchanged);

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

/* The Boot Report characteristic a descriptor belongs to, NULL when the
 * descriptor belongs to a characteristic that is not a Boot Report.
 */
static struct boot_chrc *boot_owning(uint16_t handle)
{
	const struct chrc_entry *owner = chrc_before(handle);

	if (owner == NULL) {
		return NULL;
	}

	switch (owner->uuid_val) {
	case BT_UUID_HIDS_BOOT_KB_IN_REPORT_VAL:
		return &boot_kb_in;
	case BT_UUID_HIDS_BOOT_KB_OUT_REPORT_VAL:
		return &boot_kb_out;
	case BT_UUID_HIDS_BOOT_MOUSE_IN_REPORT_VAL:
		return &boot_mouse_in;
	default:
		return NULL;
	}
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
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_HIDS_BOOT_KB_IN_REPORT) == 0) {
			boot_kb_in.value_handle = chrc->value_handle;
			boot_kb_in.properties = chrc->properties;
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_HIDS_BOOT_KB_OUT_REPORT) == 0) {
			boot_kb_out.value_handle = chrc->value_handle;
			boot_kb_out.properties = chrc->properties;
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_HIDS_BOOT_MOUSE_IN_REPORT) == 0) {
			boot_mouse_in.value_handle = chrc->value_handle;
			boot_mouse_in.properties = chrc->properties;
		}

		return BT_GATT_ITER_CONTINUE;
	}
	case BT_GATT_DISCOVER_DESCRIPTOR: {
		struct report_chrc *report = report_owning(attr->handle);
		struct boot_chrc *boot;

		if (report != NULL) {
			if (bt_uuid_cmp(attr->uuid, BT_UUID_GATT_CCC) == 0) {
				report->ccc_handle = attr->handle;
			} else if (bt_uuid_cmp(attr->uuid, BT_UUID_HIDS_REPORT_REF) == 0) {
				report->ref_handle = attr->handle;
			}

			return BT_GATT_ITER_CONTINUE;
		}

		boot = boot_owning(attr->handle);
		if (boot != NULL && bt_uuid_cmp(attr->uuid, BT_UUID_GATT_CCC) == 0) {
			boot->ccc_handle = attr->handle;
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
	protocol_mode_handle = 0U;
	report_map_handle = 0U;
	hid_info_handle = 0U;
	ctrl_point_handle = 0U;
	(void)memset(reports, 0, sizeof(reports));
	(void)memset(&boot_kb_in, 0, sizeof(boot_kb_in));
	(void)memset(&boot_kb_out, 0, sizeof(boot_kb_out));
	(void)memset(&boot_mouse_in, 0, sizeof(boot_mouse_in));

	params.uuid = BT_UUID_HIDS;
	params.func = discover_func;
	params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	params.type = BT_GATT_DISCOVER_PRIMARY;

	UNSET_FLAG(flag_discover_complete);

	err = bt_gatt_discover(client_conn, &params);
	TEST_ASSERT(err == 0, "Service discovery failed: %d", err);

	WAIT_FOR_FLAG(flag_discover_complete);

	TEST_ASSERT(protocol_mode_handle != 0U, "Protocol Mode not found");
	TEST_ASSERT(report_map_handle != 0U, "Report Map not found");
	TEST_ASSERT(hid_info_handle != 0U, "HID Information not found");
	TEST_ASSERT(ctrl_point_handle != 0U, "HID Control Point not found");
	TEST_ASSERT(reports_cnt == MAX_REPORTS, "Expected %u Report characteristics, got %u",
		    MAX_REPORTS, reports_cnt);
}

/* Reads and writes */

static uint8_t read_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_read_params *params,
		       const void *data, uint16_t length)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(params);

	att_err = err;

	if (data != NULL) {
		TEST_ASSERT(length <= sizeof(read_data), "Read value of %u bytes", length);
		(void)memcpy(read_data, data, length);
		read_len = length;
	}

	SET_FLAG(flag_read_complete);

	return BT_GATT_ITER_STOP;
}

/* Appends every blob, so read_data holds the whole value once the read
 * completes. Returning BT_GATT_ITER_CONTINUE makes the stack carry on with ATT
 * Read Blob Requests, which is the Read Long Characteristic Value
 * sub-procedure.
 */
static uint8_t read_long_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_read_params *params,
			    const void *data, uint16_t length)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(params);

	att_err = err;

	if (data == NULL) {
		SET_FLAG(flag_read_complete);
		return BT_GATT_ITER_STOP;
	}

	TEST_ASSERT((read_len + length) <= sizeof(read_data), "Read value of %u bytes",
		    read_len + length);
	(void)memcpy(&read_data[read_len], data, length);
	read_len += length;

	return BT_GATT_ITER_CONTINUE;
}

static void gatt_read_offset(uint16_t handle, uint16_t offset, bt_gatt_read_func_t func)
{
	static struct bt_gatt_read_params params;
	int err;

	params.func = func;
	params.handle_count = 1U;
	params.single.handle = handle;
	params.single.offset = offset;

	read_len = 0U;
	att_err = 0U;
	UNSET_FLAG(flag_read_complete);

	err = bt_gatt_read(client_conn, &params);
	TEST_ASSERT(err == 0, "Read of handle %u failed: %d", handle, err);

	WAIT_FOR_FLAG(flag_read_complete);
}

static void gatt_read(uint16_t handle)
{
	gatt_read_offset(handle, 0U, read_cb);
	TEST_ASSERT(att_err == 0U, "Read of handle %u failed (att err 0x%02x)", handle, att_err);
}

/* Read the whole value with the Read Long Characteristic Value sub-procedure */
static void gatt_read_long(uint16_t handle)
{
	gatt_read_offset(handle, 0U, read_long_cb);
	TEST_ASSERT(att_err == 0U, "Long read of handle %u failed (att err 0x%02x)", handle,
		    att_err);
}

/* A single ATT Read Blob Request, without the read that would precede it in a
 * long read
 */
static void gatt_read_blob(uint16_t handle, uint16_t offset)
{
	gatt_read_offset(handle, offset, read_cb);
	TEST_ASSERT(att_err == 0U, "Read Blob of handle %u at offset %u failed (att err 0x%02x)",
		    handle, offset, att_err);
}

static void gatt_read_blob_expect_err(uint16_t handle, uint16_t offset, uint8_t expected)
{
	gatt_read_offset(handle, offset, read_cb);
	TEST_ASSERT(att_err == expected,
		    "Read Blob of handle %u at offset %u: att err 0x%02x, expected 0x%02x", handle,
		    offset, att_err, expected);
}

static void write_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_write_params *params)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(params);

	att_err = err;
	SET_FLAG(flag_write_complete);
}

static void gatt_write_raw(uint16_t handle, const uint8_t *data, uint16_t len)
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

	err = bt_gatt_write(client_conn, &params);
	TEST_ASSERT(err == 0, "Write to handle %u failed: %d", handle, err);

	WAIT_FOR_FLAG(flag_write_complete);
}

static void gatt_write(uint16_t handle, const uint8_t *data, uint16_t len)
{
	gatt_write_raw(handle, data, len);
	TEST_ASSERT(att_err == 0U, "Write to handle %u failed (att err 0x%02x)", handle, att_err);
}

static void gatt_write_expect_err(uint16_t handle, const uint8_t *data, uint16_t len,
				  uint8_t expected)
{
	gatt_write_raw(handle, data, len);
	TEST_ASSERT(att_err == expected, "Write to handle %u: att err 0x%02x, expected 0x%02x",
		    handle, att_err, expected);
}

/* The Protocol Mode and the HID Control Point are written without a response */
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
			      BT_ATT_ERR_VALUE_NOT_ALLOWED);

	/* Masking the Reserved bits does not make the indication bit acceptable */
	gatt_write_expect_err(ccc_handle, indicate_reserved, sizeof(indicate_reserved),
			      BT_ATT_ERR_VALUE_NOT_ALLOWED);

	/* A Reserved bit on its own is accepted and leaves the descriptor
	 * disabled: the server waits for the Input Report to be subscribed
	 * before it notifies, and this write does not subscribe it.
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

/* A Boot Report is longer than an Input Report of this test, so it gets a
 * buffer and a callback of its own.
 */
static uint8_t boot_notify_cb(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
			      const void *data, uint16_t length)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(params);

	if (data == NULL) {
		return BT_GATT_ITER_STOP;
	}

	TEST_ASSERT(length <= sizeof(boot_notified_data), "Boot Report notification of %u bytes",
		    length);

	(void)memcpy(boot_notified_data, data, length);
	boot_notified_len = length;

	SET_FLAG(flag_boot_notified);

	return BT_GATT_ITER_CONTINUE;
}

static struct bt_gatt_subscribe_params boot_sub_params;

static void subscribe_boot_kb_in(void)
{
	int err;

	boot_sub_params.value_handle = boot_kb_in.value_handle;
	boot_sub_params.ccc_handle = boot_kb_in.ccc_handle;
	boot_sub_params.value = BT_GATT_CCC_NOTIFY;
	boot_sub_params.notify = boot_notify_cb;
	boot_sub_params.subscribe = subscribe_cb;

	UNSET_FLAG(flag_subscribed);
	UNSET_FLAG(flag_boot_notified);

	err = bt_gatt_subscribe(client_conn, &boot_sub_params);
	TEST_ASSERT(err == 0, "Failed to subscribe to the Boot Keyboard Input Report: %d", err);

	WAIT_FOR_FLAG(flag_subscribed);
}

static void wait_for_boot_kb_in_report(void)
{
	WAIT_FOR_FLAG(flag_boot_notified);
	UNSET_FLAG(flag_boot_notified);

	TEST_ASSERT(boot_notified_len == BT_HIDS_BOOT_KB_IN_LEN,
		    "Boot Keyboard Input Report of %u octets, expected %u", boot_notified_len,
		    BT_HIDS_BOOT_KB_IN_LEN);
	TEST_ASSERT(memcmp(boot_notified_data, test_boot_kb_in_data, BT_HIDS_BOOT_KB_IN_LEN) == 0,
		    "Notified Boot Keyboard Input Report payload mismatch");
}

/* The three Boot Report characteristics have the properties the HID Service
 * defines for them, only the two Boot Input Reports have a CCC descriptor, and
 * the length of each is fixed by the USB HID Specification.
 */
static void verify_boot_reports(void)
{
	TEST_ASSERT(boot_kb_in.value_handle != 0U, "The Boot Keyboard Input Report is missing");
	TEST_ASSERT(boot_kb_out.value_handle != 0U, "The Boot Keyboard Output Report is missing");
	TEST_ASSERT(boot_mouse_in.value_handle != 0U, "The Boot Mouse Input Report is missing");

	TEST_ASSERT(boot_kb_in.properties == (BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY),
		    "Boot Keyboard Input Report properties 0x%02x", boot_kb_in.properties);
	TEST_ASSERT(boot_kb_out.properties == (BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE |
					       BT_GATT_CHRC_WRITE_WITHOUT_RESP),
		    "Boot Keyboard Output Report properties 0x%02x", boot_kb_out.properties);
	TEST_ASSERT(boot_mouse_in.properties == (BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY),
		    "Boot Mouse Input Report properties 0x%02x", boot_mouse_in.properties);

	TEST_ASSERT(boot_kb_in.ccc_handle != 0U,
		    "The Boot Keyboard Input Report has no CCC descriptor");
	TEST_ASSERT(boot_mouse_in.ccc_handle != 0U,
		    "The Boot Mouse Input Report has no CCC descriptor");
	TEST_ASSERT(boot_kb_out.ccc_handle == 0U,
		    "The Boot Keyboard Output Report has a CCC descriptor");

	/* A read returns the value the server holds, which it set with
	 * bt_hids_boot_report_set(). The three lengths differ, so these reads
	 * also tell the three Boot Report contexts apart.
	 */
	gatt_read(boot_kb_in.value_handle);
	TEST_ASSERT(read_len == BT_HIDS_BOOT_KB_IN_LEN,
		    "Boot Keyboard Input Report of %u octets, expected %u", read_len,
		    BT_HIDS_BOOT_KB_IN_LEN);
	TEST_ASSERT(memcmp(read_data, test_boot_kb_in_stored, BT_HIDS_BOOT_KB_IN_LEN) == 0,
		    "Boot Keyboard Input Report payload mismatch");

	gatt_read(boot_kb_out.value_handle);
	TEST_ASSERT(read_len == BT_HIDS_BOOT_KB_OUT_LEN,
		    "Boot Keyboard Output Report of %u octets, expected %u", read_len,
		    BT_HIDS_BOOT_KB_OUT_LEN);

	gatt_read(boot_mouse_in.value_handle);
	TEST_ASSERT(read_len == BT_HIDS_BOOT_MOUSE_IN_LEN,
		    "Boot Mouse Input Report of %u octets, expected %u", read_len,
		    BT_HIDS_BOOT_MOUSE_IN_LEN);
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
		TEST_ASSERT(read_len == 2U, "Report Reference of %u bytes, expected 2", read_len);

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
				    "The Input Report has no CCC descriptor");
			break;
		case BT_HID_REPORT_TYPE_OUTPUT:
			TEST_ASSERT(report->id == TEST_REPORT_ID_OUTPUT,
				    "Output Report ID %u, expected %u", report->id,
				    TEST_REPORT_ID_OUTPUT);
			TEST_ASSERT(report->ccc_handle == 0U,
				    "The Output Report has a CCC descriptor");
			break;
		case BT_HID_REPORT_TYPE_FEATURE:
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
	TEST_ASSERT(read_len == sizeof(test_report_map), "Report Map of %u bytes, expected %zu",
		    read_len, sizeof(test_report_map));
	TEST_ASSERT(memcmp(read_data, test_report_map, sizeof(test_report_map)) == 0,
		    "Report Map content mismatch");

	gatt_read(hid_info_handle);
	TEST_ASSERT(read_len == sizeof(expected_info), "HID Information of %u bytes, expected %zu",
		    read_len, sizeof(expected_info));
	TEST_ASSERT(memcmp(read_data, expected_info, sizeof(expected_info)) == 0,
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
	TEST_ASSERT(read_len == TEST_LONG_REPORT_LEN, "Long read returned %u bytes, expected %u",
		    read_len, TEST_LONG_REPORT_LEN);

	first_seq = read_data[0];
	for (uint16_t i = 1U; i < read_len; i++) {
		TEST_ASSERT(read_data[i] == first_seq,
			    "Octet %u of the long read is 0x%02x, expected 0x%02x", i, read_data[i],
			    first_seq);
	}

	/* A Read Blob Request inside the value returns the same value */
	gatt_read_blob(long_feature->value_handle, TEST_LONG_REPORT_LEN / 4U);
	TEST_ASSERT(read_len > 0U, "Read Blob returned nothing");
	TEST_ASSERT(read_data[0] == first_seq, "Read Blob returned 0x%02x, expected 0x%02x",
		    read_data[0], first_seq);

	/* Offset at the end of the value: an empty response, not an error */
	gatt_read_blob(long_feature->value_handle, TEST_LONG_REPORT_LEN);
	TEST_ASSERT(read_len == 0U, "Read Blob at the end of the value returned %u bytes",
		    read_len);

	/* Offset past the value */
	gatt_read_blob_expect_err(long_feature->value_handle, TEST_LONG_REPORT_LEN + 1U,
				  BT_ATT_ERR_INVALID_OFFSET);

	/* The server replaces the value, so the next read returns the new one */
	bk_sync_send();
	bk_sync_wait();

	gatt_read_long(long_feature->value_handle);
	TEST_ASSERT(read_len == TEST_LONG_REPORT_LEN, "Long read returned %u bytes, expected %u",
		    read_len, TEST_LONG_REPORT_LEN);
	TEST_ASSERT(read_data[0] != first_seq,
		    "The read returned 0x%02x, the value from before the server replaced it",
		    read_data[0]);
	for (uint16_t i = 1U; i < read_len; i++) {
		TEST_ASSERT(read_data[i] == read_data[0],
			    "Octet %u of the long read is 0x%02x, expected 0x%02x", i, read_data[i],
			    read_data[0]);
	}
}

static void verify_protocol_mode(uint8_t expected)
{
	gatt_read(protocol_mode_handle);
	TEST_ASSERT(read_len == 1U, "Protocol Mode of %u bytes, expected 1", read_len);
	TEST_ASSERT(read_data[0] == expected, "Protocol Mode is %u, expected %u", read_data[0],
		    expected);
}

static void test_hids_client(void)
{
	/* One octet more than the fixed length of the Boot Keyboard Output
	 * Report
	 */
	static const uint8_t boot_kb_out_too_long[BT_HIDS_BOOT_KB_OUT_LEN + 1] = {0x03U, 0x00U};
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
	verify_protocol_mode(BT_HID_PROTOCOL_REPORT);

	input = report_by_type(BT_HID_REPORT_TYPE_INPUT);
	output = report_by_type(BT_HID_REPORT_TYPE_OUTPUT);
	feature = report_by_id(TEST_REPORT_ID_FEATURE);
	TEST_ASSERT(input != NULL && output != NULL && feature != NULL, "A Report is missing");

	/* The server notifies its first Input Report once we have subscribed */
	test_ccc_validation(input->ccc_handle);
	subscribe_input_report(input);
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

	/* Protocol Mode to Boot Protocol Mode and back */
	gatt_write_cmd(protocol_mode_handle, BT_HID_PROTOCOL_BOOT);
	bk_sync_send();
	bk_sync_wait();
	verify_protocol_mode(BT_HID_PROTOCOL_BOOT);

	gatt_write_cmd(protocol_mode_handle, BT_HID_PROTOCOL_REPORT);
	bk_sync_send();
	bk_sync_wait();
	verify_protocol_mode(BT_HID_PROTOCOL_REPORT);

	/* The Boot Report characteristics, and a Boot Keyboard Input Report
	 * notification once we have subscribed
	 */
	verify_boot_reports();
	subscribe_boot_kb_in();
	bk_sync_send();
	bk_sync_wait();
	wait_for_boot_kb_in_report();

	/* A Boot Report has a fixed length, so a write of any other length is
	 * refused and does not reach the application, while one of the fixed
	 * length does.
	 */
	gatt_write_expect_err(boot_kb_out.value_handle, boot_kb_out_too_long,
			      sizeof(boot_kb_out_too_long), BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	gatt_write(boot_kb_out.value_handle, test_boot_kb_out_data, BT_HIDS_BOOT_KB_OUT_LEN);
	bk_sync_send();
	bk_sync_wait();

	/* GET_REPORT of the Input Report */
	gatt_read(input->value_handle);
	TEST_ASSERT(read_len == TEST_INPUT_REPORT_LEN, "GET_REPORT returned %u bytes, expected %u",
		    read_len, TEST_INPUT_REPORT_LEN);
	TEST_ASSERT(memcmp(read_data, test_input_data, TEST_INPUT_REPORT_LEN) == 0,
		    "GET_REPORT payload mismatch");
	bk_sync_send();
	bk_sync_wait();

	/* GET_REPORT of the Feature Report returns what we wrote to it */
	gatt_read(feature->value_handle);
	TEST_ASSERT(read_len == TEST_FEATURE_REPORT_LEN,
		    "GET_REPORT returned %u bytes, expected %u", read_len, TEST_FEATURE_REPORT_LEN);
	TEST_ASSERT(memcmp(read_data, test_feature_data, TEST_FEATURE_REPORT_LEN) == 0,
		    "GET_REPORT payload mismatch");
	bk_sync_send();
	bk_sync_wait();

	verify_long_report_reads();
	bk_sync_send();
	bk_sync_wait();

	/* Reconnect: the server serves the same HID Service again, with the
	 * per connection state back at its default values.
	 */
	err = bt_gatt_unsubscribe(client_conn, &boot_sub_params);
	TEST_ASSERT(err == 0, "Failed to unsubscribe from the Boot Report: %d", err);

	err = bt_gatt_unsubscribe(client_conn, &sub_params);
	TEST_ASSERT(err == 0, "Failed to unsubscribe: %d", err);

	UNSET_FLAG(flag_disconnected);
	err = bt_conn_disconnect(client_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	TEST_ASSERT(err == 0, "Failed to disconnect: %d", err);
	WAIT_FOR_FLAG(flag_disconnected);

	connect_to_server();
	exchange_mtu();

	discover_hids();
	resolve_reports();
	verify_protocol_mode(BT_HID_PROTOCOL_REPORT);

	input = report_by_type(BT_HID_REPORT_TYPE_INPUT);
	TEST_ASSERT(input != NULL, "The Input Report is missing after reconnecting");

	subscribe_input_report(input);
	bk_sync_wait();
	wait_for_input_report();

	/* The server unregisters the HID Service while we are connected */
	UNSET_FLAG(flag_disconnected);
	bk_sync_send();
	bk_sync_wait();
	TEST_ASSERT(!IS_FLAG_SET(flag_disconnected),
		    "The server disconnected us when it unregistered the HID Service");

	err = bt_conn_disconnect(client_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	TEST_ASSERT(err == 0, "Failed to disconnect: %d", err);
	WAIT_FOR_FLAG(flag_disconnected);

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
