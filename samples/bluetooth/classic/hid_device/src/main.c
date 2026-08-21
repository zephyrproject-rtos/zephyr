/* main.c - Bluetooth Classic HID Device (Mouse) sample */

/*
 * Copyright (c) 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/types.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/classic/classic.h>
#include <zephyr/bluetooth/classic/hid_device.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/input/input.h>
#include <zephyr/settings/settings.h>
#include <zephyr/usb/class/hid.h>

#define HID_DEVICE_VERSION      0x0101U
#define HID_PARSER_VERSION      0x0111U
#define HID_DEVICE_SUBCLASS     0x80U /* Pointing device (mouse) */
#define HID_DEVICE_COUNTRY_CODE 0x21U
#define HID_PROTO_CONTROL       0x0011U
#define HID_PROTO_INTERRUPT     0x0013U

#define HID_LANG_ID_ENGLISH 0x0409U
#define HID_LANG_ID_OFFSET  0x0100U

#define HID_SUPERVISION_TIMEOUT  1000
#define HID_SSR_HOST_MAX_LATENCY 240
#define HID_SSR_HOST_MIN_TIMEOUT 0

/* Byte indices and length of the mouse input report. The report layout
 * matches HID_MOUSE_REPORT_DESC() below: buttons + X + Y + wheel, no Report ID.
 */
enum mouse_report_idx {
	MOUSE_BTN_REPORT_IDX = 0,
	MOUSE_X_REPORT_IDX = 1,
	MOUSE_Y_REPORT_IDX = 2,
	MOUSE_WHEEL_REPORT_IDX = 3,
	MOUSE_REPORT_COUNT = 4,
};

#define MOUSE_BTN_LEFT  0
#define MOUSE_BTN_RIGHT 1

