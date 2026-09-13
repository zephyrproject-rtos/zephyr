/* main.c - Classic Bluetooth HID Device (keyboard) sample */

/*
 * Copyright (c) 2026 SiFli Technologies(Nanjing) Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ctype.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <zephyr/types.h>

#include <zephyr/console/console.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/classic/hid_device.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/settings/settings.h>
#include <zephyr/usb/class/hid.h>

#define BT_HID_DEVICE_VERSION      0x0101
#define BT_HID_PARSER_VERSION      0x0111
#define BT_HID_DEVICE_SUBCLASS     0x40 /* Boot keyboard */
#define BT_HID_DEVICE_COUNTRY_CODE 0x21
#define BT_L2CAP_PSM_HID_CONTROL   0x0011
#define BT_L2CAP_PSM_HID_INTERRUPT 0x0013

#define BT_HID_LANG_ID_ENGLISH 0x0409
#define BT_HID_LANG_ID_OFFSET  0x0100

#define BT_HID_SUPERVISION_TIMEOUT  1000
#define BT_HID_SSR_HOST_MAX_LATENCY 240
#define BT_HID_SSR_HOST_MIN_TIMEOUT 0

#define KB_MOD_KEY   0
#define KB_RESERVED  1
#define KB_KEY_CODE1 2
#define KB_REPORT_SIZE 8

NET_BUF_POOL_FIXED_DEFINE(hid_tx_pool, 2, BT_L2CAP_BUF_SIZE(CONFIG_BT_L2CAP_TX_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static const uint8_t keyboard_descriptor[] = HID_KEYBOARD_REPORT_DESC();

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
				BT_SDP_ARRAY_16(BT_L2CAP_PSM_HID_CONTROL)
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
				BT_SDP_ARRAY_16(BT_HID_DEVICE_VERSION)
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
					BT_SDP_ARRAY_16(BT_L2CAP_PSM_HID_INTERRUPT)
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
	BT_SDP_SERVICE_NAME("HID Keyboard"),
	{
		BT_SDP_ATTR_HID_DEVICE_RELEASE_NUMBER,
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
			BT_SDP_ARRAY_16(BT_HID_DEVICE_VERSION)
		}
	},
	{
		BT_SDP_ATTR_HID_PARSER_VERSION,
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
			BT_SDP_ARRAY_16(BT_HID_PARSER_VERSION)
		}
	},
	{
		BT_SDP_ATTR_HID_DEVICE_SUBCLASS,
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
			BT_SDP_ARRAY_8(BT_HID_DEVICE_SUBCLASS)
		}
	},
	{
		BT_SDP_ATTR_HID_COUNTRY_CODE,
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
			BT_SDP_ARRAY_8(BT_HID_DEVICE_COUNTRY_CODE)
		}
	},
	{
		BT_SDP_ATTR_HID_VIRTUAL_CABLE,
		{
			BT_SDP_TYPE_SIZE(BT_SDP_BOOL),
			BT_SDP_ARRAY_8(0x01)
		}
	},
	{
		BT_SDP_ATTR_HID_RECONNECT_INITIATE,
		{
			BT_SDP_TYPE_SIZE(BT_SDP_BOOL),
			BT_SDP_ARRAY_8(0x01)
		}
	},
	BT_SDP_LIST(
		BT_SDP_ATTR_HID_DESCRIPTOR_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ16, sizeof(keyboard_descriptor) + 8),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ16, sizeof(keyboard_descriptor) + 5),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
				BT_SDP_ARRAY_8(0x22),
			},
			{
				BT_SDP_TYPE_SIZE_VAR(BT_SDP_TEXT_STR16,
						     sizeof(keyboard_descriptor)),
				keyboard_descriptor,
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
				BT_SDP_ARRAY_16(BT_HID_LANG_ID_ENGLISH),
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(BT_HID_LANG_ID_OFFSET),
			}
			),
		}
		)
	),
	{
		BT_SDP_ATTR_HID_BOOT_DEVICE,
		{
			BT_SDP_TYPE_SIZE(BT_SDP_BOOL),
			BT_SDP_ARRAY_8(0x01)
		}
	},
	{
		BT_SDP_ATTR_HID_PROFILE_VERSION,
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
			BT_SDP_ARRAY_16(0x0101)
		}
	},
	{
		BT_SDP_ATTR_HID_BATTERY_POWER,
		{
			BT_SDP_TYPE_SIZE(BT_SDP_BOOL),
			BT_SDP_ARRAY_8(0x01)
		}
	},
	{
		BT_SDP_ATTR_HID_REMOTE_WAKEUP,
		{
			BT_SDP_TYPE_SIZE(BT_SDP_BOOL),
			BT_SDP_ARRAY_8(0x01)
		}
	},
	{
		BT_SDP_ATTR_HID_NORMALLY_CONNECTABLE,
		{
			BT_SDP_TYPE_SIZE(BT_SDP_BOOL),
			BT_SDP_ARRAY_8(0x01)
		}
	},
	{
		BT_SDP_ATTR_HID_SUPERVISION_TIMEOUT,
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
			BT_SDP_ARRAY_16(BT_HID_SUPERVISION_TIMEOUT)
		}
	},
	{
		BT_SDP_ATTR_HID_SSR_HOST_MAX_LATENCY,
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
			BT_SDP_ARRAY_16(BT_HID_SSR_HOST_MAX_LATENCY)
		}
	},
	{
		BT_SDP_ATTR_HID_SSR_HOST_MIN_TIMEOUT,
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
			BT_SDP_ARRAY_16(BT_HID_SSR_HOST_MIN_TIMEOUT)
		}
	},
};

