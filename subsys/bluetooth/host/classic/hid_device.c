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

static void bt_hid_deinit(struct bt_hid_device *hid);

static struct bt_hid_device *hid_get_connection(struct bt_conn *conn)
{
	struct bt_hid_device *hid = &hid_devs[bt_conn_index(conn)];

	if (hid->conn == NULL) {
		/* Clean the memory area before returning */
		(void)memset(hid, 0, sizeof(*hid));
	}

	return hid;
}

static void bt_hid_l2cap_ctrl_connected(struct bt_l2cap_chan *chan)
{
	struct bt_hid_device *hid;
	struct bt_hid_session *session;
	int err;

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

	if (session->role != BT_HID_SESSION_ROLE_CTRL) {
		LOG_ERR("HID invalid role:%d", session->role);
		return;
	}

	hid->state = BT_HID_STATE_CTRL_CONNECTED;

	if (hid->role == BT_HID_ROLE_ACCEPTOR) {
		/* Wait for INTR channel connection from remote */
		LOG_DBG("HID wait for INTR channel connection from remote");
		return;
	}

	err = bt_l2cap_chan_connect(chan->conn, &hid->intr_session.br_chan.chan,
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

	if (session->role != BT_HID_SESSION_ROLE_INTR) {
		LOG_ERR("HID invalid role:%d", session->role);
		return;
	}

	hid->state = BT_HID_STATE_CONNECTED;

	if (hid_cb && hid_cb->connected) {
		hid_cb->connected(hid);
	}
}

static void bt_hid_l2cap_ctrl_disconnected(struct bt_l2cap_chan *chan)
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
	LOG_DBG("HID disconnected, session role:%d state:%d", session->role, hid->state);

	session->br_chan.chan.conn = NULL;
	if (session->role != BT_HID_SESSION_ROLE_CTRL) {
		LOG_ERR("HID invalid role:%d", session->role);
		return;
	}

	/* if INTR session connected, it need to to disconnected */
	if (hid->intr_session.br_chan.chan.conn) {
		LOG_DBG("HID disconnect INTR channel");
		bt_l2cap_chan_disconnect(&hid->intr_session.br_chan.chan);
		return;
	}

	if (hid->pending_vc_unplug && hid_cb && hid_cb->vc_unplug) {
		hid_cb->vc_unplug(hid);
		hid->pending_vc_unplug = 0;
	}

	if (hid_cb && hid_cb->disconnected) {
		hid_cb->disconnected(hid);
	}

	bt_hid_deinit(hid);
}

static void bt_hid_l2cap_intr_disconnected(struct bt_l2cap_chan *chan)
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
	LOG_DBG("HID disconnected, session role:%d state:%d", session->role, hid->state);

	session->br_chan.chan.conn = NULL;
	if (session->role != BT_HID_SESSION_ROLE_INTR) {
		LOG_ERR("HID invalid role:%d", session->role);
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

	if (hid->pending_vc_unplug && hid_cb && hid_cb->vc_unplug) {
		hid_cb->vc_unplug(hid);
		hid->pending_vc_unplug = 0;
	}

	if (hid_cb && hid_cb->disconnected) {
		hid_cb->disconnected(hid);
	}

	bt_hid_deinit(hid);
}

static void bt_hid_init(struct bt_hid_device *hid, struct bt_conn *conn, enum bt_hid_role role)
{
	static const struct bt_l2cap_chan_ops ctrl_ops = {
		.connected = bt_hid_l2cap_ctrl_connected,
		.disconnected = bt_hid_l2cap_ctrl_disconnected,
		.recv = NULL,
	};

	static const struct bt_l2cap_chan_ops intr_ops = {
		.connected = bt_hid_l2cap_intr_connected,
		.disconnected = bt_hid_l2cap_intr_disconnected,
		.recv = NULL,
	};

	hid->ctrl_session.br_chan.chan.ops = (struct bt_l2cap_chan_ops *)&ctrl_ops;
	hid->ctrl_session.br_chan.rx.mtu = BT_HID_MAX_MTU;
	hid->ctrl_session.role = BT_HID_SESSION_ROLE_CTRL;

	hid->intr_session.br_chan.chan.ops = (struct bt_l2cap_chan_ops *)&intr_ops;
	hid->intr_session.br_chan.rx.mtu = BT_HID_MAX_MTU;
	hid->intr_session.role = BT_HID_SESSION_ROLE_INTR;

	hid->role = role;
	hid->state = BT_HID_STATE_CTRL_CONNECTING;
	hid->conn = conn;
	hid->buf = NULL;
	hid->pending_vc_unplug = 0;
}

static void bt_hid_deinit(struct bt_hid_device *hid)
{

	if (hid->buf != NULL) {
		net_buf_unref(hid->buf);
		hid->buf = NULL;
	}

	hid->conn = NULL;
	hid->pending_vc_unplug = 0;
	hid->state = BT_HID_STATE_DISTCONNECTED;
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
	return -ENOTSUP;
}

int bt_hid_device_send_intr_data(struct bt_hid_device *hid, uint8_t type, uint8_t *data,
				 uint16_t len)
{
	return -ENOTSUP;
}

int bt_hid_device_report_error(struct bt_hid_device *hid, uint8_t error)
{
	return -ENOTSUP;
}

struct bt_hid_device *bt_hid_device_connect(struct bt_conn *conn)
{
	return NULL;
}

int bt_hid_device_disconnect(struct bt_hid_device *hid)
{
	return -ENOTSUP;
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