NET_BUF_POOL_FIXED_DEFINE(hid_tx_pool, 2, BT_L2CAP_BUF_SIZE(CONFIG_BT_L2CAP_TX_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

/* HID callbacks run in the Bluetooth RX thread while the input callback runs
 * in the input subsystem context. The instance below is only written from the
 * callbacks and read from the input path; bt_hid_device_input_report() rejects
 * a stale instance with -ENOTCONN, so no extra locking is needed.
 */
static struct bt_hid_device *default_hid;

/* Protocol mode negotiated with the host. The HID spec defaults to Report
 * Protocol Mode, and there is no API to query the stack, so the application
 * tracks the mode itself from the Set_Protocol callback.
 */
static bool hid_boot_mode;

/* Deferred Virtual Cable Unplug bookkeeping, see hid_vc_unplug_cb() */
static bool vcu_unplug_pending;
static bt_addr_t vcu_peer;

/* Standard mouse report descriptor from the HID class helpers. It declares
 * two buttons and relative X/Y/wheel with no Report ID, matching the input
 * report built in send_mouse_report().
 */
static const uint8_t mouse_descriptor[] = HID_MOUSE_REPORT_DESC(2);

/* clang-format off */
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
		{ BT_SDP_TYPE_SIZE(BT_SDP_UINT16), BT_SDP_ARRAY_16(HID_DEVICE_VERSION) }
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
/* clang-format on */

static struct bt_sdp_record hid_rec = BT_SDP_RECORD(hid_attrs);

static void send_mouse_report(const uint8_t report[MOUSE_REPORT_COUNT])
{
	struct net_buf *buf;
	int err;

	if (default_hid == NULL) {
		return;
	}

	buf = bt_hid_device_create_pdu(&hid_tx_pool);
	if (buf == NULL) {
		printk("Failed to allocate HID PDU\n");
		return;
	}

	/* Both protocol modes carry buttons + X + Y with no Report ID. Report
	 * Protocol adds the wheel byte; Boot Protocol omits it.
	 */
	net_buf_add_mem(buf, report, hid_boot_mode ? 3 : MOUSE_REPORT_COUNT);

	err = bt_hid_device_input_report(default_hid, buf);
	if (err != 0) {
		printk("Failed to send input report (err %d)\n", err);
		net_buf_unref(buf);
	}
}

/* Turn board button events into mouse input reports. Modeled on the USB HID
 * mouse sample: two buttons plus relative motion driven by four keys. The
 * board's devicetree maps its buttons to INPUT_KEY_0..3 (see the bundled
 * native_sim overlay). gpio-keys delivers events from a workqueue, so calling
 * the HID send path directly here is safe.
 */
static void input_cb(struct input_event *evt, void *user_data)
{
	static uint8_t report[MOUSE_REPORT_COUNT];

	ARG_UNUSED(user_data);

	switch (evt->code) {
	case INPUT_KEY_0:
		WRITE_BIT(report[MOUSE_BTN_REPORT_IDX], MOUSE_BTN_LEFT, evt->value);
		break;
	case INPUT_KEY_1:
		WRITE_BIT(report[MOUSE_BTN_REPORT_IDX], MOUSE_BTN_RIGHT, evt->value);
		break;
	case INPUT_KEY_2:
		if (evt->value) {
			report[MOUSE_X_REPORT_IDX] += 10U;
		}

		break;
	case INPUT_KEY_3:
		if (evt->value) {
			report[MOUSE_Y_REPORT_IDX] += 10U;
		}

		break;
	default:
		printk("Unrecognized input code %u value %d\n", evt->code, evt->value);
		return;
	}

	send_mouse_report(report);

	/* Relative axes are one-shot; button bits persist until released */
	report[MOUSE_X_REPORT_IDX] = 0U;
	report[MOUSE_Y_REPORT_IDX] = 0U;
	report[MOUSE_WHEEL_REPORT_IDX] = 0U;
}
INPUT_CALLBACK_DEFINE(NULL, input_cb, NULL);

static void hid_connected_cb(struct bt_hid_device *hid)
{
	printk("HID connected\n");

	default_hid = hid;

	/* A new connection starts in Report Protocol Mode */
	hid_boot_mode = false;
}

static void hid_disconnected_cb(struct bt_hid_device *hid)
{
	ARG_UNUSED(hid);

	printk("HID disconnected\n");

	default_hid = NULL;

	if (vcu_unplug_pending) {
		vcu_unplug_pending = false;
		bt_br_unpair(&vcu_peer);
	}
}

static int hid_set_report_cb(struct bt_hid_device *hid, uint8_t type, struct net_buf *buf)
{
	ARG_UNUSED(hid);

	/* The mouse descriptor declares no Report ID, so the payload carries no
	 * ID byte in either protocol mode. A mouse has no state the host needs
	 * to push, so the payload is only logged here. A device with OUTPUT or
	 * FEATURE reports would apply it, and reject the types its descriptor
	 * does not declare.
	 */
	printk("Set Report: type %u len %u\n", type, buf->len);

	return 0;
}

static int hid_get_report_cb(struct bt_hid_device *hid, uint8_t type, bool size_present,
			     struct net_buf *req, struct net_buf *rsp)
{
	/* Idle mouse report with no Report ID: Report Protocol carries buttons +
	 * X + Y + wheel, Boot Protocol carries buttons + X + Y only.
	 */
	static const uint8_t report[] = {0x00, 0x00, 0x00, 0x00};
	static const uint8_t boot_report[] = {0x00, 0x00, 0x00};
	uint16_t buffer_size = 0;
	const uint8_t *data;
	uint16_t len;

	ARG_UNUSED(hid);

	if (size_present) {
		if (req->len < sizeof(buffer_size)) {
			return -EINVAL;
		}

		buffer_size = net_buf_pull_le16(req);
	}

	printk("Get Report: type %u size %u\n", type, buffer_size);

	/* The mouse descriptor above declares INPUT reports only */
	if (type != BT_HID_REPORT_TYPE_INPUT) {
		/* Mapped to ERR_INVALID_PARAMETER by the stack */
		return -EINVAL;
	}

	if (hid_boot_mode) {
		data = boot_report;
		len = (uint16_t)sizeof(boot_report);
	} else {
		data = report;
		len = (uint16_t)sizeof(report);
	}

	/* When the Size bit is set the response must not exceed BufferSize */
	if (size_present) {
		if (buffer_size == 0U) {
			return -EINVAL;
		}

		len = MIN(len, buffer_size);
	}

	if (len > net_buf_tailroom(rsp)) {
		return -ENOMEM;
	}

	/* The stack prepends the HIDP DATA header and sends the response once
	 * this callback returns 0.
	 */
	net_buf_add_mem(rsp, data, len);

	return 0;
}

static int hid_set_protocol_cb(struct bt_hid_device *hid, uint8_t protocol)
{
	ARG_UNUSED(hid);

	printk("Set Protocol: %s\n", protocol == BT_HID_PROTOCOL_BOOT_MODE ? "Boot" : "Report");

	hid_boot_mode = (protocol == BT_HID_PROTOCOL_BOOT_MODE);

	return 0;
}

static void hid_vc_unplug_cb(struct bt_hid_device *hid)
{
	struct bt_conn *conn = bt_hid_device_get_conn(hid);

	printk("Virtual cable unplug\n");

	/* The bonding information for the peer that requested the unplug has to
	 * be destroyed. Defer it to the disconnected callback: bt_br_unpair()
	 * drops the ACL, so unpairing here would tear down the still-open HID
	 * channels out of order.
	 */
	if (conn != NULL) {
		bt_addr_copy(&vcu_peer, bt_conn_get_dst_br(conn));
		vcu_unplug_pending = true;
		bt_conn_unref(conn);
	}
}

static void hid_suspend_cb(struct bt_hid_device *hid, bool suspended)
{
	ARG_UNUSED(hid);

	printk("HID %s\n", suspended ? "suspended" : "exit suspend");
}

static const struct bt_hid_device_cb hid_cb = {
	.connected = hid_connected_cb,
	.disconnected = hid_disconnected_cb,
	.set_report = hid_set_report_cb,
	.get_report = hid_get_report_cb,
	.set_protocol = hid_set_protocol_cb,
	/* A mouse has no output report, so no output_report callback */
	.vc_unplug = hid_vc_unplug_cb,
	.suspend = hid_suspend_cb,
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	struct bt_conn_info info;

	if (bt_conn_get_info(conn, &info) != 0) {
		return;
	}

	if (info.type != BT_CONN_TYPE_BR) {
		return;
	}

	if (err != 0) {
		printk("Connection failed (err 0x%02x)\n", err);
		return;
	}

	printk("Connected\n");
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct bt_conn_info info;

	if (bt_conn_get_info(conn, &info) != 0) {
		return;
	}

	if (info.type != BT_CONN_TYPE_BR) {
		return;
	}

	printk("Disconnected (reason 0x%02x)\n", reason);
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	char addr[BT_ADDR_STR_LEN];
	struct bt_conn_info info;

	if (bt_conn_get_info(conn, &info) != 0) {
		return;
	}

	if (info.type != BT_CONN_TYPE_BR) {
		return;
	}

	bt_addr_to_str(bt_conn_get_dst_br(conn), addr, sizeof(addr));

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

	err = bt_conn_cb_register(&conn_callbacks);
	if (err != 0) {
		printk("Failed to register connection callbacks (err %d)\n", err);
		return;
	}

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
