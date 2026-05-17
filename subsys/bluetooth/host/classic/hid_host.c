/*
 * Copyright 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/hid_host.h>
#include <zephyr/bluetooth/classic/sdp.h>

#include "hid_internal.h"
#include "host/hci_core.h"
#include "host/conn_internal.h"
#include "host/l2cap_internal.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_hid_host);

#define HID_HOST_INTR_TIMEOUT K_SECONDS(30)

#define HID_HOST_BY_CTRL_CHAN(ch) \
	CONTAINER_OF(CONTAINER_OF(ch, struct bt_l2cap_br_chan, chan), \
		     struct bt_hid_host_session, br_chan)

#define HID_HOST_FROM_CTRL(session) \
	CONTAINER_OF(session, struct bt_hid_host, ctrl_session)

#define HID_HOST_FROM_INTR(session) \
	CONTAINER_OF(session, struct bt_hid_host, intr_session)


enum {
	HID_HOST_W4_NONE = 0,
	HID_HOST_W4_GET_REPORT,
	HID_HOST_W4_SET_REPORT,
	HID_HOST_W4_GET_PROTOCOL,
	HID_HOST_W4_SET_PROTOCOL,
};

static const struct bt_hid_host_cb *hid_host_cb;
static bool hid_host_registered;

static struct bt_hid_host hid_host_pool[CONFIG_BT_MAX_CONN];

NET_BUF_POOL_FIXED_DEFINE(hid_host_sdp_pool, CONFIG_BT_MAX_CONN,
			  BT_L2CAP_BUF_SIZE(CONFIG_BT_L2CAP_TX_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static int hid_host_l2cap_ctrl_recv(struct bt_l2cap_chan *chan, struct net_buf *buf);
static int hid_host_l2cap_intr_recv(struct bt_l2cap_chan *chan, struct net_buf *buf);
static void hid_host_ctrl_connected(struct bt_l2cap_chan *chan);
static void hid_host_ctrl_disconnected(struct bt_l2cap_chan *chan);
static void hid_host_intr_connected(struct bt_l2cap_chan *chan);
static void hid_host_intr_disconnected(struct bt_l2cap_chan *chan);
static int hid_host_ctrl_accept(struct bt_conn *conn, struct bt_l2cap_server *server,
				struct bt_l2cap_chan **chan);
static int hid_host_intr_accept(struct bt_conn *conn, struct bt_l2cap_server *server,
				struct bt_l2cap_chan **chan);

static struct bt_l2cap_chan_ops ctrl_ops = {
	.connected = hid_host_ctrl_connected,
	.disconnected = hid_host_ctrl_disconnected,
	.recv = hid_host_l2cap_ctrl_recv,
};

static struct bt_l2cap_chan_ops intr_ops = {
	.connected = hid_host_intr_connected,
	.disconnected = hid_host_intr_disconnected,
	.recv = hid_host_l2cap_intr_recv,
};

static struct bt_l2cap_server hid_host_server_ctrl = {
	.psm = BT_L2CAP_PSM_HID_CTL,
	.sec_level = BT_SECURITY_L2,
	.accept = hid_host_ctrl_accept,
};

static struct bt_l2cap_server hid_host_server_intr = {
	.psm = BT_L2CAP_PSM_HID_INT,
	.sec_level = BT_SECURITY_L2,
	.accept = hid_host_intr_accept,
};

static struct bt_hid_host *hid_host_get_connection(struct bt_conn *conn)
{
	size_t index;
	struct bt_hid_host *hid;

	index = (size_t)bt_conn_index(conn);
	__ASSERT(index < ARRAY_SIZE(hid_host_pool), "Conn index out of bounds");

	hid = &hid_host_pool[index];

	if (hid->state == BT_HID_STATE_DISCONNECTED) {
		memset(hid, 0, sizeof(*hid));
	}

	return hid;
}

static struct bt_hid_host *hid_host_find_by_conn(struct bt_conn *conn)
{
	size_t index;
	struct bt_hid_host *hid;

	if (!conn) {
		return NULL;
	}

	index = (size_t)bt_conn_index(conn);
	__ASSERT(index < ARRAY_SIZE(hid_host_pool), "Conn index out of bounds");

	hid = &hid_host_pool[index];

	if (hid->state == BT_HID_STATE_DISCONNECTED) {
		return NULL;
	}

	return hid;
}

static void hid_host_cleanup(struct bt_hid_host *hid)
{
	hid->state = BT_HID_STATE_DISCONNECTED;
	hid->w4_response = HID_HOST_W4_NONE;
	k_work_cancel_delayable(&hid->timeout_work);
}

static int hid_host_send_ctrl(struct bt_hid_host *hid, uint8_t header)
{
	struct net_buf *buf;

	buf = bt_conn_create_pdu(NULL, sizeof(struct bt_l2cap_hdr));
	if (!buf) {
		return -ENOMEM;
	}

	net_buf_add_u8(buf, header);

	return bt_l2cap_chan_send(&hid->ctrl_session.br_chan.chan, buf);
}

static int hid_host_send_ctrl_buf(struct bt_hid_host *hid, uint8_t header,
				  struct net_buf *buf)
{
	net_buf_push_u8(buf, header);

	return bt_l2cap_chan_send(&hid->ctrl_session.br_chan.chan, buf);
}

/* ─── L2CAP Accept (incoming Device reconnection) ─── */

