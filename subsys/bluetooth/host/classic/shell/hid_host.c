/*
 * Copyright 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/classic/classic.h>
#include <zephyr/bluetooth/classic/hid_host.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_string_conv.h>
#include <zephyr/sys/util.h>
#include <zephyr/usb/class/hid.h>

#include "host/shell/bt.h"
#include "common/bt_shell_private.h"

#define HID_HOST_TX_BUF_COUNT 4

NET_BUF_POOL_FIXED_DEFINE(hid_host_pool, HID_HOST_TX_BUF_COUNT,
			  BT_L2CAP_BUF_SIZE(CONFIG_BT_L2CAP_TX_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

#define SDP_CLIENT_USER_BUF_LEN 512

/* Enough for the report descriptors of the usual HID peripherals. A descriptor
 * that does not fit is rejected rather than truncated, because it cannot be
 * parsed for Report IDs.
 */
#define HID_DESCRIPTOR_MAX_LEN 512

NET_BUF_POOL_FIXED_DEFINE(sdp_client_pool, CONFIG_BT_MAX_CONN, SDP_CLIENT_USER_BUF_LEN, 8, NULL);

/* Report Descriptor item decoding. The item type and tag values come from
 * <zephyr/usb/class/hid.h>, which only provides the HID_ITEM() encoder, so the
 * matching decode masks are defined here and checked against that encoder.
 */
#define HID_ITEM_SIZE_MASK GENMASK(1, 0)
#define HID_ITEM_TYPE_MASK GENMASK(3, 2)
#define HID_ITEM_TAG_MASK  GENMASK(7, 4)

BUILD_ASSERT(HID_ITEM(HID_ITEM_TAG_REPORT_ID, HID_ITEM_TYPE_GLOBAL, 0) ==
		     (FIELD_PREP(HID_ITEM_TAG_MASK, HID_ITEM_TAG_REPORT_ID) |
		      FIELD_PREP(HID_ITEM_TYPE_MASK, HID_ITEM_TYPE_GLOBAL)),
	     "HID item tag/type decode masks do not match HID_ITEM()");

/* bSize encodes 0, 1, 2 or 4 data bytes, the last one being encoded as the
 * largest value the field can hold.
 */
#define HID_ITEM_SIZE_4_BYTES     HID_ITEM_SIZE_MASK
#define HID_ITEM_SIZE_4_BYTES_LEN 4U

BUILD_ASSERT(HID_ITEM(0, 0, HID_ITEM_SIZE_4_BYTES) == HID_ITEM_SIZE_MASK,
	     "HID item size decode mask does not match HID_ITEM()");

/* Long item prefix, followed by bDataSize and bLongItemTag. Not covered by
 * <zephyr/usb/class/hid.h>, which only builds short items.
 */
#define HID_ITEM_LONG_PREFIX  0xfe
#define HID_ITEM_LONG_HDR_LEN 2U

static struct bt_hid_host *default_hid;
static bool hid_registered;
static bool vcu_unplug_pending;
static bt_addr_t vcu_peer;

/* Service record data of the peer. The profile does not read the record, so the
 * application discovers it and keeps whatever it needs to interpret reports.
 */
static uint8_t hid_descriptor[HID_DESCRIPTOR_MAX_LEN];
static uint16_t hid_descriptor_len;
static bool hid_has_report_id;
static bool hid_boot_mode;

/* True when the ReportID field has to be present in GET_REPORT requests and in
 * report payloads: HID spec v1.1.2 Section 3.1.2.3 for Report Protocol Mode and
 * Section 3.3.1 for Boot Protocol Mode, which always carries a Report ID.
 */
static bool hid_report_id_used(void)
{
	return hid_has_report_id || hid_boot_mode;
}

/* Walk the HID Report Descriptor looking for a Report ID Global item.
 *
 * HID spec v1.1.2 Section 3.1.2.3 makes the ReportID field of GET_REPORT (and of
 * the reports themselves) mandatory in Report Protocol Mode as soon as any
 * Report ID Global item is declared, so the host has to know whether the
 * descriptor declares one.
 */
