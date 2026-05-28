/* hid_device.c - HID Profile - HID Device side handling */

/*
 * Copyright 2025 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/util.h>

#include "common/assert.h"

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/classic/hid_device.h>
#include <zephyr/bluetooth/classic/classic.h>
#include <zephyr/bluetooth/l2cap.h>

#include "host/hci_core.h"
#include "host/conn_internal.h"
#include "host/l2cap_internal.h"

#include "hid_internal.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_hid_device);

/* Get the HID device from CTRL L2CAP channel */
#define HID_DEVICE_BY_CTRL_CHAN(ch) \
	CONTAINER_OF(ch, struct bt_hid_device, ctrl_session.br_chan.chan)

/* Get the HID device from INTR L2CAP channel */
#define HID_DEVICE_BY_INTR_CHAN(ch) \
	CONTAINER_OF(ch, struct bt_hid_device, intr_session.br_chan.chan)

/* Get the HID session from L2CAP channel */
#define HID_SESSION_BY_CHAN(ch) \
	CONTAINER_OF(ch, struct bt_hid_session, br_chan.chan)

#define HID_CHAN_TYPE(ch) \
	(((ch) == NULL) ? BT_HID_CHANNEL_UNKNOWN : HID_SESSION_BY_CHAN((ch))->type)

/* Timeout for INTR channel connection after CTRL is established (seconds) */
#define HID_INTR_CONN_TIMEOUT K_SECONDS(30)

/* Dedicated buffer pool for HANDSHAKE responses.  HANDSHAKE is a mandatory
 * protocol response (HID spec v1.1.2 Section 3.2) that must not fail due to
 * buffer contention with data traffic.  2 buffers covers back-to-back error
 * handshakes before the first TX completes.  Payload is a single-byte header.
 */