static int hid_host_ctrl_accept(struct bt_conn *conn, struct bt_l2cap_server *server,
				struct bt_l2cap_chan **chan)
{
	struct bt_hid_host *hid;

	LOG_DBG("ctrl accept conn %p", conn);

	hid = hid_host_find_by_conn(conn);
	if (!hid) {
		hid = hid_host_get_connection(conn);
		if (!hid) {
			LOG_ERR("No free HID Host slots");
			return -ENOMEM;
		}

		hid->role = BT_HID_ROLE_ACCEPTOR;
		hid->state = BT_HID_STATE_CTRL_CONNECTED;
	}

	hid->ctrl_session.type = BT_HID_CHANNEL_CTRL;
	hid->ctrl_session.br_chan.chan.ops = &ctrl_ops;
	*chan = &hid->ctrl_session.br_chan.chan;

	return 0;
}

static int hid_host_intr_accept(struct bt_conn *conn, struct bt_l2cap_server *server,
				struct bt_l2cap_chan **chan)
{
	struct bt_hid_host *hid;

	LOG_DBG("intr accept conn %p", conn);

	hid = hid_host_find_by_conn(conn);
	if (!hid) {
		LOG_ERR("No ctrl session for intr accept");
		return -ENOTCONN;
	}

	hid->intr_session.type = BT_HID_CHANNEL_INTR;
	hid->intr_session.br_chan.chan.ops = &intr_ops;
	*chan = &hid->intr_session.br_chan.chan;

	return 0;
}

/* ─── L2CAP Connected/Disconnected Callbacks ─── */

static void hid_host_ctrl_connected(struct bt_l2cap_chan *chan)
{
	struct bt_hid_host_session *session = HID_HOST_BY_CTRL_CHAN(chan);
	struct bt_hid_host *hid = HID_HOST_FROM_CTRL(session);

	LOG_DBG("ctrl connected, state %d", hid->state);

	if (hid->role == BT_HID_ROLE_INITIATOR) {
		hid->state = BT_HID_STATE_INTR_CONNECTING;
		hid->intr_session.type = BT_HID_CHANNEL_INTR;
		hid->intr_session.br_chan.chan.ops = &intr_ops;
		bt_l2cap_chan_connect(chan->conn,
				     &hid->intr_session.br_chan.chan,
				     BT_L2CAP_PSM_HID_INT);
	} else {
		hid->state = BT_HID_STATE_CTRL_CONNECTED;
	}
}

static void hid_host_ctrl_disconnected(struct bt_l2cap_chan *chan)
{
	struct bt_hid_host_session *session = HID_HOST_BY_CTRL_CHAN(chan);
	struct bt_hid_host *hid = HID_HOST_FROM_CTRL(session);

	LOG_DBG("ctrl disconnected");

	hid_host_cleanup(hid);

	if (hid_host_cb && hid_host_cb->disconnected) {
		hid_host_cb->disconnected(hid);
	}
}

