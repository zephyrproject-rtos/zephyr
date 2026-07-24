/** @file
 *  @brief Bluetooth Human Interface Device Profile (HID) device role
 */

/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/hid.h>
#include <zephyr/bluetooth/classic/sdp.h>

#include "host/conn_internal.h"
#include "l2cap_br_internal.h"
#include "hid_internal.h"

#define LOG_LEVEL CONFIG_BT_HID_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_hid);

#define BT_HID_MSG_HDR(type, param) ((uint8_t)(((type) << 4) | ((param) & 0x0fU)))

#define BT_HID_REPORT_DESC_TYPE 0x22U

#define BT_HID_FLAG_CTRL_CONNECTED BIT(0)
#define BT_HID_FLAG_INTR_CONNECTED BIT(1)

struct bt_hid_device {
	struct bt_conn *conn;
	struct bt_l2cap_br_chan ctrl_chan;
	struct bt_l2cap_br_chan intr_chan;
	enum bt_hid_protocol protocol;
	uint8_t idle_rate;
	atomic_t flags[1];
};

static struct bt_hid_device hid_device_pool[CONFIG_BT_MAX_CONN];
static const struct bt_hid_device_cb *hid_cb;
static const struct bt_hid_device_register_param *hid_param;
static bool hid_registered;

static uint8_t hid_desc_list_buf[CONFIG_BT_HID_MAX_REPORT_DESC + 16U];
static uint8_t hid_service_name_buf[CONFIG_BT_HID_MAX_NAME_LEN + 1U];
static struct bt_sdp_attribute hid_attrs[17];
static struct bt_sdp_record hid_rec;

