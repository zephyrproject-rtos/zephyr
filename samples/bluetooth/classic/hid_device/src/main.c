/* main.c - Bluetooth Classic HID Device (Mouse) sample */

/*
 * Copyright 2026 Xiaomi Corporation
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
#include <zephyr/settings/settings.h>
#include <zephyr/usb/class/hid.h>

#define HID_DEVICE_VERSION      0x0101
#define HID_PARSER_VERSION      0x0111
#define HID_DEVICE_SUBCLASS     0x80 /* Pointing device (mouse) */
#define HID_DEVICE_COUNTRY_CODE 0x21
#define HID_PROTO_CONTROL       0x0011
#define HID_PROTO_INTERRUPT     0x0013

#define HID_LANG_ID_ENGLISH 0x0409
#define HID_LANG_ID_OFFSET  0x0100

#define HID_SUPERVISION_TIMEOUT  1000
#define HID_SSR_HOST_MAX_LATENCY 240
#define HID_SSR_HOST_MIN_TIMEOUT 0

/* The report descriptor below declares a single Report ID */
#define MOUSE_REPORT_ID          2
#define MOUSE_REPORT_INTERVAL_MS 100
#define MOUSE_CIRCLE_STEPS       64

NET_BUF_POOL_FIXED_DEFINE(hid_tx_pool, 2, BT_L2CAP_BUF_SIZE(CONFIG_BT_L2CAP_TX_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

/* HID callbacks run in the Bluetooth RX thread while the report timer work
 * runs in the system workqueue. The shared state below is only ever written
 * from the callbacks and read from the work handler; bt_hid_device_input_report()
 * rejects a stale instance with -ENOTCONN, so no extra locking is needed.
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

static void mouse_report_work_handler(struct k_work *work);
static K_WORK_DEFINE(mouse_report_work, mouse_report_work_handler);
static void mouse_timer_handler(struct k_timer *timer);
static K_TIMER_DEFINE(mouse_timer, mouse_timer_handler, NULL);

static uint8_t mouse_step;

/* clang-format off */
static const uint8_t mouse_descriptor[] = {
	HID_USAGE_PAGE(HID_USAGE_GEN_DESKTOP),
	HID_USAGE(HID_USAGE_GEN_DESKTOP_MOUSE),
	HID_COLLECTION(HID_COLLECTION_APPLICATION),
		HID_REPORT_ID(MOUSE_REPORT_ID),
		HID_USAGE(HID_USAGE_GEN_DESKTOP_POINTER),
		HID_COLLECTION(HID_COLLECTION_PHYSICAL),
			HID_USAGE_PAGE(HID_USAGE_GEN_BUTTON),
			HID_USAGE_MIN8(1),
			HID_USAGE_MAX8(8),
			HID_LOGICAL_MIN8(0),
			HID_LOGICAL_MAX8(1),
			HID_REPORT_COUNT(8),
			HID_REPORT_SIZE(1),
			HID_INPUT(0x02),
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

/* Integer sine table for 64-step circular movement, amplitude ~10 pixels */
static const int8_t sine_table[MOUSE_CIRCLE_STEPS] = {
	  0,   1,   2,   3,   4,   5,   6,   6,   7,   8,   8,   9,   9,   9,  10,  10,
	 10,  10,  10,   9,   9,   9,   8,   8,   7,   6,   6,   5,   4,   3,   2,   1,
	  0,  -1,  -2,  -3,  -4,  -5,  -6,  -6,  -7,  -8,  -8,  -9,  -9,  -9, -10, -10,
	-10, -10, -10,  -9,  -9,  -9,  -8,  -8,  -7,  -6,  -6,  -5,  -4,  -3,  -2,  -1,
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

	/* Boot Protocol mouse report: buttons(1) + X(1) + Y(1), no Report ID.
	 * Report Protocol mouse report: Report ID(1) + buttons(1) + X(1) +
	 * Y(1) + wheel(1), matching mouse_descriptor above.
	 */
	if (hid_boot_mode) {
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

	err = bt_hid_device_input_report(default_hid, buf);
	if (err != 0) {
		printk("Failed to send input report (err %d)\n", err);
		net_buf_unref(buf);
		return;
	}

	mouse_step = (mouse_step + 1) % MOUSE_CIRCLE_STEPS;
}

static void mouse_report_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	send_mouse_report();
}

static void mouse_timer_handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	k_work_submit(&mouse_report_work);
}

static void hid_connected_cb(struct bt_hid_device *hid)
{
	printk("HID connected\n");

	default_hid = hid;

	/* A new connection starts in Report Protocol Mode */
	hid_boot_mode = false;

	mouse_step = 0;
	k_timer_start(&mouse_timer, K_MSEC(MOUSE_REPORT_INTERVAL_MS),
		      K_MSEC(MOUSE_REPORT_INTERVAL_MS));
}

static void hid_disconnected_cb(struct bt_hid_device *hid)
{
	ARG_UNUSED(hid);

	printk("HID disconnected\n");

	k_timer_stop(&mouse_timer);
	default_hid = NULL;

	if (vcu_unplug_pending) {
		vcu_unplug_pending = false;
		bt_br_unpair(&vcu_peer);
	}
}

static int hid_set_report_cb(struct bt_hid_device *hid, uint8_t type, struct net_buf *buf)
{
	uint8_t report_id = 0;

	ARG_UNUSED(hid);

	/* The Report ID byte is only present when the report descriptor in use
	 * declares Report IDs, which the Boot Protocol reports do not.
	 */
	if (!hid_boot_mode) {
		if (buf->len < sizeof(report_id)) {
			return -EINVAL;
		}

		report_id = net_buf_pull_u8(buf);

		if (report_id != MOUSE_REPORT_ID) {
			/* Mapped to ERR_INVALID_REPORT_ID by the stack */
			return -ENOENT;
		}
	}

	/* A mouse has no state the host needs to push, so the payload is only
	 * logged here. A device with OUTPUT or FEATURE reports would apply it,
	 * and reject the types its descriptor does not declare.
	 */
	printk("Set Report: type %u id %u len %u\n", type, report_id, buf->len);

	return 0;
}

static int hid_get_report_cb(struct bt_hid_device *hid, uint8_t type, bool size_present,
			     struct net_buf *req, struct net_buf *rsp)
{
	/* Idle mouse report: Report Protocol adds the Report ID and the wheel
	 * byte, Boot Protocol carries buttons + X + Y only.
	 */
	static const uint8_t report[] = {MOUSE_REPORT_ID, 0x00, 0x00, 0x00, 0x00};
	static const uint8_t boot_report[] = {0x00, 0x00, 0x00};
	uint16_t buffer_size = 0;
	uint8_t report_id = 0;
	const uint8_t *data;
	uint16_t len;

	ARG_UNUSED(hid);

	/* The Report ID byte is only present when the report descriptor in use
	 * declares Report IDs, which the Boot Protocol reports do not.
	 */
	if (!hid_boot_mode) {
		if (req->len < sizeof(report_id)) {
			return -EINVAL;
		}

		report_id = net_buf_pull_u8(req);
	}

	if (size_present) {
		if (req->len < sizeof(buffer_size)) {
			return -EINVAL;
		}

		buffer_size = net_buf_pull_le16(req);
	}

	printk("Get Report: type %u id %u size %u\n", type, report_id, buffer_size);

	/* The mouse descriptor above declares INPUT reports only */
	if (type != BT_HID_REPORT_TYPE_INPUT) {
		/* Mapped to ERR_INVALID_PARAMETER by the stack */
		return -EINVAL;
	}

	if (!hid_boot_mode && (report_id != MOUSE_REPORT_ID)) {
		/* Mapped to ERR_INVALID_REPORT_ID by the stack */
		return -ENOENT;
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

static void hid_output_report_cb(struct bt_hid_device *hid, struct net_buf *buf)
{
	uint8_t report_id;

	ARG_UNUSED(hid);

	if (hid_boot_mode) {
		printk("Output report: len %u\n", buf->len);
		return;
	}

	if (buf->len < sizeof(report_id)) {
		printk("Malformed output report (len %u)\n", buf->len);
		return;
	}

	report_id = net_buf_pull_u8(buf);
	printk("Output report: id %u len %u\n", report_id, buf->len);
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
	.output_report = hid_output_report_cb,
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