static void hid_host_intr_connected(struct bt_l2cap_chan *chan)
{
	struct bt_hid_host_session *session =
		CONTAINER_OF(CONTAINER_OF(chan, struct bt_l2cap_br_chan, chan),
			     struct bt_hid_host_session, br_chan);
	struct bt_hid_host *hid = HID_HOST_FROM_INTR(session);

	LOG_DBG("intr connected");

	hid->state = BT_HID_STATE_CONNECTED;

	if (hid_host_cb && hid_host_cb->connected) {
		hid_host_cb->connected(hid);
	}
}

static void hid_host_intr_disconnected(struct bt_l2cap_chan *chan)
{
	struct bt_hid_host_session *session =
		CONTAINER_OF(CONTAINER_OF(chan, struct bt_l2cap_br_chan, chan),
			     struct bt_hid_host_session, br_chan);
	struct bt_hid_host *hid = HID_HOST_FROM_INTR(session);

	LOG_DBG("intr disconnected, state %d", hid->state);

	if (hid->state == BT_HID_STATE_DISCONNECTING) {
		bt_l2cap_chan_disconnect(&hid->ctrl_session.br_chan.chan);
	}
}

/* ─── Control Channel Receive ─── */

static int hid_host_l2cap_ctrl_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_hid_host_session *session = HID_HOST_BY_CTRL_CHAN(chan);
	struct bt_hid_host *hid = HID_HOST_FROM_CTRL(session);
	struct bt_hid_hdr *hdr;
	uint8_t type;
	uint8_t param;

	if (buf->len < sizeof(*hdr)) {
		LOG_ERR("ctrl recv too short (%u)", buf->len);
		return -EINVAL;
	}

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	type = BT_HID_GET_TRANS_FROM_HDR(hdr->header);
	param = BT_HID_GET_PARAM_FROM_HDR(hdr->header);

	LOG_DBG("ctrl recv type 0x%x param 0x%x len %u", type, param, buf->len);

	switch (type) {
	case BT_HID_MSG_TYPE_HANDSHAKE:
		hid->w4_response = HID_HOST_W4_NONE;
		if (hid_host_cb && hid_host_cb->handshake) {
			hid_host_cb->handshake(hid, param);
		}
		break;

	case BT_HID_MSG_TYPE_DATA: {
		uint8_t report_type = param & BT_HID_PARAM_REPORT_TYPE_MASK;

		if (hid->w4_response == HID_HOST_W4_GET_PROTOCOL) {
			hid->w4_response = HID_HOST_W4_NONE;
			if (buf->len >= 1 && hid_host_cb && hid_host_cb->protocol_mode) {
				uint8_t mode = net_buf_pull_u8(buf);

				hid_host_cb->protocol_mode(hid, mode & BT_HID_PROTOCOL_MASK);
			}
		} else if (hid->w4_response == HID_HOST_W4_GET_REPORT) {
			uint8_t report_id = 0;

			hid->w4_response = HID_HOST_W4_NONE;
			if (hid->has_report_id && buf->len >= 1) {
				report_id = net_buf_pull_u8(buf);
			}
			if (hid_host_cb && hid_host_cb->get_report_rsp) {
				hid_host_cb->get_report_rsp(hid, report_type, report_id, buf);
			}
		}
		break;
	}

	case BT_HID_MSG_TYPE_CONTROL:
		if (param == BT_HID_PAR_CONTROL_VIRTUAL_CABLE_UNPLUG) {
			LOG_DBG("received VCU from device");
			if (hid_host_cb && hid_host_cb->vc_unplug) {
				hid_host_cb->vc_unplug(hid);
			}
			hid->suspended = false;
			hid->descriptor_len = 0;
			bt_br_unpair(bt_conn_get_dst_br(chan->conn));
			bt_hid_host_disconnect(hid);
		}
		break;

	default:
		LOG_WRN("ctrl unhandled type 0x%x", type);
		break;
	}

	return 0;
}

/* ─── Interrupt Channel Receive ─── */