NET_BUF_POOL_FIXED_DEFINE(hid_pool, CONFIG_BT_MAX_CONN,
			  BT_L2CAP_BUF_SIZE(CONFIG_BT_HID_MAX_REPORT_LEN + 1U),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static uint8_t hid_device_release[2];
static uint8_t hid_parser_version[2];
static uint8_t hid_device_subclass[1];
static uint8_t hid_country_code[1];
static uint8_t hid_virtual_cable[1];
static uint8_t hid_reconnect_initiate[1];
static uint8_t hid_profile_version[2];
static uint8_t hid_supervision_timeout[2];
static uint8_t hid_normally_connectable[1];
static uint8_t hid_boot_device[1];

static struct bt_hid_device *hid_device_alloc(struct bt_conn *conn)
{
	struct bt_hid_device *hid;

	for (size_t i = 0; i < ARRAY_SIZE(hid_device_pool); i++) {
		hid = &hid_device_pool[i];

		if (hid->conn == NULL) {
			(void)memset(hid, 0, sizeof(*hid));
			hid->conn = bt_conn_ref(conn);
			hid->protocol = BT_HID_PROTOCOL_REPORT;
			atomic_set(hid->flags, 0);
			return hid;
		}
	}

	return NULL;
}

static struct bt_hid_device *hid_device_lookup(struct bt_conn *conn)
{
	for (size_t i = 0; i < ARRAY_SIZE(hid_device_pool); i++) {
		if (hid_device_pool[i].conn == conn) {
			return &hid_device_pool[i];
		}
	}

	return NULL;
}

static void hid_device_free(struct bt_hid_device *hid)
{
	if (hid == NULL) {
		return;
	}

	if (hid->conn != NULL) {
		bt_conn_unref(hid->conn);
		hid->conn = NULL;
	}

	(void)memset(hid, 0, sizeof(*hid));
}

static int hid_chan_send(struct bt_l2cap_br_chan *chan, struct net_buf *buf)
{
	if ((chan == NULL) || (chan->chan.conn == NULL)) {
		return -ENOTCONN;
	}

	return bt_l2cap_br_chan_send(&chan->chan, buf);
}

static struct net_buf *hid_buf_alloc(size_t len)
{
	return bt_l2cap_create_pdu_timeout(&hid_pool, 1U + len, K_NO_WAIT);
}

static int hid_send_handshake(struct bt_l2cap_br_chan *chan,
			      enum bt_hid_handshake_result result)
{
	struct net_buf *buf;
	uint8_t *hdr;

	buf = hid_buf_alloc(0U);
	if (buf == NULL) {
		return -ENOMEM;
	}

	hdr = net_buf_add(buf, 1U);
	*hdr = BT_HID_MSG_HDR(BT_HID_MSG_HANDSHAKE, result);

	return hid_chan_send(chan, buf);
}

static int hid_send_data(struct bt_l2cap_br_chan *chan, enum bt_hid_msg_type type,
			 enum bt_hid_report_type report_type, const uint8_t *data, size_t len)
{
	struct net_buf *buf;
	uint8_t *hdr;

	buf = hid_buf_alloc(len);
	if (buf == NULL) {
		return -ENOMEM;
	}

	hdr = net_buf_add(buf, 1U);
	*hdr = BT_HID_MSG_HDR(type, report_type);

	if ((len > 0U) && (data != NULL)) {
		net_buf_add_mem(buf, data, len);
	}

	return hid_chan_send(chan, buf);
}

static int hid_get_report(struct bt_hid_device *hid, enum bt_hid_report_type type,
			  uint8_t report_id, struct bt_l2cap_br_chan *chan)
{
	uint8_t report[CONFIG_BT_HID_MAX_REPORT_LEN];
	size_t len = sizeof(report);
	int err;

	if (hid_cb == NULL || hid_cb->get_report == NULL) {
		return hid_send_handshake(chan, BT_HID_HANDSHAKE_ERR_UNSUPPORTED_REQUEST);
	}

	if (report_id != 0U) {
		report[0] = report_id;
		len = sizeof(report) - 1U;
		err = hid_cb->get_report(hid, type, report_id, &report[1], &len);
		if (err == 0) {
			len += 1U;
		}
	} else {
		err = hid_cb->get_report(hid, type, 0U, report, &len);
	}

	if (err != 0) {
		return hid_send_handshake(chan, BT_HID_HANDSHAKE_ERR_INVALID_REPORT_ID);
	}

	return hid_send_data(chan, BT_HID_MSG_DATA, type, report, len);
}

static int hid_ctrl_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_hid_device *hid = CONTAINER_OF(BR_CHAN(chan), struct bt_hid_device, ctrl_chan);
	enum bt_hid_msg_type type;
	uint8_t hdr;
	uint8_t param;
	int err = 0;

	if (buf->len == 0U) {
		return 0;
	}

	hdr = net_buf_pull_u8(buf);
	type = (enum bt_hid_msg_type)(hdr >> 4);
	param = hdr & 0x0fU;

	switch (type) {
	case BT_HID_MSG_HID_CONTROL:
		LOG_DBG("HID control op 0x%02x", param);
		break;

	case BT_HID_MSG_GET_REPORT: {
		uint8_t report_id = 0U;
		enum bt_hid_report_type report_type = (enum bt_hid_report_type)(param & 0x03U);

		if ((param & BIT(3)) != 0U) {
			if (buf->len == 0U) {
				err = hid_send_handshake(&hid->ctrl_chan,
							 BT_HID_HANDSHAKE_ERR_INVALID_PARAMETER);
				break;
			}

			report_id = net_buf_pull_u8(buf);
		}

		err = hid_get_report(hid, report_type, report_id, &hid->ctrl_chan);
		break;
	}

	case BT_HID_MSG_SET_REPORT: {
		uint8_t report_id = 0U;
		const uint8_t *data = buf->data;
		size_t len = buf->len;

		if (len == 0U) {
			err = hid_send_handshake(&hid->ctrl_chan,
						 BT_HID_HANDSHAKE_ERR_INVALID_PARAMETER);
			break;
		}

		if ((param & BIT(3)) != 0U) {
			report_id = net_buf_pull_u8(buf);
			data = buf->data;
			len = buf->len;
		}

		if (hid_cb != NULL && hid_cb->set_report != NULL) {
			err = hid_cb->set_report(hid, (enum bt_hid_report_type)(param & 0x03U),
						 report_id, data, len);
		}

		if (err != 0) {
			err = hid_send_handshake(&hid->ctrl_chan,
						 BT_HID_HANDSHAKE_ERR_INVALID_PARAMETER);
		} else {
			err = hid_send_handshake(&hid->ctrl_chan, BT_HID_HANDSHAKE_SUCCESS);
		}
		break;
	}

	case BT_HID_MSG_GET_PROTOCOL:
		err = hid_send_data(&hid->ctrl_chan, BT_HID_MSG_DATA, 0U,
				    (const uint8_t *)&hid->protocol, sizeof(hid->protocol));
		break;

	case BT_HID_MSG_SET_PROTOCOL:
		hid->protocol = (enum bt_hid_protocol)param;
		if (hid_cb != NULL && hid_cb->protocol_changed != NULL) {
			hid_cb->protocol_changed(hid, hid->protocol);
		}
		err = hid_send_handshake(&hid->ctrl_chan, BT_HID_HANDSHAKE_SUCCESS);
		break;

	case BT_HID_MSG_GET_IDLE:
		err = hid_send_data(&hid->ctrl_chan, BT_HID_MSG_DATA, 0U, &hid->idle_rate,
				    sizeof(hid->idle_rate));
		break;

	case BT_HID_MSG_SET_IDLE:
		hid->idle_rate = param;
		err = hid_send_handshake(&hid->ctrl_chan, BT_HID_HANDSHAKE_SUCCESS);
		break;

	default:
		LOG_WRN("Unsupported HID transaction 0x%02x", type);
		err = hid_send_handshake(&hid->ctrl_chan,
					 BT_HID_HANDSHAKE_ERR_UNSUPPORTED_REQUEST);
		break;
	}

	if (err != 0) {
		LOG_WRN("HID control handling failed (%d)", err);
	}

	return 0;
}

