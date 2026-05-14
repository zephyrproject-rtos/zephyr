/* main.c - Bluetooth Classic HID Device (Mouse) sample */

/*
 * Copyright 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/classic/classic.h>
#include <zephyr/bluetooth/classic/hid_device.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/settings/settings.h>

#define HID_DEVICE_VERSION      0x0101
#define HID_PARSER_VERSION      0x0111
#define HID_DEVICE_SUBCLASS     0x80
#define HID_DEVICE_COUNTRY_CODE 0x21
#define HID_PROTO_CONTROL       0x0011
#define HID_PROTO_INTERRUPT     0x0013

#define HID_LANG_ID_ENGLISH     0x0409
#define HID_LANG_ID_OFFSET      0x0100

#define HID_SUPERVISION_TIMEOUT    1000
#define HID_SSR_HOST_MAX_LATENCY   240
#define HID_SSR_HOST_MIN_TIMEOUT   0

#define MOUSE_REPORT_ID         2
#define MOUSE_REPORT_INTERVAL_MS 100
#define MOUSE_CIRCLE_STEPS       64

NET_BUF_POOL_FIXED_DEFINE(hid_tx_pool, 2, BT_L2CAP_BUF_SIZE(CONFIG_BT_L2CAP_TX_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static struct bt_hid_device *default_hid;
static struct bt_conn *default_conn;

static void mouse_report_work_handler(struct k_work *work);
static K_WORK_DEFINE(mouse_report_work, mouse_report_work_handler);
static void mouse_timer_handler(struct k_timer *timer);
K_TIMER_DEFINE(mouse_timer, mouse_timer_handler, NULL);

static uint8_t mouse_step;

static const uint8_t mouse_descriptor[] = {
	0x05, 0x01,       /* USAGE_PAGE (Generic Desktop) */
	0x09, 0x02,       /* USAGE (Mouse) */
	0xa1, 0x01,       /* COLLECTION (Application) */
	0x85, MOUSE_REPORT_ID, /* REPORT_ID (2) */
	0x09, 0x01,       /*   USAGE (Pointer) */
	0xa1, 0x00,       /*   COLLECTION (Physical) */
	0x05, 0x09,       /*     USAGE_PAGE (Button) */
	0x19, 0x01,       /*     USAGE_MINIMUM (1) */
	0x29, 0x08,       /*     USAGE_MAXIMUM (8) */
	0x15, 0x00,       /*     LOGICAL_MINIMUM (0) */
	0x25, 0x01,       /*     LOGICAL_MAXIMUM (1) */
	0x95, 0x08,       /*     REPORT_COUNT (8) */
	0x75, 0x01,       /*     REPORT_SIZE (1) */
	0x81, 0x02,       /*     INPUT (Data,Var,Abs) */
	0x05, 0x01,       /*     USAGE_PAGE (Generic Desktop) */
	0x09, 0x30,       /*     USAGE (X) */
	0x09, 0x31,       /*     USAGE (Y) */
	0x09, 0x38,       /*     USAGE (Wheel) */
	0x15, 0x81,       /*     LOGICAL_MINIMUM (-127) */
	0x25, 0x7f,       /*     LOGICAL_MAXIMUM (127) */
	0x75, 0x08,       /*     REPORT_SIZE (8) */
	0x95, 0x03,       /*     REPORT_COUNT (3) */
	0x81, 0x06,       /*     INPUT (Data,Var,Rel) */
	0xc0,             /*   END_COLLECTION */
	0xc0              /* END_COLLECTION */
};

/* Integer sine table for 64-step circular movement, amplitude ~10 pixels */
static const int8_t sine_table[MOUSE_CIRCLE_STEPS] = {
	 0,  1,  2,  3,  4,  5,  6,  6,  7,  8,  8,  9,  9,  9, 10, 10,
	10, 10, 10,  9,  9,  9,  8,  8,  7,  6,  6,  5,  4,  3,  2,  1,
	 0, -1, -2, -3, -4, -5, -6, -6, -7, -8, -8, -9, -9, -9, -10, -10,
	-10, -10, -10, -9, -9, -9, -8, -8, -7, -6, -6, -5, -4, -3, -2, -1,
};

