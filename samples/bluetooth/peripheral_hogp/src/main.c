/*
 * Copyright (c) 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/services/bas.h>
#include <zephyr/bluetooth/hogp_device.h>
#include <zephyr/usb/class/hid.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(hogp_sample, LOG_LEVEL_INF);

/* HID Report Map: Generic Desktop, Mouse (3 buttons + X + Y + Wheel) */
static const uint8_t report_map[] = {
	HID_USAGE_PAGE(HID_USAGE_GEN_DESKTOP),
	HID_USAGE(HID_USAGE_GEN_DESKTOP_MOUSE),
	HID_COLLECTION(HID_COLLECTION_APPLICATION),
		HID_REPORT_ID(1),
		HID_USAGE(HID_USAGE_GEN_DESKTOP_POINTER),
		HID_COLLECTION(HID_COLLECTION_PHYSICAL),
			HID_USAGE_PAGE(HID_USAGE_GEN_BUTTON),
			HID_USAGE_MIN8(1),
			HID_USAGE_MAX8(3),
			HID_LOGICAL_MIN8(0),
			HID_LOGICAL_MAX8(1),
			HID_REPORT_COUNT(3),
			HID_REPORT_SIZE(1),
			HID_INPUT(0x02),
			HID_REPORT_COUNT(1),
			HID_REPORT_SIZE(5),
			HID_INPUT(0x03),
			HID_USAGE_PAGE(HID_USAGE_GEN_DESKTOP),
			HID_USAGE(HID_USAGE_GEN_DESKTOP_X),
			HID_USAGE(HID_USAGE_GEN_DESKTOP_Y),
			HID_USAGE(HID_USAGE_GEN_DESKTOP_WHEEL),
			HID_LOGICAL_MIN8(-127),
			HID_LOGICAL_MAX8(127),
			HID_REPORT_SIZE(8),
			HID_REPORT_COUNT(3),
			HID_INPUT(0x06),
		HID_END_COLLECTION,
	HID_END_COLLECTION,
};

static const struct bt_hogp_device_report reports[] = {
	{ .id = 1, .type = BT_HID_REPORT_TYPE_INPUT },
};

static int8_t mouse_x_dir = 5;

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err != 0U) {
		LOG_ERR("Failed to connect to %s (err 0x%02x %s)",
			bt_conn_dst_str(conn), err, bt_hci_err_to_str(err));
		return;
	}

	LOG_INF("Connected %s", bt_conn_dst_str(conn));

	if (bt_conn_set_security(conn, BT_SECURITY_L2)) {
		LOG_WRN("Failed to set security");
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Disconnected from %s (reason 0x%02x %s)",
		bt_conn_dst_str(conn), reason, bt_hci_err_to_str(reason));
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	if (err == BT_SECURITY_ERR_SUCCESS) {
		LOG_INF("Security changed: %s level %u",
			bt_conn_dst_str(conn), level);
	} else {
		LOG_ERR("Security failed: %s level %u err %s(%d)",
			bt_conn_dst_str(conn), level,
			bt_security_err_to_str(err), err);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,
};

static ssize_t get_report_cb(struct bt_conn *conn, uint8_t report_type,
			     uint8_t report_id, uint8_t *buf,
			     uint16_t buf_size)
{
	ARG_UNUSED(conn);

	if (report_id == 1 && report_type == BT_HID_REPORT_TYPE_INPUT) {
		uint8_t report[4] = { 0x00, mouse_x_dir, 0, 0 };
		uint16_t len = MIN(buf_size, sizeof(report));

		(void)memcpy(buf, report, len);
		LOG_INF("GET_REPORT: returning mouse state");
		return (ssize_t)len;
	}

	return -ENOENT;
}

static void set_report_cb(struct bt_conn *conn, uint8_t report_type,
			  uint8_t report_id, const uint8_t *data,
			  uint16_t len)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(data);

	LOG_INF("SET_REPORT: type=%u id=%u len=%u", report_type, report_id,
		len);
}

static void protocol_mode_changed_cb(struct bt_conn *conn, uint8_t protocol)
{
	ARG_UNUSED(conn);

	LOG_INF("Protocol mode changed: %s",
		protocol == BT_HID_PROTOCOL_BOOT ? "Boot" : "Report");
}

static void suspend_changed_cb(struct bt_conn *conn, bool suspended)
{
	ARG_UNUSED(conn);

	LOG_INF("Suspend state: %s", suspended ? "Suspended" : "Active");
}

static void ccc_changed_cb(struct bt_conn *conn, uint8_t report_id,
			   uint8_t report_type, bool enabled)
{
	ARG_UNUSED(conn);

	LOG_INF("CCC changed: report_id=%u type=%u enabled=%d",
		report_id, report_type, enabled);
}

static const struct bt_hogp_device_cb hogp_cb = {
	.get_report = get_report_cb,
	.set_report = set_report_cb,
	.protocol_mode_changed = protocol_mode_changed_cb,
	.suspend_changed = suspend_changed_cb,
	.ccc_changed = ccc_changed_cb,
};

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
		      BT_UUID_16_ENCODE(BT_UUID_HIDS_VAL),
		      BT_UUID_16_ENCODE(BT_UUID_BAS_VAL)),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME,
		sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static void send_mouse_report(void)
{
	static int count;
	int err;

	uint8_t report[4] = { 0x00, mouse_x_dir, 0, 0 };

	count++;
	if (count >= 20) {
		mouse_x_dir = -mouse_x_dir;
		count = 0;
	}

	err = bt_hogp_device_send_report(NULL, 1, report, sizeof(report),
					 NULL, NULL);
	if (err && err != -ENOTCONN) {
		LOG_WRN("Send report failed: %d", err);
	}
}

int main(void)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed: %d", err);
		return 0;
	}

	LOG_INF("Bluetooth initialized");

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	struct bt_hogp_device_init_param param = {
		.info = {
			.bcd_hid = 0x0111,
			.b_country_code = 0x00,
			.flags = BT_HID_INFO_FLAG_NORMALLY_CONNECTABLE,
		},
		.report_map = report_map,
		.report_map_len = sizeof(report_map),
		.reports = reports,
		.report_count = ARRAY_SIZE(reports),
		.cb = &hogp_cb,
	};

	err = bt_hogp_device_register(&param);
	if (err) {
		LOG_ERR("HOGP Device register failed: %d", err);
		return 0;
	}

	LOG_INF("HOGP Device registered");

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad),
			      sd, ARRAY_SIZE(sd));
	if (err) {
		LOG_ERR("Advertising start failed: %d", err);
		return 0;
	}

	LOG_INF("Advertising started");

	while (1) {
		k_sleep(K_MSEC(100));
		send_mouse_report();
	}

	return 0;
}