static struct bt_sdp_record hid_rec = BT_SDP_RECORD(hid_attrs);

static struct bt_hid_device *default_hid;
static bool hid_boot_mode;
static bool hid_suspended;
static bool vcu_unplug_pending;
static bt_addr_t vcu_peer;
static uint8_t kb_leds;
static uint8_t kb_report[KB_REPORT_SIZE];

struct ascii_hid_map {
	char c;
	uint8_t key;
	uint8_t modifier;
};

/* US keyboard map for printable ASCII and common controls. */
static const struct ascii_hid_map ascii_map[] = {
	{ '\n', HID_KEY_ENTER, HID_KBD_MODIFIER_NONE },
	{ '\r', HID_KEY_ENTER, HID_KBD_MODIFIER_NONE },
	{ '\b', HID_KEY_BACKSPACE, HID_KBD_MODIFIER_NONE },
	{ '\t', HID_KEY_TAB, HID_KBD_MODIFIER_NONE },
	{ 0x1b, HID_KEY_ESC, HID_KBD_MODIFIER_NONE },
	{ ' ', HID_KEY_SPACE, HID_KBD_MODIFIER_NONE },
	{ '-', HID_KEY_MINUS, HID_KBD_MODIFIER_NONE },
	{ '=', HID_KEY_EQUAL, HID_KBD_MODIFIER_NONE },
	{ '[', HID_KEY_LEFTBRACE, HID_KBD_MODIFIER_NONE },
	{ ']', HID_KEY_RIGHTBRACE, HID_KBD_MODIFIER_NONE },
	{ '\\', HID_KEY_BACKSLASH, HID_KBD_MODIFIER_NONE },
	{ ';', HID_KEY_SEMICOLON, HID_KBD_MODIFIER_NONE },
	{ '\'', HID_KEY_APOSTROPHE, HID_KBD_MODIFIER_NONE },
	{ '`', HID_KEY_GRAVE, HID_KBD_MODIFIER_NONE },
	{ ',', HID_KEY_COMMA, HID_KBD_MODIFIER_NONE },
	{ '.', HID_KEY_DOT, HID_KBD_MODIFIER_NONE },
	{ '/', HID_KEY_SLASH, HID_KBD_MODIFIER_NONE },
	{ '!', HID_KEY_1, HID_KBD_MODIFIER_LEFT_SHIFT },
	{ '@', HID_KEY_2, HID_KBD_MODIFIER_LEFT_SHIFT },
	{ '#', HID_KEY_3, HID_KBD_MODIFIER_LEFT_SHIFT },
	{ '$', HID_KEY_4, HID_KBD_MODIFIER_LEFT_SHIFT },
	{ '%', HID_KEY_5, HID_KBD_MODIFIER_LEFT_SHIFT },
	{ '^', HID_KEY_6, HID_KBD_MODIFIER_LEFT_SHIFT },
	{ '&', HID_KEY_7, HID_KBD_MODIFIER_LEFT_SHIFT },
	{ '*', HID_KEY_8, HID_KBD_MODIFIER_LEFT_SHIFT },
	{ '(', HID_KEY_9, HID_KBD_MODIFIER_LEFT_SHIFT },
	{ ')', HID_KEY_0, HID_KBD_MODIFIER_LEFT_SHIFT },
	{ '_', HID_KEY_MINUS, HID_KBD_MODIFIER_LEFT_SHIFT },
	{ '+', HID_KEY_EQUAL, HID_KBD_MODIFIER_LEFT_SHIFT },
	{ '{', HID_KEY_LEFTBRACE, HID_KBD_MODIFIER_LEFT_SHIFT },
	{ '}', HID_KEY_RIGHTBRACE, HID_KBD_MODIFIER_LEFT_SHIFT },
	{ '|', HID_KEY_BACKSLASH, HID_KBD_MODIFIER_LEFT_SHIFT },
	{ ':', HID_KEY_SEMICOLON, HID_KBD_MODIFIER_LEFT_SHIFT },
	{ '"', HID_KEY_APOSTROPHE, HID_KBD_MODIFIER_LEFT_SHIFT },
	{ '~', HID_KEY_GRAVE, HID_KBD_MODIFIER_LEFT_SHIFT },
	{ '<', HID_KEY_COMMA, HID_KBD_MODIFIER_LEFT_SHIFT },
	{ '>', HID_KEY_DOT, HID_KBD_MODIFIER_LEFT_SHIFT },
	{ '?', HID_KEY_SLASH, HID_KBD_MODIFIER_LEFT_SHIFT },
};