static bool hid_desc_has_report_id(const uint8_t *desc, uint16_t len)
{
	uint16_t pos = 0U;

	while (pos < len) {
		uint8_t prefix = desc[pos++];
		uint16_t data_len;

		if (prefix == HID_ITEM_LONG_PREFIX) {
			/* Long items carry bDataSize and bLongItemTag before the data
			 * and are never Report ID items.
			 */
			if ((pos + HID_ITEM_LONG_HDR_LEN) > len) {
				break;
			}

			data_len = desc[pos];
			pos += HID_ITEM_LONG_HDR_LEN;
		} else {
			uint8_t size = FIELD_GET(HID_ITEM_SIZE_MASK, prefix);

			data_len = (size == HID_ITEM_SIZE_4_BYTES) ? HID_ITEM_SIZE_4_BYTES_LEN
								   : size;

			if ((FIELD_GET(HID_ITEM_TYPE_MASK, prefix) == HID_ITEM_TYPE_GLOBAL) &&
			    (FIELD_GET(HID_ITEM_TAG_MASK, prefix) == HID_ITEM_TAG_REPORT_ID)) {
				return true;
			}
		}

		if ((pos + data_len) > len) {
			bt_shell_warn("Report Descriptor truncated at %u", pos);
			break;
		}

		pos += data_len;
	}

	return false;
}

static int hid_sdp_get_uint_attr(const struct net_buf *buf, uint16_t id, uint32_t *val)
{
	struct bt_sdp_attribute attr;
	struct bt_sdp_attr_value value;
	int err;

	err = bt_sdp_get_attr(buf, id, &attr);
	if (err < 0) {
		return err;
	}

	err = bt_sdp_attr_read(&attr, NULL, &value);
	if (err < 0) {
		return err;
	}

	if (value.type != BT_SDP_ATTR_VALUE_TYPE_UINT) {
		return -EINVAL;
	}

	*val = value.uint.u32;

	return 0;
}

static int hid_sdp_get_bool_attr(const struct net_buf *buf, uint16_t id, bool *val)
{
	struct bt_sdp_attribute attr;
	struct bt_sdp_attr_value value;
	int err;

	err = bt_sdp_get_attr(buf, id, &attr);
	if (err < 0) {
		return err;
	}

	err = bt_sdp_attr_read(&attr, NULL, &value);
	if (err < 0) {
		return err;
	}

	if (value.type != BT_SDP_ATTR_VALUE_TYPE_BOOL) {
		return -EINVAL;
	}

	*val = value.value;

	return 0;
}

static bool hid_desc_list_cb(const struct bt_sdp_attr_value_pair *vp, void *user_data)
{
	ARG_UNUSED(user_data);

	if ((vp == NULL) || (vp->value == NULL)) {
		return true;
	}

	if ((vp->value->type != BT_SDP_ATTR_VALUE_TYPE_TEXT) || (vp->value->text.len == 0U)) {
		return true;
	}

	/* A descriptor that does not fit cannot be parsed for Report IDs, so it is
	 * rejected rather than silently truncated.
	 */
	if (vp->value->text.len > sizeof(hid_descriptor)) {
		bt_shell_error("Report Descriptor too long (%u > %zu)", vp->value->text.len,
			       sizeof(hid_descriptor));
		return false;
	}

	memcpy(hid_descriptor, vp->value->text.text, vp->value->text.len);
	hid_descriptor_len = vp->value->text.len;
	hid_has_report_id = hid_desc_has_report_id(hid_descriptor, hid_descriptor_len);

	return false;
}

