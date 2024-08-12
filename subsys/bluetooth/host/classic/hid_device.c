/*
 * Copyright 2024 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/classic/hid_device.h>
#include <zephyr/bluetooth/l2cap.h>

#include "host/hci_core.h"
#include "host/conn_internal.h"
#include "host/l2cap_internal.h"

#include "hid_internal.h"

#define HID_PAR_REPORT_TYPE_MASK 0x03

#define LOG_LEVEL CONFIG_BT_HID_DEVICE_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_hid_device);

static struct bt_hid_device_cb *hid_device_cb;
static struct bt_hid_device hid_devices[CONFIG_BT_MAX_CONN];

static struct bt_hid_device *hid_find_by_conn(struct bt_conn *conn)
{
	struct bt_hid_device *hid;

	if (!conn) {
		return NULL;
	}

	hid = &hid_devices[bt_conn_index(conn)];
	if (hid->conn != conn) {
		LOG_ERR("hid conn:%p not match conn:%p", hid->conn, conn);
		return NULL;
	}

	return hid;
}

static struct net_buf *hid_create_pdu(uint8_t type, uint8_t param)
{
	struct net_buf *buf;
	struct bt_hid_hdr *hdr;

	buf = bt_l2cap_create_pdu(NULL, sizeof(*hdr));
	if (!buf) {
		LOG_ERR("can't create buf buf for type:%d, param:%d", type, param);
		return NULL;
	}

	LOG_DBG("hdr type = 0x%02X, param = 0x%02X", type, param);

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->type = type;
	hdr->param = param;

	return buf;
}

static int hid_send(struct bt_hid_session *session, struct net_buf *buf)
{
	int ret;

	ret = bt_l2cap_chan_send(&session->br_chan.chan, buf);
	if (ret < 0) {
		LOG_ERR("l2cap send fail - ret = %d", ret);
		net_buf_unref(buf);
		return ret;
	}

	return ret;
}

static int hid_send_data(struct bt_hid_session *session, uint8_t type, uint8_t *data, uint16_t len)
{
	struct net_buf *buf;

	buf = hid_create_pdu(BT_HID_TYPE_DATA, type);
	if (!buf) {
		return -ENOMEM;
	}

	if (len > 0) {
		net_buf_add_mem(buf, data, len);
	}

	return hid_send(session, buf);
}

static int hid_send_handshake(struct bt_hid_session *session, uint8_t response)
{
	struct net_buf *buf;

	buf = hid_create_pdu(BT_HID_TYPE_HANDSHAKE, response);
	if (!buf) {
		return -ENOMEM;
	}

	return hid_send(session, buf);
}

static int hid_control_handle(struct bt_hid_session *session, struct net_buf *buf, uint8_t control)
{
	struct bt_hid_device *hid;
	int ret = 0;

	hid = hid_find_by_conn(session->br_chan.chan.conn);
	if (!hid) {
		LOG_ERR("hid not found");
		return -EIO;
	}

	LOG_DBG("hid recv control:%d", control);

	switch (control) {
	case BT_HID_CONTROL_VIRTUAL_CABLE_UNPLUG:
		bt_hid_devie_disconnect(hid);
		hid->unplug = 1;
		break;
	case BT_HID_CONTROL_SUSPEND:
		break;
	case BT_HID_CONTROL_EXIT_SUSPEND:
		break;
	default:
		LOG_ERR("hid control:%d not handle", control);
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int hid_get_report_handle(struct bt_hid_session *session, struct net_buf *buf, uint8_t param)
{
	struct bt_hid_device *hid;
	struct bt_hid_report report = {0};

	hid = hid_find_by_conn(session->br_chan.chan.conn);
	if (!hid) {
		LOG_ERR("hid not found");
		return -EIO;
	}

	LOG_DBG("hid recv get report");

	memcpy(report.data, buf->data, buf->len);
	report.type = param & HID_PAR_REPORT_TYPE_MASK;
	report.len = buf->len;

	if (!hid_device_cb || !hid_device_cb->get_report) {
		return -ESRCH;
	}

	hid_device_cb->get_report(hid, (uint8_t *)&report, sizeof(report));
	return 0;
}

static int hid_set_report_handle(struct bt_hid_session *session, struct net_buf *buf, uint8_t param)
{
	struct bt_hid_device *hid;
	struct bt_hid_report report = {0};

	hid = hid_find_by_conn(session->br_chan.chan.conn);
	if (!hid) {
		LOG_ERR("hid not found");
		return -EIO;
	}

	LOG_DBG("hid recv get report");

	memcpy(report.data, buf->data, buf->len);
	report.type = param & HID_PAR_REPORT_TYPE_MASK;
	report.len = buf->len;

	if (!hid_device_cb || !hid_device_cb->set_report) {
		return -ESRCH;
	}

	hid_device_cb->set_report(hid, (uint8_t *)&report, sizeof(report));
	return 0;
}

static int hid_get_protocol_handle(struct bt_hid_session *session, struct net_buf *buf,
				   uint8_t param)
{
	struct bt_hid_device *hid;

	hid = hid_find_by_conn(session->br_chan.chan.conn);
	if (!hid) {
		LOG_ERR("hid not found");
		return -EIO;
	}

	if (!hid_device_cb || !hid_device_cb->get_protocol) {
		return -ESRCH;
	}

	hid_device_cb->get_protocol(hid);
	return 0;
}

static int hid_set_protocol_handle(struct bt_hid_session *session, struct net_buf *buf,
				   uint8_t param)
{
	struct bt_hid_device *hid;
	uint8_t protocol;

	hid = hid_find_by_conn(session->br_chan.chan.conn);
	if (!hid) {
		LOG_ERR("hid not found");
		return -EIO;
	}

	protocol = param & BT_HID_PROTOCOL_MASK;

	if (!hid_device_cb || !hid_device_cb->set_protocol) {
		return -ESRCH;
	}

	hid_device_cb->set_protocol(hid, protocol);
	hid_send_handshake(session, BT_HID_HANDSHAKE_RSP_SUCCESS);
	return 0;
}

static int hid_intr_handle(struct bt_hid_session *session, struct net_buf *buf, uint8_t param)
{
	struct bt_hid_device *hid;
	struct bt_hid_report report = {0};

	hid = hid_find_by_conn(session->br_chan.chan.conn);
	if (!hid) {
		LOG_ERR("hid not found");
		return -EIO;
	}

	report.type = param & HID_PAR_REPORT_TYPE_MASK;
	report.len = buf->len;
	memcpy(report.data, buf->data, buf->len);

	if (session->role != BT_HID_SESSION_ROLE_INTR) {
		LOG_ERR("hid not intr data");
		return -EIO;
	}

	if (!hid_device_cb || !hid_device_cb->intr_data) {
		return -ESRCH;
	}

	hid_device_cb->intr_data(hid, (uint8_t *)&report, sizeof(report));
	return 0;
}

struct hid_msg_handler {
	uint8_t type;
	int (*func)(struct bt_hid_session *session, struct net_buf *buf, uint8_t param);
};

static const struct hid_msg_handler handler[] = {
	{BT_HID_TYPE_CONTROL, hid_control_handle},
	{BT_HID_TYPE_GET_REPORT, hid_get_report_handle},
	{BT_HID_TYPE_SET_REPORT, hid_set_report_handle},
	{BT_HID_TYPE_GET_PROTOCOL, hid_get_protocol_handle},
	{BT_HID_TYPE_SET_PROTOCOL, hid_set_protocol_handle},
	{BT_HID_TYPE_DATA, hid_intr_handle},
};

static void bt_hid_l2cap_connected(struct bt_l2cap_chan *chan)
{
	struct bt_hid_device *hid;
	struct bt_hid_session *session;

	if (!chan) {
		LOG_ERR("Invalid hid chan");
		return;
	}

	hid = hid_find_by_conn(chan->conn);
	if (!hid) {
		LOG_ERR("can't find hid");
		return;
	}

	session = HID_SESSION_BY_CHAN(chan);
	LOG_DBG("chan %p session role %d", chan, session->role);

	if (hid->role == BT_HID_ROLE_INITIATOR) {
		if (session->role == BT_HID_SESSION_ROLE_CTRL) {
			bt_l2cap_chan_connect(chan->conn, &hid->intr_session.br_chan.chan,
					      BT_L2CAP_PSM_HID_INT);
			hid->state = BT_HID_STATE_CTRL_CONNECTED;
		} else {
			hid->state = BT_HID_STATE_CONNECTED;
			if (hid_device_cb && hid_device_cb->connected) {
				hid_device_cb->connected(hid);
			}
		}
	} else {
		if (session->role == BT_HID_SESSION_ROLE_CTRL) {
			hid->state = BT_HID_STATE_CTRL_CONNECTED;
		} else {
			hid->state = BT_HID_STATE_CONNECTED;
			if (hid_device_cb && hid_device_cb->connected) {
				hid_device_cb->connected(hid);
			}
		}
	}

	LOG_DBG("hid connected:%d state %d", session->role, hid->state);
}

static void bt_hid_l2cap_disconnected(struct bt_l2cap_chan *chan)
{
	struct bt_hid_device *hid;
	struct bt_hid_session *session;

	if (!chan) {
		LOG_ERR("Invalid hid chan");
		return;
	}

	hid = hid_find_by_conn(chan->conn);
	if (!hid) {
		LOG_ERR("can't find hid");
		return;
	}

	session = HID_SESSION_BY_CHAN(chan);
	LOG_DBG("hid disconnected:%d state %d", session->role, hid->state);

	if (session->role == BT_HID_SESSION_ROLE_INTR && hid->state == BT_HID_STATE_DISCONNECT &&
	    hid->ctrl_session.br_chan.chan.conn != NULL) {
		bt_l2cap_chan_disconnect(&hid->ctrl_session.br_chan.chan);
	}

	session->br_chan.chan.conn = NULL;
	if (hid->intr_session.br_chan.chan.conn || hid->ctrl_session.br_chan.chan.conn) {
		return;
	}

	if (hid->unplug && hid_device_cb && hid_device_cb->vc_unplug) {
		hid_device_cb->vc_unplug(hid);
		hid->unplug = 0;
	}

	if (hid_device_cb && hid_device_cb->disconnected) {
		hid_device_cb->disconnected(hid);
	}

	if (hid->buf != NULL) {
		net_buf_unref(hid->buf);
		hid->buf = NULL;
	}

	hid->state = BT_HID_STATE_IDLE;
	hid->conn = NULL;
}

static int bt_hid_l2cap_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_hid_hdr *hdr;
	struct bt_hid_session *session;
	int i;
	int ret = -ESRCH;
	int response;

	if (!chan) {
		LOG_ERR("Invalid hid chan");
		return -EIO;
	}

	hdr = (void *)buf->data;
	net_buf_pull(buf, sizeof(*hdr));
	LOG_DBG("hid rcv type[0x%x] param[0x%x]", hdr->type, hdr->param);

	session = HID_SESSION_BY_CHAN(chan);

	for (i = 0; i < ARRAY_SIZE(handler); i++) {
		if (hdr->type == handler[i].type) {
			ret = handler[i].func(session, buf, hdr->param);
			break;
		}
	}

	if (ret == 0) {
		return ret;
	}

	switch (ret) {
	case -EINVAL:
		response = BT_HID_HANDSHAKE_RSP_ERR_INVALID_PARAM;
		break;
	case -ESRCH:
		response = BT_HID_HANDSHAKE_RSP_ERR_UNSUPPORTED_REQ;
	default:
		response = BT_HID_HANDSHAKE_RSP_ERR_UNKNOWN;
		break;
	}

	LOG_ERR("response error:%d", response);
	hid_send_handshake(session, response);

	return 0;
}

static void bt_hid_init(struct bt_hid_device *hid, struct bt_conn *conn, bt_hid_role_t role)
{
	static const struct bt_l2cap_chan_ops ops = {
		.connected = bt_hid_l2cap_connected,
		.disconnected = bt_hid_l2cap_disconnected,
		.recv = bt_hid_l2cap_recv,
	};

	LOG_DBG("hid conn %p initialized", hid);

	hid->ctrl_session.br_chan.chan.ops = (struct bt_l2cap_chan_ops *)&ops;
	hid->ctrl_session.br_chan.rx.mtu = BT_HID_MAX_MTU;
	hid->intr_session.br_chan.chan.ops = (struct bt_l2cap_chan_ops *)&ops;
	hid->intr_session.br_chan.rx.mtu = BT_HID_MAX_MTU;
	hid->role = role;
	hid->ctrl_session.role = BT_HID_SESSION_ROLE_CTRL;
	hid->intr_session.role = BT_HID_SESSION_ROLE_INTR;
	hid->state = BT_HID_STATE_CTRL_CONNECTING;
	hid->conn = conn;
	hid->buf = NULL;
	hid->unplug = 0;
}

static int bt_hid_l2cap_ctrl_accept(struct bt_conn *conn, struct bt_l2cap_server *server,
				    struct bt_l2cap_chan **chan)
{
	struct bt_hid_device *hid;

	hid = &hid_devices[bt_conn_index(conn)];

	bt_hid_init(hid, conn, BT_HID_ROLE_ACCEPTOR);
	*chan = &hid->ctrl_session.br_chan.chan;

	return 0;
}

static int bt_hid_l2cap_intr_accept(struct bt_conn *conn, struct bt_l2cap_server *server,
				    struct bt_l2cap_chan **chan)
{
	struct bt_hid_device *hid;

	hid = hid_find_by_conn(conn);
	if (!hid) {
		LOG_ERR("can't find hid");
		return -EINVAL;
	}

	hid->state = BT_HID_STATE_INTR_CONNECTING;
	*chan = &hid->intr_session.br_chan.chan;
	return 0;
}

int bt_hid_device_send_ctrl_data(struct bt_hid_device *hid, uint8_t type, uint8_t *data,
				 uint16_t len)
{
	if (!hid) {
		LOG_ERR("no hid connected");
		return -EINVAL;
	}

	return hid_send_data(&hid->ctrl_session, type, data, len);
}

int bt_hid_device_send_intr_data(struct bt_hid_device *hid, uint8_t type, uint8_t *data,
				 uint16_t len)
{
	if (!hid) {
		LOG_ERR("no hid connected");
		return -EINVAL;
	}

	return hid_send_data(&hid->intr_session, type, data, len);
}

int bt_hid_device_report_error(struct bt_hid_device *hid, uint8_t error)
{
	if (!hid) {
		LOG_ERR("no hid connected");
		return -EINVAL;
	}

	return hid_send_handshake(&hid->ctrl_session, error);
}

struct bt_hid_device *bt_hid_device_connect(struct bt_conn *conn)
{
	struct bt_hid_device *hid;
	int ret;

	hid = &hid_devices[bt_conn_index(conn)];

	if (hid->state != BT_HID_STATE_IDLE) {
		LOG_WRN("hid connection state:%d", hid->state);
		return hid;
	}

	bt_hid_init(hid, conn, BT_HID_ROLE_INITIATOR);

	ret = bt_l2cap_chan_connect(conn, &hid->ctrl_session.br_chan.chan, BT_L2CAP_PSM_HID_CTL);
	if (ret != 0) {
		return NULL;
	}

	hid->state = BT_HID_STATE_CTRL_CONNECTING;
	return hid;
}

int bt_hid_devie_disconnect(struct bt_hid_device *hid)
{
	int ret;

	if (!hid) {
		LOG_ERR("no hid connected");
		return -EINVAL;
	}

	ret = bt_l2cap_chan_disconnect(&hid->intr_session.br_chan.chan);
	if (ret != 0) {
		LOG_WRN("hid disconnect session, err:%d", ret);
		return ret;
	}

	hid->state = BT_HID_STATE_DISCONNECT;
	return 0;
}

int bt_hid_device_register(struct bt_hid_device_cb *cb)
{
	LOG_DBG("");

	hid_device_cb = cb;
	return 0;
}

int bt_hid_dev_init(void)
{
	int ret;
	static struct bt_l2cap_server hiddev_ctrl_l2cap = {
		.psm = BT_L2CAP_PSM_HID_CTL,
		.sec_level = BT_SECURITY_L2,
		.accept = bt_hid_l2cap_ctrl_accept,
	};

	static struct bt_l2cap_server hiddev_intr_l2cap = {
		.psm = BT_L2CAP_PSM_HID_INT,
		.sec_level = BT_SECURITY_L2,
		.accept = bt_hid_l2cap_intr_accept,
	};

	/* Register hid ctrl PSM with l2cap */
	ret = bt_l2cap_br_server_register(&hiddev_ctrl_l2cap);
	if (ret < 0) {
		LOG_ERR("hid ctrl l2cap registration failed, err:%d", ret);
		return ret;
	}

	/* Register hid intr PSM with l2cap */
	ret = bt_l2cap_br_server_register(&hiddev_intr_l2cap);
	if (ret < 0) {
		LOG_ERR("hid intr l2cap registration failed, err:%d", ret);
		return ret;
	}

	return ret;
}
