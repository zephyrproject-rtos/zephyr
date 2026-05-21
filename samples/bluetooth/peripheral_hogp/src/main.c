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

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(hogp_sample, LOG_LEVEL_INF);

/* HID Report Map: Generic Desktop, Mouse (3 buttons + X + Y + Wheel) */
static const uint8_t report_map[] = {
	0x05, 0x01,       /* Usage Page (Generic Desktop) */
	0x09, 0x02,       /* Usage (Mouse) */
	0xA1, 0x01,       /* Collection (Application) */
	0x85, 0x01,       /*   Report ID (1) */
	0x09, 0x01,       /*   Usage (Pointer) */
	0xA1, 0x00,       /*   Collection (Physical) */
	0x05, 0x09,       /*     Usage Page (Buttons) */
	0x19, 0x01,       /*     Usage Minimum (1) */
	0x29, 0x03,       /*     Usage Maximum (3) */
	0x15, 0x00,       /*     Logical Minimum (0) */
	0x25, 0x01,       /*     Logical Maximum (1) */
	0x95, 0x03,       /*     Report Count (3) */
	0x75, 0x01,       /*     Report Size (1) */
	0x81, 0x02,       /*     Input (Variable) */
	0x95, 0x01,       /*     Report Count (1) */
	0x75, 0x05,       /*     Report Size (5) */
	0x81, 0x03,       /*     Input (Constant) - padding */
	0x05, 0x01,       /*     Usage Page (Generic Desktop) */
	0x09, 0x30,       /*     Usage (X) */
	0x09, 0x31,       /*     Usage (Y) */
	0x09, 0x38,       /*     Usage (Wheel) */
	0x15, 0x81,       /*     Logical Minimum (-127) */
	0x25, 0x7F,       /*     Logical Maximum (127) */
	0x75, 0x08,       /*     Report Size (8) */
	0x95, 0x03,       /*     Report Count (3) */
	0x81, 0x06,       /*     Input (Variable, Relative) */
	0xC0,             /*   End Collection */
	0xC0,             /* End Collection */
};

static const struct bt_hogp_device_report reports[] = {
	{ .id = 1, .type = BT_HID_REPORT_TYPE_INPUT },
};

static void connected_cb(struct bt_conn *conn)
{
	LOG_INF("Host connected");
}

static void disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Host disconnected (reason 0x%02x)", reason);
}

static void get_report_cb(struct bt_conn *conn, uint8_t report_type,
			   uint8_t report_id, uint16_t buf_size)
{
	LOG_INF("GET_REPORT: type=%u id=%u", report_type, report_id);
}

static void set_report_cb(struct bt_conn *conn, uint8_t report_type,
			   uint8_t report_id, const uint8_t *data,
			   uint16_t len)
{
	LOG_INF("SET_REPORT: type=%u id=%u len=%u", report_type, report_id,
		len);
}

static void set_protocol_cb(struct bt_conn *conn, uint8_t protocol)
{
	LOG_INF("Protocol mode: %s",
		protocol == BT_HID_PROTOCOL_BOOT ? "Boot" : "Report");
}

static void ctrl_point_cb(struct bt_conn *conn, uint8_t value)
{
	LOG_INF("Control Point: %s", value == 0 ? "Suspend" : "Exit Suspend");
}

static void ccc_changed_cb(struct bt_conn *conn, uint8_t report_id,
			    uint8_t report_type, bool enabled)
{
	LOG_INF("CCC changed: report_id=%u type=%u enabled=%d",
		report_id, report_type, enabled);
}

static const struct bt_hogp_device_cb hogp_cb = {
	.connected = connected_cb,
	.disconnected = disconnected_cb,
	.get_report = get_report_cb,
	.set_report = set_report_cb,
	.set_protocol = set_protocol_cb,
	.ctrl_point = ctrl_point_cb,
	.ccc_changed = ccc_changed_cb,
};

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_HIDS_VAL)),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME,
		sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static void send_mouse_report(void)
{
	static int8_t x_dir = 5;
	static int count;

	uint8_t report[4] = { 0x00, x_dir, 0, 0 }; /* buttons, X, Y, wheel */

	count++;
	if (count >= 20) {
		x_dir = -x_dir;
		count = 0;
	}

	int err = bt_hogp_device_send_report(NULL, 1, report, sizeof(report),
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

	struct bt_hogp_device_init_param param = {
		.info = {
			.bcd_hid = 0x0111,
			.b_country_code = 0x00,
			.flags = 0x02, /* NormallyConnectable */
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