static int hid_host_l2cap_intr_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_hid_host_session *session =
		CONTAINER_OF(CONTAINER_OF(chan, struct bt_l2cap_br_chan, chan),
			     struct bt_hid_host_session, br_chan);
	struct bt_hid_host *hid = HID_HOST_FROM_INTR(session);
	struct bt_hid_hdr *hdr;
	uint8_t type;
	uint8_t param;
	uint8_t report_id = 0;

	if (buf->len < sizeof(*hdr)) {
		LOG_ERR("intr recv too short (%u)", buf->len);
		return -EINVAL;
	}

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	type = BT_HID_GET_TRANS_FROM_HDR(hdr->header);
	param = BT_HID_GET_PARAM_FROM_HDR(hdr->header);

	if (type != BT_HID_MSG_TYPE_DATA) {
		LOG_WRN("intr unexpected type 0x%x", type);
		return 0;
	}

	if ((param & BT_HID_PARAM_REPORT_TYPE_MASK) != BT_HID_PAR_REP_TYPE_INPUT) {
		LOG_WRN("intr non-input report type %u", param);
		return 0;
	}

	if ((hid->has_report_id || hid->boot_mode) && buf->len >= 1) {
		report_id = net_buf_pull_u8(buf);
	}

	if (hid_host_cb && hid_host_cb->input_report) {
		hid_host_cb->input_report(hid, report_id, buf);
	}

	return 0;
}

/* ─── SDP Discovery ─── */

static const struct bt_uuid *hid_sdp_uuid = BT_UUID_DECLARE_16(BT_SDP_HID_SVCLASS);
static struct bt_sdp_discover_params hid_host_sdp_params;

static bool hid_host_desc_list_cb(const struct bt_sdp_attr_value_pair *vp, void *user_data)
{
	struct bt_hid_host *hid = user_data;

	if (!vp || !vp->value) {
		return true;
	}

	if (vp->value->type == BT_SDP_ATTR_VALUE_TYPE_TEXT && vp->value->text.len > 0) {
		uint16_t copy_len = MIN(vp->value->text.len,
					CONFIG_BT_HID_HOST_DESCRIPTOR_MAX_LEN);

		memcpy(hid->descriptor, vp->value->text.text, copy_len);
		hid->descriptor_len = copy_len;
		LOG_DBG("HID descriptor parsed (%u bytes)", copy_len);
		return false;
	}

	return true;
}

static void hid_host_parse_sdp_attrs(struct bt_hid_host *hid, struct net_buf *buf)
{
	struct bt_sdp_attribute attr;
	struct bt_sdp_attr_value value;

	/* HIDDeviceSubclass (0x0202) - UINT8 */
	if (bt_sdp_get_attr(buf, BT_SDP_ATTR_HID_DEVICE_SUBCLASS, &attr) == 0 &&
	    bt_sdp_attr_read(&attr, NULL, &value) == 0 &&
	    value.type == BT_SDP_ATTR_VALUE_TYPE_UINT) {
		hid->subclass = (uint8_t)value.uint.u8;
	}

	/* HIDVirtualCable (0x0204) - BOOL */
	if (bt_sdp_get_attr(buf, BT_SDP_ATTR_HID_VIRTUAL_CABLE, &attr) == 0 &&
	    bt_sdp_attr_read(&attr, NULL, &value) == 0 &&
	    value.type == BT_SDP_ATTR_VALUE_TYPE_BOOL) {
		hid->virtual_cable = value.value;
	}

	/* HIDReconnectInitiate (0x0205) - BOOL */
	if (bt_sdp_get_attr(buf, BT_SDP_ATTR_HID_RECONNECT_INITIATE, &attr) == 0 &&
	    bt_sdp_attr_read(&attr, NULL, &value) == 0 &&
	    value.type == BT_SDP_ATTR_VALUE_TYPE_BOOL) {
		hid->reconnect_initiate = value.value;
	}

	/* HIDBootDevice (0x020E) - BOOL */
	if (bt_sdp_get_attr(buf, BT_SDP_ATTR_HID_BOOT_DEVICE, &attr) == 0 &&
	    bt_sdp_attr_read(&attr, NULL, &value) == 0 &&
	    value.type == BT_SDP_ATTR_VALUE_TYPE_BOOL) {
		hid->boot_device = value.value;
	}

	/* HIDSupervisionTimeout (0x020D) - UINT16 */
	if (bt_sdp_get_attr(buf, BT_SDP_ATTR_HID_SUPERVISION_TIMEOUT, &attr) == 0 &&
	    bt_sdp_attr_read(&attr, NULL, &value) == 0 &&
	    value.type == BT_SDP_ATTR_VALUE_TYPE_UINT) {
		hid->supervision_timeout = value.uint.u16;
	}

	/* HIDSSRHostMaxLatency (0x020F) - UINT16 */
	if (bt_sdp_get_attr(buf, BT_SDP_ATTR_HID_SSR_HOST_MAX_LATENCY, &attr) == 0 &&
	    bt_sdp_attr_read(&attr, NULL, &value) == 0 &&
	    value.type == BT_SDP_ATTR_VALUE_TYPE_UINT) {
		hid->ssr_max_latency = value.uint.u16;
	}

	/* HIDSSRHostMinTimeout (0x0210) - UINT16 */
	if (bt_sdp_get_attr(buf, BT_SDP_ATTR_HID_SSR_HOST_MIN_TIMEOUT, &attr) == 0 &&
	    bt_sdp_attr_read(&attr, NULL, &value) == 0 &&
	    value.type == BT_SDP_ATTR_VALUE_TYPE_UINT) {
		hid->ssr_min_timeout = value.uint.u16;
	}

	/* HIDDescriptorList (0x0206) - nested SEQ { SEQ { UINT8(0x22), TEXT_STR } } */
	if (bt_sdp_get_attr(buf, BT_SDP_ATTR_HID_DESCRIPTOR_LIST, &attr) == 0) {
		bt_sdp_attr_value_parse(&attr, hid_host_desc_list_cb, hid);
	}
}