static struct bt_sdp_attribute hid_attrs[] = {
	BT_SDP_NEW_SERVICE,
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
			BT_SDP_ARRAY_16(BT_SDP_HID_SVCLASS)
		}
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROTO_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 13),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(HID_PROTO_CONTROL)
			}
			)
		},
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_PROTO_HID)
			}
			)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROFILE_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_HID_SVCLASS)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(HID_DEVICE_VERSION)
			}
			)
		}
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_ADD_PROTO_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 15),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 13),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
				BT_SDP_DATA_ELEM_LIST(
				{
					BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
					BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP)
				},
				{
					BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
					BT_SDP_ARRAY_16(HID_PROTO_INTERRUPT)
				}
				)
			},
			{
				BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
				BT_SDP_DATA_ELEM_LIST(
				{
					BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
					BT_SDP_ARRAY_16(BT_SDP_PROTO_HID)
				}
				)
			}
			)
		}
		)
	),
	BT_SDP_SERVICE_NAME("HID Mouse"),
	{
		BT_SDP_ATTR_HID_DEVICE_RELEASE_NUMBER,
		{ BT_SDP_TYPE_SIZE(BT_SDP_UINT16), BT_SDP_ARRAY_16(HID_DEVICE_VERSION) }
	},
	{
		BT_SDP_ATTR_HID_PARSER_VERSION,
		{ BT_SDP_TYPE_SIZE(BT_SDP_UINT16), BT_SDP_ARRAY_16(HID_PARSER_VERSION) }
	},
	{
		BT_SDP_ATTR_HID_DEVICE_SUBCLASS,
		{ BT_SDP_TYPE_SIZE(BT_SDP_UINT8), BT_SDP_ARRAY_8(HID_DEVICE_SUBCLASS) }
	},
	{
		BT_SDP_ATTR_HID_COUNTRY_CODE,
		{ BT_SDP_TYPE_SIZE(BT_SDP_UINT8), BT_SDP_ARRAY_8(HID_DEVICE_COUNTRY_CODE) }
	},
	{
		BT_SDP_ATTR_HID_VIRTUAL_CABLE,
		{ BT_SDP_TYPE_SIZE(BT_SDP_BOOL), BT_SDP_ARRAY_8(0x01) }
	},
	{
		BT_SDP_ATTR_HID_RECONNECT_INITIATE,
		{ BT_SDP_TYPE_SIZE(BT_SDP_BOOL), BT_SDP_ARRAY_8(0x01) }
	},
	BT_SDP_LIST(
		BT_SDP_ATTR_HID_DESCRIPTOR_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ16, sizeof(mouse_descriptor) + 8),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ16, sizeof(mouse_descriptor) + 5),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
				BT_SDP_ARRAY_8(0x22),
			},
			{
				BT_SDP_TYPE_SIZE_VAR(BT_SDP_TEXT_STR16, sizeof(mouse_descriptor)),
				mouse_descriptor,
			}
			)
		}
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_HID_LANG_ID_BASE_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(HID_LANG_ID_ENGLISH),
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(HID_LANG_ID_OFFSET),
			}
			),
		}
		)
	),
	{
		BT_SDP_ATTR_HID_BOOT_DEVICE,
		{ BT_SDP_TYPE_SIZE(BT_SDP_BOOL), BT_SDP_ARRAY_8(0x01) }
	},
	{
		BT_SDP_ATTR_HID_PROFILE_VERSION,
		{ BT_SDP_TYPE_SIZE(BT_SDP_UINT16), BT_SDP_ARRAY_16(0x0101) }
	},
	{
		BT_SDP_ATTR_HID_BATTERY_POWER,
		{ BT_SDP_TYPE_SIZE(BT_SDP_BOOL), BT_SDP_ARRAY_8(0x01) }
	},
	{
		BT_SDP_ATTR_HID_REMOTE_WAKEUP,
		{ BT_SDP_TYPE_SIZE(BT_SDP_BOOL), BT_SDP_ARRAY_8(0x01) }
	},
	{
		BT_SDP_ATTR_HID_NORMALLY_CONNECTABLE,
		{ BT_SDP_TYPE_SIZE(BT_SDP_BOOL), BT_SDP_ARRAY_8(0x01) }
	},
	{
		BT_SDP_ATTR_HID_SUPERVISION_TIMEOUT,
		{ BT_SDP_TYPE_SIZE(BT_SDP_UINT16), BT_SDP_ARRAY_16(HID_SUPERVISION_TIMEOUT) }
	},
	{
		BT_SDP_ATTR_HID_SSR_HOST_MAX_LATENCY,
		{ BT_SDP_TYPE_SIZE(BT_SDP_UINT16), BT_SDP_ARRAY_16(HID_SSR_HOST_MAX_LATENCY) }
	},
	{
		BT_SDP_ATTR_HID_SSR_HOST_MIN_TIMEOUT,
		{ BT_SDP_TYPE_SIZE(BT_SDP_UINT16), BT_SDP_ARRAY_16(HID_SSR_HOST_MIN_TIMEOUT) }
	},
};