static bool ascii_to_hid(char c, uint8_t *key, uint8_t *modifier)
{
	*modifier = HID_KBD_MODIFIER_NONE;

	if ((c >= 'a') && (c <= 'z')) {
		*key = HID_KEY_A + (c - 'a');
		return true;
	}

	if ((c >= 'A') && (c <= 'Z')) {
		*key = HID_KEY_A + (c - 'A');
		*modifier = HID_KBD_MODIFIER_LEFT_SHIFT;
		return true;
	}

	if ((c >= '1') && (c <= '9')) {
		*key = HID_KEY_1 + (c - '1');
		return true;
	}

	if (c == '0') {
		*key = HID_KEY_0;
		return true;
	}

	for (size_t i = 0; i < ARRAY_SIZE(ascii_map); i++) {
		if (ascii_map[i].c == c) {
			*key = ascii_map[i].key;
			*modifier = ascii_map[i].modifier;
			return true;
		}
	}

	return false;
}

static int send_keyboard_report(struct bt_hid_device *hid, const uint8_t report[KB_REPORT_SIZE])
{
	struct net_buf *buf;
	int err;

	buf = bt_hid_device_create_pdu(&hid_tx_pool);
	if (buf == NULL) {
		return -ENOMEM;
	}

	/* Boot/Report keyboard: modifier(1) + reserved(1) + keycodes(6). */
	net_buf_add_mem(buf, report, KB_REPORT_SIZE);

	err = bt_hid_device_input_report(hid, buf);
	if (err != 0) {
		net_buf_unref(buf);
	}

	return err;
}

static int send_key_press(char c)
{
	uint8_t key;
	uint8_t modifier;
	int err;

	if ((default_hid == NULL) || hid_suspended) {
		return -ENOTCONN;
	}

	if (!ascii_to_hid(c, &key, &modifier)) {
		return -ENOTSUP;
	}

	memset(kb_report, 0, sizeof(kb_report));
	kb_report[KB_MOD_KEY] = modifier;
	kb_report[KB_KEY_CODE1] = key;

	err = send_keyboard_report(default_hid, kb_report);
	if (err != 0) {
		return err;
	}

	/* Key release. */
	memset(kb_report, 0, sizeof(kb_report));
	return send_keyboard_report(default_hid, kb_report);
}

static void handle_led_report(const uint8_t *data, size_t len)
{
	if (len < 1U) {
		return;
	}

	kb_leds = data[0];
	printk("Keyboard LEDs: NUM=%u CAPS=%u SCROLL=%u\n",
	       (kb_leds & HID_KBD_LED_NUM_LOCK) != 0,
	       (kb_leds & HID_KBD_LED_CAPS_LOCK) != 0,
	       (kb_leds & HID_KBD_LED_SCROLL_LOCK) != 0);
}

