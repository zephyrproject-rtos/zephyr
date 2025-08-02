/*
 * Copyright 2025 Xiaomi Corporation
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

#include "common/assert.h"

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/classic/hid_device.h>
#include <zephyr/bluetooth/l2cap.h>

#include "host/hci_core.h"
#include "host/conn_internal.h"
#include "host/l2cap_internal.h"

#include "hid_internal.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_hid_device);

#define HID_PAR_REPORT_TYPE_MASK 0x03

/* HID device callback */
static struct bt_hid_device_cb *hid_cb;

/* HID device connections */
static struct bt_hid_device hid_devs[CONFIG_BT_MAX_CONN];

static struct bt_hid_device *hid_get_connection(struct bt_conn *conn)
{
	struct bt_hid_device *hid = &hid_devs[bt_conn_index(conn)];

	if (hid->conn == NULL) {
		/* Clean the memory area before returning */
		(void)memset(hid, 0, sizeof(*hid));
	}

	return hid;
}

static struct net_buf *hid_create_pdu(uint8_t type, uint8_t param)
{
	struct net_buf *buf;
	struct bt_hid_header *hdr;

	buf = bt_l2cap_create_pdu(NULL, sizeof(*hdr));
	if (!buf) {
		LOG_ERR("Can't create buf buf for type:%d, param:%d", type, param);
		return NULL;
	}

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->type = type;
	hdr->param = param;

	return buf;
}

static int hid_send_pdu(struct bt_hid_session *session, struct net_buf *buf)
{
	int err;

	err = bt_l2cap_chan_send(&session->br_chan.chan, buf);
	if (err < 0) {
		LOG_ERR("L2CAP send fail, err:%d", err);
		net_buf_unref(buf);
		return err;
	}

	return err;
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

	return hid_send_pdu(session, buf);
}

static int hid_send_handshake(struct bt_hid_session *session, uint8_t response)
{
	struct net_buf *buf;

	buf = hid_create_pdu(BT_HID_TYPE_HANDSHAKE, response);
	if (!buf) {
		return -ENOMEM;
	}

	return hid_send_pdu(session, buf);
}

static int hid_control_handle(struct bt_hid_session *session, struct net_buf *buf, uint8_t control)
{
	struct bt_hid_device *hid;
	int err = 0;

	hid = hid_get_connection(session->br_chan.chan.conn);
	if (!hid) {
		LOG_ERR("HID not found");
		return -EIO;
	}

	switch (control) {
	case BT_HID_CONTROL_VIRTUAL_CABLE_UNPLUG:
		bt_hid_device_disconnect(hid);
		break;
	case BT_HID_CONTROL_SUSPEND:
		break;
	case BT_HID_CONTROL_EXIT_SUSPEND:
		break;
	default:
		LOG_ERR("HID control:%d not handle", control);
		err = -EINVAL;
		break;
	}

	return err;
}

static int hid_get_report_handle(struct bt_hid_session *session, struct net_buf *buf, uint8_t param)
{
	struct bt_hid_device *hid;
	struct bt_hid_report report = {0};

	hid = hid_get_connection(session->br_chan.chan.conn);
	if (!hid) {
		LOG_ERR("HID not found");
		return -EIO;
	}

	memcpy(report.data, buf->data, buf->len);
	report.type = param & HID_PAR_REPORT_TYPE_MASK;
	report.len = buf->len;

	if (hid_cb == NULL || hid_cb->get_report == NULL) {
		LOG_ERR("HID get report callback not found");
		return -ESRCH;
	}

	hid_cb->get_report(hid, (uint8_t *)&report, sizeof(report));
	return 0;
}