static uint8_t hid_host_sdp_cb(struct bt_conn *conn,
				struct bt_sdp_client_result *result,
				const struct bt_sdp_discover_params *params)
{
	struct bt_hid_host *hid = hid_host_find_by_conn(conn);
	int ret;

	if (!hid) {
		return BT_SDP_DISCOVER_UUID_STOP;
	}

	if (result && result->resp_buf && result->resp_buf->len > 0) {
		LOG_DBG("SDP response received (len %u)", result->resp_buf->len);
		hid_host_parse_sdp_attrs(hid, result->resp_buf);
	}

	LOG_DBG("SDP done, connecting L2CAP ctrl");

	hid->state = BT_HID_STATE_CTRL_CONNECTING;
	hid->ctrl_session.type = BT_HID_CHANNEL_CTRL;
	hid->ctrl_session.br_chan.chan.ops = &ctrl_ops;

	ret = bt_l2cap_chan_connect(conn, &hid->ctrl_session.br_chan.chan,
				   BT_L2CAP_PSM_HID_CTL);
	if (ret < 0) {
		LOG_ERR("L2CAP ctrl connect failed (%d)", ret);
		hid_host_cleanup(hid);
	}

	return BT_SDP_DISCOVER_UUID_STOP;
}

/* ─── Public API ─── */

int bt_hid_host_register(const struct bt_hid_host_cb *cb)
{
	int err;

	if (hid_host_registered) {
		return -EALREADY;
	}

	if (!cb) {
		return -EINVAL;
	}

	err = bt_l2cap_br_server_register(&hid_host_server_ctrl);
	if (err && err != -EEXIST) {
		LOG_ERR("ctrl server register failed (%d)", err);
		return err;
	}

	err = bt_l2cap_br_server_register(&hid_host_server_intr);
	if (err && err != -EEXIST) {
		LOG_ERR("intr server register failed (%d)", err);
		return err;
	}

	hid_host_cb = cb;
	hid_host_registered = true;

	LOG_DBG("HID Host registered");
	return 0;
}

int bt_hid_host_unregister(void)
{
	if (!hid_host_registered) {
		return -EALREADY;
	}

	for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		if (hid_host_pool[i].state != BT_HID_STATE_DISCONNECTED) {
			bt_hid_host_disconnect(&hid_host_pool[i]);
		}
	}

	hid_host_cb = NULL;
	hid_host_registered = false;

	return 0;
}