static int hid_intr_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	ARG_UNUSED(chan);
	LOG_DBG("Unexpected data on interrupt channel (%u bytes)", buf->len);

	return 0;
}

static void hid_ctrl_connected(struct bt_l2cap_chan *chan)
{
	struct bt_hid_device *hid = CONTAINER_OF(BR_CHAN(chan), struct bt_hid_device, ctrl_chan);

	atomic_set_bit(hid->flags, BT_HID_FLAG_CTRL_CONNECTED);

	if (hid_cb != NULL && hid_cb->connected != NULL) {
		hid_cb->connected(hid->conn, hid);
	}
}

static void hid_ctrl_disconnected(struct bt_l2cap_chan *chan)
{
	struct bt_hid_device *hid = CONTAINER_OF(BR_CHAN(chan), struct bt_hid_device, ctrl_chan);

	atomic_clear_bit(hid->flags, BT_HID_FLAG_CTRL_CONNECTED);

	if (hid_cb != NULL && hid_cb->disconnected != NULL) {
		hid_cb->disconnected(hid);
	}

	hid_device_free(hid);
}

static void hid_intr_connected(struct bt_l2cap_chan *chan)
{
	struct bt_hid_device *hid = CONTAINER_OF(BR_CHAN(chan), struct bt_hid_device, intr_chan);

	atomic_set_bit(hid->flags, BT_HID_FLAG_INTR_CONNECTED);

	if (hid_cb != NULL && hid_cb->intr_connected != NULL) {
		hid_cb->intr_connected(hid);
	}
}

static void hid_intr_disconnected(struct bt_l2cap_chan *chan)
{
	struct bt_hid_device *hid = CONTAINER_OF(BR_CHAN(chan), struct bt_hid_device, intr_chan);

	if (hid->conn == NULL) {
		return;
	}

	atomic_clear_bit(hid->flags, BT_HID_FLAG_INTR_CONNECTED);
}

static const struct bt_l2cap_chan_ops hid_ctrl_ops = {
	.connected = hid_ctrl_connected,
	.disconnected = hid_ctrl_disconnected,
	.recv = hid_ctrl_recv,
};

static const struct bt_l2cap_chan_ops hid_intr_ops = {
	.connected = hid_intr_connected,
	.disconnected = hid_intr_disconnected,
	.recv = hid_intr_recv,
};