static int hid_set_report_handle(struct bt_hid_session *session, struct net_buf *buf, uint8_t param)
{
	struct bt_hid_device *hid;
	struct bt_hid_report report = {0};

	hid = hid_get_connection(session->br_chan.chan.conn);
	if (!hid) {
		LOG_ERR("HID not found");
		return -EIO;
	}

	memcpy(report.data, buf->data, buf->len);
	report.type = param & HID_PAR_REPORT_TYPE_MASK;
	report.len = buf->len;

	if (hid_cb == NULL || hid_cb->set_report == NULL) {
		LOG_ERR("HID set report callback not found");
		return -ESRCH;
	}

	hid_cb->set_report(hid, (uint8_t *)&report, sizeof(report));
	return 0;
}

static int hid_get_protocol_handle(struct bt_hid_session *session, struct net_buf *buf,
				   uint8_t param)
{
	struct bt_hid_device *hid;

	hid = hid_get_connection(session->br_chan.chan.conn);
	if (!hid) {
		LOG_ERR("HID not found");
		return -EIO;
	}

	if (hid_cb == NULL || hid_cb->get_protocol == NULL) {
		LOG_ERR("HID get protocol callback not found");
		return -ESRCH;
	}

	hid_cb->get_protocol(hid);
	return 0;
}

static int hid_set_protocol_handle(struct bt_hid_session *session, struct net_buf *buf,
				   uint8_t param)
{
	struct bt_hid_device *hid;
	uint8_t protocol;

	hid = hid_get_connection(session->br_chan.chan.conn);
	if (!hid) {
		LOG_ERR("HID not found");
		return -EIO;
	}

	protocol = param & BT_HID_PROTOCOL_MASK;

	if (hid_cb == NULL || hid_cb->set_protocol == NULL) {
		LOG_ERR("HID set protocol callback not found");
		return -ESRCH;
	}

	hid_cb->set_protocol(hid, protocol);
	return hid_send_handshake(session, BT_HID_HANDSHAKE_RSP_SUCCESS);
}

static int hid_intr_handle(struct bt_hid_session *session, struct net_buf *buf, uint8_t param)
{
	struct bt_hid_device *hid;
	struct bt_hid_report report = {0};

	hid = hid_get_connection(session->br_chan.chan.conn);
	if (!hid) {
		LOG_ERR("HID not found");
		return -EIO;
	}

	report.type = param & HID_PAR_REPORT_TYPE_MASK;
	report.len = buf->len;
	memcpy(report.data, buf->data, buf->len);

	if (session->role != BT_HID_SESSION_ROLE_INTR) {
		LOG_ERR("HID invalid role");
		return -EIO;
	}

	if (hid_cb == NULL || hid_cb->intr_data == NULL) {
		LOG_ERR("HID intr data callback not found");
		return -ESRCH;
	}

	hid_cb->intr_data(hid, (uint8_t *)&report, sizeof(report));
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

	hid = hid_get_connection(chan->conn);
	if (!hid) {
		LOG_ERR("can't find hid");
		return;
	}

	session = HID_SESSION_BY_CHAN(chan);

	LOG_DBG("HID session:%d connected, state %d", session->role, hid->state);

	if (session->role == BT_HID_SESSION_ROLE_CTRL) {
		hid->state = BT_HID_STATE_CTRL_CONNECTED;
	} else if (session->role == BT_HID_SESSION_ROLE_INTR) {
		hid->state = BT_HID_STATE_CONNECTED;
	} else {
		LOG_ERR("HID invalid role:%d", session->role);
		return;
	}

	if ((hid->role == BT_HID_ROLE_INITIATOR) && (session->role == BT_HID_SESSION_ROLE_CTRL)) {
		int err;

		err = bt_l2cap_chan_connect(chan->conn, &hid->intr_session.br_chan.chan,
					    BT_L2CAP_PSM_HID_INT);
		if (err) {
			LOG_ERR("HID connect INTR failed");
			hid->state = BT_HID_STATE_DISCONNECTING;
			bt_l2cap_chan_disconnect(&hid->ctrl_session.br_chan.chan);
			return;
		}
	}

	if (hid->state != BT_HID_STATE_CONNECTED) {
		return;
	}

	if (hid_cb && hid_cb->connected) {
		hid_cb->connected(hid);
	}
}