int bt_hid_host_connect(struct bt_conn *conn, struct bt_hid_host **hid)
{
	struct bt_hid_host *h;
	int err;

	LOG_DBG("conn %p", conn);

	if (!hid_host_registered) {
		return -ENOTSUP;
	}

	if (!conn || !hid) {
		return -EINVAL;
	}

	h = hid_host_find_by_conn(conn);
	if (h) {
		return -EALREADY;
	}

	h = hid_host_get_connection(conn);
	if (!h) {
		return -ENOMEM;
	}

	h->role = BT_HID_ROLE_INITIATOR;
	h->state = BT_HID_STATE_CTRL_CONNECTING;

	/* If descriptor already cached (reconnection), skip SDP */
	if (h->descriptor_len > 0) {
		h->ctrl_session.type = BT_HID_CHANNEL_CTRL;
		h->ctrl_session.br_chan.chan.ops = &ctrl_ops;
		err = bt_l2cap_chan_connect(conn, &h->ctrl_session.br_chan.chan,
					   BT_L2CAP_PSM_HID_CTL);
		if (err < 0) {
			hid_host_cleanup(h);
			return err;
		}
	} else {
		/* SDP discovery first */
		hid_host_sdp_params.type = BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR;
		hid_host_sdp_params.uuid = hid_sdp_uuid;
		hid_host_sdp_params.func = hid_host_sdp_cb;
		hid_host_sdp_params.pool = &hid_host_sdp_pool;

		/* Temporarily store conn in ctrl_session for sdp_cb lookup */
		h->ctrl_session.br_chan.chan.conn = conn;

		err = bt_sdp_discover(conn, &hid_host_sdp_params);
		if (err < 0) {
			LOG_ERR("SDP discover failed (%d)", err);
			hid_host_cleanup(h);
			return err;
		}
	}

	*hid = h;
	return 0;
}

int bt_hid_host_disconnect(struct bt_hid_host *hid)
{
	LOG_DBG("hid %p", hid);

	if (!hid || hid->state == BT_HID_STATE_DISCONNECTED) {
		return -ENOTCONN;
	}

	hid->state = BT_HID_STATE_DISCONNECTING;

	if (hid->intr_session.br_chan.chan.conn) {
		bt_l2cap_chan_disconnect(&hid->intr_session.br_chan.chan);
	} else if (hid->ctrl_session.br_chan.chan.conn) {
		bt_l2cap_chan_disconnect(&hid->ctrl_session.br_chan.chan);
	} else {
		hid_host_cleanup(hid);
	}

	return 0;
}

int bt_hid_host_get_report(struct bt_hid_host *hid, uint8_t type,
			   uint8_t report_id, uint16_t buffer_size)
{
	struct net_buf *buf;
	uint8_t param;

	LOG_DBG("hid %p type %u id %u size %u", hid, type, report_id, buffer_size);

	if (!hid || hid->state != BT_HID_STATE_CONNECTED) {
		return -ENOTCONN;
	}

	if (hid->w4_response != HID_HOST_W4_NONE) {
		return -EBUSY;
	}

	param = type & BT_HID_PARAM_REPORT_TYPE_MASK;
	if (buffer_size > 0) {
		param |= BT_HID_PARAM_REPORT_SIZE_MASK;
	}

	buf = bt_conn_create_pdu(NULL, sizeof(struct bt_l2cap_hdr));
	if (!buf) {
		return -ENOMEM;
	}

	net_buf_add_u8(buf, BT_HID_BUILD_HDR(BT_HID_MSG_TYPE_GET_REPORT, param));

	if (hid->has_report_id || hid->boot_mode) {
		net_buf_add_u8(buf, report_id);
	}

	if (buffer_size > 0) {
		net_buf_add_le16(buf, buffer_size);
	}

	hid->w4_response = HID_HOST_W4_GET_REPORT;

	return bt_l2cap_chan_send(&hid->ctrl_session.br_chan.chan, buf);
}

int bt_hid_host_set_report(struct bt_hid_host *hid, uint8_t type,
			   struct net_buf *buf)
{
	uint8_t header;

	LOG_DBG("hid %p type %u len %u", hid, type, buf ? buf->len : 0);

	if (!hid || hid->state != BT_HID_STATE_CONNECTED) {
		return -ENOTCONN;
	}

	if (hid->w4_response != HID_HOST_W4_NONE) {
		return -EBUSY;
	}

	if (!buf) {
		return -EINVAL;
	}