NET_BUF_POOL_FIXED_DEFINE(hid_hs_pool, 2,
			  BT_L2CAP_BUF_SIZE(sizeof(struct bt_hid_hdr)),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

/* HID device callback */
static const struct bt_hid_device_cb *hid_cb;

/* HID L2CAP servers registration state */
static bool hid_l2cap_registered;

/* HID device connection (single session only) */
static struct bt_hid_device hid_device_conn;

static void bt_hid_device_cleanup(struct bt_hid_device *hid);

static struct bt_hid_device *hid_allocate(void)
{
	if (hid_device_conn.state == BT_HID_STATE_DISCONNECTED) {
		return &hid_device_conn;
	}

	return NULL;
}

static struct bt_hid_device *hid_find(const struct bt_conn *conn, uint8_t state)
{
	if (hid_device_conn.state != state) {
		return NULL;
	}

	if (hid_device_conn.ctrl_session.br_chan.chan.conn != conn) {
		return NULL;
	}

	return &hid_device_conn;
}

struct bt_conn *bt_hid_device_get_conn(struct bt_hid_device *hid)
{
	struct bt_conn *conn;

	if (hid == NULL) {
		return NULL;
	}

	conn = hid->ctrl_session.br_chan.chan.conn;
	if (conn == NULL) {
		return NULL;
	}

	return bt_conn_ref(conn);
}

struct net_buf *bt_hid_device_create_pdu(struct net_buf_pool *pool)
{
	return bt_l2cap_create_pdu(pool, sizeof(struct bt_hid_hdr));
}

static void hid_send_handshake(struct bt_hid_device *hid, uint8_t result_code)
{
	struct net_buf *buf;
	struct bt_hid_hdr *hdr;
	int err;

	buf = bt_l2cap_create_pdu(&hid_hs_pool, sizeof(struct bt_hid_hdr));
	if (buf == NULL) {
		LOG_ERR("Handshake buf alloc failed");
		return;
	}

	hdr = net_buf_add(buf, sizeof(struct bt_hid_hdr));
	hdr->header = BT_HID_BUILD_HDR(BT_HID_MSG_TYPE_HANDSHAKE, result_code);

	/* HANDSHAKE is only ever sent on the Control channel. */
	err = bt_l2cap_chan_send(&hid->ctrl_session.br_chan.chan, buf);
	if (err < 0) {
		LOG_ERR("Handshake send failed (%d)", err);
		net_buf_unref(buf);
	}
}

static uint8_t hid_err_to_handshake(int err)
{
	switch (err) {
	case 0:
		return BT_HID_HS_RSP_SUCCESS;
	case -EOPNOTSUPP:
	case -ENOSYS:
		return BT_HID_HS_RSP_ERR_UNSUPPORTED_REQ;
	case -EINVAL:
		return BT_HID_HS_RSP_ERR_INVALID_PARAM;
	case -ENOENT:
		return BT_HID_HS_RSP_ERR_INVALID_REPORT_ID;
	case -EBUSY:
	case -EAGAIN:
		return BT_HID_HS_RSP_NOT_READY;
	default:
		return BT_HID_HS_RSP_ERR_UNKNOWN;
	}
}

static int hid_control_handle(struct bt_hid_device *hid, struct net_buf *buf, uint8_t control)
{
	int err;

	switch (control) {
	case BT_HID_CONTROL_VIRTUAL_CABLE_UNPLUG:
		if ((hid_cb != NULL) && (hid_cb->vc_unplug != NULL)) {
			hid_cb->vc_unplug(hid);
		}

		/* HID spec v1.1.2 Section 3.1.2.2.3 requires the recipient to
		 * destroy bonding/Virtual Cable info on VC unplug, then
		 * disconnect INTR followed by CTRL without sending HANDSHAKE.
		 * Clearing bonding is delegated to the application through the
		 * vc_unplug callback above: the only host API, bt_br_unpair(),
		 * unconditionally disconnects the whole ACL, which would tear
		 * down any co-located profile (A2DP/AVRCP/HFP) sharing it. The
		 * spec only allows the ACL to be dropped "if there are no other
		 * profiles using the ACL", so the stack does not clear bonding
		 * here; the application decides when it is safe to do so (see
		 * bt_hid_device_get_conn()).
		 */
		err = bt_hid_device_disconnect(hid);
		if ((err != 0) && (err != -ENOTCONN)) {
			/* Disconnect is requested as part of VC unplug teardown.
			 * There is no recovery path here and no HANDSHAKE is sent
			 * for VC unplug, so only log the failure for diagnostics.
			 * -ENOTCONN means teardown is already in progress, which
			 * matches the intent and is not treated as an error.
			 */
			LOG_WRN("VC unplug: disconnect failed (%d)", err);
		}
		break;
	case BT_HID_CONTROL_SUSPEND:
		/* Suspend is host-owned state; the device only relays the
		 * request to the application. No local state is tracked here
		 * (see the suspend callback documentation).
		 */
		if ((hid_cb != NULL) && (hid_cb->suspend != NULL)) {
			hid_cb->suspend(hid, true);
		}
		break;
	case BT_HID_CONTROL_EXIT_SUSPEND:
		if ((hid_cb != NULL) && (hid_cb->suspend != NULL)) {
			hid_cb->suspend(hid, false);
		}
		break;
	default:
		/* HID spec v1.1.2 Section 3.1.2.2: unsupported HID_CONTROL
		 * operations shall be silently ignored.
		 */
		LOG_WRN("control %u not handled, ignoring", control);
		break;
	}

	return 0;
}

static int hid_get_report_handle(struct bt_hid_device *hid, struct net_buf *buf, uint8_t param)
{
	struct bt_hid_hdr *hdr;
	struct net_buf *rsp;
	uint8_t report_type;
	bool size_present;
	int err;

	report_type = FIELD_GET(BT_HID_PARAM_REPORT_TYPE_MASK, param);
	size_present = (param & BT_HID_PARAM_REPORT_SIZE_MASK) != 0;

	if (size_present && (buf->len < sizeof(uint16_t))) {
		return -EINVAL;
	}

	if ((hid_cb == NULL) || (hid_cb->get_report == NULL)) {
		return -EOPNOTSUPP;
	}

	rsp = bt_hid_device_create_pdu(NULL);
	if (rsp == NULL) {
		return -EAGAIN;
	}

	/* GET_REPORT is synchronous: the application fills rsp with the report
	 * payload and returns 0, or returns a negative errno (e.g. -EAGAIN for
	 * NOT_READY). The transaction is fully resolved on return, so no pending
	 * state needs to be tracked here. The error handshake is sent by the
	 * dispatcher; on success the DATA response below is the reply.
	 */
	err = hid_cb->get_report(hid, report_type, size_present, buf, rsp);
	if (err < 0) {
		LOG_ERR("Get_Report cb err %d", err);
		net_buf_unref(rsp);
		return err;
	}

	hdr = net_buf_push(rsp, sizeof(struct bt_hid_hdr));
	hdr->header = BT_HID_BUILD_HDR(BT_HID_MSG_TYPE_DATA, report_type);

	err = bt_l2cap_chan_send(&hid->ctrl_session.br_chan.chan, rsp);
	if (err < 0) {
		LOG_ERR("Get_Report rsp send failed (%d)", err);
		net_buf_unref(rsp);
		return err;
	}

	return 0;
}

static int hid_set_report_handle(struct bt_hid_device *hid, struct net_buf *buf, uint8_t param)
{
	uint8_t report_type;
	int err;

	report_type = FIELD_GET(BT_HID_PARAM_REPORT_TYPE_MASK, param);

	if ((hid_cb == NULL) || (hid_cb->set_report == NULL)) {
		return -EOPNOTSUPP;
	}

	err = hid_cb->set_report(hid, report_type, buf);
	if (err < 0) {
		LOG_ERR("Set_Report cb err %d", err);
		return err;
	}

	hid_send_handshake(hid, BT_HID_HS_RSP_SUCCESS);
	return 0;
}

static int hid_get_protocol_handle(struct bt_hid_device *hid, struct net_buf *buf, uint8_t param)
{
	struct net_buf *rsp;
	struct bt_hid_hdr *hdr;
	int err;

	rsp = bt_hid_device_create_pdu(NULL);
	if (rsp == NULL) {
		return -EAGAIN;
	}

	hdr = net_buf_add(rsp, sizeof(struct bt_hid_hdr));
	hdr->header = BT_HID_BUILD_HDR(BT_HID_MSG_TYPE_DATA, BT_HID_PAR_REP_TYPE_OTHER);
	net_buf_add_u8(rsp, hid->boot_mode ? BT_HID_PROTOCOL_BOOT_MODE
					     : BT_HID_PROTOCOL_REPORT_MODE);

	err = bt_l2cap_chan_send(&hid->ctrl_session.br_chan.chan, rsp);
	if (err < 0) {
		LOG_ERR("L2CAP send failed (%d)", err);
		net_buf_unref(rsp);
		return err;
	}

	return 0;
}

static int hid_set_protocol_handle(struct bt_hid_device *hid, struct net_buf *buf, uint8_t param)
{
	uint8_t protocol;
	int err;

	protocol = FIELD_GET(BT_HID_PROTOCOL_MASK, param);

	if ((hid_cb == NULL) || (hid_cb->set_protocol == NULL)) {
		return -EOPNOTSUPP;
	}

	err = hid_cb->set_protocol(hid, protocol);
	if (err != 0) {
		return err;
	}

	hid->boot_mode = (protocol == BT_HID_PROTOCOL_BOOT_MODE);
	hid_send_handshake(hid, BT_HID_HS_RSP_SUCCESS);

	return 0;
}

static int hid_intr_handle(struct bt_hid_device *hid, struct net_buf *buf)
{
	if ((hid_cb != NULL) && (hid_cb->output_report != NULL)) {
		hid_cb->output_report(hid, buf);
	}

	return 0;
}

typedef int (*hid_ctrl_handler_t)(struct bt_hid_device *hid, struct net_buf *buf, uint8_t param);

static const struct {
	uint8_t type;
	hid_ctrl_handler_t handler;
} hid_ctrl_handlers[] = {
	{BT_HID_MSG_TYPE_CONTROL, hid_control_handle},
	{BT_HID_MSG_TYPE_GET_REPORT, hid_get_report_handle},
	{BT_HID_MSG_TYPE_SET_REPORT, hid_set_report_handle},
	{BT_HID_MSG_TYPE_GET_PROTOCOL, hid_get_protocol_handle},
	{BT_HID_MSG_TYPE_SET_PROTOCOL, hid_set_protocol_handle},
};

static void hid_intr_timeout_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct bt_hid_device *hid = CONTAINER_OF(dwork, struct bt_hid_device, intr_timeout);
	int err;

	LOG_WRN("INTR connection timeout");

	/* Timer is started when CTRL connects and cancelled when INTR connects
	 * or when cleanup runs.  If this fires, the HID connection did not
	 * complete in time — tear down whatever is up.
	 */
	err = bt_hid_device_disconnect(hid);
	if (err != 0) {
		LOG_WRN("INTR timeout: disconnect failed (%d)", err);
	}
}