static void bt_hid_l2cap_disconnected(struct bt_l2cap_chan *chan)
{
	struct bt_hid_device *hid;
	struct bt_hid_session *session;

	if (!chan) {
		LOG_ERR("Invalid hid chan");
		return;
	}

	hid = hid_get_connection(chan->conn);
	if (!hid) {
		LOG_ERR("Can't get hid");
		return;
	}

	session = HID_SESSION_BY_CHAN(chan);
	LOG_DBG("HID disconnected:%d state %d", session->role, hid->state);

	session->br_chan.chan.conn = NULL;

	/* Local request disconnect(INTR channel), and it need to disconnect CTRL channel as well */
	if ((session->role == BT_HID_SESSION_ROLE_INTR) &&
	    (hid->state == BT_HID_STATE_DISCONNECTING)) {
		bt_l2cap_chan_disconnect(&hid->ctrl_session.br_chan.chan);
		return;
	}

	if ((hid->intr_session.br_chan.chan.conn) || (hid->ctrl_session.br_chan.chan.conn)) {
		return;
	}

	if (hid_cb && hid_cb->vc_unplug) {
		hid_cb->vc_unplug(hid);
	}

	if (hid_cb && hid_cb->disconnected) {
		hid_cb->disconnected(hid);
	}

	if (hid->buf != NULL) {
		net_buf_unref(hid->buf);
		hid->buf = NULL;
	}

	hid->conn = NULL;
	hid->plugged = 0;
	hid->state = BT_HID_STATE_DISTCONNECTED;
}

static int bt_hid_l2cap_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_hid_header *hdr;
	struct bt_hid_session *session;
	int i;
	int err = -ESRCH;
	int response;

	if (!chan) {
		LOG_ERR("Invalid hid chan");
		return -EIO;
	}

	hdr = (struct bt_hid_header *)buf->data;
	net_buf_pull(buf, sizeof(*hdr));
	session = HID_SESSION_BY_CHAN(chan);

	LOG_DBG("HID recv type[0x%x] param[0x%x]", hdr->type, hdr->param);

	for (i = 0; i < ARRAY_SIZE(handler); i++) {
		if (hdr->type == handler[i].type) {
			err = handler[i].func(session, buf, hdr->param);
			break;
		}
	}

	if (err == 0) {
		return err;
	}

	switch (err) {
	case -EINVAL:
		response = BT_HID_HANDSHAKE_RSP_ERR_INVALID_PARAM;
		break;
	case -ESRCH:
		response = BT_HID_HANDSHAKE_RSP_ERR_UNSUPPORTED_REQ;
		break;
	default:
		response = BT_HID_HANDSHAKE_RSP_ERR_UNKNOWN;
		break;
	}

	LOG_ERR("HID handle err %d", err);

	return hid_send_handshake(session, response);
}

static void bt_hid_init(struct bt_hid_device *hid, struct bt_conn *conn, enum bt_hid_role role)
{
	static const struct bt_l2cap_chan_ops ops = {
		.connected = bt_hid_l2cap_connected,
		.disconnected = bt_hid_l2cap_disconnected,
		.recv = bt_hid_l2cap_recv,
	};

	LOG_ERR("HID conn %p initialized", hid);

	hid->ctrl_session.br_chan.chan.ops = (struct bt_l2cap_chan_ops *)&ops;
	hid->ctrl_session.br_chan.rx.mtu = BT_HID_MAX_MTU;
	hid->ctrl_session.role = BT_HID_SESSION_ROLE_CTRL;

	hid->intr_session.br_chan.chan.ops = (struct bt_l2cap_chan_ops *)&ops;
	hid->intr_session.br_chan.rx.mtu = BT_HID_MAX_MTU;
	hid->intr_session.role = BT_HID_SESSION_ROLE_INTR;

	hid->role = role;
	hid->state = BT_HID_STATE_CTRL_CONNECTING;
	hid->conn = conn;
	hid->buf = NULL;
	hid->plugged = 1;
}

