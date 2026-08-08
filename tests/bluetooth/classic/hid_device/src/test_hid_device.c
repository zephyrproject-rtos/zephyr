/* test_hid_device.c - Bluetooth classic HID Device functional test */

/*
 * Copyright 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/types.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/classic/hid_device.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/usb/class/hid.h>

#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_string_conv.h>

#include "host/shell/bt.h"
#include "common/bt_shell_private.h"

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
#define MOUSE_REPORT_ID  2
#define HID_TX_BUF_COUNT 4

NET_BUF_POOL_FIXED_DEFINE(hid_pool, HID_TX_BUF_COUNT, BT_L2CAP_BUF_SIZE(CONFIG_BT_L2CAP_TX_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

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
	BT_SDP_SERVICE_NAME("HID CONTROL"),
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

static struct bt_hid_device *default_hid;
static bool hid_registered;

/* Protocol mode tracked from the Set_Protocol callback: the stack owns the
 * mode used for Get_Protocol responses, and does not expose a getter.
 */
static bool hid_boot_mode;

static void hid_connected_cb(struct bt_hid_device *hid)
{
	default_hid = hid;

	/* A new connection starts in Report Protocol Mode */
	hid_boot_mode = false;

	bt_shell_print("HID connected");
}

static void hid_disconnected_cb(struct bt_hid_device *hid)
{
	ARG_UNUSED(hid);

	default_hid = NULL;

	bt_shell_print("HID disconnected");
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
	}

	bt_shell_print("set_report type %u id %u len %u", type, report_id, buf->len);

	if (!hid_boot_mode && (report_id != MOUSE_REPORT_ID)) {
		/* Mapped to ERR_INVALID_REPORT_ID by the stack */
		return -ENOENT;
	}

	return 0;
}

static int hid_get_report_cb(struct bt_hid_device *hid, uint8_t type, bool size_present,
			     struct net_buf *req, struct net_buf *rsp)
{
	static const uint8_t report[] = {MOUSE_REPORT_ID, 0x00, 0x00, 0x00, 0x00};
	static const uint8_t boot_report[] = {0x00, 0x00, 0x00};
	uint16_t buffer_size = 0;
	uint8_t report_id = 0;
	const uint8_t *data;
	uint16_t len;

	ARG_UNUSED(hid);

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

	bt_shell_print("get_report type %u id %u size %u", type, report_id, buffer_size);

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

	hid_boot_mode = (protocol == BT_HID_PROTOCOL_BOOT_MODE);

	bt_shell_print("set_protocol %u", protocol);

	return 0;
}

static void hid_output_report_cb(struct bt_hid_device *hid, struct net_buf *buf)
{
	uint8_t report_id = 0;

	ARG_UNUSED(hid);

	if (!hid_boot_mode) {
		if (buf->len < sizeof(report_id)) {
			bt_shell_warn("malformed output report (len %u)", buf->len);
			return;
		}

		report_id = net_buf_pull_u8(buf);
	}

	bt_shell_print("output_report id %u len %u", report_id, buf->len);
}

static void hid_vc_unplug_cb(struct bt_hid_device *hid)
{
	ARG_UNUSED(hid);

	/* The bonding information is intentionally left in place: this test
	 * re-pairs on every case and the peer address is reused.
	 */
	bt_shell_print("virtual_cable_unplug");
}

static void hid_suspend_cb(struct bt_hid_device *hid, bool suspended)
{
	ARG_UNUSED(hid);

	bt_shell_print("%s", suspended ? "suspended" : "exit_suspend");
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

static int cmd_register(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (hid_registered) {
		shell_error(sh, "HID already registered");
		return -EALREADY;
	}

	err = bt_hid_device_register(&hid_cb);
	if (err != 0) {
		shell_error(sh, "register failed (%d)", err);
		return err;
	}

	err = bt_sdp_register_service(&hid_rec);
	if ((err != 0) && (err != -EEXIST)) {
		shell_error(sh, "SDP register failed (%d)", err);
		bt_hid_device_unregister();
		return err;
	}

	hid_registered = true;
	shell_print(sh, "HID registered");

	return 0;
}

static int cmd_unregister(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (!hid_registered) {
		shell_error(sh, "HID not registered");
		return -ENOEXEC;
	}

	err = bt_hid_device_unregister();
	if (err != 0) {
		shell_error(sh, "unregister failed (%d)", err);
		return err;
	}

	hid_registered = false;
	default_hid = NULL;
	shell_print(sh, "HID unregistered");

	return 0;
}

static int cmd_connect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (!hid_registered) {
		shell_error(sh, "HID not registered");
		return -ENOEXEC;
	}

	if (default_conn == NULL) {
		shell_error(sh, "not connected");
		return -ENOEXEC;
	}

	err = bt_hid_device_connect(default_conn, &default_hid);
	if (err != 0) {
		shell_error(sh, "connect failed (%d)", err);
		return err;
	}

	return 0;
}

