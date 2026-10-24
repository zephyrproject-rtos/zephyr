/* btp_hid_device.c - Bluetooth HID Device Profile Tester */

/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/hid_device.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/usb/class/hid.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
#define LOG_MODULE_NAME btp_hid_device
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BTTESTER_LOG_LEVEL);

#include "btp/btp.h"

#define HID_DEVICE_VERSION       0x0101
#define HID_PARSER_VERSION       0x0111
#define HID_DEVICE_SUBCLASS      0xc0 /* Combo: keyboard (0x40) + pointing (0x80) */

#define HID_DEVICE_COUNTRY_CODE  0x21
#define HID_L2CAP_PSM_CONTROL    0x0011
#define HID_L2CAP_PSM_INTERRUPT  0x0013

#define HID_LANG_ID_ENGLISH      0x0409
#define HID_LANG_ID_OFFSET       0x0100
#define HID_LANG_ENCODING_UTF8   106 /* IANA MIBenum for UTF-8 */

#define HID_SUPERVISION_TIMEOUT  1000
#define HID_SSR_HOST_MAX_LATENCY 240
#define HID_SSR_HOST_MIN_TIMEOUT 0

/* HID descriptor type, HID v1.1.2 section 6.2.1 */
#define HID_SDP_DESCRIPTOR_TYPE_REPORT 0x22

#define HID_KEYBOARD_REPORT_ID 1
#define HID_MOUSE_REPORT_ID    2

#define HID_KEYBOARD_INPUT_LEN   8
#define HID_KEYBOARD_OUTPUT_LEN  1
#define HID_MOUSE_INPUT_LEN      4
#define HID_MOUSE_INPUT_BOOT_LEN 3