/* HIDDescriptorList (0x0206) - nested SEQ { SEQ { UINT8(0x22), TEXT_STR } } */
static int hid_sdp_get_descriptor(const struct net_buf *buf)
{
	struct bt_sdp_attribute attr;
	int err;

	err = bt_sdp_get_attr(buf, BT_SDP_ATTR_HID_DESCRIPTOR_LIST, &attr);
	if (err < 0) {
		return err;
	}

	hid_descriptor_len = 0U;
	hid_has_report_id = false;

	bt_sdp_attr_value_parse(&attr, hid_desc_list_cb, NULL);

	return (hid_descriptor_len != 0U) ? 0 : -ENOENT;
}

static uint8_t hid_sdp_cb(struct bt_conn *conn, struct bt_sdp_client_result *result,
			  const struct bt_sdp_discover_params *params)
{
	uint32_t uval;
	bool bval;
	int err;

	ARG_UNUSED(params);

	if ((result == NULL) || (result->resp_buf == NULL)) {
		bt_shell_error("No SDP HID data from remote %s", bt_conn_dst_str(conn));
		return BT_SDP_DISCOVER_UUID_CONTINUE;
	}

	bt_shell_print("SDP HID data@%p (len %u) from remote %s", result->resp_buf,
		       result->resp_buf->len, bt_conn_dst_str(conn));

	err = hid_sdp_get_descriptor(result->resp_buf);
	if (err < 0) {
		bt_shell_error("HID report descriptor not found, err %d", err);
		goto done;
	}
	bt_shell_print("HID descriptor %u bytes, report id %d", hid_descriptor_len,
		       hid_has_report_id);

	err = hid_sdp_get_uint_attr(result->resp_buf, BT_SDP_ATTR_HID_DEVICE_SUBCLASS, &uval);
	if (err < 0) {
		bt_shell_error("HID subclass not found, err %d", err);
		goto done;
	}
	bt_shell_print("HID subclass 0x%02x", uval);

	err = hid_sdp_get_bool_attr(result->resp_buf, BT_SDP_ATTR_HID_VIRTUAL_CABLE, &bval);
	if (err < 0) {
		bt_shell_error("HID virtual cable not found, err %d", err);
		goto done;
	}
	bt_shell_print("HID virtual cable %d", bval);

	err = hid_sdp_get_bool_attr(result->resp_buf, BT_SDP_ATTR_HID_RECONNECT_INITIATE, &bval);
	if (err < 0) {
		bt_shell_error("HID reconnect initiate not found, err %d", err);
		goto done;
	}
	bt_shell_print("HID reconnect initiate %d", bval);

	err = hid_sdp_get_bool_attr(result->resp_buf, BT_SDP_ATTR_HID_BOOT_DEVICE, &bval);
	if (err < 0) {
		bt_shell_error("HID boot device not found, err %d", err);
		goto done;
	}
	bt_shell_print("HID boot device %d", bval);

	err = hid_sdp_get_uint_attr(result->resp_buf, BT_SDP_ATTR_HID_SUPERVISION_TIMEOUT, &uval);
	if (err < 0) {
		bt_shell_error("HID supervision timeout not found, err %d", err);
		goto done;
	}
	bt_shell_print("HID supervision timeout %u", uval);

done:
	return BT_SDP_DISCOVER_UUID_CONTINUE;
}

static struct bt_sdp_discover_params discov_hid = {
	.type = BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR,
	.uuid = BT_UUID_DECLARE_16(BT_SDP_HID_SVCLASS),
	.func = hid_sdp_cb,
	.pool = &sdp_client_pool,
};

static void hid_connected_cb(struct bt_hid_host *hid)
{
	bt_shell_print("HID Host: connected (%p)", hid);
	default_hid = hid;

	if (hid_descriptor_len == 0U) {
		/* Reports cannot be split into Report ID and data without the report
		 * descriptor, so run sdp_discover to read it.
		 */
		bt_shell_warn("HID Host: no report descriptor, run sdp_discover");
	}
}