static int cmd_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (default_hid == NULL) {
		shell_error(sh, "not connected");
		return -ENOEXEC;
	}

	err = bt_hid_device_disconnect(default_hid);
	if (err != 0) {
		shell_error(sh, "disconnect failed (%d)", err);
		return err;
	}

	return 0;
}

static int cmd_send(const struct shell *sh, size_t argc, char *argv[])
{
	long button, dx, dy, wheel = 0;
	struct net_buf *buf;
	int err = 0;

	if (default_hid == NULL) {
		shell_error(sh, "not connected");
		return -ENOEXEC;
	}

	button = shell_strtol(argv[1], 0, &err);
	dx = shell_strtol(argv[2], 0, &err);
	dy = shell_strtol(argv[3], 0, &err);
	if (argc >= 5U) {
		wheel = shell_strtol(argv[4], 0, &err);
	}

	if (err != 0) {
		shell_error(sh, "invalid parameter");
		return -EINVAL;
	}

	if ((button < 0) || (button > 0xFF) || (dx < -127) || (dx > 127) || (dy < -127) ||
	    (dy > 127) || (!hid_boot_mode && ((wheel < -127) || (wheel > 127)))) {
		shell_error(sh, "value out of bounds");
		return -ERANGE;
	}

	buf = bt_hid_device_create_pdu(&hid_pool);
	if (buf == NULL) {
		shell_error(sh, "failed to create PDU");
		return -ENOMEM;
	}

	/* Boot Protocol mouse report: buttons(1) + X(1) + Y(1), no Report ID.
	 * Report Protocol mouse report: Report ID(1) + buttons(1) + X(1) +
	 * Y(1) + wheel(1), matching mouse_descriptor above.
	 */
	if (hid_boot_mode) {
		net_buf_add_u8(buf, (uint8_t)button);
		net_buf_add_u8(buf, (uint8_t)dx);
		net_buf_add_u8(buf, (uint8_t)dy);
	} else {
		net_buf_add_u8(buf, MOUSE_REPORT_ID);
		net_buf_add_u8(buf, (uint8_t)button);
		net_buf_add_u8(buf, (uint8_t)dx);
		net_buf_add_u8(buf, (uint8_t)dy);
		net_buf_add_u8(buf, (uint8_t)wheel);
	}

	err = bt_hid_device_input_report(default_hid, buf);
	if (err != 0) {
		net_buf_unref(buf);
		shell_error(sh, "send failed (%d)", err);
		return err;
	}

	shell_print(sh, "sent");

	return 0;
}

static int cmd_vcu(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (default_hid == NULL) {
		shell_error(sh, "not connected");
		return -ENOEXEC;
	}

	err = bt_hid_device_virtual_cable_unplug(default_hid);
	if (err != 0) {
		shell_error(sh, "vcu failed (%d)", err);
		return err;
	}

	return 0;
}

/* clang-format off */
SHELL_STATIC_SUBCMD_SET_CREATE(hid_dev_cmds,
	SHELL_CMD_ARG(register, NULL, "register HID device", cmd_register, 1, 0),
	SHELL_CMD_ARG(unregister, NULL, "unregister HID device", cmd_unregister, 1, 0),
	SHELL_CMD_ARG(connect, NULL, "HID connect", cmd_connect, 1, 0),
	SHELL_CMD_ARG(disconnect, NULL, "HID disconnect", cmd_disconnect, 1, 0),
	SHELL_CMD_ARG(send, NULL, "<button> <X> <Y> [wheel]", cmd_send, 4, 1),
	SHELL_CMD_ARG(vcu, NULL, "virtual cable unplug", cmd_vcu, 1, 0),
	SHELL_SUBCMD_SET_END
);
/* clang-format on */

static int cmd_hid_dev(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1U) {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);

	return -ENOEXEC;
}

SHELL_CMD_ARG_REGISTER(hid_dev, &hid_dev_cmds, "HID Device test commands", cmd_hid_dev, 1, 1);
