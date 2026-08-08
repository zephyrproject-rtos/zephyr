/*
 * Copyright 2025 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/classic/hid_device.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/usb/class/hid.h>

#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_string_conv.h>

#include "host/shell/bt.h"
#include "common/bt_shell_private.h"

#define BT_HID_DEVICE_VERSION      0x0101
#define BT_HID_PARSER_VERSION      0x0111
#define BT_HID_DEVICE_SUBCLASS     0x80 /* Pointing device (mouse) */
#define BT_HID_DEVICE_COUNTRY_CODE 0x21
#define BT_L2CAP_PSM_HID_CONTROL   0x0011
#define BT_L2CAP_PSM_HID_INTERRUPT 0x0013

#define BT_HID_LANG_ID_ENGLISH 0x0409
#define BT_HID_LANG_ID_OFFSET  0x0100

#define BT_HID_SUPERVISION_TIMEOUT 1000
#define BT_HID_SSR_HOST_MAX_LATENCY 240
#define BT_HID_SSR_HOST_MIN_TIMEOUT 0

NET_BUF_POOL_FIXED_DEFINE(pool, 1, BT_L2CAP_BUF_SIZE(CONFIG_BT_L2CAP_TX_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

/* Shell mouse descriptor declares a single Report ID */
#define SHELL_MOUSE_REPORT_ID 2

static const uint8_t mouse_descriptor[] = {
	HID_USAGE_PAGE(HID_USAGE_GEN_DESKTOP),
	HID_USAGE(HID_USAGE_GEN_DESKTOP_MOUSE),
	HID_COLLECTION(HID_COLLECTION_APPLICATION),
		HID_REPORT_ID(SHELL_MOUSE_REPORT_ID),
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
	BT_SDP_SERVICE_NAME("HID CONTROL"),
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

static struct bt_hid_device *default_hid;
static bool hid_registered;
static bool hid_boot_mode;
static bool vcu_unplug_pending;
static bt_addr_t vcu_peer;
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

	if (vcu_unplug_pending) {
		vcu_unplug_pending = false;
		bt_br_unpair(&vcu_peer);
	}
}

static int hid_set_report_cb(struct bt_hid_device *hid, uint8_t type, struct net_buf *buf)
{
	uint8_t report_id;

	if (buf->len < sizeof(report_id)) {
		return -EINVAL;
	}

	report_id = net_buf_pull_u8(buf);
	bt_shell_print("HID: set report type %u id %u len %u", type, report_id, buf->len);

	if (report_id != SHELL_MOUSE_REPORT_ID) {
		return -ENOENT;
	}

	bt_shell_hexdump(buf->data, buf->len);
	return 0;
}

static int hid_get_report_cb(struct bt_hid_device *hid, uint8_t type, bool size_present,
			     struct net_buf *req, struct net_buf *rsp)
{
	uint8_t report_id = 0;

	ARG_UNUSED(hid);
	ARG_UNUSED(type);

	if (req->len < sizeof(report_id)) {
		return -EINVAL;
	}

	report_id = net_buf_pull_u8(req);
	bt_shell_print("HID: get report type %u id %u", type, report_id);

	if (size_present) {
		if (req->len < sizeof(uint16_t)) {
			return -EINVAL;
		}

		/* Size bit set: the host caps the response at this BufferSize.
		 * The fixed mouse report below is always shorter, so this
		 * sample only logs it; a real device must trim its response
		 * to not exceed BufferSize.
		 */
		bt_shell_print("HID: requested buffer size %u", net_buf_pull_le16(req));
	}

	if (report_id != SHELL_MOUSE_REPORT_ID) {
		return -ENOENT;
	}

	/* Fill the stack-provided response buffer; the stack prepends the HIDP
	 * header and sends it on return.
	 */
	net_buf_add_u8(rsp, SHELL_MOUSE_REPORT_ID); /* Report ID */
	net_buf_add_u8(rsp, 0x00);                  /* buttons */
	net_buf_add_u8(rsp, 0x00);                  /* X displacement */
	net_buf_add_u8(rsp, 0x00);                  /* Y displacement */

	if (!hid_boot_mode) {
		net_buf_add_u8(rsp, 0x00);          /* wheel (report mode only) */
	}

	return 0;
}

static int hid_set_protocol_cb(struct bt_hid_device *hid, uint8_t protocol)
{
	bt_shell_print("HID: set protocol %u", protocol);
	hid_boot_mode = (protocol == BT_HID_PROTOCOL_BOOT_MODE);
	return 0;
}

static void hid_output_report_cb(struct bt_hid_device *hid, struct net_buf *buf)
{
	uint8_t report_id;

	if (buf->len < sizeof(report_id)) {
		bt_shell_warn("HID: malformed output report (len %u)", buf->len);
		return;
	}

	report_id = net_buf_pull_u8(buf);
	bt_shell_print("HID: output report id %u len %u", report_id, buf->len);
	bt_shell_hexdump(buf->data, buf->len);
}

static void hid_vc_unplug_cb(struct bt_hid_device *hid)
{
	struct bt_conn *conn = bt_hid_device_get_conn(hid);

	bt_shell_print("HID: virtual cable unplug");

	/* HID spec v1.1.2 Section 3.1.2.2.3: destroy the bonding information
	 * for the peer that requested the Virtual Cable Unplug. Defer the
	 * unpair until the HID connection is fully torn down (disconnected
	 * callback): bt_br_unpair() drops the ACL, so doing it here would kill
	 * the still-open HID channels instead of letting them close in order.
	 */
	if (conn != NULL) {
		bt_addr_copy(&vcu_peer, bt_conn_get_dst_br(conn));
		vcu_unplug_pending = true;
		bt_conn_unref(conn);
	}
}

static void hid_suspend_cb(struct bt_hid_device *hid, bool suspended)
{
	bt_shell_print("HID: %s", suspended ? "suspended" : "exit suspend");
}

static const struct bt_hid_device_cb hid_cb = {
	.connected = hid_connect_cb,
	.disconnected = hid_disconnected_cb,
	.set_report = hid_set_report_cb,
	.get_report = hid_get_report_cb,
	.set_protocol = hid_set_protocol_cb,
	.output_report = hid_output_report_cb,
	.vc_unplug = hid_vc_unplug_cb,
	.suspend = hid_suspend_cb,
};

static int cmd_hid_register(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (hid_registered) {
		shell_error(sh, "HID: already registered");
		return -EALREADY;
	}

	err = bt_hid_device_register(&hid_cb);
	if (err != 0) {
		shell_error(sh, "HID: register failed (%d)", err);
		return err;
	}

	err = bt_sdp_register_service(&hid_rec);
	if (err != 0 && err != -EEXIST) {
		shell_error(sh, "HID: SDP register failed (%d)", err);
		bt_hid_device_unregister();
		return err;
	}

	hid_registered = true;
	shell_print(sh, "HID: registered");
	return 0;
}

static int cmd_hid_unregister(const struct shell *sh, size_t argc, char *argv[])
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

	hid_registered = false;
	default_hid = NULL;
	shell_print(sh, "HID: unregistered");
	return 0;
}

static int cmd_hid_connect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (!hid_registered) {
		shell_error(sh, "HID: not registered");
		return -ENOEXEC;
	}

	if (default_conn == NULL) {
		shell_error(sh, "HID: not connected");
		return -ENOEXEC;
	}

	err = bt_hid_device_connect(default_conn, &default_hid);
	if (err != 0) {
		shell_error(sh, "HID: connect failed (%d)", err);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_hid_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (!hid_registered) {
		shell_error(sh, "HID: not registered");
		return -ENOEXEC;
	}

	if (default_hid == NULL) {
		shell_error(sh, "HID: not connected");
		return -ENOEXEC;
	}

	err = bt_hid_device_disconnect(default_hid);
	if (err != 0) {
		shell_error(sh, "HID: disconnect failed (%d)", err);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_hid_send_report(const struct shell *sh, size_t argc, char *argv[])
{
	long button, dx, dy, wheel = 0;
	struct net_buf *buf;
	int err = 0;

	if (!hid_registered) {
		shell_error(sh, "HID: not registered");
		return -ENOEXEC;
	}

	if (default_hid == NULL) {
		shell_error(sh, "HID: not connected");
		return -ENOEXEC;
	}

	button = shell_strtol(argv[1], 0, &err);
	dx = shell_strtol(argv[2], 0, &err);
	dy = shell_strtol(argv[3], 0, &err);
	if (argc >= 5U) {
		wheel = shell_strtol(argv[4], 0, &err);
	}

	if (err != 0) {
		shell_error(sh, "HID: invalid parameter");
		return -EINVAL;
	}

	if ((button < 0) || (button > 0xFF) ||
	    (dx < -127) || (dx > 127) ||
	    (dy < -127) || (dy > 127) ||
	    (!hid_boot_mode && ((wheel < -127) || (wheel > 127)))) {
		shell_error(sh, "HID: value out of bounds");
		return -ERANGE;
	}

	buf = bt_hid_device_create_pdu(&pool);
	if (buf == NULL) {
		shell_error(sh, "HID: failed to create PDU");
		return -ENOEXEC;
	}

	/* BDIT/BV-01-C: Boot Protocol mouse = buttons(1) + X(1) + Y(1).
	 * Report Protocol mouse = Report ID(1) + buttons(1) + X(1) + Y(1) + wheel(1).
	 *
	 * Button byte (bit fields):
	 *  bit0 = Left, bit1 = Right, bit2 = Middle, bit3..7 = Button 4..8
	 * X/Y: signed 8-bit relative displacement
	 * Wheel: signed 8-bit scroll (Report Protocol only)
	 */
	if (hid_boot_mode) {
		net_buf_add_u8(buf, (uint8_t)button); /* buttons */
		net_buf_add_u8(buf, (uint8_t)dx);     /* X displacement */
		net_buf_add_u8(buf, (uint8_t)dy);     /* Y displacement */
	} else {
		net_buf_add_u8(buf, SHELL_MOUSE_REPORT_ID); /* Report ID */
		net_buf_add_u8(buf, (uint8_t)button);        /* buttons */
		net_buf_add_u8(buf, (uint8_t)dx);            /* X displacement */
		net_buf_add_u8(buf, (uint8_t)dy);            /* Y displacement */
		net_buf_add_u8(buf, (uint8_t)wheel);         /* wheel scroll */
	}

	err = bt_hid_device_input_report(default_hid, buf);
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
	if (argc == 1U) {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	shell_print(sh, "%s unknown parameter: %s", argv[0], argv[1]);

	return -ENOEXEC;
}

SHELL_CMD_ARG_REGISTER(hid_device, &hid_device_cmds, "Bluetooth HID Device sh commands",
		       cmd_hid_device, 1, 1);
