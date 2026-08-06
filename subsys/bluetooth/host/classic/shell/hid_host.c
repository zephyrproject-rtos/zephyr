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
#define HID_ITEM_LONG_PREFIX  0xfeU
#define HID_ITEM_LONG_HDR_LEN 2U

/* ClassDescriptorType values of a HIDDescriptorList entry, HID spec v1.1.2
 * Section 5.3.4.7. Only the Report descriptor carries report definitions.
 */
#define HID_CLASS_DESC_TYPE_NONE   0x00U
#define HID_CLASS_DESC_TYPE_REPORT 0x22U

static struct bt_hid_host *default_hid;
static bool hid_registered;

/* Service record data of the peer. The profile does not read the record, so the
 * application discovers it and keeps whatever it needs to interpret reports.
 *
 * A HID Host can be associated with several devices at the same time and each
 * one has its own descriptor, protocol mode and unplug state, so this is kept
 * per ACL connection rather than globally: a keyboard that declares Report IDs
 * and a mouse that does not have to be told apart when their reports arrive,
 * and an unplug of one peer must not destroy the bonding of another.
 */
struct hid_peer {
	/** Association on this connection, NULL when there is none.
	 *
	 * Kept so that the select command can reach an instance other than the one
	 * that connected last.
	 */
	struct bt_hid_host *hid;
	/** Length of the Report Descriptor read from the record, 0 if unknown. */
	uint16_t descriptor_len;
	/** True when the descriptor declares a Report ID Global item. */
	bool has_report_id;
	/** True while the peer is in Boot Protocol Mode. */
	bool boot_mode;
	/** Mode of the most recent SET_PROTOCOL request.
	 *
	 * The set_protocol callback only carries the result code, so the requested
	 * mode is remembered here and committed to @ref boot_mode once the peer
	 * accepts it.
	 */
	bool req_boot_mode;
	/** True while a Virtual Cable Unplug is waiting for the channels to close. */
	bool vcu_unplug_pending;
	/** Peer address captured for the unpair deferred to the disconnected callback. */
	bt_addr_t vcu_addr;
};

static struct hid_peer hid_peers[CONFIG_BT_MAX_CONN];

static struct hid_peer *hid_peer_by_conn(const struct bt_conn *conn)
{
	size_t index = (size_t)bt_conn_index(conn);

	__ASSERT(index < ARRAY_SIZE(hid_peers), "Index is out of bounds");

	return &hid_peers[index];
}

static struct hid_peer *hid_peer_get(struct bt_hid_host *hid)
{
	struct bt_conn *conn = bt_hid_host_get_conn(hid);
	struct hid_peer *peer;

	if (conn == NULL) {
		return NULL;
	}

	/* Only the connection index is needed, so the reference is released right
	 * away: the entry stays valid because it is owned by this module.
	 */
	peer = hid_peer_by_conn(conn);
	bt_conn_unref(conn);

	return peer;
}

/* True when the ReportID field has to be present in GET_REPORT requests and in
 * report payloads: HID spec v1.1.2 Section 3.1.2.3 for Report Protocol Mode and
 * Section 3.3.1 for Boot Protocol Mode, which always carries a Report ID.
 */
static bool hid_report_id_used(const struct hid_peer *peer)
{
	return (peer != NULL) && (peer->has_report_id || peer->boot_mode);
}

/* Walk the HID Report Descriptor looking for Report ID Global items.
 *
 * HID spec v1.1.2 Section 3.1.2.3 makes the ReportID field of GET_REPORT (and of
 * the reports themselves) mandatory in Report Protocol Mode as soon as any
 * Report ID Global item is declared, so the host has to know whether the
 * descriptor declares one. The values are printed as they are found because the
 * shell commands take a Report ID as an argument.
 */
static bool hid_desc_report_ids(const uint8_t *desc, uint16_t len)
{
	uint16_t pos = 0U;
	bool found = false;

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
				if ((data_len >= sizeof(uint8_t)) &&
				    ((pos + data_len) <= len)) {
					bt_shell_print("HID Report ID 0x%02x", desc[pos]);
				}

				found = true;
			}
		}

		if ((pos + data_len) > len) {
			bt_shell_warn("Report Descriptor truncated at %u", pos);
			break;
		}

		pos += data_len;
	}

	return found;
}