static void hid_connected_cb(struct bt_hid_device *hid)
{
	printk("HID connected\n");
	default_hid = hid;
	hid_suspended = false;
	memset(kb_report, 0, sizeof(kb_report));
}

static void hid_disconnected_cb(struct bt_hid_device *hid)
{
	ARG_UNUSED(hid);

	printk("HID disconnected\n");
	default_hid = NULL;
	hid_suspended = false;

	if (vcu_unplug_pending) {
		vcu_unplug_pending = false;
		bt_br_unpair(&vcu_peer);
	}
}

static int hid_set_report_cb(struct bt_hid_device *hid, uint8_t type, struct net_buf *buf)
{
	ARG_UNUSED(hid);

	if (type != BT_HID_REPORT_TYPE_OUTPUT) {
		return -EINVAL;
	}

	handle_led_report(buf->data, buf->len);
	return 0;
}

static int hid_get_report_cb(struct bt_hid_device *hid, uint8_t type, bool size_present,
			     struct net_buf *req, struct net_buf *rsp)
{
	ARG_UNUSED(hid);

	if (size_present) {
		if (req->len < sizeof(uint16_t)) {
			return -EINVAL;
		}

		(void)net_buf_pull_le16(req);
	}

	if (type == BT_HID_REPORT_TYPE_INPUT) {
		net_buf_add_mem(rsp, kb_report, sizeof(kb_report));
		return 0;
	}

	if (type == BT_HID_REPORT_TYPE_OUTPUT) {
		net_buf_add_u8(rsp, kb_leds);
		return 0;
	}

	return -ENOENT;
}

static int hid_set_protocol_cb(struct bt_hid_device *hid, uint8_t protocol)
{
	ARG_UNUSED(hid);

	hid_boot_mode = (protocol == BT_HID_PROTOCOL_BOOT_MODE);
	printk("HID set protocol %s\n", hid_boot_mode ? "boot" : "report");

	return 0;
}

static void hid_output_report_cb(struct bt_hid_device *hid, struct net_buf *buf)
{
	ARG_UNUSED(hid);

	handle_led_report(buf->data, buf->len);
}

static void hid_vc_unplug_cb(struct bt_hid_device *hid)
{
	struct bt_conn *conn = bt_hid_device_get_conn(hid);

	printk("HID virtual cable unplug\n");

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
		hid_suspended = true;
	} else if (default_hid != NULL) {
		hid_suspended = false;
	} else {
		/* No active HID session. */
		hid_suspended = false;
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

	err = bt_hid_device_register(&hid_cb);
	if (err != 0) {
		printk("HID device register failed (err %d)\n", err);
		return;
	}

	err = bt_sdp_register_service(&hid_rec);
	if ((err != 0) && (err != -EEXIST)) {
		printk("HID SDP register failed (err %d)\n", err);
		bt_hid_device_unregister();
		return;
	}

	err = bt_br_set_connectable(true, NULL);
	if (err != 0) {
		printk("BR/EDR set connectable failed (err %d)\n", err);
		return;
	}

	err = bt_br_set_discoverable(true, false);
	if (err != 0) {
		printk("BR/EDR set discoverable failed (err %d)\n", err);
		return;
	}

	printk("HID keyboard ready and discoverable as %s\n", CONFIG_BT_DEVICE_NAME);
	printk("Type characters on the console to send HID key reports\n");
}

static void console_input_loop(void)
{
	while (1) {
		uint8_t c = console_getchar();
		int err;

		/* Echo received key back to the console. */
		if ((c == '\r') || (c == '\n')) {
			(void)console_putchar('\r');
			(void)console_putchar('\n');
		} else {
			(void)console_putchar(c);
		}

		if (default_hid == NULL) {
			printk("Ignoring '%c' (0x%02x): HID host not connected\n",
			       isprint(c) ? c : '?', c);
			continue;
		}

		err = send_key_press(c);
		if (err == -ENOTSUP) {
			printk("Unsupported character 0x%02x\n", c);
		} else if (err != 0) {
			printk("Failed to send key report (err %d)\n", err);
		}
	}
}

int main(void)
{
	int err;

	console_init();

	err = bt_enable(bt_ready);
	if (err != 0) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	console_input_loop();
	return 0;
}
