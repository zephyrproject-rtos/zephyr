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

/* Get the HID device from CTRL L2CAP channel */
#define HID_DEVICE_BY_CTRL_CHAN(_ch)                                                               \
	CONTAINER_OF(_ch, struct bt_hid_device, ctrl_session.br_chan.chan)

/* Get the HID device from INTR L2CAP channel */
#define HID_DEVICE_BY_INTR_CHAN(_ch)                                                               \
	CONTAINER_OF(_ch, struct bt_hid_device, intr_session.br_chan.chan)

/* Get the HID session from L2CAP channel */
#define HID_SESSION_BY_CHAN(_ch) CONTAINER_OF(_ch, struct bt_hid_session, br_chan.chan)

/* HID device callback */
static struct bt_hid_device_cb *hid_cb;

/* HID device connections */
static struct bt_hid_device connections[CONFIG_BT_MAX_CONN];

static void bt_hid_deinit(struct bt_hid_device *hid);

static struct bt_hid_device *hid_get_connection(struct bt_conn *conn)
{
	struct bt_hid_device *hid = &connections[bt_conn_index(conn)];

	if (hid->conn == NULL) {
		/* Clean the memory area before returning */
		(void)memset(hid, 0, sizeof(*hid));
	}

	return hid;
}

static enum bt_hid_session_role get_session_role_from_chan(struct bt_l2cap_chan *chan)
{
	struct bt_hid_session *session;

	session = HID_SESSION_BY_CHAN(chan);
	if (session == NULL) {
		LOG_ERR("HID session not found for channel %p", chan);
		return BT_HID_SESSION_ROLE_UNKNOWN;
	}

	return session->role;
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

	role = get_session_role_from_chan(chan);

	LOG_DBG("HID session:%d connected, state %d", role, hid->state);

	if (role != BT_HID_SESSION_ROLE_CTRL) {
		LOG_ERR("HID invalid role:%d", role);
		return;
	}

	hid->state = BT_HID_STATE_CTRL_CONNECTED;
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

	role = get_session_role_from_chan(chan);

	LOG_DBG("HID session:%d connected, state %d", role, hid->state);

	if (role != BT_HID_SESSION_ROLE_INTR) {
		LOG_ERR("HID invalid role:%d", role);
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

	role = get_session_role_from_chan(chan);

	LOG_DBG("HID session:%d connected, state %d", role, hid->state);

	if (role != BT_HID_SESSION_ROLE_CTRL) {
		LOG_ERR("HID invalid role:%d", role);
		return;
	}
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

	role = get_session_role_from_chan(chan);

	LOG_DBG("HID session:%d connected, state %d", role, hid->state);

	if (role != BT_HID_SESSION_ROLE_INTR) {
		LOG_ERR("HID invalid role:%d", role);
		return;
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
		LOG_ERR("Cannot allocate memory for HID device");
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
		LOG_ERR("Cannot get HID device");
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