static void hid_vcu_disconnect_work(struct k_work *work)
{
	struct bt_hid_device *hid = CONTAINER_OF(work, struct bt_hid_device, vcu_disconnect);
	int err;

	LOG_DBG("VCU disconnect");

	err = bt_hid_device_disconnect(hid);
	if (err != 0) {
		LOG_WRN("VCU disconnect failed (%d)", err);
	}
}

static void bt_hid_l2cap_ctrl_connected(struct bt_l2cap_chan *chan)
{
	struct bt_hid_device *hid = HID_DEVICE_BY_CTRL_CHAN(chan);
	__maybe_unused enum bt_hid_channel_type chtype;
	int err;

	chtype = HID_CHAN_TYPE(chan);

	LOG_DBG("session %d connected (state %d)", chtype, hid->state);

	__ASSERT_NO_MSG(chtype == BT_HID_CHANNEL_CTRL);

	hid->state = BT_HID_STATE_CTRL_CONNECTED;
	hid->ctrl_connected = true;

	if (hid->role == BT_HID_ROLE_ACCEPTOR) {
		/* Wait for INTR channel connection from remote */
		LOG_DBG("wait for INTR connection");
		k_work_schedule(&hid->intr_timeout, HID_INTR_CONN_TIMEOUT);
		return;
	}

	err = bt_l2cap_chan_connect(hid->ctrl_session.br_chan.chan.conn,
				    &hid->intr_session.br_chan.chan, BT_L2CAP_PSM_HID_INTR);
	if (err != 0) {
		LOG_ERR("INTR connect failed");
		hid->state = BT_HID_STATE_DISCONNECTING;
		bt_l2cap_chan_disconnect(&hid->ctrl_session.br_chan.chan);
		return;
	}

	hid->state = BT_HID_STATE_INTR_CONNECTING;
}