static int hid_l2cap_accept(struct bt_conn *conn, struct bt_l2cap_server *server,
			    struct bt_l2cap_chan **chan)
{
	struct bt_hid_device *hid;
	struct bt_l2cap_br_chan *br_chan;

	if (!hid_registered) {
		return -EINVAL;
	}

	hid = hid_device_lookup(conn);
	if (hid == NULL) {
		hid = hid_device_alloc(conn);
		if (hid == NULL) {
			return -ENOMEM;
		}
	}

	if (server->psm == BT_HID_PSM_CTRL) {
		br_chan = &hid->ctrl_chan;
		br_chan->chan.ops = &hid_ctrl_ops;
	} else if (server->psm == BT_HID_PSM_INTR) {
		br_chan = &hid->intr_chan;
		br_chan->chan.ops = &hid_intr_ops;
	} else {
		return -EINVAL;
	}

	br_chan->rx.mtu = BT_L2CAP_RX_MTU;
	br_chan->required_sec_level = BT_SECURITY_L2;
	*chan = &br_chan->chan;

	return 0;
}

static void hid_sdp_set_attr(struct bt_sdp_attribute *attr, uint16_t id, uint8_t type,
			     const void *data, size_t size)
{
	attr->id = id;
	attr->val.type = type;
	attr->val.data_size = size;
	attr->val.total_size = size;
	attr->val.data = data;
}

static int hid_sdp_build_desc_list(const uint8_t *report_desc, size_t report_desc_len)
{
	size_t inner_content_len = 3U + 2U + report_desc_len;
	size_t inner_seq_len = 2U + inner_content_len;
	size_t outer_seq_len = 2U + inner_seq_len;
	size_t offset = 0U;

	if (report_desc_len > CONFIG_BT_HID_MAX_REPORT_DESC) {
		return -EINVAL;
	}

	if (outer_seq_len > sizeof(hid_desc_list_buf)) {
		return -ENOMEM;
	}

	hid_desc_list_buf[offset++] = BT_SDP_SEQ8;
	hid_desc_list_buf[offset++] = (uint8_t)inner_seq_len;
	hid_desc_list_buf[offset++] = BT_SDP_SEQ8;
	hid_desc_list_buf[offset++] = (uint8_t)inner_content_len;
	hid_desc_list_buf[offset++] = BT_SDP_UINT8;
	hid_desc_list_buf[offset++] = 0x01U;
	hid_desc_list_buf[offset++] = BT_HID_REPORT_DESC_TYPE;
	hid_desc_list_buf[offset++] = BT_SDP_TEXT_STR8;
	hid_desc_list_buf[offset++] = (uint8_t)report_desc_len;
	memcpy(&hid_desc_list_buf[offset], report_desc, report_desc_len);
	offset += report_desc_len;

	hid_sdp_set_attr(&hid_attrs[8], BT_SDP_ATTR_HID_DESCRIPTOR_LIST, BT_SDP_SEQ8,
			 hid_desc_list_buf, offset);

	return 0;
}

