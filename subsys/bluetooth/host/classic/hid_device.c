/* hid_device.c - HID Profile - HID Device side handling */

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

/* Get the HID device from CTRL L2CAP channel */
#define HID_DEVICE_BY_CTRL_CHAN(ch)                                                                \
	CONTAINER_OF(ch, struct bt_hid_device, ctrl_session.br_chan.chan)

/* Get the HID device from INTR L2CAP channel */
#define HID_DEVICE_BY_INTR_CHAN(ch)                                                                \
	CONTAINER_OF(ch, struct bt_hid_device, intr_session.br_chan.chan)

/* Get the HID session from L2CAP channel */
#define HID_SESSION_BY_CHAN(ch) CONTAINER_OF(ch, struct bt_hid_session, br_chan.chan)

#define HID_SESSION_ROLE(ch)                                                                       \
	(((ch) == NULL) ? BT_HID_SESSION_ROLE_UNKNOWN : HID_SESSION_BY_CHAN((ch))->role)

/* HID device callback */
static struct bt_hid_device_cb *hid_cb;

/* HID device connections */
static struct bt_hid_device connections[CONFIG_BT_MAX_CONN];

static void bt_hid_device_cleanup(struct bt_hid_device *hid);

static struct bt_hid_device *hid_get_connection(struct bt_conn *conn)
{
	struct bt_hid_device *hid;

	if (conn == NULL) {
		LOG_ERR("Invalid conn");
		return NULL;
	}

	if (bt_conn_index(conn) >= ARRAY_SIZE(connections)) {
		LOG_ERR("Conn index out of range");
		return NULL;
	}

	hid = &connections[bt_conn_index(conn)];
	if (hid->conn == NULL) {
		/* Clean the memory area before returning */
		(void)memset(hid, 0, sizeof(*hid));
	}

	return hid;
}

struct net_buf *bt_hid_device_create_pdu(struct net_buf_pool *pool)
{
	return bt_conn_create_pdu(pool, sizeof(struct bt_l2cap_hdr) + sizeof(struct bt_hid_header));
}

static int hid_send_handshake(struct bt_hid_session *session, uint8_t param)
{
	struct net_buf *buf;
	struct bt_hid_header *hdr;
	int err;

	buf = bt_hid_device_create_pdu(NULL);
	if (buf == NULL) {
		return -ENOMEM;
	}

	if (net_buf_tailroom(buf) < sizeof(struct bt_hid_header)) {
		net_buf_unref(buf);
		return -ENOMEM;
	}

	hdr = net_buf_add(buf, sizeof(struct bt_hid_header));
	memset(hdr, 0, sizeof(struct bt_hid_header));
	hdr->param = param & 0x0F;
	hdr->type = BT_HID_TYPE_HANDSHAKE;

	err = bt_l2cap_chan_send(&session->br_chan.chan, buf);
	if (err < 0) {
		LOG_ERR("L2CAP send fail, err:%d", err);
		net_buf_unref(buf);
		return err;
	}

	return err;
}