static void bt_hid_l2cap_intr_connected(struct bt_l2cap_chan *chan)
{
	struct bt_hid_device *hid = HID_DEVICE_BY_INTR_CHAN(chan);
	__maybe_unused enum bt_hid_channel_type chtype;

	chtype = HID_CHAN_TYPE(chan);

	LOG_DBG("session %d connected (state %d)", chtype, hid->state);

	__ASSERT_NO_MSG(chtype == BT_HID_CHANNEL_INTR);

	/* HID spec v1.1.1 Section 5.2.2: both CTRL and INTR channels
	 * must be established for the HID connection to be valid.
	 */
	if (!hid->ctrl_connected) {
		LOG_WRN("CTRL not connected, reject INTR");
		bt_l2cap_chan_disconnect(chan);
		return;
	}

	hid->state = BT_HID_STATE_CONNECTED;
	hid->intr_connected = true;
	k_work_cancel_delayable(&hid->intr_timeout);

	if ((hid_cb != NULL) && (hid_cb->connected != NULL)) {
		hid_cb->connected(hid);
	}
}

static void bt_hid_l2cap_ctrl_disconnected(struct bt_l2cap_chan *chan)
{
	struct bt_hid_device *hid = HID_DEVICE_BY_CTRL_CHAN(chan);
	__maybe_unused enum bt_hid_channel_type chtype;

	chtype = HID_CHAN_TYPE(chan);

	LOG_DBG("session %d disconnected (state %d)", chtype, hid->state);

	__ASSERT_NO_MSG(chtype == BT_HID_CHANNEL_CTRL);

	hid->ctrl_connected = false;

	/* HID spec v1.1.2 Section 5.2.2: INTR shall be disconnected before
	 * CTRL. If remote disconnects CTRL first, clean up INTR.
	 */
	if (hid->intr_connected) {
		int err = bt_l2cap_chan_disconnect(&hid->intr_session.br_chan.chan);

		if ((err != 0) && (err != -EALREADY)) {
			LOG_WRN("INTR disconnect failed (%d)", err);
		}
		return;
	}

	if ((hid_cb != NULL) && (hid_cb->disconnected != NULL)) {
		hid_cb->disconnected(hid);
	}

	bt_hid_device_cleanup(hid);
}