static int hid_sdp_get_uint_attr(const struct net_buf *buf, uint16_t id, uint32_t *val)
{
	struct bt_sdp_attribute attr;
	struct bt_sdp_attr_value value;
	int err;

	err = bt_sdp_get_attr(buf, id, &attr);
	if (err != 0) {
		return err;
	}

	err = bt_sdp_attr_read(&attr, NULL, &value);
	if (err != 0) {
		return err;
	}

	if (value.type != BT_SDP_ATTR_VALUE_TYPE_UINT) {
		return -EINVAL;
	}

	/* bt_sdp_attr_read() writes only the union member matching value.uint.size
	 * and leaves the wider members untouched, so reading .u32 for a u8/u16
	 * attribute would return the uninitialised high bytes. Read at the parsed
	 * width.
	 */
	switch (value.uint.size) {
	case sizeof(uint8_t):
		*val = value.uint.u8;
		break;
	case sizeof(uint16_t):
		*val = value.uint.u16;
		break;
	case sizeof(uint32_t):
		*val = value.uint.u32;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int hid_sdp_get_bool_attr(const struct net_buf *buf, uint16_t id, bool *val)
{
	struct bt_sdp_attribute attr;
	struct bt_sdp_attr_value value;
	int err;

	err = bt_sdp_get_attr(buf, id, &attr);
	if (err != 0) {
		return err;
	}

	err = bt_sdp_attr_read(&attr, NULL, &value);
	if (err != 0) {
		return err;
	}

	if (value.type != BT_SDP_ATTR_VALUE_TYPE_BOOL) {
		return -EINVAL;
	}

	*val = value.value;

	return 0;
}

/* Parse state for HIDDescriptorList: the elements are handed over one by one, so
 * the ClassDescriptorType is remembered until its ClassDescriptorData arrives.
 */
struct hid_desc_parse {
	struct hid_peer *peer;
	uint8_t class_desc_type;
};

static bool hid_desc_list_cb(const struct bt_sdp_attr_value_pair *vp, void *user_data)
{
	struct hid_desc_parse *ctx = user_data;
	struct hid_peer *peer = ctx->peer;

	if ((vp == NULL) || (vp->value == NULL)) {
		return true;
	}

	if ((vp->value->type == BT_SDP_ATTR_VALUE_TYPE_UINT) &&
	    (vp->value->uint.size == sizeof(uint8_t))) {
		ctx->class_desc_type = vp->value->uint.u8;
		return true;
	}

	if ((vp->value->type != BT_SDP_ATTR_VALUE_TYPE_TEXT) || (vp->value->text.len == 0U)) {
		return true;
	}

	/* HID spec v1.1.2 Section 5.3.4.7: each entry pairs a ClassDescriptorType
	 * with its data, and only the Report descriptor carries the report
	 * definitions. A device may also list a Physical descriptor, which must not
	 * be mistaken for it.
	 */
	if (ctx->class_desc_type != HID_CLASS_DESC_TYPE_REPORT) {
		bt_shell_print("Skipping class descriptor 0x%02x", ctx->class_desc_type);
		return true;
	}

	/* The descriptor is only tracked by its length here, and that is kept in a
	 * uint16_t, so refuse one that cannot be represented rather than truncate.
	 */
	if (vp->value->text.len > UINT16_MAX) {
		bt_shell_error("Report Descriptor too long (%u)", vp->value->text.len);
		return false;
	}

	/* Walked in place: only the outcome is kept, the bytes live in the SDP
	 * response buffer for the duration of this callback.
	 */
	peer->descriptor_len = (uint16_t)vp->value->text.len;
	peer->has_report_id = hid_desc_report_ids(vp->value->text.text, peer->descriptor_len);

	return false;
}

/* HIDDescriptorList (0x0206) - nested SEQ { SEQ { UINT8(0x22), TEXT_STR } } */
static int hid_sdp_get_descriptor(const struct net_buf *buf, struct hid_peer *peer)
{
	struct hid_desc_parse ctx = {
		.peer = peer,
		.class_desc_type = HID_CLASS_DESC_TYPE_NONE,
	};
	struct bt_sdp_attribute attr;
	int err;

	err = bt_sdp_get_attr(buf, BT_SDP_ATTR_HID_DESCRIPTOR_LIST, &attr);
	if (err != 0) {
		return err;
	}

	peer->descriptor_len = 0U;
	peer->has_report_id = false;

	bt_sdp_attr_value_parse(&attr, hid_desc_list_cb, &ctx);

	return (peer->descriptor_len != 0U) ? 0 : -ENOENT;
}

/* bt_sdp_discover() links the parameters into a per-connection request list
 * through the node embedded in them, so each connection needs its own set:
 * sharing one would put the same node on two lists. pending stays true while the
 * query on that connection has not been answered, because re-arming a node that
 * is still queued would append it to the list twice. Held outside struct
 * hid_peer because the SDP client owns the parameters until it calls back, which
 * can outlive the association that hid_disconnected_cb() resets. The parameters
 * are filled from discov_hid_template below.
 */
static struct hid_sdp_discov {
	struct bt_sdp_discover_params params;
	bool pending;
} hid_discov[CONFIG_BT_MAX_CONN];

static uint8_t hid_sdp_cb(struct bt_conn *conn, struct bt_sdp_client_result *result,
			  const struct bt_sdp_discover_params *params)
{
	struct hid_peer *peer;
	size_t index;
	bool first;
	uint32_t uval;
	bool bval;
	int err;

	ARG_UNUSED(params);

	index = (size_t)bt_conn_index(conn);

	/* The SDP client releases the parameters only after this callback has
	 * returned for the last time, so pending cannot be cleared any later than
	 * here. Returning BT_SDP_DISCOVER_UUID_STOP below keeps that to one call for
	 * the record this shell is after, instead of one per record. A query stopped
	 * that way is reported once more on the release path, which is recognised by
	 * pending being clear already.
	 */
	first = hid_discov[index].pending;
	hid_discov[index].pending = false;

	if ((result == NULL) || (result->resp_buf == NULL)) {
		if (first) {
			bt_shell_error("No SDP HID data from remote %s", bt_conn_dst_str(conn));
		}

		return BT_SDP_DISCOVER_UUID_STOP;
	}

	bt_shell_print("SDP HID data@%p (len %u) from remote %s", result->resp_buf,
		       result->resp_buf->len, bt_conn_dst_str(conn));

	peer = hid_peer_by_conn(conn);

	/* Only HIDDescriptorList is required: without it reports cannot be split
	 * into Report ID and data. The attributes read below are informational for
	 * this shell, and a device is free to leave the optional ones out, so a
	 * missing one is reported and the rest of the record is still read.
	 */
	err = hid_sdp_get_descriptor(result->resp_buf, peer);
	if (err != 0) {
		bt_shell_error("HID report descriptor not found, err %d", err);
		return BT_SDP_DISCOVER_UUID_CONTINUE;
	}
	bt_shell_print("HID descriptor %u bytes, Report IDs in use: %s", peer->descriptor_len,
		       peer->has_report_id ? "yes" : "no");

	err = hid_sdp_get_uint_attr(result->resp_buf, BT_SDP_ATTR_HID_DEVICE_SUBCLASS, &uval);
	if (err != 0) {
		bt_shell_warn("HID subclass not read, err %d", err);
	} else {
		bt_shell_print("HID subclass 0x%02x", uval);
	}

	err = hid_sdp_get_bool_attr(result->resp_buf, BT_SDP_ATTR_HID_VIRTUAL_CABLE, &bval);
	if (err != 0) {
		bt_shell_warn("HID virtual cable not read, err %d", err);
	} else {
		bt_shell_print("HID virtual cable %d", bval);
	}

	err = hid_sdp_get_bool_attr(result->resp_buf, BT_SDP_ATTR_HID_RECONNECT_INITIATE, &bval);
	if (err != 0) {
		bt_shell_warn("HID reconnect initiate not read, err %d", err);
	} else {
		bt_shell_print("HID reconnect initiate %d", bval);
	}

	err = hid_sdp_get_bool_attr(result->resp_buf, BT_SDP_ATTR_HID_BOOT_DEVICE, &bval);
	if (err != 0) {
		bt_shell_warn("HID boot device not read, err %d", err);
	} else {
		bt_shell_print("HID boot device %d", bval);
	}

	err = hid_sdp_get_uint_attr(result->resp_buf, BT_SDP_ATTR_HID_SUPERVISION_TIMEOUT, &uval);
	if (err != 0) {
		bt_shell_warn("HID supervision timeout not read, err %d", err);
	} else {
		bt_shell_print("HID supervision timeout %u", uval);
	}

	/* The profile associates with one HID Device per ACL connection, so the
	 * first record carrying a report descriptor is the only one of interest.
	 */
	return BT_SDP_DISCOVER_UUID_STOP;
}

/* Only the HID attributes read above are requested, so the peer does not send
 * back the whole record and sdp_client_pool can stay small. Leaving ids unset
 * would ask for the default (0x0000, 0xffff).
 */
static struct bt_sdp_attribute_id_range hid_attr_ranges[] = {
	{
		.beginning = BT_SDP_ATTR_HID_DEVICE_SUBCLASS,
		.ending = BT_SDP_ATTR_HID_BOOT_DEVICE,
	},
};

static struct bt_sdp_attribute_id_list hid_attr_ids = {
	.count = ARRAY_SIZE(hid_attr_ranges),
	.ranges = hid_attr_ranges,
};

/* Copied into the per-connection instance on each query. Kept at file scope
 * because BT_UUID_DECLARE_16() expands to a compound literal: here it has static
 * storage duration, inside a function it would die when the command returns while
 * the query is still in flight.
 */
static const struct bt_sdp_discover_params discov_hid_template = {
	.type = BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR,
	.uuid = BT_UUID_DECLARE_16(BT_SDP_HID_SVCLASS),
	.func = hid_sdp_cb,
	.pool = &sdp_client_pool,
	.ids = &hid_attr_ids,
};

static void hid_connected_cb(struct bt_hid_host *hid)
{
	struct hid_peer *peer = hid_peer_get(hid);

	bt_shell_print("HID Host: connected (%p)", hid);
	default_hid = hid;

	if (peer != NULL) {
		peer->hid = hid;
	}

	if ((peer == NULL) || (peer->descriptor_len == 0U)) {
		/* Reports cannot be split into Report ID and data without the report
		 * descriptor, so run sdp_discover to read it.
		 */
		bt_shell_warn("HID Host: no report descriptor, run sdp_discover");
	}
}

static void hid_disconnected_cb(struct bt_hid_host *hid)
{
	struct hid_peer *peer = hid_peer_get(hid);
	bool unplugged = false;
	bt_addr_t addr;

	bt_shell_print("HID Host: disconnected");
	if (default_hid == hid) {
		default_hid = NULL;
	}

	/* The record data describes the peer that is going away, so none of it
	 * carries over to whatever connects on this index next. The pending unplug
	 * is read out first: it belongs to this peer only, another association must
	 * not have its bonding destroyed here.
	 */
	if (peer != NULL) {
		unplugged = peer->vcu_unplug_pending;
		bt_addr_copy(&addr, &peer->vcu_addr);
		memset(peer, 0, sizeof(*peer));
	}

	if (unplugged) {
		bt_br_unpair(&addr);
	}
}

static void hid_input_report_cb(struct bt_hid_host *hid, struct net_buf *buf)
{
	uint8_t report_id = 0U;

	if (hid_report_id_used(hid_peer_get(hid))) {
		if (buf->len < sizeof(report_id)) {
			bt_shell_error("HID Host: input report without Report ID");
			return;
		}

		report_id = net_buf_pull_u8(buf);
	}

	bt_shell_print("HID Host: input_report id %u len %u", report_id, buf->len);
	bt_shell_hexdump(buf->data, buf->len);
}

static void hid_get_report_cb(struct bt_hid_host *hid, uint8_t result_code, uint8_t type,
			      struct net_buf *buf)
{
	uint8_t report_id = 0U;

	if (result_code != BT_HID_HS_RSP_SUCCESS) {
		bt_shell_error("HID Host: get_report failed (0x%02x)", result_code);
		return;
	}

	/* A GET_REPORT reply carries its payload in a DATA message; a HANDSHAKE has
	 * no payload, so the core delivers a NULL buffer. The spec (v1.1.2 Section
	 * 3.2.3) does not define SUCCESS as a HANDSHAKE result for GET_REPORT, but a
	 * misbehaving device could send one, so guard the buffer independently of
	 * result_code before any dereference rather than trusting the result code.
	 */
	if (buf == NULL) {
		bt_shell_error("HID Host: get_report success without a report payload");
		return;
	}

	if (hid_report_id_used(hid_peer_get(hid))) {
		if (buf->len < sizeof(report_id)) {
			bt_shell_error("HID Host: GET_REPORT response without Report ID");
			return;
		}

		report_id = net_buf_pull_u8(buf);
	}

	bt_shell_print("HID Host: get_report type %u id %u len %u", type, report_id, buf->len);
	bt_shell_hexdump(buf->data, buf->len);
}

static void hid_set_report_cb(struct bt_hid_host *hid, uint8_t result_code)
{
	if (result_code != BT_HID_HS_RSP_SUCCESS) {
		bt_shell_error("HID Host: set_report failed (0x%02x)", result_code);
		return;
	}

	bt_shell_print("HID Host: set_report done");
}

static void hid_get_protocol_cb(struct bt_hid_host *hid, uint8_t result_code, uint8_t protocol)
{
	struct hid_peer *peer = hid_peer_get(hid);
	bool boot_mode = (protocol == BT_HID_PROTOCOL_BOOT_MODE);

	if (result_code != BT_HID_HS_RSP_SUCCESS) {
		bt_shell_error("HID Host: get_protocol failed (0x%02x)", result_code);
		return;
	}

	if (peer != NULL) {
		peer->boot_mode = boot_mode;
	}

	bt_shell_print("HID Host: get_protocol %u (%s)", protocol, boot_mode ? "boot" : "report");
}

static void hid_set_protocol_cb(struct bt_hid_host *hid, uint8_t result_code)
{
	struct hid_peer *peer = hid_peer_get(hid);

	if (result_code != BT_HID_HS_RSP_SUCCESS) {
		bt_shell_error("HID Host: set_protocol failed (0x%02x)", result_code);
		return;
	}

	/* The peer accepted the mode, so the reports that follow are framed for it.
	 * Without this hid_report_id_used() would keep using the previous mode.
	 */
	if (peer != NULL) {
		peer->boot_mode = peer->req_boot_mode;
	}

	bt_shell_print("HID Host: set_protocol done");
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
		struct hid_peer *peer = hid_peer_by_conn(conn);

		bt_addr_copy(&peer->vcu_addr, bt_conn_get_dst_br(conn));
		peer->vcu_unplug_pending = true;
		bt_conn_unref(conn);
	}

	if (default_hid == hid) {
		default_hid = NULL;
	}
}

static const struct bt_hid_host_cb host_cb = {
	.connected = hid_connected_cb,
	.disconnected = hid_disconnected_cb,
	.input_report = hid_input_report_cb,
	.get_report = hid_get_report_cb,
	.set_report = hid_set_report_cb,
	.get_protocol = hid_get_protocol_cb,
	.set_protocol = hid_set_protocol_cb,
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
	if (err != 0) {
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
	if (err != 0) {
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

	if (default_conn == NULL) {
		shell_error(sh, "not connected");
		return -ENOEXEC;
	}

	err = bt_hid_host_connect(default_conn, &default_hid);
	if (err != 0) {
		shell_error(sh, "connect failed (%d)", err);
		return err;
	}

	return 0;
}

static int cmd_hid_sdp_discover(const struct shell *sh, size_t argc, char *argv[])
{
	size_t index;
	int err;

	if (default_conn == NULL) {
		shell_error(sh, "not connected");
		return -ENOEXEC;
	}

	index = (size_t)bt_conn_index(default_conn);
	__ASSERT(index < ARRAY_SIZE(hid_discov), "Index is out of bounds");

	if (hid_discov[index].pending) {
		shell_error(sh, "a service record query is already outstanding");
		return -EBUSY;
	}

	hid_discov[index].params = discov_hid_template;
	hid_discov[index].pending = true;

	err = bt_sdp_discover(default_conn, &hid_discov[index].params);
	if (err != 0) {
		hid_discov[index].pending = false;
		shell_error(sh, "SDP discovery failed: err %d", err);
		return -ENOEXEC;
	}

	shell_print(sh, "SDP discovery started");

	return 0;
}

static int cmd_hid_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (default_hid == NULL) {
		shell_error(sh, "not connected");
		return -ENOEXEC;
	}

	err = bt_hid_host_disconnect(default_hid);
	if (err != 0) {
		shell_error(sh, "disconnect failed (%d)", err);
		return err;
	}

	return 0;
}

/* The connected callback points default_hid at whatever associated last, so this
 * is how a second association is reached while both are up. Keyed by address as
 * the BR/EDR select command is, since an association belongs to one ACL.
 */
static int cmd_hid_select(const struct shell *sh, size_t argc, char *argv[])
{
	const struct hid_peer *peer;
	struct bt_conn *conn;
	bt_addr_t addr;
	int err;

	err = bt_addr_from_str(argv[1], &addr);
	if (err != 0) {
		shell_error(sh, "invalid peer address (err %d)", err);
		return err;
	}

	conn = bt_conn_lookup_addr_br(&addr);
	if (conn == NULL) {
		shell_error(sh, "no matching connection");
		return -ENOEXEC;
	}

	peer = hid_peer_by_conn(conn);
	bt_conn_unref(conn);

	if (peer->hid == NULL) {
		shell_error(sh, "no HID association on that connection");
		return -ENOEXEC;
	}

	default_hid = peer->hid;
	shell_print(sh, "selected HID Host %p", default_hid);

	return 0;
}

static int cmd_hid_get_report(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	long type, report_id, buf_size;

	if (default_hid == NULL) {
		shell_error(sh, "not connected");
		return -ENOEXEC;
	}

	type = shell_strtol(argv[1], 0, &err);
	report_id = shell_strtol(argv[2], 0, &err);
	buf_size = shell_strtol(argv[3], 0, &err);
	if (err != 0) {
		shell_error(sh, "invalid parameter");
		return -EINVAL;
	}

	if (!IN_RANGE(type, BT_HID_REPORT_TYPE_INPUT, BT_HID_REPORT_TYPE_FEATURE) ||
	    !IN_RANGE(report_id, 0, UINT8_MAX) || !IN_RANGE(buf_size, 0, UINT16_MAX)) {
		shell_error(sh, "parameter out of range");
		return -EINVAL;
	}

	err = bt_hid_host_get_report(default_hid, (uint8_t)type,
				     hid_report_id_used(hid_peer_get(default_hid))
					     ? (uint8_t)report_id
					     : 0U,
				     (uint16_t)buf_size);
	if (err != 0) {
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

	if (default_hid == NULL) {
		shell_error(sh, "not connected");
		return -ENOEXEC;
	}

	type = shell_strtol(argv[1], 0, &err);
	if ((err != 0) || (argc < 3)) {
		shell_error(sh, "Usage: set_report <type> <hex...>, Report ID first if any");
		return -EINVAL;
	}

	if (!IN_RANGE(type, BT_HID_REPORT_TYPE_INPUT, BT_HID_REPORT_TYPE_FEATURE)) {
		shell_error(sh, "invalid report type");
		return -EINVAL;
	}

	buf = bt_hid_host_create_pdu(&hid_host_pool);
	if (buf == NULL) {
		shell_error(sh, "no buffer");
		return -ENOBUFS;
	}

	for (int i = 2; i < argc; i++) {
		long val = shell_strtol(argv[i], 16, &err);

		if ((err != 0) || !IN_RANGE(val, 0, UINT8_MAX)) {
			net_buf_unref(buf);
			shell_error(sh, "invalid hex byte");
			return -EINVAL;
		}
		net_buf_add_u8(buf, (uint8_t)val);
	}

	err = bt_hid_host_set_report(default_hid, (uint8_t)type, buf);
	if (err != 0) {
		net_buf_unref(buf);
		shell_error(sh, "set_report failed (%d)", err);
		return err;
	}

	return 0;
}

static int cmd_hid_get_protocol(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (default_hid == NULL) {
		shell_error(sh, "not connected");
		return -ENOEXEC;
	}

	err = bt_hid_host_get_protocol(default_hid);
	if (err != 0) {
		shell_error(sh, "get_protocol failed (%d)", err);
		return err;
	}

	return 0;
}

static int cmd_hid_set_protocol(const struct shell *sh, size_t argc, char *argv[])
{
	struct hid_peer *peer;
	int err;
	uint8_t protocol;

	if (default_hid == NULL) {
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

	/* Recorded before the request goes out: the set_protocol callback carries
	 * only the result code, and it runs on the RX thread, so it can arrive
	 * before this function returns.
	 */
	peer = hid_peer_get(default_hid);
	if (peer != NULL) {
		peer->req_boot_mode = (protocol == BT_HID_PROTOCOL_BOOT_MODE);
	}

	err = bt_hid_host_set_protocol(default_hid, protocol);
	if (err != 0) {
		shell_error(sh, "set_protocol failed (%d)", err);
		return err;
	}

	return 0;
}

static int cmd_hid_send(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_buf *buf;
	int err = 0;

	if (default_hid == NULL) {
		shell_error(sh, "not connected");
		return -ENOEXEC;
	}

	if (argc < 2) {
		shell_error(sh, "Usage: send <hex...>, Report ID first if any");
		return -EINVAL;
	}

	buf = bt_hid_host_create_pdu(&hid_host_pool);
	if (buf == NULL) {
		shell_error(sh, "no buffer");
		return -ENOBUFS;
	}

	for (int i = 1; i < argc; i++) {
		long val = shell_strtol(argv[i], 16, &err);

		if ((err != 0) || !IN_RANGE(val, 0, UINT8_MAX)) {
			net_buf_unref(buf);
			shell_error(sh, "invalid hex byte");
			return -EINVAL;
		}
		net_buf_add_u8(buf, (uint8_t)val);
	}

	err = bt_hid_host_output_report(default_hid, buf);
	if (err != 0) {
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

	if (default_hid == NULL) {
		shell_error(sh, "not connected");
		return -ENOEXEC;
	}

	err = bt_hid_host_suspend(default_hid);
	if (err != 0) {
		shell_error(sh, "suspend failed (%d)", err);
		return err;
	}

	shell_print(sh, "suspended");
	return 0;
}

static int cmd_hid_exit_suspend(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (default_hid == NULL) {
		shell_error(sh, "not connected");
		return -ENOEXEC;
	}

	err = bt_hid_host_exit_suspend(default_hid);
	if (err != 0) {
		shell_error(sh, "exit_suspend failed (%d)", err);
		return err;
	}

	shell_print(sh, "exit_suspend");
	return 0;
}

static int cmd_hid_vcu(const struct shell *sh, size_t argc, char *argv[])
{
	struct hid_peer *peer = NULL;
	struct bt_conn *conn;
	int err;

	if (default_hid == NULL) {
		shell_error(sh, "not connected");
		return -ENOEXEC;
	}

	/* HID spec v1.1.2 Section 3.1.2.2.3: the initiator also destroys the
	 * bonding. Capture the peer before the association goes away and unpair
	 * from the disconnected callback, once the channels have closed.
	 */
	conn = bt_hid_host_get_conn(default_hid);
	if (conn != NULL) {
		peer = hid_peer_by_conn(conn);
		bt_addr_copy(&peer->vcu_addr, bt_conn_get_dst_br(conn));
		peer->vcu_unplug_pending = true;
		bt_conn_unref(conn);
	}

	err = bt_hid_host_virtual_cable_unplug(default_hid);
	if (err != 0) {
		if (peer != NULL) {
			peer->vcu_unplug_pending = false;
		}
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
	SHELL_CMD_ARG(select, NULL, "<address: XX:XX:XX:XX:XX:XX>", cmd_hid_select, 2, 0),
	SHELL_CMD_ARG(get_report, NULL, "<type> <id> <buf_size>", cmd_hid_get_report, 4, 0),
	SHELL_CMD_ARG(set_report, NULL, "<type> <hex...>, Report ID first if the report has one",
		      cmd_hid_set_report, 3, 8),
	SHELL_CMD_ARG(get_protocol, NULL, "get protocol mode", cmd_hid_get_protocol, 1, 0),
	SHELL_CMD_ARG(set_protocol, NULL, "<boot|report>", cmd_hid_set_protocol, 2, 0),
	SHELL_CMD_ARG(send, NULL, "<hex...> output report, Report ID first if the report has one",
		      cmd_hid_send, 2, 8),
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