static int hid_sdp_register_record(const struct bt_hid_device_register_param *param)
{
	static const uint8_t svclass[] = {
		BT_SDP_SEQ8, 3U,
		BT_SDP_UUID16, 0x11U, 0x24U,
	};
	static const uint8_t proto_desc[] = {
		BT_SDP_SEQ8, 22U,
		BT_SDP_SEQ8, 20U,
		BT_SDP_SEQ8, 6U,
		BT_SDP_UUID16, 0x01U, 0x00U,
		BT_SDP_UINT16, 0x00U, 0x11U,
		BT_SDP_SEQ8, 6U,
		BT_SDP_UUID16, 0x01U, 0x00U,
		BT_SDP_UINT16, 0x00U, 0x13U,
		BT_SDP_SEQ8, 3U,
		BT_SDP_UUID16, 0x00U, 0x11U,
	};
	static const uint8_t lang_base[] = {
		BT_SDP_SEQ8, 9U,
		BT_SDP_SEQ8, 7U,
		BT_SDP_UINT16, 0x65U, 0x6eU,
		BT_SDP_UINT16, 0x00U, 0x6aU,
		BT_SDP_UINT16, 0x01U, 0x00U,
	};
	static const uint8_t profile_desc[] = {
		BT_SDP_SEQ8, 8U,
		BT_SDP_SEQ8, 6U,
		BT_SDP_UUID16, 0x11U, 0x24U,
		BT_SDP_UINT16, 0x01U, 0x01U,
	};
	static const uint8_t record_handle[] = { 0U, 0U, 0U, 0U };
	size_t name_len;
	int err;

	if ((param == NULL) || (param->report_desc == NULL) || (param->report_desc_len == 0U)) {
		return -EINVAL;
	}

	name_len = strnlen(param->name, CONFIG_BT_HID_MAX_NAME_LEN);
	memcpy(hid_service_name_buf, param->name, name_len);
	hid_service_name_buf[name_len] = '\0';

	sys_put_le16(param->device_release_number, hid_device_release);
	sys_put_le16(BT_HID_PARSER_VERSION, hid_parser_version);
	hid_device_subclass[0] = (uint8_t)param->subclass;
	hid_country_code[0] = param->country_code;
	hid_virtual_cable[0] = 0U;
	hid_reconnect_initiate[0] = 1U;
	sys_put_le16(BT_HID_PROFILE_VERSION, hid_profile_version);
	sys_put_le16(param->supervision_timeout, hid_supervision_timeout);
	hid_normally_connectable[0] = param->normally_connectable ? 1U : 0U;
	hid_boot_device[0] = param->boot_device ? 1U : 0U;

	err = hid_sdp_build_desc_list(param->report_desc, param->report_desc_len);
	if (err != 0) {
		return err;
	}

	hid_sdp_set_attr(&hid_attrs[0], BT_SDP_ATTR_RECORD_HANDLE, BT_SDP_UINT32,
			 record_handle, sizeof(record_handle));
	hid_sdp_set_attr(&hid_attrs[1], BT_SDP_ATTR_SVCLASS_ID_LIST, BT_SDP_SEQ8, svclass,
			 sizeof(svclass));
	hid_sdp_set_attr(&hid_attrs[2], BT_SDP_ATTR_PROTO_DESC_LIST, BT_SDP_SEQ8, proto_desc,
			 sizeof(proto_desc));
	hid_sdp_set_attr(&hid_attrs[3], BT_SDP_ATTR_LANG_BASE_ATTR_ID_LIST, BT_SDP_SEQ8, lang_base,
			 sizeof(lang_base));
	hid_sdp_set_attr(&hid_attrs[4], BT_SDP_ATTR_PROFILE_DESC_LIST, BT_SDP_SEQ8, profile_desc,
			 sizeof(profile_desc));
	hid_sdp_set_attr(&hid_attrs[5], 0x0100U, BT_SDP_TEXT_STR8, hid_service_name_buf, name_len);
	hid_sdp_set_attr(&hid_attrs[6], BT_SDP_ATTR_HID_DEVICE_RELEASE_NUMBER, BT_SDP_UINT16,
			 hid_device_release, sizeof(hid_device_release));
	hid_sdp_set_attr(&hid_attrs[7], BT_SDP_ATTR_HID_PARSER_VERSION, BT_SDP_UINT16,
			 hid_parser_version, sizeof(hid_parser_version));
	hid_sdp_set_attr(&hid_attrs[9], BT_SDP_ATTR_HID_DEVICE_SUBCLASS, BT_SDP_UINT8,
			 hid_device_subclass, sizeof(hid_device_subclass));
	hid_sdp_set_attr(&hid_attrs[10], BT_SDP_ATTR_HID_COUNTRY_CODE, BT_SDP_UINT8,
			 hid_country_code, sizeof(hid_country_code));
	hid_sdp_set_attr(&hid_attrs[11], BT_SDP_ATTR_HID_VIRTUAL_CABLE, BT_SDP_BOOL,
			 hid_virtual_cable, sizeof(hid_virtual_cable));
	hid_sdp_set_attr(&hid_attrs[12], BT_SDP_ATTR_HID_RECONNECT_INITIATE, BT_SDP_BOOL,
			 hid_reconnect_initiate, sizeof(hid_reconnect_initiate));
	hid_sdp_set_attr(&hid_attrs[13], BT_SDP_ATTR_HID_PROFILE_VERSION, BT_SDP_UINT16,
			 hid_profile_version, sizeof(hid_profile_version));
	hid_sdp_set_attr(&hid_attrs[14], BT_SDP_ATTR_HID_SUPERVISION_TIMEOUT, BT_SDP_UINT16,
			 hid_supervision_timeout, sizeof(hid_supervision_timeout));
	hid_sdp_set_attr(&hid_attrs[15], BT_SDP_ATTR_HID_NORMALLY_CONNECTABLE, BT_SDP_BOOL,
			 hid_normally_connectable, sizeof(hid_normally_connectable));
	hid_sdp_set_attr(&hid_attrs[16], BT_SDP_ATTR_HID_BOOT_DEVICE, BT_SDP_BOOL,
			 hid_boot_device, sizeof(hid_boot_device));

	hid_rec.attrs = hid_attrs;
	hid_rec.attr_count = ARRAY_SIZE(hid_attrs);

	return bt_sdp_register_service(&hid_rec);
}