static int hid_control_handle(struct bt_l2cap_chan *chan, struct net_buf *buf, uint8_t control)
{
	struct bt_hid_device *hid;
	int err = 0;

	if (chan == NULL) {
		LOG_ERR("Invalid hid chan");
		return -EINVAL;
	}

	hid = HID_DEVICE_BY_CTRL_CHAN(chan);
	if (hid == NULL) {
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

static int hid_get_report_handle(struct bt_l2cap_chan *chan, struct net_buf *buf, uint8_t param)
{
	struct bt_hid_device *hid;
	uint8_t report_id;
	uint16_t buffer_size;
	uint8_t report_type;

	if (chan == NULL) {
		LOG_ERR("Invalid hid chan");
		return -EINVAL;
	}

	hid = HID_DEVICE_BY_CTRL_CHAN(chan);
	if (hid == NULL) {
		LOG_ERR("HID not found");
		return -EIO;
	}

	if (buf->len > BT_HID_MAX_MTU) {
		LOG_ERR("HID get report buf len %d > max mtu %d", buf->len, BT_HID_MAX_MTU);
		hid_send_handshake(&hid->ctrl_session, BT_HID_HANDSHAKE_RSP_ERR_INVALID_PARAM);
		return -EINVAL;
	}

	report_type = param & HID_PAR_REPORT_TYPE_MASK;

	if (param & HID_PAR_REPORT_SIZE_MASK) {
		if (buf->len < sizeof(report_id)) {
			LOG_ERR("HID get report buf len %d < min len 3", buf->len);
			hid_send_handshake(&hid->ctrl_session,
					   BT_HID_HANDSHAKE_RSP_ERR_INVALID_PARAM);
			return -EINVAL;
		}

		report_id = net_buf_pull_u8(buf);
	} else {
		report_id = 0;
	}

	if (buf->len < sizeof(buffer_size)) {
		LOG_ERR("HID get report buf len %d < min len 3", buf->len);
		hid_send_handshake(&hid->ctrl_session, BT_HID_HANDSHAKE_RSP_ERR_INVALID_PARAM);
		return -EINVAL;
	}

	buffer_size = net_buf_pull_be16(buf);

	if ((hid_cb == NULL) || (hid_cb->get_report == NULL)) {
		hid_send_handshake(&hid->ctrl_session, BT_HID_HANDSHAKE_RSP_ERR_UNSUPPORTED_REQ);
		return -EOPNOTSUPP;
	}

	hid_cb->get_report(hid, report_type, report_id, buffer_size);
	return 0;
}

static int hid_set_report_handle(struct bt_l2cap_chan *chan, struct net_buf *buf, uint8_t param)
{
	struct bt_hid_device *hid;

	if (chan == NULL) {
		LOG_ERR("Invalid hid chan");
		return -EINVAL;
	}

	hid = HID_DEVICE_BY_CTRL_CHAN(chan);
	if (hid == NULL) {
		LOG_ERR("HID not found");
		return -EIO;
	}

	if (buf->len > BT_HID_MAX_MTU) {
		LOG_ERR("HID set report buf len %d > max mtu %d", buf->len, BT_HID_MAX_MTU);
		hid_send_handshake(&hid->ctrl_session, BT_HID_HANDSHAKE_RSP_ERR_INVALID_PARAM);
		return -EINVAL;
	}

	param &= HID_PAR_REPORT_TYPE_MASK;

	if ((hid_cb != NULL) && (hid_cb->set_report != NULL)) {
		hid_cb->set_report(hid, param, buf);
	}

	return 0;
}

static int hid_get_protocol_handle(struct bt_l2cap_chan *chan, struct net_buf *buf, uint8_t param)
{
	struct bt_hid_device *hid;

	if (chan == NULL) {
		LOG_ERR("Invalid hid chan");
		return -EINVAL;
	}

	hid = HID_DEVICE_BY_CTRL_CHAN(chan);
	if (hid == NULL) {
		LOG_ERR("HID not found");
		return -EIO;
	}

	if ((hid_cb == NULL) || (hid_cb->get_protocol == NULL)) {
		hid_send_handshake(&hid->ctrl_session, BT_HID_HANDSHAKE_RSP_ERR_UNSUPPORTED_REQ);
		return -EOPNOTSUPP;
	}

	hid_cb->get_protocol(hid);
	return 0;
}

static int hid_set_protocol_handle(struct bt_l2cap_chan *chan, struct net_buf *buf, uint8_t param)
{
	struct bt_hid_device *hid;
	uint8_t protocol;

	if (chan == NULL) {
		LOG_ERR("Invalid hid chan");
		return -EINVAL;
	}

	hid = HID_DEVICE_BY_CTRL_CHAN(chan);
	if (hid == NULL) {
		LOG_ERR("HID not found");
		return -EIO;
	}

	protocol = param & BT_HID_PROTOCOL_MASK;
	if ((hid_cb != NULL) && (hid_cb->set_protocol != NULL)) {
		hid_cb->set_protocol(hid, protocol);
	}

	return hid_send_handshake(HID_SESSION_BY_CHAN(chan), BT_HID_HANDSHAKE_RSP_SUCCESS);
}

static int hid_intr_handle(struct bt_l2cap_chan *chan, struct net_buf *buf, uint8_t param)
{
	struct bt_hid_device *hid;
	uint8_t type;

	if (chan == NULL) {
		LOG_ERR("Invalid hid chan");
		return -EINVAL;
	}

	hid = HID_DEVICE_BY_INTR_CHAN(chan);
	if (hid == NULL) {
		LOG_ERR("HID not found");
		return -EIO;
	}

	if (buf->len > BT_HID_MAX_MTU) {
		LOG_ERR("HID intr buf len %d > max mtu %d", buf->len, BT_HID_MAX_MTU);
		hid_send_handshake(&hid->ctrl_session, BT_HID_HANDSHAKE_RSP_ERR_INVALID_PARAM);
		return -EINVAL;
	}

	type = param & HID_PAR_REPORT_TYPE_MASK;

	if ((hid_cb != NULL) && (hid_cb->intr_data != NULL)) {
		hid_cb->intr_data(hid, type, buf);
	}

	return 0;
}

static void bt_hid_l2cap_ctrl_connected(struct bt_l2cap_chan *chan)
{
	struct bt_hid_device *hid;
	enum bt_hid_session_role role;
	int err;

	if (chan == NULL) {
		LOG_ERR("Invalid hid chan");
		return;
	}

	hid = HID_DEVICE_BY_CTRL_CHAN(chan);
	if (hid == NULL) {
		LOG_ERR("HID not found");
		return;
	}

	role = HID_SESSION_ROLE(chan);

	LOG_DBG("HID session:%d connected, state %d", role, hid->state);

	if (role != BT_HID_SESSION_ROLE_CTRL) {
		LOG_ERR("HID invalid role:%d", role);
		return;
	}

	hid->state = BT_HID_STATE_CTRL_CONNECTED;

	if (hid->role == BT_HID_ROLE_ACCEPTOR) {
		/* Wait for INTR channel connection from remote */
		LOG_DBG("HID wait for INTR channel connection from remote");
		return;
	}

	err = bt_l2cap_chan_connect(hid->conn, &hid->intr_session.br_chan.chan,
				    BT_L2CAP_PSM_HID_INT);
	if (err) {
		LOG_ERR("HID connect INTR failed");
		hid->state = BT_HID_STATE_DISCONNECTING;
		bt_l2cap_chan_disconnect(&hid->ctrl_session.br_chan.chan);
		return;
	}
}

static void bt_hid_l2cap_intr_connected(struct bt_l2cap_chan *chan)
{
	struct bt_hid_device *hid;
	enum bt_hid_session_role role;

	if (chan == NULL) {
		LOG_ERR("Invalid hid chan");
		return;
	}

	hid = HID_DEVICE_BY_INTR_CHAN(chan);
	if (hid == NULL) {
		LOG_ERR("HID not found");
		return;
	}

	role = HID_SESSION_ROLE(chan);

	LOG_DBG("HID session:%d connected, state %d", role, hid->state);

	if (role != BT_HID_SESSION_ROLE_INTR) {
		LOG_ERR("HID invalid role:%d", role);
		return;
	}

	hid->state = BT_HID_STATE_CONNECTED;

	if ((hid_cb != NULL) && (hid_cb->connected != NULL)) {
		hid_cb->connected(hid);
	}
}

static void bt_hid_l2cap_ctrl_disconnected(struct bt_l2cap_chan *chan)
{
	struct bt_hid_device *hid;
	enum bt_hid_session_role role;

	if (chan == NULL) {
		LOG_ERR("Invalid hid chan");
		return;
	}

	hid = HID_DEVICE_BY_CTRL_CHAN(chan);
	if (hid == NULL) {
		LOG_ERR("HID not found");
		return;
	}

	role = HID_SESSION_ROLE(chan);

	LOG_DBG("HID session:%d connected, state %d", role, hid->state);

	if (role != BT_HID_SESSION_ROLE_CTRL) {
		LOG_ERR("HID invalid role:%d", role);
		return;
	}

	/* if INTR session connected, it need to be disconnected */
	if (hid->intr_session.br_chan.chan.conn) {
		LOG_DBG("HID disconnect INTR channel");
		bt_l2cap_chan_disconnect(&hid->intr_session.br_chan.chan);
		return;
	}

	if ((hid_cb != NULL) && (hid_cb->disconnected != NULL)) {
		hid_cb->disconnected(hid);
	}

	bt_hid_device_cleanup(hid);
}

static void bt_hid_l2cap_intr_disconnected(struct bt_l2cap_chan *chan)
{
	struct bt_hid_device *hid;
	enum bt_hid_session_role role;

	if (chan == NULL) {
		LOG_ERR("Invalid hid chan");
		return;
	}

	hid = HID_DEVICE_BY_INTR_CHAN(chan);
	if (hid == NULL) {
		LOG_ERR("HID not found");
		return;
	}

	role = HID_SESSION_ROLE(chan);

	LOG_DBG("HID session:%d connected, state %d", role, hid->state);

	if (role != BT_HID_SESSION_ROLE_INTR) {
		LOG_ERR("HID invalid role:%d", role);
		return;
	}

	/* Local request disconnect(INTR channel), and it need to disconnect CTRL channel as well */
	if (hid->state == BT_HID_STATE_DISCONNECTING) {
		LOG_DBG("HID disconnect CTRL channel");
		bt_l2cap_chan_disconnect(&hid->ctrl_session.br_chan.chan);
		return;
	}

	/* Wait for remote disconnect CTRL channel */
	if (hid->ctrl_session.br_chan.chan.conn) {
		LOG_DBG("Wait for remote disconnect CTRL channel");
		return;
	}

	if ((hid_cb != NULL) && (hid_cb->disconnected != NULL)) {
		hid_cb->disconnected(hid);
	}

	bt_hid_device_cleanup(hid);
}

static int bt_hid_l2cap_ctrl_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_hid_header *hdr;
	struct bt_hid_device *hid;
	uint8_t type;
	uint8_t param;

	if (chan == NULL) {
		LOG_ERR("Invalid hid chan");
		return -EIO;
	}

	hid = HID_DEVICE_BY_CTRL_CHAN(chan);
	if (hid == NULL) {
		LOG_ERR("HID not found");
		return -EIO;
	}

	if (HID_SESSION_ROLE(chan) != BT_HID_SESSION_ROLE_CTRL) {
		LOG_ERR("HID invalid role for CTRL channel");
		return -EIO;
	}

	if (buf->len < sizeof(*hdr) || (buf->len - sizeof(*hdr) > BT_HID_MAX_MTU)) {
		LOG_ERR("HID ctrl buf len %d invalid ", buf->len);
		hid_send_handshake(&hid->ctrl_session, BT_HID_HANDSHAKE_RSP_ERR_INVALID_PARAM);
		return -EINVAL;
	}

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	param = hdr->param & 0x0F;
	type = hdr->type & 0xF0;

	LOG_DBG("HID CTRL recv type[0x%x] param[0x%x]", type, param);

	switch (type) {
	case BT_HID_TYPE_CONTROL:
		hid_control_handle(chan, buf, param);
		break;
	case BT_HID_TYPE_GET_REPORT:
		hid_get_report_handle(chan, buf, param);
		break;
	case BT_HID_TYPE_SET_REPORT:
		hid_set_report_handle(chan, buf, param);
		break;
	case BT_HID_TYPE_GET_PROTOCOL:
		hid_get_protocol_handle(chan, buf, param);
		break;
	case BT_HID_TYPE_SET_PROTOCOL:
		hid_set_protocol_handle(chan, buf, param);
		break;
	default:
		LOG_ERR("HID type:%d not supported", type);
		hid_send_handshake(HID_SESSION_BY_CHAN(chan),
				   BT_HID_HANDSHAKE_RSP_ERR_UNSUPPORTED_REQ);
		break;
	}

	return 0;
}

static int bt_hid_l2cap_intr_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_hid_header *hdr;
	struct bt_hid_device *hid;
	uint8_t param;
	uint8_t type;

	if (chan == NULL) {
		LOG_ERR("Invalid hid chan");
		return -EINVAL;
	}

	hid = HID_DEVICE_BY_INTR_CHAN(chan);
	if (hid == NULL) {
		LOG_ERR("HID not found");
		return -EIO;
	}

	if ((buf->len < sizeof(*hdr)) || (buf->len - sizeof(*hdr) > BT_HID_MAX_MTU)) {
		LOG_ERR("HID intr buf len %d invalid", buf->len);
		hid_send_handshake(&hid->ctrl_session, BT_HID_HANDSHAKE_RSP_ERR_INVALID_PARAM);
		return -EINVAL;
	}

	if (HID_SESSION_ROLE(chan) != BT_HID_SESSION_ROLE_INTR) {
		LOG_ERR("HID invalid role for INTR channel");
		return -EIO;
	}

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	param = hdr->param & 0x0F;
	type = hdr->type & 0xF0;

	LOG_DBG("HID INTR recv type[0x%x] param[0x%x]", type, param);

	return hid_intr_handle(chan, buf, param);
}