static struct bt_sdp_record hid_rec = BT_SDP_RECORD(hid_attrs);

static void send_mouse_report(void)
{
	struct net_buf *buf;
	int8_t dx, dy;
	int err;

	if (default_hid == NULL) {
		return;
	}

	buf = bt_hid_device_create_pdu(&hid_tx_pool);
	if (buf == NULL) {
		printk("Failed to allocate HID PDU\n");
		return;
	}

	dx = sine_table[(mouse_step + (MOUSE_CIRCLE_STEPS / 4)) % MOUSE_CIRCLE_STEPS];
	dy = sine_table[mouse_step % MOUSE_CIRCLE_STEPS];

	if (default_hid->boot_mode) {
		net_buf_add_u8(buf, 0x00);
		net_buf_add_u8(buf, (uint8_t)dx);
		net_buf_add_u8(buf, (uint8_t)dy);
	} else {
		net_buf_add_u8(buf, MOUSE_REPORT_ID);
		net_buf_add_u8(buf, 0x00);
		net_buf_add_u8(buf, (uint8_t)dx);
		net_buf_add_u8(buf, (uint8_t)dy);
		net_buf_add_u8(buf, 0x00);
	}

	err = bt_hid_device_send_input_report(default_hid, buf);
	if (err != 0) {
		net_buf_unref(buf);
	}

	mouse_step = (mouse_step + 1) % MOUSE_CIRCLE_STEPS;
}

static void mouse_report_work_handler(struct k_work *work)
{
	send_mouse_report();
}

static void mouse_timer_handler(struct k_timer *timer)
{
	k_work_submit(&mouse_report_work);
}

static void hid_connected_cb(struct bt_hid_device *hid)
{
	printk("HID connected\n");
	default_hid = hid;

	bt_hid_device_set_report_id(hid, true);
	bt_hid_device_set_boot_device(hid, true);

	mouse_step = 0;
	k_timer_start(&mouse_timer, K_MSEC(MOUSE_REPORT_INTERVAL_MS),
		       K_MSEC(MOUSE_REPORT_INTERVAL_MS));
}

static void hid_disconnected_cb(struct bt_hid_device *hid)
{
	printk("HID disconnected\n");
	k_timer_stop(&mouse_timer);
	default_hid = NULL;
}

static int hid_set_report_cb(struct bt_hid_device *hid, uint8_t type, uint8_t report_id,
			     struct net_buf *buf)
{
	printk("Set Report: type %u id %u len %u\n", type, report_id, buf->len);

	if (report_id != MOUSE_REPORT_ID) {
		return -ENOENT;
	}

	return 0;
}

static int hid_get_report_cb(struct bt_hid_device *hid, uint8_t type, uint8_t report_id,
			     uint16_t buffer_size)
{
	struct net_buf *buf;
	uint8_t boot_report[] = {0x00, 0x00, 0x00};
	uint8_t report_data[] = {0x00, 0x00, 0x00, 0x00};
	uint8_t *data;
	uint16_t len;
	int err;

	printk("Get Report: type %u id %u size %u\n", type, report_id, buffer_size);

	if (!hid->boot_mode && report_id != MOUSE_REPORT_ID) {
		return -ENOENT;
	}

	buf = bt_hid_device_create_pdu(&hid_tx_pool);
	if (buf == NULL) {
		return -ENOMEM;
	}

	if (hid->boot_mode) {
		data = boot_report;
		len = sizeof(boot_report);
	} else {
		data = report_data;
		len = 1 + sizeof(report_data);
	}

	if (buffer_size > 0 && buffer_size < len) {
		len = buffer_size;
	}

	if (hid->boot_mode) {
		net_buf_add_mem(buf, data, len);
	} else {
		net_buf_add_u8(buf, report_id);
		net_buf_add_mem(buf, data, len - 1);
	}

	err = bt_hid_device_get_report_response(hid, type, buf);
	if (err != 0) {
		net_buf_unref(buf);
		return err;
	}

	return 0;
}

