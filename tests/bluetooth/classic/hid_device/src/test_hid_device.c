/*
 * Copyright 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include <zephyr/settings/settings.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/classic/hid_device.h>
#include <zephyr/bluetooth/hci.h>

#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_string_conv.h>

#include "host/shell/bt.h"
#include "common/bt_shell_private.h"

#define HELP_NONE "[none]"

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

#define MOUSE_REPORT_ID 2
#define HID_TX_BUF_COUNT 4

NET_BUF_POOL_FIXED_DEFINE(hid_pool, HID_TX_BUF_COUNT, BT_L2CAP_BUF_SIZE(CONFIG_BT_L2CAP_TX_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static uint8_t mouse_descriptor[] = {
	0x05, 0x01,
	0x09, 0x02,
	0xa1, 0x01,
	0x85, MOUSE_REPORT_ID,
	0x09, 0x01,
	0xa1, 0x00,
	0x05, 0x09,
	0x19, 0x01,
	0x29, 0x08,
	0x15, 0x00,
	0x25, 0x01,
	0x95, 0x08,
	0x75, 0x01,
	0x81, 0x02,
	0x05, 0x01,
	0x09, 0x30,
	0x09, 0x31,
	0x09, 0x38,
	0x15, 0x81,
	0x25, 0x7f,
	0x75, 0x08,
	0x95, 0x03,
	0x81, 0x06,
	0xc0, 0xc0
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

static struct bt_hid_device *default_hid;
static bool hid_registered;
static struct bt_sdp_record hid_rec = BT_SDP_RECORD(hid_attrs);

static void hid_connect_cb(struct bt_hid_device *hid)
{
	bt_shell_print("HID connected");
	bt_hid_device_set_report_id(hid, true);
	bt_hid_device_set_boot_device(hid, true);
	default_hid = hid;
}

static void hid_disconnected_cb(struct bt_hid_device *hid)
{
	bt_shell_print("HID disconnected");
	default_hid = NULL;
}

static int hid_set_report_cb(struct bt_hid_device *hid, uint8_t type, uint8_t report_id,
			     struct net_buf *buf)
{
	bt_shell_print("set_report type %u id %u len %u", type, report_id, buf->len);

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
	int err;

	bt_shell_print("get_report type %u id %u size %u", type, report_id, buffer_size);

	if (!hid->boot_mode && report_id != MOUSE_REPORT_ID) {
		return -ENOENT;
	}

	buf = bt_hid_device_create_pdu(&hid_pool);
	if (!buf) {
		return -ENOMEM;
	}

	if (hid->boot_mode) {
		net_buf_add_mem(buf, boot_report, sizeof(boot_report));
	} else {
		net_buf_add_u8(buf, report_id);
		net_buf_add_mem(buf, report_data, sizeof(report_data));
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
	bt_shell_print("set_protocol %u", protocol);
	return 0;
}

static int hid_get_protocol_cb(struct bt_hid_device *hid)
{
	uint8_t protocol = hid->boot_mode ? BT_HID_PROTOCOL_BOOT_MODE
					   : BT_HID_PROTOCOL_REPORT_MODE;
	int err;

	bt_shell_print("get_protocol");

	err = bt_hid_device_get_protocol_response(hid, protocol);
	if (err != 0) {
		return err;
	}

	return 0;
}

static void hid_intr_data_cb(struct bt_hid_device *hid, uint8_t report_id, struct net_buf *buf)
{
	bt_shell_print("intr_data id %u len %u", report_id, buf->len);
}

static void hid_vc_unplug_cb(struct bt_hid_device *hid)
{
	bt_shell_print("virtual_cable_unplug");
}

static void hid_suspend_cb(struct bt_hid_device *hid, bool suspended)
{
	bt_shell_print("%s", suspended ? "suspended" : "exit_suspend");
}

static const struct bt_hid_device_cb hid_cb = {
	.connected = hid_connect_cb,
	.disconnected = hid_disconnected_cb,
	.set_report = hid_set_report_cb,
	.get_report = hid_get_report_cb,
	.set_protocol = hid_set_protocol_cb,
	.get_protocol = hid_get_protocol_cb,
	.intr_data = hid_intr_data_cb,
	.vc_unplug = hid_vc_unplug_cb,
	.suspend = hid_suspend_cb,
};

static int cmd_register(const struct shell *sh, int32_t argc, char *argv[])
{
	int err;

	if (hid_registered) {
		shell_error(sh, "already registered");
		return -EALREADY;
	}

	err = bt_hid_device_register(&hid_cb);
	if (err != 0) {
		shell_error(sh, "register failed (%d)", err);
		return err;
	}

	err = bt_sdp_register_service(&hid_rec);
	if (err != 0 && err != -EEXIST) {
		shell_error(sh, "SDP register failed (%d)", err);
		bt_hid_device_unregister();
		return err;
	}

	hid_registered = true;
	shell_print(sh, "registered");
	return 0;
}

static int cmd_unregister(const struct shell *sh, int32_t argc, char *argv[])
{
	int err;

	if (!hid_registered) {
		shell_error(sh, "not registered");
		return -ENOEXEC;
	}

	err = bt_hid_device_unregister();
	if (err != 0) {
		shell_error(sh, "unregister failed (%d)", err);
		return err;
	}

	hid_registered = false;
	default_hid = NULL;
	shell_print(sh, "unregistered");
	return 0;
}

static int cmd_connect(const struct shell *sh, int32_t argc, char *argv[])
{
	int err;

	if (!hid_registered) {
		shell_error(sh, "not registered");
		return -ENOEXEC;
	}

	if (!default_conn) {
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

static int cmd_disconnect(const struct shell *sh, int32_t argc, char *argv[])
{
	if (!default_hid) {
		shell_error(sh, "not connected");
		return -ENOEXEC;
	}

	bt_hid_device_disconnect(default_hid);
	return 0;
}

static int cmd_reconnect(const struct shell *sh, int32_t argc, char *argv[])
{
	int err;

	if (!hid_registered) {
		shell_error(sh, "not registered");
		return -ENOEXEC;
	}

	if (!default_conn) {
		shell_error(sh, "not connected");
		return -ENOEXEC;
	}

	err = bt_hid_device_reconnect(default_conn, &default_hid);
	if (err != 0) {
		shell_error(sh, "reconnect failed (%d)", err);
		return err;
	}

	return 0;
}

static int cmd_send(const struct shell *sh, size_t argc, char *argv[])
{
	long button, dx, dy, wheel = 0;
	struct net_buf *buf;
	int err = 0;

	if (!default_hid) {
		shell_error(sh, "not connected");
		return -ENOEXEC;
	}

	if (argc < 4) {
		shell_error(sh, "Usage: send <button> <X> <Y> [wheel]");
		return -ENOEXEC;
	}

	button = shell_strtol(argv[1], 0, &err);
	dx = shell_strtol(argv[2], 0, &err);
	dy = shell_strtol(argv[3], 0, &err);
	if (argc >= 5) {
		wheel = shell_strtol(argv[4], 0, &err);
	}

	if (err) {
		shell_error(sh, "invalid parameter");
		return -EINVAL;
	}

	buf = bt_hid_device_create_pdu(&hid_pool);
	if (!buf) {
		shell_error(sh, "failed to create PDU");
		return -ENOMEM;
	}

	if (default_hid->boot_mode) {
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

	err = bt_hid_device_send_input_report(default_hid, buf);
	if (err != 0) {
		net_buf_unref(buf);
		shell_error(sh, "send failed (%d)", err);
		return err;
	}

	shell_print(sh, "sent");
	return 0;
}

static int cmd_vcu(const struct shell *sh, int32_t argc, char *argv[])
{
	int err;

	if (!default_hid) {
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

SHELL_STATIC_SUBCMD_SET_CREATE(hid_dev_cmds,
	SHELL_CMD_ARG(register, NULL, "register HID device", cmd_register, 1, 0),
	SHELL_CMD_ARG(unregister, NULL, "unregister HID device", cmd_unregister, 1, 0),
	SHELL_CMD_ARG(connect, NULL, "HID connect", cmd_connect, 1, 0),
	SHELL_CMD_ARG(disconnect, NULL, "HID disconnect", cmd_disconnect, 1, 0),
	SHELL_CMD_ARG(reconnect, NULL, "HID reconnect", cmd_reconnect, 1, 0),
	SHELL_CMD_ARG(send, NULL, "<button> <X> <Y> [wheel]", cmd_send, 4, 1),
	SHELL_CMD_ARG(vcu, NULL, "virtual cable unplug", cmd_vcu, 1, 0),
	SHELL_SUBCMD_SET_END
);

static int cmd_hid_dev(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);
	return -ENOEXEC;
}

SHELL_CMD_ARG_REGISTER(hid_dev, &hid_dev_cmds, "Bluetooth HID Device test commands",
		       cmd_hid_dev, 1, 0);