static void bt_hid_session_init(struct bt_hid_device *hid, struct bt_conn *conn,
				enum bt_hid_role role)
{
	static const struct bt_l2cap_chan_ops ctrl_ops = {
		.connected = bt_hid_l2cap_ctrl_connected,
		.disconnected = bt_hid_l2cap_ctrl_disconnected,
		.recv = bt_hid_l2cap_ctrl_recv,
	};

	static const struct bt_l2cap_chan_ops intr_ops = {
		.connected = bt_hid_l2cap_intr_connected,
		.disconnected = bt_hid_l2cap_intr_disconnected,
		.recv = bt_hid_l2cap_intr_recv,
	};

	hid->ctrl_session.br_chan.chan.ops = (struct bt_l2cap_chan_ops *)&ctrl_ops;
	hid->ctrl_session.br_chan.rx.mtu = BT_HID_MAX_MTU;
	hid->ctrl_session.role = BT_HID_SESSION_ROLE_CTRL;

	hid->intr_session.br_chan.chan.ops = (struct bt_l2cap_chan_ops *)&intr_ops;
	hid->intr_session.br_chan.rx.mtu = BT_HID_MAX_MTU;
	hid->intr_session.role = BT_HID_SESSION_ROLE_INTR;

	hid->role = role;
	hid->conn = conn;
	bt_conn_ref(hid->conn);
}