static void hid_disconnected_cb(struct bt_hid_host *hid)
{
	bt_shell_print("HID Host: disconnected");
	if (default_hid == hid) {
		default_hid = NULL;
	}

	hid_boot_mode = false;

	if (vcu_unplug_pending) {
		vcu_unplug_pending = false;
		bt_br_unpair(&vcu_peer);
	}
}

static void hid_input_report_cb(struct bt_hid_host *hid, struct net_buf *buf)
{
	uint8_t report_id = 0U;

	if (hid_report_id_used()) {
		if (buf->len < sizeof(report_id)) {
			bt_shell_error("HID Host: input report without Report ID");
			return;
		}

		report_id = net_buf_pull_u8(buf);
	}

	bt_shell_print("HID Host: input_report id %u len %u", report_id, buf->len);
	bt_shell_hexdump(buf->data, buf->len);
}

static void hid_get_report_rsp_cb(struct bt_hid_host *hid, uint8_t type, struct net_buf *buf)
{
	uint8_t report_id = 0U;

	if (hid_report_id_used()) {
		if (buf->len < sizeof(report_id)) {
			bt_shell_error("HID Host: GET_REPORT response without Report ID");
			return;
		}

		report_id = net_buf_pull_u8(buf);
	}

	bt_shell_print("HID Host: get_report_rsp type %u id %u len %u", type, report_id,
		       buf->len);
	bt_shell_hexdump(buf->data, buf->len);
}

static void hid_handshake_cb(struct bt_hid_host *hid, uint8_t result)
{
	bt_shell_print("HID Host: handshake %u", result);
}

static void hid_protocol_mode_cb(struct bt_hid_host *hid, uint8_t mode)
{
	hid_boot_mode = (mode == BT_HID_PROTOCOL_BOOT_MODE);

	bt_shell_print("HID Host: protocol_mode %u (%s)", mode,
		       hid_boot_mode ? "boot" : "report");
}

static void hid_vc_unplug_cb(struct bt_hid_host *hid)
{
	struct bt_conn *conn = bt_hid_host_get_conn(hid);

	bt_shell_print("HID Host: virtual_cable_unplug");

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

	default_hid = NULL;
}

static const struct bt_hid_host_cb host_cb = {
	.connected = hid_connected_cb,
	.disconnected = hid_disconnected_cb,
	.input_report = hid_input_report_cb,
	.get_report_rsp = hid_get_report_rsp_cb,
	.handshake = hid_handshake_cb,
	.protocol_mode = hid_protocol_mode_cb,
	.vc_unplug = hid_vc_unplug_cb,
};

static int cmd_hid_register(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (hid_registered) {
		shell_error(sh, "already registered");
		return -EALREADY;
	}

	err = bt_hid_host_register(&host_cb);
	if (err) {
		shell_error(sh, "register failed (%d)", err);
		return err;
	}

	hid_registered = true;
	shell_print(sh, "registered");
	return 0;
}

static int cmd_hid_unregister(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (!hid_registered) {
		shell_error(sh, "not registered");
		return -ENOEXEC;
	}

	err = bt_hid_host_unregister();
	if (err) {
		shell_error(sh, "unregister failed (%d)", err);
		return err;
	}

	hid_registered = false;
	default_hid = NULL;
	shell_print(sh, "unregistered");
	return 0;
}

static int cmd_hid_connect(const struct shell *sh, size_t argc, char *argv[])
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

	err = bt_hid_host_connect(default_conn, &default_hid);
	if (err) {
		shell_error(sh, "connect failed (%d)", err);
		return err;
	}

	return 0;
}

static int cmd_hid_sdp_discover(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (!default_conn) {
		shell_error(sh, "not connected");
		return -ENOEXEC;
	}

	err = bt_sdp_discover(default_conn, &discov_hid);
	if (err != 0) {
		shell_error(sh, "SDP discovery failed: err %d", err);
		return -ENOEXEC;
	}

	shell_print(sh, "SDP discovery started");

	return 0;
}