NET_BUF_POOL_FIXED_DEFINE(hid_device_tx_pool, 1, BT_L2CAP_BUF_SIZE(CONFIG_BT_L2CAP_TX_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static const uint8_t hid_report_desc[] = {
	/* Keyboard report (Report ID 1) */
	HID_USAGE_PAGE(HID_USAGE_GEN_DESKTOP),
	HID_USAGE(HID_USAGE_GEN_DESKTOP_KEYBOARD),
	HID_COLLECTION(HID_COLLECTION_APPLICATION),
		HID_REPORT_ID(HID_KEYBOARD_REPORT_ID),
		/* Modifier byte: 8 x 1-bit flags */
		HID_USAGE_PAGE(HID_USAGE_GEN_KEYBOARD),
		HID_USAGE_MIN8(0xE0),
		HID_USAGE_MAX8(0xE7),
		HID_LOGICAL_MIN8(0),
		HID_LOGICAL_MAX8(1),
		HID_REPORT_SIZE(1),
		HID_REPORT_COUNT(8),
		HID_INPUT(0x02),
		/* Reserved byte */
		HID_REPORT_SIZE(8),
		HID_REPORT_COUNT(1),
		HID_INPUT(0x03),
		/* 6 key codes */
		HID_USAGE_PAGE(HID_USAGE_GEN_KEYBOARD),
		HID_USAGE_MIN8(0),
		HID_USAGE_MAX8(101),
		HID_LOGICAL_MIN8(0),
		HID_LOGICAL_MAX8(101),
		HID_REPORT_SIZE(8),
		HID_REPORT_COUNT(6),
		HID_INPUT(0x00),
		/*
		 * LED output report (host -> device): 5 LED bits plus 3
		 * padding bits. An OUTPUT report is needed to test SET_REPORT.
		 */
		HID_USAGE_PAGE(HID_USAGE_GEN_LEDS),
		HID_USAGE_MIN8(1),
		HID_USAGE_MAX8(5),
		HID_LOGICAL_MIN8(0),
		HID_LOGICAL_MAX8(1),
		HID_REPORT_SIZE(1),
		HID_REPORT_COUNT(5),
		HID_OUTPUT(0x02),
		/* 3 padding bits to byte-align the output report */
		HID_REPORT_SIZE(3),
		HID_REPORT_COUNT(1),
		HID_OUTPUT(0x03),
	HID_END_COLLECTION,

	/* Mouse report (Report ID 2) */
	HID_USAGE_PAGE(HID_USAGE_GEN_DESKTOP),
	HID_USAGE(HID_USAGE_GEN_DESKTOP_MOUSE),
	HID_COLLECTION(HID_COLLECTION_APPLICATION),
		HID_REPORT_ID(HID_MOUSE_REPORT_ID),
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

static struct bt_sdp_attribute hid_device_attrs[] = {
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
				BT_SDP_ARRAY_16(HID_L2CAP_PSM_CONTROL)
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
		BT_SDP_ATTR_BROWSE_GRP_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
			BT_SDP_ARRAY_16(BT_SDP_PUBLIC_BROWSE_GROUP)
		}
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_LANG_BASE_ATTR_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 9),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
			BT_SDP_ARRAY_16(HID_LANG_ID_ENGLISH)
		},
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
			BT_SDP_ARRAY_16(HID_LANG_ENCODING_UTF8)
		},
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
			BT_SDP_ARRAY_16(BT_SDP_PRIMARY_LANG_BASE)
		}
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
					BT_SDP_ARRAY_16(HID_L2CAP_PSM_INTERRUPT)
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
			BT_SDP_ARRAY_16(HID_DEVICE_VERSION)
		}
	},
	{
		BT_SDP_ATTR_HID_PARSER_VERSION,
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
			BT_SDP_ARRAY_16(HID_PARSER_VERSION)
		}
	},
	{
		BT_SDP_ATTR_HID_DEVICE_SUBCLASS,
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
			BT_SDP_ARRAY_8(HID_DEVICE_SUBCLASS)
		}
	},
	{
		BT_SDP_ATTR_HID_COUNTRY_CODE,
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
			BT_SDP_ARRAY_8(HID_DEVICE_COUNTRY_CODE)
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
		/* Outer sequence: 2 (inner SEQ8 hdr) + sizeof + 4 (inner payload) */
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, sizeof(hid_report_desc) + 6),
		BT_SDP_DATA_ELEM_LIST(
		{
			/* Inner sequence: 2 (UINT8 type+val) + 2 (STR8 hdr) + sizeof */
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, sizeof(hid_report_desc) + 4),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
				BT_SDP_ARRAY_8(HID_SDP_DESCRIPTOR_TYPE_REPORT),
			},
			{
				/*
				 * TEXT_STR8 (1-byte length): some PTS versions
				 * mis-parse the 2-byte TEXT_STR16 length.
				 */
				BT_SDP_TYPE_SIZE_VAR(BT_SDP_TEXT_STR8,
					sizeof(hid_report_desc)
				),
				hid_report_desc,
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
		BT_SDP_ATTR_HID_PROFILE_VERSION,
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
			/* HID profile version, matches ProfileDescriptorList */
			BT_SDP_ARRAY_16(HID_DEVICE_VERSION)
		}
	},
	{
		BT_SDP_ATTR_HID_SUPERVISION_TIMEOUT,
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
			BT_SDP_ARRAY_16(HID_SUPERVISION_TIMEOUT)
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
		BT_SDP_ATTR_HID_BOOT_DEVICE,
		{
			BT_SDP_TYPE_SIZE(BT_SDP_BOOL),
			BT_SDP_ARRAY_8(0x01)
		}
	},
	{
		BT_SDP_ATTR_HID_SSR_HOST_MAX_LATENCY,
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
			BT_SDP_ARRAY_16(HID_SSR_HOST_MAX_LATENCY)
		}
	},
	{
		BT_SDP_ATTR_HID_SSR_HOST_MIN_TIMEOUT,
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
			BT_SDP_ARRAY_16(HID_SSR_HOST_MIN_TIMEOUT)
		}
	},
};

static struct bt_sdp_record hid_device_rec = BT_SDP_RECORD(hid_device_attrs);

static const uint8_t hid_idle_report[HID_KEYBOARD_INPUT_LEN];