static void bt_hid_device_cleanup(struct bt_hid_device *hid)
{
	if (hid->conn != NULL) {
		bt_conn_unref(hid->conn);
		hid->conn = NULL;
	}

	hid->state = BT_HID_STATE_DISCONNECTED;
}

static int hid_l2cap_ctrl_accept(struct bt_conn *conn, struct bt_l2cap_server *server,
				 struct bt_l2cap_chan **chan)
{
	struct bt_hid_device *hid;

	hid = hid_get_connection(conn);
	if (hid == NULL) {
		LOG_ERR("Cannot allocate memory for HID device");
		return -ENOMEM;
	}

	bt_hid_session_init(hid, conn, BT_HID_ROLE_ACCEPTOR);
	hid->state = BT_HID_STATE_CTRL_CONNECTING;

	*chan = &hid->ctrl_session.br_chan.chan;
	return 0;
}

static int hid_l2cap_intr_accept(struct bt_conn *conn, struct bt_l2cap_server *server,
				 struct bt_l2cap_chan **chan)
{
	struct bt_hid_device *hid;

	hid = hid_get_connection(conn);
	if (hid == NULL) {
		LOG_ERR("Cannot get HID device");
		return -ENOMEM;
	}

	hid->state = BT_HID_STATE_INTR_CONNECTING;