static int cmd_hid_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (!default_hid) {
		shell_error(sh, "not connected");
		return -ENOEXEC;
	}

	err = bt_hid_host_disconnect(default_hid);
	if (err) {
		shell_error(sh, "disconnect failed (%d)", err);
		return err;
	}

	return 0;
}

static int cmd_hid_get_report(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	long type, report_id, buf_size;
	uint8_t rid;

	if (!default_hid) {
		shell_error(sh, "not connected");
		return -ENOEXEC;
	}

	type = shell_strtol(argv[1], 0, &err);
	report_id = shell_strtol(argv[2], 0, &err);
	buf_size = shell_strtol(argv[3], 0, &err);
	if (err) {
		shell_error(sh, "invalid parameter");
		return -EINVAL;
	}

	rid = (uint8_t)report_id;

	err = bt_hid_host_get_report(default_hid, (uint8_t)type,
				     hid_report_id_used() ? &rid : NULL, (uint16_t)buf_size);
	if (err) {
		shell_error(sh, "get_report failed (%d)", err);
		return err;
	}

	return 0;
}

static int cmd_hid_set_report(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_buf *buf;
	int err = 0;
	long type;

	if (!default_hid) {
		shell_error(sh, "not connected");
		return -ENOEXEC;
	}

	type = shell_strtol(argv[1], 0, &err);
	if (err || argc < 3) {
		shell_error(sh, "Usage: set_report <type> <hex bytes...>");
		return -EINVAL;
	}

	buf = bt_hid_host_create_pdu(&hid_host_pool);
	if (!buf) {
		shell_error(sh, "no buffer");
		return -ENOMEM;
	}

	for (int i = 2; i < argc; i++) {
		long val = shell_strtol(argv[i], 16, &err);

		if (err) {
			net_buf_unref(buf);
			shell_error(sh, "invalid hex byte");
			return -EINVAL;
		}
		net_buf_add_u8(buf, (uint8_t)val);
	}

	err = bt_hid_host_set_report(default_hid, (uint8_t)type, buf);
	if (err) {
		net_buf_unref(buf);
		shell_error(sh, "set_report failed (%d)", err);
		return err;
	}

	return 0;
}

static int cmd_hid_get_protocol(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (!default_hid) {
		shell_error(sh, "not connected");
		return -ENOEXEC;
	}

	err = bt_hid_host_get_protocol(default_hid);
	if (err) {
		shell_error(sh, "get_protocol failed (%d)", err);
		return err;
	}

	return 0;
}

static int cmd_hid_set_protocol(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	uint8_t protocol;

	if (!default_hid) {
		shell_error(sh, "not connected");
		return -ENOEXEC;
	}

	if (strcmp(argv[1], "boot") == 0) {
		protocol = BT_HID_PROTOCOL_BOOT_MODE;
	} else if (strcmp(argv[1], "report") == 0) {
		protocol = BT_HID_PROTOCOL_REPORT_MODE;
	} else {
		shell_error(sh, "Usage: set_protocol <boot|report>");
		return -EINVAL;
	}

	err = bt_hid_host_set_protocol(default_hid, protocol);
	if (err) {
		shell_error(sh, "set_protocol failed (%d)", err);
		return err;
	}

	return 0;
}

static int cmd_hid_send(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_buf *buf;
	int err = 0;

	if (!default_hid) {
		shell_error(sh, "not connected");
		return -ENOEXEC;
	}

	if (argc < 2) {
		shell_error(sh, "Usage: send <hex bytes...>");
		return -EINVAL;
	}

	buf = bt_hid_host_create_pdu(&hid_host_pool);
	if (!buf) {
		shell_error(sh, "no buffer");
		return -ENOMEM;
	}

	for (int i = 1; i < argc; i++) {
		long val = shell_strtol(argv[i], 16, &err);

		if (err) {
			net_buf_unref(buf);
			shell_error(sh, "invalid hex byte");
			return -EINVAL;
		}
		net_buf_add_u8(buf, (uint8_t)val);
	}

	err = bt_hid_host_send_output_report(default_hid, buf);
	if (err) {
		net_buf_unref(buf);
		shell_error(sh, "send failed (%d)", err);
		return err;
	}

	shell_print(sh, "sent");
	return 0;
}