static struct bt_hid_device *default_hid_device;
static bool hid_device_registered;
static bool hid_device_sdp_registered;
static bool hid_device_handlers_registered;
static bool hid_device_boot_mode;
static bt_addr_t hid_device_peer_addr;

/* ---- HID device callbacks --------------------------------------------- */
static void hid_device_send_conn_event(uint8_t opcode)
{
	struct btp_hid_device_connected_ev ev;

	memset(&ev, 0, sizeof(ev));
	bt_addr_copy(&ev.address, &hid_device_peer_addr);

	tester_event(BTP_SERVICE_ID_HID_DEVICE, opcode, &ev, sizeof(ev));
}

static void hid_device_connected_cb(struct bt_hid_device *hid)
{
	struct bt_conn *conn;
	const bt_addr_t *peer;

	LOG_DBG("HID connected (%p)", hid);
	default_hid_device = hid;

	bt_addr_copy(&hid_device_peer_addr, BT_ADDR_ANY);

	conn = bt_hid_device_get_conn(hid);
	if (conn != NULL) {
		peer = bt_conn_get_dst_br(conn);
		if (peer != NULL) {
			bt_addr_copy(&hid_device_peer_addr, peer);
		}
		bt_conn_unref(conn);
	}

	hid_device_send_conn_event(BTP_HID_DEVICE_EV_CONNECTED);
}

static void hid_device_disconnected_cb(struct bt_hid_device *hid)
{
	LOG_DBG("HID disconnected (%p)", hid);

	hid_device_send_conn_event(BTP_HID_DEVICE_EV_DISCONNECTED);

	bt_addr_copy(&hid_device_peer_addr, BT_ADDR_ANY);
	default_hid_device = NULL;
	/* A new association always starts in Report Protocol Mode. */
	hid_device_boot_mode = false;
}

static int hid_expected_report_len(uint8_t type, uint8_t report_id)
{
	switch (report_id) {
	case HID_KEYBOARD_REPORT_ID:
		if (type == BT_HID_REPORT_TYPE_INPUT) {
			return HID_KEYBOARD_INPUT_LEN;
		}
		if (type == BT_HID_REPORT_TYPE_OUTPUT) {
			return HID_KEYBOARD_OUTPUT_LEN;
		}
		return -ENOENT;
	case HID_MOUSE_REPORT_ID:
		if (type == BT_HID_REPORT_TYPE_INPUT) {
			return hid_device_boot_mode ? HID_MOUSE_INPUT_BOOT_LEN
						    : HID_MOUSE_INPUT_LEN;
		}
		return -ENOENT;
	default:
		return -ENOENT;
	}
}

static int hid_device_set_report_cb(struct bt_hid_device *hid, uint8_t type, struct net_buf *buf)
{
	uint8_t report_id;
	int expected_len;

	if (buf->len < sizeof(report_id)) {
		return -EINVAL;
	}

	report_id = net_buf_pull_u8(buf);
	LOG_DBG("HID set report type %u id %u len %u", type, report_id, buf->len);

	if (hid_device_boot_mode) {
		if (buf->len == 0U) {
			LOG_WRN("HID boot mode set report with no payload");
			return -EINVAL;
		}

		return 0;
	}

	expected_len = hid_expected_report_len(type, report_id);

	if (expected_len < 0) {
		/* Report Type / Report ID combination not declared. */
		return -ENOENT;
	}

	if (buf->len != (uint16_t)expected_len) {
		LOG_WRN("HID set report invalid size %u (expected %d)", buf->len, expected_len);
		return -EINVAL;
	}

	return 0;
}

static int hid_device_get_report_cb(struct bt_hid_device *hid, uint8_t type, bool size_present,
				    struct net_buf *req, struct net_buf *rsp)
{
	uint8_t report_id;
	uint16_t buffer_size = 0;
	uint16_t report_len;

	ARG_UNUSED(hid);
	ARG_UNUSED(type);

	if (req->len < sizeof(report_id)) {
		return -EINVAL;
	}

	report_id = net_buf_pull_u8(req);
	LOG_DBG("HID get report type %u id %u", type, report_id);