static void bt_hid_l2cap_intr_disconnected(struct bt_l2cap_chan *chan)
{
	struct bt_hid_device *hid = HID_DEVICE_BY_INTR_CHAN(chan);
	__maybe_unused enum bt_hid_channel_type chtype;

	chtype = HID_CHAN_TYPE(chan);

	LOG_DBG("session %d disconnected (state %d)", chtype, hid->state);

	__ASSERT_NO_MSG(chtype == BT_HID_CHANNEL_INTR);

	hid->intr_connected = false;

	/* Local disconnect of INTR must be followed by disconnecting CTRL. */
	if (hid->state == BT_HID_STATE_DISCONNECTING && hid->ctrl_connected) {
		int err = bt_l2cap_chan_disconnect(&hid->ctrl_session.br_chan.chan);

		if ((err != 0) && (err != -EALREADY)) {
			LOG_WRN("CTRL disconnect failed (%d)", err);
		}

		return;
	}

	/* Wait for remote to disconnect CTRL. */
	if (hid->ctrl_connected) {
		hid->state = BT_HID_STATE_CTRL_CONNECTED;
		LOG_DBG("wait for remote CTRL disconnect");
		return;
	}

	if ((hid_cb != NULL) && (hid_cb->disconnected != NULL)) {
		hid_cb->disconnected(hid);
	}

	bt_hid_device_cleanup(hid);
}

static int bt_hid_l2cap_ctrl_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_hid_hdr *hdr;
	struct bt_hid_device *hid = HID_DEVICE_BY_CTRL_CHAN(chan);
	uint8_t type;
	uint8_t param;

	__ASSERT_NO_MSG(HID_CHAN_TYPE(chan) == BT_HID_CHANNEL_CTRL);

	/* HID spec v1.1.2 Section 5.2.2: once the Control channel is open the
	 * host may send Control channel commands; the Interrupt channel need
	 * not be established for them. Drop anything received before the
	 * Control channel itself is connected.
	 */
	if (hid->state < BT_HID_STATE_CTRL_CONNECTED) {
		LOG_WRN("CTRL recv in state %d, ignoring", hid->state);
		return 0;
	}

	if (buf->len < sizeof(*hdr)) {
		LOG_ERR("CTRL buf len %u invalid", buf->len);
		hid_send_handshake(hid, BT_HID_HS_RSP_ERR_INVALID_PARAM);
		return 0;
	}

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	param = BT_HID_HDR_GET_PARAM(hdr->header);
	type = BT_HID_HDR_GET_TYPE(hdr->header);

	LOG_DBG("CTRL recv type 0x%x param 0x%x", type, param);

	ARRAY_FOR_EACH(hid_ctrl_handlers, i) {
		if (hid_ctrl_handlers[i].type == type) {
			int err = hid_ctrl_handlers[i].handler(hid, buf, param);

			/* HID_CONTROL has no handshake response (HID spec v1.1.2
			 * Section 3.2.1.3): errors are ignored. For every other
			 * transaction an error handshake is sent here, so the
			 * handlers only need to return the result code. SUCCESS
			 * handshakes remain in the SET_* handlers, and the GET_*
			 * handlers send their own DATA response.
			 */
			if ((type != BT_HID_MSG_TYPE_CONTROL) && (err != 0)) {
				hid_send_handshake(hid, hid_err_to_handshake(err));
			}

			return 0;
		}
	}

	LOG_WRN("type %u not supported", type);
	hid_send_handshake(hid, BT_HID_HS_RSP_ERR_UNSUPPORTED_REQ);

	return 0;
}

