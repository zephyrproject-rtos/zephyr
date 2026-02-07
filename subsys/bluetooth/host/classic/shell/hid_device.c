/*
 * Copyright 2025 Xiaomi Corporation
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

#include "host/shell/bt.h"
#include "common/bt_shell_private.h"

#define HELP_NONE "[none]"

#define BT_HID_DEVICE_VERSION      0x0101
#define BT_HID_PARSER_VERSION      0x0111
#define BT_HID_DEVICE_SUBCLASS     0xc0
#define BT_HID_DEVICE_COUNTRY_CODE 0x21
#define BT_HID_PROTO_INTERRUPT     0x0013

#define BT_HID_LANG_ID_ENGLISH 0x0409
#define BT_HID_LANG_ID_OFFSET  0x0100

#define BT_HID_SUPERVISION_TIMEOUT 1000
#define BT_HID_MAX_LATENCY         240
#define BT_HID_MIN_LATENCY         0

NET_BUF_POOL_FIXED_DEFINE(pool, 1, BT_HID_MAX_MTU, 8, NULL);

static uint8_t mouse_descriptor[] = {
	0x05, 0x01, /* USAGE_PAGE (Generic Desktop Controls) */
	0x09, 0x02, /* USAGE (Mouse) */
	0xa1, 0x01, /* COLLECTION (Application (mouse, keyboard)) */
	0x85, 0x02, /* REPORT_ID (2) */
	0x09, 0x01, /* USAGE (Pointer) */
	0xa1, 0x00, /* COLLECTION (Physical (group of axes)) */
	0x05, 0x09, /* usage page(Button) */
	0x19, 0x01, /* Usage Minimum */
	0x29, 0x08, /* Usage Maximum */
	0x15, 0x00, /* Logical Minimum */
	0x25, 0x01, /* Logical Maximum */
	0x95, 0x08, /* Report Count */
	0x75, 0x01, /* Report size */
	0x81, 0x02, /* input() */
	0x05, 0x01, /* usage page() */
	0x09, 0x30, /* usage() */
	0x09, 0x31, /* usage() */
	0x09, 0x38, /* usage() */
	0x15, 0x81, /* logical minimum */
	0x25, 0x7f, /* logical maximum */
	0x75, 0x08, /* report size */
	0x95, 0x03, /* report count */
	0x81, 0x06, /* input */
	0xc0, 0xc0  /* END_COLLECTION */
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
				BT_SDP_ARRAY_16(BT_SDP_PROTO_HID)
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
	BT_SDP_LIST(BT_SDP_ATTR_PROFILE_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_HID_SVCLASS)},
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
					BT_SDP_ARRAY_16(BT_HID_PROTO_INTERRUPT)
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
			BT_SDP_ARRAY_16(BT_HID_DEVICE_SUBCLASS)
		}
	},
	{
		BT_SDP_ATTR_HID_COUNTRY_CODE,
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
			BT_SDP_ARRAY_16(BT_HID_DEVICE_COUNTRY_CODE)
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
				BT_SDP_TYPE_SIZE_VAR(BT_SDP_TEXT_STR16,
					sizeof(mouse_descriptor)
				),
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
		BT_SDP_ATTR_HID_SUPERVISION_TIMEOUT,
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
			BT_SDP_ARRAY_16(BT_HID_SUPERVISION_TIMEOUT)
		}
	},
	{
		BT_SDP_ATTR_HID_MAX_LATENCY,
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
			BT_SDP_ARRAY_16(BT_HID_MAX_LATENCY)
		}
	},
	{
		BT_SDP_ATTR_HID_MIN_LATENCY,
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
			BT_SDP_ARRAY_16(BT_HID_MIN_LATENCY)
		}
	},
};

static struct bt_hid_device *default_hid;
static int hid_registered;
static struct bt_sdp_record hid_rec = BT_SDP_RECORD(hid_attrs);

static void hid_connect_cb(struct bt_hid_device *hid)
{
	bt_shell_print("HID: connected (%p)", hid);

	default_hid = hid;
}

static void hid_disconnected_cb(struct bt_hid_device *hid)
{
	bt_shell_print("HID: disconnected (%p)", hid);

	default_hid = NULL;
}

int hid_set_report_cb(struct bt_hid_device *hid, uint8_t type, struct net_buf *buf)
{
	bt_shell_print("HID: set report type %u len %u", type, buf->len);
	bt_shell_hexdump(buf->data, buf->len);
	return 0;
}

int hid_get_report_cb(struct bt_hid_device *hid, uint8_t type, uint8_t report_id,
		      uint16_t buffer_size)
{
	struct net_buf *buf;
	int err;

	bt_shell_print("HID: get report type %u id %u size %u", type, report_id, buffer_size);

	buf = bt_hid_device_create_pdu(&pool);
	if (!buf) {
		bt_shell_error("HID: failed to create PDU");
		return -ENOMEM;
	}

	if (net_buf_tailroom(buf) < buffer_size) {
		net_buf_unref(buf);
		bt_shell_error("HID: tailroom %u < size %u", net_buf_tailroom(buf), buffer_size);
		return -ENOMEM;
	}

	for (uint16_t i = 0; i < buffer_size; i++) {
		net_buf_add_u8(buf, i);
	}

	err = bt_hid_device_get_report_response(hid, type, buf);
	if (err != 0) {
		net_buf_unref(buf);
		bt_shell_error("HID: send Get_Report rsp failed (%d)", err);
		return err;
	}

	return 0;
}

int hid_set_protocol_cb(struct bt_hid_device *hid, uint8_t protocol)
{
	bt_shell_print("HID: set protocol %u", protocol);
	return 0;
}