	*chan = &hid->intr_session.br_chan.chan;
	return 0;
}

int bt_hid_device_register(const struct bt_hid_device_cb *cb)
{
	LOG_DBG("");

	hid_cb = cb;
	return 0;
}

struct bt_hid_device *bt_hid_device_connect(struct bt_conn *conn)
{
	struct bt_hid_device *hid;
	int err;

	hid = hid_get_connection(conn);
	if (hid == NULL) {
		LOG_ERR("Cannot allocate memory");
		return NULL;
	}

	if (hid->state != BT_HID_STATE_DISCONNECTED) {
		LOG_ERR("HID device is busy, current state:%d", hid->state);
		return NULL;
	}

	bt_hid_session_init(hid, conn, BT_HID_ROLE_INITIATOR);

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

	if (hid->state != BT_HID_STATE_CONNECTED) {
		LOG_ERR("HID device not connected, state:%d", hid->state);
		return -ENOTCONN;
	}

	err = bt_l2cap_chan_disconnect(&hid->intr_session.br_chan.chan);
	if (err != 0) {
		LOG_WRN("HID disconnect session, err:%d", err);
		return err;
	}

	hid->state = BT_HID_STATE_DISCONNECTING;
	return 0;
}

int bt_hid_device_send_ctrl_data(struct bt_hid_device *hid, uint8_t param, struct net_buf *buf)
{
	struct bt_hid_header *hdr;
	int err;

	__ASSERT_NO_MSG(hid);

	if (hid->state != BT_HID_STATE_CONNECTED) {
		LOG_ERR("HID device not connected, state:%d", hid->state);
		return -ENOTCONN;
	}

	hdr = net_buf_push(buf, sizeof(struct bt_hid_header));
	memset(hdr, 0, sizeof(struct bt_hid_header));
	hdr->param = param & 0x0F;
	hdr->type = BT_HID_TYPE_DATA;

	err = bt_l2cap_chan_send(&hid->ctrl_session.br_chan.chan, buf);
	if (err < 0) {
		LOG_ERR("L2CAP send fail, err:%d", err);
		return err;
	}

	return err;
}