static int bt_hid_l2cap_intr_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_hid_hdr *hdr;
	struct bt_hid_device *hid = HID_DEVICE_BY_INTR_CHAN(chan);
	uint8_t type;
	uint8_t param;
	uint8_t report_type;

	if (buf->len < sizeof(*hdr)) {
		LOG_ERR("INTR buf len %u invalid", buf->len);
		return 0;
	}

	__ASSERT_NO_MSG(HID_CHAN_TYPE(chan) == BT_HID_CHANNEL_INTR);

	if (hid->state != BT_HID_STATE_CONNECTED) {
		LOG_WRN("INTR recv in state %d, ignoring", hid->state);
		return 0;
	}

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	type = BT_HID_HDR_GET_TYPE(hdr->header);
	param = BT_HID_HDR_GET_PARAM(hdr->header);

	LOG_DBG("INTR recv type 0x%x param 0x%x", type, param);

	if (type != BT_HID_MSG_TYPE_DATA) {
		LOG_WRN("INTR type %u not supported", type);
		return 0;
	}

	/* HID spec v1.1.2 Section 7.4.3: only Output reports are sent from the
	 * host to the device on the Interrupt channel. The Interrupt channel has
	 * no handshake mechanism, so a non-Output report is silently dropped.
	 */
	report_type = FIELD_GET(BT_HID_PARAM_REPORT_TYPE_MASK, param);
	if (report_type != BT_HID_PAR_REP_TYPE_OUTPUT) {
		LOG_WRN("INTR DATA report type %u not Output, ignoring", report_type);
		return 0;
	}

	return hid_intr_handle(hid, buf);
}

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

static void bt_hid_session_init(struct bt_hid_device *hid, enum bt_hid_role role)
{
	hid->ctrl_session.br_chan.chan.ops = &ctrl_ops;
	hid->ctrl_session.br_chan.rx.mtu = BT_L2CAP_RX_MTU;
	hid->ctrl_session.type = BT_HID_CHANNEL_CTRL;

	hid->intr_session.br_chan.chan.ops = &intr_ops;
	hid->intr_session.br_chan.rx.mtu = BT_L2CAP_RX_MTU;
	hid->intr_session.type = BT_HID_CHANNEL_INTR;

	hid->role = role;
	hid->boot_mode = false;
	hid->ctrl_connected = false;
	hid->intr_connected = false;

	k_work_init_delayable(&hid->intr_timeout, hid_intr_timeout_handler);
	k_work_init(&hid->vcu_disconnect, hid_vcu_disconnect_work);
}

static void bt_hid_device_cleanup(struct bt_hid_device *hid)
{
	k_work_cancel_delayable(&hid->intr_timeout);
	k_work_cancel(&hid->vcu_disconnect);

	/* HID spec v1.1.2 Section 2.1.2: default protocol mode is Report
	 * Protocol Mode. Reset on disconnect for next connection.
	 */
	hid->boot_mode = false;
	hid->ctrl_connected = false;
	hid->intr_connected = false;
	hid->state = BT_HID_STATE_DISCONNECTED;
}

static int hid_l2cap_ctrl_accept(struct bt_conn *conn, struct bt_l2cap_server *server,
				 struct bt_l2cap_chan **chan)
{
	struct bt_hid_device *hid;

	hid = hid_allocate();
	if (hid == NULL) {
		return -EBUSY;
	}

	bt_hid_session_init(hid, BT_HID_ROLE_ACCEPTOR);
	hid->state = BT_HID_STATE_CTRL_CONNECTING;

	*chan = &hid->ctrl_session.br_chan.chan;
	return 0;
}

static int hid_l2cap_intr_accept(struct bt_conn *conn, struct bt_l2cap_server *server,
				 struct bt_l2cap_chan **chan)
{
	struct bt_hid_device *hid;

	/* HID spec v1.1.2 Section 5.2.1: the control channel must be fully
	 * established (and belong to this conn) before the interrupt channel.
	 */
	hid = hid_find(conn, BT_HID_STATE_CTRL_CONNECTED);
	if (hid == NULL) {
		LOG_ERR("CTRL channel not connected");
		return -ENOTCONN;
	}