int hid_get_protocol_cb(struct bt_hid_device *hid)
{
	uint8_t protocol = BT_HID_PROTOCOL_REPORT_MODE;
	int err;

	bt_shell_print("HID: get protocol");

	err = bt_hid_device_get_protocol_response(hid, protocol);
	if (err != 0) {
		bt_shell_error("HID: send Get_Protocol rsp failed (%d)", err);
		return err;
	}

	return 0;
}

void hid_intr_data_cb(struct bt_hid_device *hid, int8_t report_id, struct net_buf *buf)
{
	bt_shell_print("HID: intr data report_id %d len %u", report_id, buf->len);
	bt_shell_hexdump(buf->data, buf->len);
}

void hid_vc_unplug_cb(struct bt_hid_device *hid)
{
	bt_shell_print("HID: virtual cable unplug");
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
};

static int cmd_hid_register(const struct shell *sh, int32_t argc, char *argv[])
{
	int err;

	err = bt_hid_device_register(&hid_cb);
	if (err != 0) {
		shell_error(sh, "HID: register failed (%d)", err);
		return err;
	}

	err = bt_sdp_register_service(&hid_rec);
	if (err != 0) {
		shell_error(sh, "HID: SDP register failed (%d)", err);
		return err;
	}

	hid_registered = 1;
	shell_print(sh, "HID: registered");
	return err;
}

static int cmd_hid_unregister(const struct shell *sh, int32_t argc, char *argv[])
{
	int err;

	if (!hid_registered) {
		shell_error(sh, "HID: not registered");
		return -ENOEXEC;
	}

	err = bt_hid_device_unregister();
	if (err != 0) {
		shell_error(sh, "HID: unregister failed (%d)", err);
		return err;
	}

	hid_registered = 0;
	default_hid = NULL;
	shell_print(sh, "HID: unregistered");
	return 0;
}

static int cmd_hid_connect(const struct shell *sh, size_t argc, char *argv[])
{
	if (!hid_registered) {
		shell_error(sh, "HID: not registered");
		return -ENOEXEC;
	}

	if (!default_conn) {
		shell_error(sh, "HID: not connected");
		return -ENOEXEC;
	}

	default_hid = bt_hid_device_connect(default_conn);
	if (!default_hid) {
		shell_error(sh, "HID: connect failed");
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_hid_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	if (!hid_registered) {
		shell_error(sh, "HID: not registered");
		return -ENOEXEC;
	}

	if (!default_hid) {
		shell_error(sh, "HID: not connected");
		return -ENOEXEC;
	}

	bt_hid_device_disconnect(default_hid);
	default_hid = NULL;

	return 0;
}

static int cmd_hid_send_report(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_buf *buf;
	int err;

	if (!hid_registered) {
		shell_error(sh, "HID: not registered");
		return -ENOEXEC;
	}

	if (!default_hid) {
		shell_error(sh, "HID: not connected");
		return -ENOEXEC;
	}

	if (argc < 4) {
		shell_error(
			sh,
			"invalid parameters: use 'mouse <X> <Y> [wheel]' or 'raw <b1> <b2> ...'");
		return -ENOEXEC;
	}

	buf = bt_hid_device_create_pdu(&pool);
	if (!buf) {
		shell_error(sh, "HID: failed to create PDU");
		return -ENOEXEC;
	}

	if (net_buf_tailroom(buf) < 3) {
		shell_error(sh, "HID: tailroom %u < report len %u", net_buf_tailroom(buf), 3);
		net_buf_unref(buf);
		return -ENOEXEC;
	}

	/* Mouse (descriptor identifier kept for compatibility) */
	net_buf_add_u8(buf, 0x02);

	/* Button byte (bit fields):
	 *  bit0 = Button Left
	 *  bit1 = Button Right
	 *  bit2 = Button Middle
	 *  bit3..bit7 = Button 4..8
	 */
	net_buf_add_u8(buf, atoi(argv[1])); /* Button: flags */
	net_buf_add_u8(buf, atoi(argv[2])); /* X Displacement */
	net_buf_add_u8(buf, atoi(argv[3])); /* Y Displacement */

	if (argc >= 5) {
		if (net_buf_tailroom(buf) < 1) {
			net_buf_unref(buf);
			shell_error(sh, "HID: tailroom %u < report len %u", net_buf_tailroom(buf),
				    1);
			return -ENOEXEC;
		}

		net_buf_add_u8(buf, atoi(argv[4])); /* Horizontal Scroll Wheel: optional */
	}

	err = bt_hid_device_send(default_hid, buf);
	if (err != 0) {
		net_buf_unref(buf);
		shell_error(sh, "HID: send report failed (%d)", err);
		return -ENOEXEC;
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(hid_device_cmds,
	SHELL_CMD_ARG(register, NULL, "register hid mouse device", cmd_hid_register, 1, 0),
	SHELL_CMD_ARG(unregister, NULL, "unregister hid mouse device", cmd_hid_unregister, 1, 0),
	SHELL_CMD_ARG(connect, NULL, "hid connect", cmd_hid_connect, 1, 0),
	SHELL_CMD_ARG(disconnect, NULL, "hid disconnect", cmd_hid_disconnect, 1, 0),
	SHELL_CMD_ARG(send, NULL,
			  "send mouse report: <(button bits: "
			  "0=Left,1=Right,2=Middle,b3..7=4..8)> <X> <Y> [wheel]",
			  cmd_hid_send_report, 4, 1),
	SHELL_SUBCMD_SET_END
);

static int cmd_hid_device(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	shell_print(sh, "%s unknown parameter: %s", argv[0], argv[1]);

	return -ENOEXEC;
}

SHELL_CMD_ARG_REGISTER(hid_device, &hid_device_cmds, "Bluetooth HID Device sh commands",
		       cmd_hid_device, 1, 1);