static int hid_set_protocol_cb(struct bt_hid_device *hid, uint8_t protocol)
{
	printk("Set Protocol: %s\n",
	       protocol == BT_HID_PROTOCOL_BOOT_MODE ? "Boot" : "Report");
	return 0;
}

static int hid_get_protocol_cb(struct bt_hid_device *hid)
{
	uint8_t protocol = hid->boot_mode ? BT_HID_PROTOCOL_BOOT_MODE
					   : BT_HID_PROTOCOL_REPORT_MODE;
	int err;

	printk("Get Protocol: %s\n",
	       protocol == BT_HID_PROTOCOL_BOOT_MODE ? "Boot" : "Report");

	err = bt_hid_device_get_protocol_response(hid, protocol);
	if (err != 0) {
		printk("Get Protocol response failed (%d)\n", err);
		return err;
	}

	return 0;
}

static void hid_intr_data_cb(struct bt_hid_device *hid, uint8_t report_id, struct net_buf *buf)
{
	printk("Interrupt data: report_id %u len %u\n", report_id, buf->len);
}

static void hid_vc_unplug_cb(struct bt_hid_device *hid)
{
	printk("Virtual cable unplug\n");
}

static void hid_suspend_cb(struct bt_hid_device *hid, bool suspended)
{
	printk("HID %s\n", suspended ? "suspended" : "exit suspend");

	if (suspended) {
		k_timer_stop(&mouse_timer);
	} else if (default_hid != NULL) {
		k_timer_start(&mouse_timer, K_MSEC(MOUSE_REPORT_INTERVAL_MS),
			       K_MSEC(MOUSE_REPORT_INTERVAL_MS));
	}
}

static const struct bt_hid_device_cb hid_cb = {
	.connected = hid_connected_cb,
	.disconnected = hid_disconnected_cb,
	.set_report = hid_set_report_cb,
	.get_report = hid_get_report_cb,
	.set_protocol = hid_set_protocol_cb,
	.get_protocol = hid_get_protocol_cb,
	.intr_data = hid_intr_data_cb,
	.vc_unplug = hid_vc_unplug_cb,
	.suspend = hid_suspend_cb,
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	struct bt_conn_info info;

	if (err != 0) {
		printk("Connection failed (err 0x%02x)\n", err);
		return;
	}

	bt_conn_get_info(conn, &info);
	if (info.type != BT_CONN_TYPE_BR) {
		return;
	}

	if (default_conn != NULL) {
		bt_conn_unref(default_conn);
	}

	default_conn = bt_conn_ref(conn);
	printk("Connected\n");
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason 0x%02x)\n", reason);

	if (default_conn == conn) {
		bt_conn_unref(default_conn);
		default_conn = NULL;
	}
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	char addr[BT_ADDR_STR_LEN];
	struct bt_conn_info info;

	bt_conn_get_info(conn, &info);
	bt_addr_to_str(info.br.dst, addr, sizeof(addr));

	printk("Security changed: %s level %u (err %d)\n", addr, level, err);
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,
};

static void bt_ready(int err)
{
	if (err != 0) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	printk("Bluetooth initialized\n");

	bt_conn_cb_register(&conn_callbacks);

	err = bt_hid_device_register(&hid_cb);
	if (err != 0) {
		printk("HID register failed (err %d)\n", err);
		return;
	}

	err = bt_sdp_register_service(&hid_rec);
	if (err != 0) {
		printk("SDP register failed (err %d)\n", err);
		return;
	}

	err = bt_br_set_connectable(true, NULL);
	if (err != 0) {
		printk("Set connectable failed (err %d)\n", err);
		return;
	}

	err = bt_br_set_discoverable(true, false);
	if (err != 0) {
		printk("Set discoverable failed (err %d)\n", err);
		return;
	}

	printk("HID Mouse Device ready, waiting for connections...\n");
}

int main(void)
{
	int err;

	printk("Bluetooth HID Device Mouse demo\n");

	err = bt_enable(bt_ready);
	if (err != 0) {
		printk("Bluetooth init failed (err %d)\n", err);
	}

	return 0;
}