	hid->state = BT_HID_STATE_INTR_CONNECTING;

	*chan = &hid->intr_session.br_chan.chan;
	return 0;
}

static struct bt_l2cap_server hid_server_ctrl = {
	.psm = BT_L2CAP_PSM_HID_CTRL,
	.sec_level = BT_SECURITY_L2,
	.accept = hid_l2cap_ctrl_accept,
};

static struct bt_l2cap_server hid_server_intr = {
	.psm = BT_L2CAP_PSM_HID_INTR,
	.sec_level = BT_SECURITY_L2,
	.accept = hid_l2cap_intr_accept,
};

static int bt_hid_device_init(void)
{
	int err;

	LOG_DBG("init");

	err = bt_l2cap_br_server_register(&hid_server_ctrl);
	if (err < 0) {
		LOG_ERR("CTRL L2CAP register failed (%d)", err);
		return err;
	}

	err = bt_l2cap_br_server_register(&hid_server_intr);
	if (err < 0) {
		LOG_ERR("INTR L2CAP register failed (%d)", err);
		bt_l2cap_br_server_unregister(&hid_server_ctrl);
		return err;
	}

	return 0;
}

static void bt_hid_device_deinit(void)
{
	int err;

	LOG_DBG("deinit");

	err = bt_l2cap_br_server_unregister(&hid_server_ctrl);
	if (err < 0) {
		LOG_ERR("CTRL L2CAP unregister failed (%d)", err);
	}

	err = bt_l2cap_br_server_unregister(&hid_server_intr);
	if (err < 0) {
		LOG_ERR("INTR L2CAP unregister failed (%d)", err);
	}
}

int bt_hid_device_register(const struct bt_hid_device_cb *cb)
{
	int err;

	LOG_DBG("register");

	/* GET_REPORT and SET_REPORT are mandatory HID Device transactions
	 * (HID Profile spec v1.1.2), so both callbacks are required.
	 */
	if ((cb == NULL) || (cb->get_report == NULL) || (cb->set_report == NULL)) {
		return -EINVAL;
	}

	if (hid_cb != NULL) {
		return -EALREADY;
	}

	if (!hid_l2cap_registered) {
		err = bt_hid_device_init();
		if (err < 0) {
			return err;
		}

		hid_l2cap_registered = true;
	}

	hid_cb = cb;
	return 0;
}

int bt_hid_device_unregister(void)
{
	LOG_DBG("unregister");

	if (hid_device_conn.state != BT_HID_STATE_DISCONNECTED) {
		LOG_ERR("cannot unregister while connected (state %d)",
			hid_device_conn.state);
		return -EBUSY;
	}

	hid_cb = NULL;

	if (hid_l2cap_registered) {
		bt_hid_device_deinit();
		hid_l2cap_registered = false;
	}

	return 0;
}

int bt_hid_device_connect(struct bt_conn *conn, struct bt_hid_device **hid)
{
	struct bt_hid_device *inst;
	int err;

	LOG_DBG("connect");

	if (conn == NULL || hid == NULL) {
		return -EINVAL;
	}

	inst = hid_allocate();
	if (inst == NULL) {
		LOG_ERR("busy (state %d)", hid_device_conn.state);
		return -EBUSY;
	}

	bt_hid_session_init(inst, BT_HID_ROLE_INITIATOR);

	inst->state = BT_HID_STATE_CTRL_CONNECTING;

	err = bt_l2cap_chan_connect(conn, &inst->ctrl_session.br_chan.chan, BT_L2CAP_PSM_HID_CTRL);
	if (err != 0) {
		LOG_ERR("connect failed (%d)", err);
		bt_hid_device_cleanup(inst);
		return err;
	}

	*hid = inst;
	return 0;
}