int bt_hid_device_send_intr_data(struct bt_hid_device *hid, uint8_t param, struct net_buf *buf)
{
	struct bt_hid_header *hdr;
	int err;

	__ASSERT_NO_MSG(hid);

	if (hid->state != BT_HID_STATE_CONNECTED) {
		LOG_ERR("HID device not connected, state:%d", hid->state);
		return -ENOTCONN;
	}

	hdr = net_buf_push(buf, sizeof(struct bt_hid_header));
	memset(hdr, 0, sizeof(struct bt_hid_header));
	hdr->param = param & 0x0F;
	hdr->type = BT_HID_TYPE_DATA;

	err = bt_l2cap_chan_send(&hid->intr_session.br_chan.chan, buf);
	if (err < 0) {
		LOG_ERR("L2CAP send fail, err:%d", err);
		return err;
	}

	return err;
}

int bt_hid_device_report_error(struct bt_hid_device *hid, uint8_t error)
{
	__ASSERT_NO_MSG(hid);

	if (hid->state != BT_HID_STATE_CONNECTED) {
		LOG_ERR("HID device not connected, state:%d", hid->state);
		return -ENOTCONN;
	}

	return hid_send_handshake(&hid->ctrl_session, error);
}

int bt_hid_device_virtual_cable_unplug(struct bt_hid_device *hid)
{
	__ASSERT_NO_MSG(hid);

	return -ENOTSUP;
}

int bt_hid_dev_init(void)
{
	int err;
	static struct bt_l2cap_server server_ctrl = {
		.psm = BT_L2CAP_PSM_HID_CTL,
		.sec_level = BT_SECURITY_L2,
		.accept = hid_l2cap_ctrl_accept,
	};
	static struct bt_l2cap_server server_intr = {
		.psm = BT_L2CAP_PSM_HID_INT,
		.sec_level = BT_SECURITY_L2,
		.accept = hid_l2cap_intr_accept,
	};

	LOG_DBG("");

	/* Register HID CTRL PSM with L2CAP */
	err = bt_l2cap_br_server_register(&server_ctrl);
	if (err < 0) {
		LOG_ERR("HID CTRL L2CAP register failed, err:%d", err);
		return err;
	}

	/* Register HID INTR PSM with L2CAP */
	err = bt_l2cap_br_server_register(&server_intr);
	if (err < 0) {
		LOG_ERR("HID INTR L2CAP register failed, err:%d", err);
		return err;
	}

	return err;
}