	if (size_present) {
		if (req->len < sizeof(uint16_t)) {
			return -EINVAL;
		}

		buffer_size = net_buf_pull_le16(req);
		LOG_DBG("HID requested buffer size %u", buffer_size);
	}

	/* Idle report: a Report ID byte plus an all-zero payload. */
	switch (report_id) {
	case HID_KEYBOARD_REPORT_ID:
		report_len = 1U + HID_KEYBOARD_INPUT_LEN;
		break;
	case HID_MOUSE_REPORT_ID:
		report_len = 1U + (hid_device_boot_mode ? HID_MOUSE_INPUT_BOOT_LEN
							: HID_MOUSE_INPUT_LEN);
		break;
	default:
		return -ENOENT;
	}

	if (size_present) {
		report_len = MIN(report_len, buffer_size);
	}

	if (report_len > net_buf_tailroom(rsp)) {
		LOG_ERR("HID get report response does not fit (%u > %zu)", report_len,
			net_buf_tailroom(rsp));
		return -ENOMEM;
	}

	if (report_len != 0U) {
		net_buf_add_u8(rsp, report_id);
		net_buf_add_mem(rsp, hid_idle_report, report_len - 1U);
	}

	return 0;
}

static int hid_device_set_protocol_cb(struct bt_hid_device *hid, uint8_t protocol)
{
	LOG_DBG("HID set protocol %u", protocol);
	hid_device_boot_mode = (protocol == BT_HID_PROTOCOL_BOOT_MODE);
	return 0;
}

static void hid_device_output_report_cb(struct bt_hid_device *hid, struct net_buf *buf)
{
	uint8_t report_id;

	if (buf->len < sizeof(report_id)) {
		LOG_WRN("HID malformed output report (len %u)", buf->len);
		return;
	}

	report_id = net_buf_pull_u8(buf);
	LOG_DBG("HID output report id %u len %u", report_id, buf->len);
}

static void hid_device_vc_unplug_cb(struct bt_hid_device *hid)
{
	ARG_UNUSED(hid);

	/* The link is torn down next, so disconnected() reports the change. */
	LOG_DBG("HID virtual cable unplug");
}

static void hid_device_suspend_cb(struct bt_hid_device *hid, bool suspended)
{
	LOG_DBG("HID %s", suspended ? "suspended" : "exit suspend");
}

static const struct bt_hid_device_cb hid_device_cb = {
	.connected = hid_device_connected_cb,
	.disconnected = hid_device_disconnected_cb,
	.set_report = hid_device_set_report_cb,
	.get_report = hid_device_get_report_cb,
	.set_protocol = hid_device_set_protocol_cb,
	.output_report = hid_device_output_report_cb,
	.vc_unplug = hid_device_vc_unplug_cb,
	.suspend = hid_device_suspend_cb,
};