int bt_hid_device_disconnect(struct bt_hid_device *hid)
{
	int err;

	if (hid == NULL) {
		return -EINVAL;
	}

	LOG_DBG("disconnect");

	if (hid->state == BT_HID_STATE_DISCONNECTED ||
	    hid->state == BT_HID_STATE_DISCONNECTING) {
		LOG_ERR("not connected (state %d)", hid->state);
		return -ENOTCONN;
	}

	if (hid->intr_connected) {
		err = bt_l2cap_chan_disconnect(&hid->intr_session.br_chan.chan);
	} else if (hid->ctrl_connected) {
		err = bt_l2cap_chan_disconnect(&hid->ctrl_session.br_chan.chan);
	} else {
		return -ENOTCONN;
	}

	/* -EALREADY means the channel is already tearing down, which matches
	 * the intent of this call; only update state once the request is
	 * accepted so a failed disconnect leaves the state untouched.
	 */
	if ((err != 0) && (err != -EALREADY)) {
		LOG_ERR("chan disconnect failed (%d)", err);
		return err;
	}

	hid->state = BT_HID_STATE_DISCONNECTING;

	return 0;
}

int bt_hid_device_input_report(struct bt_hid_device *hid, struct net_buf *buf)
{
	struct bt_hid_hdr *hdr;
	int err;

	if (hid == NULL || buf == NULL) {
		return -EINVAL;
	}

	if (hid->state != BT_HID_STATE_CONNECTED) {
		LOG_ERR("not connected (state %d)", hid->state);
		return -ENOTCONN;
	}

	if (net_buf_headroom(buf) < sizeof(struct bt_hid_hdr)) {
		return -ENOMEM;
	}

	hdr = net_buf_push(buf, sizeof(struct bt_hid_hdr));
	hdr->header = BT_HID_BUILD_HDR(BT_HID_MSG_TYPE_DATA, BT_HID_REPORT_TYPE_INPUT);

	err = bt_l2cap_chan_send(&hid->intr_session.br_chan.chan, buf);
	if (err < 0) {
		LOG_ERR("L2CAP send failed (%d)", err);
		return err;
	}

	return 0;
}

static void virtual_cable_unplug_tx_cb(struct bt_conn *conn, void *user_data, int err)
{
	struct bt_hid_device *hid;

	if (conn == NULL || user_data == NULL) {
		return;
	}

	hid = (struct bt_hid_device *)user_data;

	/* VC unplug is a one-way teardown of the HID connection: disconnect
	 * regardless of the send result. If the PDU failed to reach the
	 * controller the host may not see the unplug, but the local link must
	 * still be torn down. Defer the disconnect to the system workqueue
	 * because this callback runs in the TX context.
	 */
	if (err != 0) {
		LOG_WRN("VCU control message send failed (%d)", err);
	}

	k_work_submit(&hid->vcu_disconnect);
}

int bt_hid_device_virtual_cable_unplug(struct bt_hid_device *hid)
{
	struct net_buf *buf;
	struct bt_hid_hdr *hdr;
	int err;

	LOG_DBG("virtual cable unplug");

	if (hid == NULL) {
		return -EINVAL;
	}

	if (hid->state != BT_HID_STATE_CONNECTED) {
		return -ENOTCONN;
	}

	buf = bt_hid_device_create_pdu(NULL);
	if (buf == NULL) {
		return -ENOBUFS;
	}

	hdr = net_buf_add(buf, sizeof(struct bt_hid_hdr));
	hdr->header = BT_HID_BUILD_HDR(BT_HID_MSG_TYPE_CONTROL,
				       BT_HID_CONTROL_VIRTUAL_CABLE_UNPLUG);

	/* Disconnect only after the VCU PDU has actually been sent: the
	 * tx callback tears down the L2CAP channel so the PDU is not lost to
	 * an early disconnect. If the remote host disconnects first
	 * (spec-correct behavior), cleanup cancels the pending work.
	 */
	err = bt_l2cap_br_chan_send_cb(&hid->ctrl_session.br_chan.chan, buf,
				       virtual_cable_unplug_tx_cb, hid);
	if (err < 0) {
		net_buf_unref(buf);
		return err;
	}

	/* HID spec v1.1.2 Section 3.1.2.2.3: the VCU initiator shall also
	 * destroy bonding/Virtual Cable info. This is intentionally left out
	 * for now for the same reason as in hid_control_handle() — see the
	 * note there: bt_br_unpair() would drop the shared ACL, and clearing
	 * the key without an ACL disconnect needs a new host API pending
	 * community agreement.
	 */

	return 0;
}