	header = BT_HID_BUILD_HDR(BT_HID_MSG_TYPE_SET_REPORT,
				  type & BT_HID_PARAM_REPORT_TYPE_MASK);

	hid->w4_response = HID_HOST_W4_SET_REPORT;

	return hid_host_send_ctrl_buf(hid, header, buf);
}

int bt_hid_host_get_protocol(struct bt_hid_host *hid)
{
	LOG_DBG("hid %p", hid);

	if (!hid || hid->state != BT_HID_STATE_CONNECTED) {
		return -ENOTCONN;
	}

	if (hid->w4_response != HID_HOST_W4_NONE) {
		return -EBUSY;
	}

	hid->w4_response = HID_HOST_W4_GET_PROTOCOL;

	return hid_host_send_ctrl(hid,
		BT_HID_BUILD_HDR(BT_HID_MSG_TYPE_GET_PROTOCOL, 0));
}

int bt_hid_host_set_protocol(struct bt_hid_host *hid, uint8_t protocol)
{
	LOG_DBG("hid %p protocol %u", hid, protocol);

	if (!hid || hid->state != BT_HID_STATE_CONNECTED) {
		return -ENOTCONN;
	}

	if (hid->w4_response != HID_HOST_W4_NONE) {
		return -EBUSY;
	}

	hid->w4_response = HID_HOST_W4_SET_PROTOCOL;

	return hid_host_send_ctrl(hid,
		BT_HID_BUILD_HDR(BT_HID_MSG_TYPE_SET_PROTOCOL,
				 protocol & BT_HID_PROTOCOL_MASK));
}

int bt_hid_host_send_output_report(struct bt_hid_host *hid, struct net_buf *buf)
{
	uint8_t header;

	LOG_DBG("hid %p len %u", hid, buf ? buf->len : 0);

	if (!hid || hid->state != BT_HID_STATE_CONNECTED) {
		return -ENOTCONN;
	}

	if (!buf) {
		return -EINVAL;
	}

	header = BT_HID_BUILD_HDR(BT_HID_MSG_TYPE_DATA, BT_HID_PAR_REP_TYPE_OUTPUT);
	net_buf_push_u8(buf, header);

	return bt_l2cap_chan_send(&hid->intr_session.br_chan.chan, buf);
}

int bt_hid_host_suspend(struct bt_hid_host *hid)
{
	LOG_DBG("hid %p", hid);

	if (!hid || hid->state != BT_HID_STATE_CONNECTED) {
		return -ENOTCONN;
	}

	hid->suspended = true;

	return hid_host_send_ctrl(hid,
		BT_HID_BUILD_HDR(BT_HID_MSG_TYPE_CONTROL,
				 BT_HID_PAR_CONTROL_SUSPEND));
}

int bt_hid_host_exit_suspend(struct bt_hid_host *hid)
{
	LOG_DBG("hid %p", hid);

	if (!hid || hid->state != BT_HID_STATE_CONNECTED) {
		return -ENOTCONN;
	}

	hid->suspended = false;

	return hid_host_send_ctrl(hid,
		BT_HID_BUILD_HDR(BT_HID_MSG_TYPE_CONTROL,
				 BT_HID_PAR_CONTROL_EXIT_SUSPEND));
}

int bt_hid_host_virtual_cable_unplug(struct bt_hid_host *hid)
{
	int err;

	LOG_DBG("hid %p", hid);

	if (!hid || hid->state != BT_HID_STATE_CONNECTED) {
		return -ENOTCONN;
	}

	err = hid_host_send_ctrl(hid,
		BT_HID_BUILD_HDR(BT_HID_MSG_TYPE_CONTROL,
				 BT_HID_PAR_CONTROL_VIRTUAL_CABLE_UNPLUG));
	if (err) {
		return err;
	}

	hid->suspended = false;
	hid->descriptor_len = 0;
	bt_br_unpair(bt_conn_get_dst_br(hid->ctrl_session.br_chan.chan.conn));

	return bt_hid_host_disconnect(hid);
}

struct net_buf *bt_hid_host_create_pdu(struct net_buf_pool *pool)
{
	return bt_conn_create_pdu(pool, sizeof(struct bt_l2cap_hdr) + BT_HID_HDR_SIZE);
}