static int hid_l2cap_ctrl_accept(struct bt_conn *conn, struct bt_l2cap_server *server,
				 struct bt_l2cap_chan **chan)
{
	struct bt_hid_device *hid;

	hid = hid_get_connection(conn);
	if (hid == NULL) {
		return -ENOMEM;
	}

	bt_hid_init(hid, conn, BT_HID_ROLE_ACCEPTOR);
	*chan = &hid->ctrl_session.br_chan.chan;

	return 0;
}

static int hid_l2cap_intr_accept(struct bt_conn *conn, struct bt_l2cap_server *server,
				 struct bt_l2cap_chan **chan)
{
	struct bt_hid_device *hid;

	hid = hid_get_connection(conn);
	if (hid == NULL) {
		return -ENOMEM;
	}

	hid->state = BT_HID_STATE_INTR_CONNECTING;
	*chan = &hid->intr_session.br_chan.chan;
	return 0;
}

int bt_hid_device_send_ctrl_data(struct bt_hid_device *hid, uint8_t type, uint8_t *data,
				 uint16_t len)
{
	__ASSERT_NO_MSG(hid);

	return hid_send_data(&hid->ctrl_session, type, data, len);
}

int bt_hid_device_send_intr_data(struct bt_hid_device *hid, uint8_t type, uint8_t *data,
				 uint16_t len)
{
	__ASSERT_NO_MSG(hid);

	return hid_send_data(&hid->intr_session, type, data, len);
}

int bt_hid_device_report_error(struct bt_hid_device *hid, uint8_t error)
{
	__ASSERT_NO_MSG(hid);

	return hid_send_handshake(&hid->ctrl_session, error);
}

struct bt_hid_device *bt_hid_device_connect(struct bt_conn *conn)
{
	struct bt_hid_device *hid;
	int err;

	hid = hid_get_connection(conn);
	if (!hid) {
		LOG_ERR("Cannot allocate memory");
		return NULL;
	}

	bt_hid_init(hid, conn, BT_HID_ROLE_INITIATOR);

	err = bt_l2cap_chan_connect(conn, &hid->ctrl_session.br_chan.chan, BT_L2CAP_PSM_HID_CTL);
	if (err != 0) {
		LOG_WRN("HID connect failed, err:%d", err);
		return NULL;
	}

	hid->state = BT_HID_STATE_CTRL_CONNECTING;
	return hid;
}

int bt_hid_device_disconnect(struct bt_hid_device *hid)
{
	int err;

	__ASSERT_NO_MSG(hid);

	err = bt_l2cap_chan_disconnect(&hid->intr_session.br_chan.chan);
	if (err != 0) {
		LOG_WRN("HID disconnect session, err:%d", err);
		return err;
	}

	hid->state = BT_HID_STATE_DISCONNECTING;
	return 0;
}

int bt_hid_device_register(struct bt_hid_device_cb *cb)
{
	LOG_DBG("");

	hid_cb = cb;
	return 0;
}

int bt_hid_dev_init(void)
{
	int err;
	static struct bt_l2cap_server hiddev_ctrl_l2cap = {
		.psm = BT_L2CAP_PSM_HID_CTL,
		.sec_level = BT_SECURITY_L2,
		.accept = hid_l2cap_ctrl_accept,
	};
	static struct bt_l2cap_server hiddev_intr_l2cap = {
		.psm = BT_L2CAP_PSM_HID_INT,
		.sec_level = BT_SECURITY_L2,
		.accept = hid_l2cap_intr_accept,
	};

	LOG_DBG("");

	/* Register HID CTRL PSM with L2CAP */
	err = bt_l2cap_br_server_register(&hiddev_ctrl_l2cap);
	if (err < 0) {
		LOG_ERR("HID ctrl L2CAP registration failed, err:%d", err);
		return err;
	}

	/* Register HID INTR PSM with L2CAP */
	err = bt_l2cap_br_server_register(&hiddev_intr_l2cap);
	if (err < 0) {
		LOG_ERR("HID intr L2CAP registration failed, err:%d", err);
		return err;
	}

	return err;
}