/* ---- BTP command handlers --------------------------------------------- */
static uint8_t hid_device_supported(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	struct btp_hid_device_read_supported_commands_rp *rp = rsp;

	*rsp_len = tester_supported_commands(BTP_SERVICE_ID_HID_DEVICE, rp->data);
	*rsp_len += sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static int hid_device_do_register(void)
{
	int err;

	if (hid_device_registered) {
		return 0;
	}

	err = bt_hid_device_register(&hid_device_cb);
	if (err != 0) {
		LOG_ERR("HID register failed (%d)", err);
		return err;
	}

	if (!hid_device_sdp_registered) {
		err = bt_sdp_register_service(&hid_device_rec);
		if (err != 0) {
			LOG_ERR("HID SDP register failed (%d)", err);
			bt_hid_device_unregister();
			return err;
		}
		hid_device_sdp_registered = true;
	}

	hid_device_registered = true;
	LOG_DBG("HID registered");
	return 0;
}

static uint8_t hid_device_register(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	if (hid_device_do_register() != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static int hid_device_do_unregister(void)
{
	int err;

	if (!hid_device_registered) {
		return 0;
	}

	err = bt_hid_device_unregister();
	if (err != 0) {
		LOG_ERR("HID unregister failed (%d)", err);
		return err;
	}

	hid_device_registered = false;
	default_hid_device = NULL;
	hid_device_boot_mode = false;
	LOG_DBG("HID unregistered");
	return 0;
}

static uint8_t hid_device_unregister(const void *cmd, uint16_t cmd_len, void *rsp,
				     uint16_t *rsp_len)
{
	if (hid_device_do_unregister() != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t hid_device_connect(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_hid_device_connect_cmd *cp = cmd;
	struct bt_conn *conn;
	bt_addr_t addr;
	int err;

	if (!hid_device_registered) {
		LOG_ERR("HID not registered");
		return BTP_STATUS_FAILED;
	}

	if (cp->address_type != BTP_BR_ADDRESS_TYPE) {
		LOG_ERR("Invalid address type: 0x%02x", cp->address_type);
		return BTP_STATUS_FAILED;
	}

	bt_addr_copy(&addr, &cp->address);

	conn = bt_conn_lookup_addr_br(&addr);
	if (conn == NULL) {
		LOG_ERR("BR/EDR connection not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_hid_device_connect(conn, &default_hid_device);
	bt_conn_unref(conn);
	if (err != 0) {
		LOG_ERR("HID connect failed (%d)", err);
		return BTP_STATUS_FAILED;
	}

	LOG_DBG("HID connect initiated");
	return BTP_STATUS_SUCCESS;
}

static uint8_t hid_device_disconnect(const void *cmd, uint16_t cmd_len, void *rsp,
				     uint16_t *rsp_len)
{
	int err;

	if (!hid_device_registered) {
		LOG_ERR("HID not registered");
		return BTP_STATUS_FAILED;
	}

	if (default_hid_device == NULL) {
		LOG_ERR("HID not connected");
		return BTP_STATUS_FAILED;
	}

	err = bt_hid_device_disconnect(default_hid_device);
	if (err != 0) {
		LOG_ERR("HID disconnect failed (%d)", err);
		return BTP_STATUS_FAILED;
	}

	LOG_DBG("HID disconnect initiated");
	return BTP_STATUS_SUCCESS;
}

static uint8_t hid_device_send_report(const void *cmd, uint16_t cmd_len, void *rsp,
				      uint16_t *rsp_len)
{
	const struct btp_hid_device_send_report_cmd *cp = cmd;
	struct net_buf *buf;
	uint16_t data_len;
	int err;

	if (!hid_device_registered) {
		LOG_ERR("HID not registered");
		return BTP_STATUS_FAILED;
	}

	if (default_hid_device == NULL) {
		LOG_ERR("HID not connected");
		return BTP_STATUS_FAILED;
	}

	if (cp->report_type != BT_HID_REPORT_TYPE_INPUT) {
		LOG_ERR("Invalid report type: 0x%02x", cp->report_type);
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	if (cmd_len < sizeof(*cp) + data_len) {
		LOG_ERR("Invalid command length");
		return BTP_STATUS_FAILED;
	}

	buf = bt_hid_device_create_pdu(&hid_device_tx_pool);
	if (buf == NULL) {
		LOG_ERR("HID failed to create PDU");
		return BTP_STATUS_FAILED;
	}

	if (data_len > net_buf_tailroom(buf)) {
		LOG_ERR("HID report too long (%u > %zu)", data_len, net_buf_tailroom(buf));
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	net_buf_add_mem(buf, cp->data, data_len);

	err = bt_hid_device_input_report(default_hid_device, buf);
	if (err != 0) {
		net_buf_unref(buf);
		LOG_ERR("HID send report failed (%d)", err);
		return BTP_STATUS_FAILED;
	}

	LOG_DBG("HID report type %u sent (%u bytes)", cp->report_type, data_len);
	return BTP_STATUS_SUCCESS;
}

static uint8_t hid_device_virtual_cable_unplug(const void *cmd, uint16_t cmd_len, void *rsp,
					       uint16_t *rsp_len)
{
	int err;

	if (!hid_device_registered) {
		LOG_ERR("HID not registered");
		return BTP_STATUS_FAILED;
	}

	if (default_hid_device == NULL) {
		LOG_ERR("HID not connected");
		return BTP_STATUS_FAILED;
	}

	err = bt_hid_device_virtual_cable_unplug(default_hid_device);
	if (err != 0) {
		LOG_ERR("HID virtual cable unplug failed (%d)", err);
		return BTP_STATUS_FAILED;
	}

	LOG_DBG("HID virtual cable unplug initiated");
	return BTP_STATUS_SUCCESS;
}

#if defined(CONFIG_BT_POWER_MODE_CONTROL)
static uint8_t hid_device_enter_sniff_mode(const void *cmd, uint16_t cmd_len, void *rsp,
					   uint16_t *rsp_len)
{
	const struct btp_hid_device_enter_sniff_mode_cmd *cp = cmd;
	struct bt_conn *conn;
	bt_addr_t addr;
	int err;

	if (cp->address_type != BTP_BR_ADDRESS_TYPE) {
		LOG_ERR("Invalid address type: 0x%02x", cp->address_type);
		return BTP_STATUS_FAILED;
	}

	bt_addr_copy(&addr, &cp->address);

	conn = bt_conn_lookup_addr_br(&addr);
	if (conn == NULL) {
		LOG_ERR("BR/EDR connection not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_conn_br_enter_sniff_mode(conn, sys_le16_to_cpu(cp->min_interval),
					  sys_le16_to_cpu(cp->max_interval),
					  sys_le16_to_cpu(cp->attempt),
					  sys_le16_to_cpu(cp->timeout));
	bt_conn_unref(conn);
	if (err != 0) {
		LOG_ERR("Enter sniff mode failed (%d)", err);
		return BTP_STATUS_FAILED;
	}

	LOG_DBG("Enter sniff mode requested");
	return BTP_STATUS_SUCCESS;
}
#endif /* CONFIG_BT_POWER_MODE_CONTROL */

/* ---- registration ------------------------------------------------------ */
static const struct btp_handler hid_device_handlers[] = {
	{
		.opcode     = BTP_HID_DEVICE_READ_SUPPORTED_COMMANDS,
		.index      = BTP_INDEX_NONE,
		.expect_len = 0,
		.func       = hid_device_supported,
	},
	{
		.opcode     = BTP_HID_DEVICE_CMD_REGISTER,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func       = hid_device_register,
	},
	{
		.opcode     = BTP_HID_DEVICE_CMD_UNREGISTER,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func       = hid_device_unregister,
	},
	{
		.opcode     = BTP_HID_DEVICE_CMD_CONNECT,
		.expect_len = sizeof(struct btp_hid_device_connect_cmd),
		.func       = hid_device_connect,
	},
	{
		.opcode     = BTP_HID_DEVICE_CMD_DISCONNECT,
		.expect_len = sizeof(struct btp_hid_device_disconnect_cmd),
		.func       = hid_device_disconnect,
	},
	{
		.opcode     = BTP_HID_DEVICE_CMD_SEND_REPORT,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func       = hid_device_send_report,
	},
	{
		.opcode     = BTP_HID_DEVICE_CMD_VIRTUAL_CABLE_UNPLUG,
		.expect_len = sizeof(struct btp_hid_device_virtual_cable_unplug_cmd),
		.func       = hid_device_virtual_cable_unplug,
	},
#if defined(CONFIG_BT_POWER_MODE_CONTROL)
	{
		.opcode     = BTP_HID_DEVICE_CMD_ENTER_SNIFF_MODE,
		.expect_len = sizeof(struct btp_hid_device_enter_sniff_mode_cmd),
		.func       = hid_device_enter_sniff_mode,
	},
#endif /* CONFIG_BT_POWER_MODE_CONTROL */
};

uint8_t tester_init_hid_device(void)
{
	if (!hid_device_handlers_registered) {
		tester_register_command_handlers(BTP_SERVICE_ID_HID_DEVICE, hid_device_handlers,
						 ARRAY_SIZE(hid_device_handlers));
		hid_device_handlers_registered = true;
	}

	if (hid_device_do_register() != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_hid_device(void)
{
	if (hid_device_do_unregister() != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}