static int cmd_hid_suspend(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (!default_hid) {
		shell_error(sh, "not connected");
		return -ENOEXEC;
	}

	err = bt_hid_host_suspend(default_hid);
	if (err) {
		shell_error(sh, "suspend failed (%d)", err);
		return err;
	}

	shell_print(sh, "suspended");
	return 0;
}

static int cmd_hid_exit_suspend(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (!default_hid) {
		shell_error(sh, "not connected");
		return -ENOEXEC;
	}

	err = bt_hid_host_exit_suspend(default_hid);
	if (err) {
		shell_error(sh, "exit_suspend failed (%d)", err);
		return err;
	}

	shell_print(sh, "exit_suspend");
	return 0;
}

static int cmd_hid_vcu(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_conn *conn;
	int err;

	if (!default_hid) {
		shell_error(sh, "not connected");
		return -ENOEXEC;
	}

	/* HID spec v1.1.2 Section 3.1.2.2.3: the initiator also destroys the
	 * bonding. Capture the peer before the association goes away and unpair
	 * from the disconnected callback, once the channels have closed.
	 */
	conn = bt_hid_host_get_conn(default_hid);
	if (conn != NULL) {
		bt_addr_copy(&vcu_peer, bt_conn_get_dst_br(conn));
		vcu_unplug_pending = true;
		bt_conn_unref(conn);
	}

	err = bt_hid_host_virtual_cable_unplug(default_hid);
	if (err) {
		vcu_unplug_pending = false;
		shell_error(sh, "vcu failed (%d)", err);
		return err;
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(hid_host_cmds,
	SHELL_CMD_ARG(register, NULL, "register HID Host", cmd_hid_register, 1, 0),
	SHELL_CMD_ARG(unregister, NULL, "unregister HID Host", cmd_hid_unregister, 1, 0),
	SHELL_CMD_ARG(sdp_discover, NULL, "read the HID service record", cmd_hid_sdp_discover,
		      1, 0),
	SHELL_CMD_ARG(connect, NULL, "connect to HID Device", cmd_hid_connect, 1, 0),
	SHELL_CMD_ARG(disconnect, NULL, "disconnect", cmd_hid_disconnect, 1, 0),
	SHELL_CMD_ARG(get_report, NULL, "<type> <id> <buf_size>", cmd_hid_get_report, 4, 0),
	SHELL_CMD_ARG(set_report, NULL, "<type> <hex...>", cmd_hid_set_report, 3, 8),
	SHELL_CMD_ARG(get_protocol, NULL, "get protocol mode", cmd_hid_get_protocol, 1, 0),
	SHELL_CMD_ARG(set_protocol, NULL, "<boot|report>", cmd_hid_set_protocol, 2, 0),
	SHELL_CMD_ARG(send, NULL, "<hex bytes...> output report", cmd_hid_send, 2, 8),
	SHELL_CMD_ARG(suspend, NULL, "send SUSPEND", cmd_hid_suspend, 1, 0),
	SHELL_CMD_ARG(exit_suspend, NULL, "send EXIT_SUSPEND", cmd_hid_exit_suspend, 1, 0),
	SHELL_CMD_ARG(vcu, NULL, "virtual cable unplug", cmd_hid_vcu, 1, 0),
	SHELL_SUBCMD_SET_END
);

static int cmd_hid_host(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);
	return -ENOEXEC;
}

SHELL_CMD_ARG_REGISTER(hid_host, &hid_host_cmds, "Bluetooth HID Host commands",
		       cmd_hid_host, 1, 0);