static struct bt_l2cap_server hid_ctrl_server = {
	.psm = BT_HID_PSM_CTRL,
	.sec_level = BT_SECURITY_L2,
	.accept = hid_l2cap_accept,
};

static struct bt_l2cap_server hid_intr_server = {
	.psm = BT_HID_PSM_INTR,
	.sec_level = BT_SECURITY_L2,
	.accept = hid_l2cap_accept,
};

void bt_hid_init(void)
{
	static bool initialized;

	if (initialized) {
		return;
	}

	initialized = true;
}

int bt_hid_device_register(const struct bt_hid_device_register_param *param,
			   const struct bt_hid_device_cb *cb)
{
	int err;

	if ((param == NULL) || (cb == NULL)) {
		return -EINVAL;
	}

	if (hid_registered) {
		return -EALREADY;
	}

	hid_param = param;
	hid_cb = cb;

	err = hid_sdp_register_record(param);
	if (err != 0) {
		hid_param = NULL;
		hid_cb = NULL;
		return err;
	}

	err = bt_l2cap_br_server_register(&hid_ctrl_server);
	if (err != 0 && err != -EEXIST) {
		return err;
	}

	err = bt_l2cap_br_server_register(&hid_intr_server);
	if (err != 0 && err != -EEXIST) {
		return err;
	}

	hid_registered = true;

	return 0;
}

int bt_hid_device_input_report_send(struct bt_hid_device *hid, uint8_t report_id,
				    const uint8_t *data, size_t len)
{
	uint8_t report[CONFIG_BT_HID_MAX_REPORT_LEN];
	size_t report_len = len;

	if ((hid == NULL) || (data == NULL) || (len == 0U)) {
		return -EINVAL;
	}

	if (!atomic_test_bit(hid->flags, BT_HID_FLAG_INTR_CONNECTED)) {
		return -ENOTCONN;
	}

	if (report_id != 0U) {
		if ((len + 1U) > sizeof(report)) {
			return -ENOMEM;
		}

		report[0] = report_id;
		memcpy(&report[1], data, len);
		report_len = len + 1U;
		return hid_send_data(&hid->intr_chan, BT_HID_MSG_DATA, BT_HID_REPORT_INPUT, report,
				     report_len);
	}

	if (len > sizeof(report)) {
		return -ENOMEM;
	}

	return hid_send_data(&hid->intr_chan, BT_HID_MSG_DATA, BT_HID_REPORT_INPUT, data, len);
}

struct bt_conn *bt_hid_device_conn_get(const struct bt_hid_device *hid)
{
	if (hid == NULL) {
		return NULL;
	}

	return hid->conn;
}

enum bt_hid_protocol bt_hid_device_protocol_get(const struct bt_hid_device *hid)
{
	if (hid == NULL) {
		return BT_HID_PROTOCOL_REPORT;
	}

	return hid->protocol;
}
