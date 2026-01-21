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
#include <zephyr/bluetooth/classic/classic.h>
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

#define HID_CHAN_TYPE(ch)                                                                          \
	(((ch) == NULL) ? BT_HID_CHANNEL_UNKNOWN : HID_SESSION_BY_CHAN((ch))->type)

/* Timeout for INTR channel connection after CTRL is established (seconds) */
#define HID_INTR_CONN_TIMEOUT K_SECONDS(30)

/* HID device callback */
static const struct bt_hid_device_cb *hid_cb;

/* HID L2CAP servers registration state */
static bool hid_l2cap_registered;

/* HID device connection (single session only) */
static struct bt_hid_device hid_device_conn;

static int bt_hid_device_init(void);
static int bt_hid_device_deinit(void);
static void bt_hid_device_cleanup(struct bt_hid_device *hid);
static int hid_l2cap_ctrl_accept(struct bt_conn *conn, struct bt_l2cap_server *server,
				 struct bt_l2cap_chan **chan);
static int hid_l2cap_intr_accept(struct bt_conn *conn, struct bt_l2cap_server *server,
				 struct bt_l2cap_chan **chan);
static int hid_get_report_handle(struct bt_l2cap_chan *chan, struct net_buf *buf, uint8_t param);
static int hid_set_report_handle(struct bt_l2cap_chan *chan, struct net_buf *buf, uint8_t param);
static int hid_get_protocol_handle(struct bt_l2cap_chan *chan, struct net_buf *buf, uint8_t param);
static int hid_set_protocol_handle(struct bt_l2cap_chan *chan, struct net_buf *buf, uint8_t param);
static int hid_set_idle_handle(struct bt_l2cap_chan *chan, struct net_buf *buf, uint8_t param);

static struct bt_l2cap_server hid_server_ctrl = {
	.psm = BT_L2CAP_PSM_HID_CTL,
	.sec_level = BT_SECURITY_L2,
	.accept = hid_l2cap_ctrl_accept,
};
static struct bt_l2cap_server hid_server_intr = {
	.psm = BT_L2CAP_PSM_HID_INT,
	.sec_level = BT_SECURITY_L2,
	.accept = hid_l2cap_intr_accept,
};

static bool hid_conn_busy(const struct bt_conn *conn)
{
	return (hid_device_conn.conn != NULL) && (hid_device_conn.conn != conn);
}

static struct bt_hid_device *hid_get_connection(struct bt_conn *conn)
{
	struct bt_hid_device *hid;

	if (conn == NULL) {
		LOG_ERR("HID: invalid conn");
		return NULL;
	}

	if (hid_conn_busy(conn)) {
		return NULL;
	}

	hid = &hid_device_conn;
	if (hid->conn == NULL) {
		/* Slot is unused; avoid clobbering L2CAP internals. */
		hid->state = BT_HID_STATE_DISCONNECTED;
	}

	return hid;
}

struct net_buf *bt_hid_device_create_pdu(struct net_buf_pool *pool)
{
	return bt_conn_create_pdu(pool, sizeof(struct bt_l2cap_hdr) + sizeof(struct bt_hid_hdr));
}

static int hid_send_handshake(struct bt_hid_session *session, uint8_t param)
{
	struct net_buf *buf;
	struct bt_hid_hdr *hdr;
	int err;

	buf = bt_hid_device_create_pdu(NULL);
	if (buf == NULL) {
		return -ENOMEM;
	}

	if (net_buf_tailroom(buf) < sizeof(struct bt_hid_hdr)) {
		net_buf_unref(buf);
		return -ENOMEM;
	}

	hdr = net_buf_add(buf, sizeof(struct bt_hid_hdr));
	memset(hdr, 0, sizeof(struct bt_hid_hdr));
	hdr->header = BT_HID_BUILD_HDR(BT_HID_TRANS_HANDSHAKE, param);

	err = bt_l2cap_chan_send(&session->br_chan.chan, buf);
	if (err < 0) {
		LOG_ERR("HID: L2CAP send failed (%d)", err);
		net_buf_unref(buf);
		return err;
	}

	return 0;
}

static uint8_t hid_err_to_handshake(int err)
{
	switch (err) {
	case 0:
		return BT_HID_HANDSHAKE_RSP_SUCCESS;
	case -EOPNOTSUPP:
	case -ENOTSUP:
	case -ENOSYS:
		return BT_HID_HANDSHAKE_RSP_ERR_UNSUPPORTED_REQ;
	case -EINVAL:
		return BT_HID_HANDSHAKE_RSP_ERR_INVALID_PARAM;
	case -ENOENT:
		return BT_HID_HANDSHAKE_RSP_ERR_INVALID_REP_ID;
	case -EBUSY:
	case -EAGAIN:
		return BT_HID_HANDSHAKE_RSP_NOT_READY;
	default:
		return BT_HID_HANDSHAKE_RSP_ERR_UNKNOWN;
	}
}

static int hid_control_handle(struct bt_l2cap_chan *chan, struct net_buf *buf, uint8_t control)
{
	struct bt_hid_device *hid;

	if (chan == NULL) {
		LOG_ERR("HID: invalid channel");
		return -EINVAL;
	}

	hid = HID_DEVICE_BY_CTRL_CHAN(chan);

	switch (control) {
	case BT_HID_PAR_CONTROL_VIRTUAL_CABLE_UNPLUG:
		if ((hid_cb != NULL) && (hid_cb->vc_unplug != NULL)) {
			hid_cb->vc_unplug(hid);
		}

		/* HID spec v1.1.2 Section 3.1.2.2.3: recipient shall destroy
		 * all bonding and virtual cable information, then disconnect
		 * INTR followed by CTRL without sending HANDSHAKE.
		 */
		hid->suspended = false;

		bt_br_unpair(bt_conn_get_dst_br(hid->conn));
		bt_hid_device_disconnect(hid);
		break;
	case BT_HID_PAR_CONTROL_SUSPEND:
		if (!hid->suspended) {
			hid->suspended = true;
			if ((hid_cb != NULL) && (hid_cb->suspend != NULL)) {
				hid_cb->suspend(hid, true);
			}
		}
		break;
	case BT_HID_PAR_CONTROL_EXIT_SUSPEND:
		if (hid->suspended) {
			hid->suspended = false;
			if ((hid_cb != NULL) && (hid_cb->suspend != NULL)) {
				hid_cb->suspend(hid, false);
			}
		}
		break;
	default:
		/* HID spec v1.1.2 Section 3.1.2.2: unsupported HID_CONTROL
		 * operations shall be silently ignored.
		 */
		LOG_WRN("HID: control %u not handled, ignoring", control);
		break;
	}

	return 0;
}

typedef int (*hid_ctrl_handler_t)(struct bt_l2cap_chan *chan, struct net_buf *buf, uint8_t param);

static const struct {
	uint8_t type;
	hid_ctrl_handler_t handler;
} hid_ctrl_handlers[] = {
	{BT_HID_TRANS_CONTROL, hid_control_handle},
	{BT_HID_TRANS_GET_REPORT, hid_get_report_handle},
	{BT_HID_TRANS_SET_REPORT, hid_set_report_handle},
	{BT_HID_TRANS_GET_PROTOCOL, hid_get_protocol_handle},
	{BT_HID_TRANS_SET_PROTOCOL, hid_set_protocol_handle},
	{BT_HID_TRANS_SET_IDLE, hid_set_idle_handle},
};

static int hid_get_report_handle(struct bt_l2cap_chan *chan, struct net_buf *buf, uint8_t param)
{
	struct bt_hid_device *hid;
	uint8_t report_id;
	uint16_t buffer_size;
	uint8_t report_type;
	int err;

	if (chan == NULL) {
		LOG_ERR("HID: invalid channel");
		return -EINVAL;
	}

	hid = HID_DEVICE_BY_CTRL_CHAN(chan);

	if (buf->len > BT_HID_RX_MTU) {
		LOG_ERR("HID: Get_Report len %u > MTU %u", buf->len, BT_HID_RX_MTU);
		err = -EINVAL;
		goto err_handshake;
	}

	report_type = param & BT_HID_PARAM_REPORT_TYPE_MASK;

	/* HID spec v1.1.2 Section 3.1.2.4: GET_REPORT includes a Report ID
	 * when in Boot Protocol Mode or when the descriptor declares Report
	 * IDs (has_report_id).
	 */
	if (hid->boot_mode || hid->has_report_id) {
		if (buf->len < 1) {
			LOG_ERR("HID: Get_Report missing report ID");
			err = -EINVAL;
			goto err_handshake;
		}
		report_id = net_buf_pull_u8(buf);
	} else {
		report_id = 0;
	}

	if (param & BT_HID_PARAM_REPORT_SIZE_MASK) {
		if (buf->len < sizeof(buffer_size)) {
			LOG_ERR("HID: Get_Report len %u < 2", buf->len);
			err = -EINVAL;
			goto err_handshake;
		}

		buffer_size = net_buf_pull_le16(buf);
	} else {
		buffer_size = 0;
	}

	if ((hid_cb == NULL) || (hid_cb->get_report == NULL)) {
		err = -EOPNOTSUPP;
		goto err_handshake;
	}

	err = hid_cb->get_report(hid, report_type, report_id, buffer_size);
	if (err < 0) {
		LOG_ERR("HID: Get_Report cb err %d", err);
		goto err_handshake;
	}

	return 0;

err_handshake:
	hid_send_handshake(&hid->ctrl_session, hid_err_to_handshake(err));
	return err;
}

static int hid_set_report_handle(struct bt_l2cap_chan *chan, struct net_buf *buf, uint8_t param)
{
	struct bt_hid_device *hid;
	uint8_t report_type;
	uint8_t report_id;
	int err;

	if (chan == NULL) {
		LOG_ERR("HID: invalid channel");
		return -EINVAL;
	}

	hid = HID_DEVICE_BY_CTRL_CHAN(chan);

	if (buf->len > BT_HID_RX_MTU) {
		LOG_ERR("HID: Set_Report len %u > MTU %u", buf->len, BT_HID_RX_MTU);
		err = -EINVAL;
		goto err_handshake;
	}

	report_type = param & BT_HID_PARAM_REPORT_TYPE_MASK;

	/* HID spec v1.1.2 Section 3.1.2.5: SET_REPORT payload starts with
	 * Report ID when in Boot Protocol Mode or when the descriptor
	 * declares Report IDs.
	 */
	if (hid->boot_mode || hid->has_report_id) {
		if (buf->len < 1) {
			LOG_ERR("HID: Set_Report missing report ID");
			err = -EINVAL;
			goto err_handshake;
		}
		report_id = net_buf_pull_u8(buf);
	} else {
		report_id = 0;
	}

	if ((hid_cb == NULL) || (hid_cb->set_report == NULL)) {
		err = -EOPNOTSUPP;
		goto err_handshake;
	}

	err = hid_cb->set_report(hid, report_type, report_id, buf);
	if (err < 0) {
		LOG_ERR("HID: Set_Report cb err %d", err);
		goto err_handshake;
	}

	hid_send_handshake(&hid->ctrl_session, BT_HID_HANDSHAKE_RSP_SUCCESS);
	return 0;

err_handshake:
	hid_send_handshake(&hid->ctrl_session, hid_err_to_handshake(err));
	return err;
}

static int hid_set_idle_handle(struct bt_l2cap_chan *chan, struct net_buf *buf, uint8_t param)
{
	struct bt_hid_device *hid;
	uint8_t idle_rate;

	if (chan == NULL) {
		return -EINVAL;
	}

	hid = HID_DEVICE_BY_CTRL_CHAN(chan);

	/* HID spec v1.1.2 Section 3.1.2.7: SET_IDLE carries the Idle Rate
	 * in the payload byte. Bluetooth HID Hosts shall only send idle
	 * rate 0. Accept 0 for legacy compatibility, reject non-zero.
	 */
	if (buf->len < 1) {
		hid_send_handshake(&hid->ctrl_session,
				   BT_HID_HANDSHAKE_RSP_ERR_INVALID_PARAM);
		return -EINVAL;
	}

	idle_rate = net_buf_pull_u8(buf);
	if (idle_rate != 0) {
		hid_send_handshake(&hid->ctrl_session,
				   BT_HID_HANDSHAKE_RSP_ERR_INVALID_PARAM);
		return -EINVAL;
	}

	hid_send_handshake(&hid->ctrl_session, BT_HID_HANDSHAKE_RSP_SUCCESS);
	return 0;
}

static int hid_get_protocol_handle(struct bt_l2cap_chan *chan, struct net_buf *buf, uint8_t param)
{
	struct bt_hid_device *hid;
	int err;

	if (chan == NULL) {
		LOG_ERR("HID: invalid channel");
		return -EINVAL;
	}

	hid = HID_DEVICE_BY_CTRL_CHAN(chan);

	/* HID spec v1.1.2 Section 3.1.2.6: GET_PROTOCOL is only valid
	 * for devices that support Boot Protocol.
	 */
	if (!hid->boot_device) {
		err = -EOPNOTSUPP;
		goto err_handshake;
	}

	if ((hid_cb == NULL) || (hid_cb->get_protocol == NULL)) {
		err = -EOPNOTSUPP;
		goto err_handshake;
	}

	err = hid_cb->get_protocol(hid);
	if (err < 0) {
		LOG_ERR("HID: Get_Protocol cb err %d", err);
		goto err_handshake;
	}

	return 0;

err_handshake:
	hid_send_handshake(&hid->ctrl_session, hid_err_to_handshake(err));
	return err;
}

static int hid_set_protocol_handle(struct bt_l2cap_chan *chan, struct net_buf *buf, uint8_t param)
{
	struct bt_hid_device *hid;
	uint8_t protocol;
	int err;

	if (chan == NULL) {
		LOG_ERR("HID: invalid channel");
		return -EINVAL;
	}

	hid = HID_DEVICE_BY_CTRL_CHAN(chan);

	/* HID spec v1.1.2 Section 3.1.2.6: SET_PROTOCOL is only valid
	 * for devices that support Boot Protocol.
	 */
	if (!hid->boot_device) {
		err = -EOPNOTSUPP;
		goto err_handshake;
	}

	protocol = param & BT_HID_PROTOCOL_MASK;
	if ((hid_cb == NULL) || (hid_cb->set_protocol == NULL)) {
		err = -EOPNOTSUPP;
		goto err_handshake;
	}

	err = hid_cb->set_protocol(hid, protocol);
	if (err < 0) {
		LOG_ERR("HID: Set_Protocol cb err %d", err);
		goto err_handshake;
	}

	hid->boot_mode = (protocol == BT_HID_PROTOCOL_BOOT_MODE);
	hid_send_handshake(&hid->ctrl_session, BT_HID_HANDSHAKE_RSP_SUCCESS);
	return 0;

err_handshake:
	hid_send_handshake(&hid->ctrl_session, hid_err_to_handshake(err));
	return err;
}

static int hid_intr_handle(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_hid_device *hid;
	uint8_t report_id;

	if (chan == NULL) {
		LOG_ERR("HID: invalid channel");
		return -EINVAL;
	}

	hid = HID_DEVICE_BY_INTR_CHAN(chan);

	if (buf->len > BT_HID_RX_MTU) {
		LOG_ERR("HID: INTR len %u > MTU %u", buf->len, BT_HID_RX_MTU);
		return 0;
	}

	/* HID spec v1.1.2 Section 3.1.2.8: DATA on the interrupt channel
	 * includes a Report ID in Boot Protocol Mode or when the descriptor
	 * declares Report IDs.
	 */
	if (hid->boot_mode || hid->has_report_id) {
		if (buf->len < 1) {
			LOG_ERR("HID: INTR missing report ID");
			return 0;
		}
		report_id = net_buf_pull_u8(buf);
	} else {
		report_id = 0;
	}

	if ((hid_cb != NULL) && (hid_cb->intr_data != NULL)) {
		hid_cb->intr_data(hid, report_id, buf);
	}

	return 0;
}

static void hid_intr_timeout_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct bt_hid_device *hid = CONTAINER_OF(dwork, struct bt_hid_device, intr_timeout);

	LOG_WRN("HID: INTR connection timeout");

	if (hid->state == BT_HID_STATE_CTRL_CONNECTED) {
		bt_l2cap_chan_disconnect(&hid->ctrl_session.br_chan.chan);
	}
}

static void bt_hid_l2cap_ctrl_connected(struct bt_l2cap_chan *chan)
{
	struct bt_hid_device *hid;
	enum bt_hid_channel_type chtype;
	int err;

	if (chan == NULL) {
		LOG_ERR("HID: invalid channel");
		return;
	}

	hid = HID_DEVICE_BY_CTRL_CHAN(chan);

	chtype = HID_CHAN_TYPE(chan);

	LOG_DBG("HID: session %d connected (state %d)", chtype, hid->state);

	if (chtype != BT_HID_CHANNEL_CTRL) {
		LOG_ERR("HID: invalid channel type %d", chtype);
		return;
	}

	hid->state = BT_HID_STATE_CTRL_CONNECTED;

	if (hid->role == BT_HID_ROLE_ACCEPTOR) {
		/* Wait for INTR channel connection from remote */
		LOG_DBG("HID: wait for INTR connection");
		k_work_schedule(&hid->intr_timeout, HID_INTR_CONN_TIMEOUT);
		return;
	}

	err = bt_l2cap_chan_connect(hid->conn, &hid->intr_session.br_chan.chan,
				    BT_L2CAP_PSM_HID_INT);
	if (err) {
		LOG_ERR("HID: INTR connect failed");
		hid->state = BT_HID_STATE_DISCONNECTING;
		bt_l2cap_chan_disconnect(&hid->ctrl_session.br_chan.chan);
		return;
	}
}

static void bt_hid_l2cap_intr_connected(struct bt_l2cap_chan *chan)
{
	struct bt_hid_device *hid;
	enum bt_hid_channel_type chtype;

	if (chan == NULL) {
		LOG_ERR("HID: invalid channel");
		return;
	}

	hid = HID_DEVICE_BY_INTR_CHAN(chan);

	chtype = HID_CHAN_TYPE(chan);

	LOG_DBG("HID: session %d connected (state %d)", chtype, hid->state);

	if (chtype != BT_HID_CHANNEL_INTR) {
		LOG_ERR("HID: invalid channel type %d", chtype);
		return;
	}

	hid->state = BT_HID_STATE_CONNECTED;
	k_work_cancel_delayable(&hid->intr_timeout);

	if ((hid_cb != NULL) && (hid_cb->connected != NULL)) {
		hid_cb->connected(hid);
	}
}

static void bt_hid_l2cap_ctrl_disconnected(struct bt_l2cap_chan *chan)
{
	struct bt_hid_device *hid;
	enum bt_hid_channel_type chtype;

	if (chan == NULL) {
		LOG_ERR("HID: invalid channel");
		return;
	}

	hid = HID_DEVICE_BY_CTRL_CHAN(chan);

	chtype = HID_CHAN_TYPE(chan);

	LOG_DBG("HID: session %d disconnected (state %d)", chtype, hid->state);

	if (chtype != BT_HID_CHANNEL_CTRL) {
		LOG_ERR("HID: invalid channel type %d", chtype);
		return;
	}

	/* HID spec v1.1.2 Section 5.2.2: INTR shall be disconnected before
	 * CTRL. If remote disconnects CTRL first, clean up INTR. The spec
	 * allows terminating at baseband level without ordered L2CAP teardown.
	 */
	if (hid->intr_session.br_chan.chan.conn) {
		LOG_DBG("HID: disconnect INTR channel");
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
	enum bt_hid_channel_type chtype;

	if (chan == NULL) {
		LOG_ERR("HID: invalid channel");
		return;
	}

	hid = HID_DEVICE_BY_INTR_CHAN(chan);

	chtype = HID_CHAN_TYPE(chan);

	LOG_DBG("HID: session %d disconnected (state %d)", chtype, hid->state);

	if (chtype != BT_HID_CHANNEL_INTR) {
		LOG_ERR("HID: invalid channel type %d", chtype);
		return;
	}

	/* Local request disconnect(INTR channel), and it need to disconnect CTRL channel as well */
	if (hid->state == BT_HID_STATE_DISCONNECTING) {
		LOG_DBG("HID: disconnect CTRL channel");
		bt_l2cap_chan_disconnect(&hid->ctrl_session.br_chan.chan);
		return;
	}

	/* Wait for remote disconnect CTRL channel */
	if (hid->ctrl_session.br_chan.chan.conn) {
		LOG_DBG("HID: wait for remote CTRL disconnect");
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
	struct bt_hid_device *hid;
	uint8_t type;
	uint8_t param;

	if (chan == NULL) {
		LOG_ERR("HID: invalid channel");
		return -EIO;
	}

	hid = HID_DEVICE_BY_CTRL_CHAN(chan);

	if (HID_CHAN_TYPE(chan) != BT_HID_CHANNEL_CTRL) {
		LOG_ERR("HID: invalid role for CTRL channel");
		return -EIO;
	}

	/* HID spec v1.1.2 Section 5.2.1: both CTRL and INTR channels must
	 * be established before HID transactions are valid.
	 * Exception: HID_CONTROL (Section 7.4.1) operates on the CTRL
	 * channel alone and must be accepted once CTRL is connected
	 * (DCT/BV-08-C, DCT/BV-09-C).
	 */
	if (hid->state < BT_HID_STATE_CTRL_CONNECTED) {
		LOG_WRN("HID: CTRL recv in state %d, ignoring", hid->state);
		return 0;
	}

	if (buf->len < sizeof(*hdr) || (buf->len - sizeof(*hdr) > BT_HID_RX_MTU)) {
		LOG_ERR("HID: CTRL buf len %u invalid", buf->len);
		hid_send_handshake(&hid->ctrl_session, BT_HID_HANDSHAKE_RSP_ERR_INVALID_PARAM);
		return -EINVAL;
	}

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	param = BT_HID_GET_PARAM_FROM_HDR(hdr->header);
	type = BT_HID_GET_TRANS_FROM_HDR(hdr->header);

	LOG_DBG("HID: CTRL recv type 0x%x param 0x%x", type, param);

	/* Non-HID_CONTROL transactions require fully connected state */
	if (type != BT_HID_TRANS_CONTROL && hid->state != BT_HID_STATE_CONNECTED) {
		LOG_WRN("HID: transaction 0x%x rejected in state %d", type, hid->state);
		hid_send_handshake(HID_SESSION_BY_CHAN(chan),
				   BT_HID_HANDSHAKE_RSP_ERR_INVALID_PARAM);
		return 0;
	}

	for (size_t i = 0; i < ARRAY_SIZE(hid_ctrl_handlers); i++) {
		if (hid_ctrl_handlers[i].type == type) {
			hid_ctrl_handlers[i].handler(chan, buf, param);
			return 0;
		}
	}

	LOG_ERR("HID: type %u not supported", type);
	hid_send_handshake(HID_SESSION_BY_CHAN(chan), BT_HID_HANDSHAKE_RSP_ERR_UNSUPPORTED_REQ);

	return 0;
}

static int bt_hid_l2cap_intr_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_hid_hdr *hdr;
	struct bt_hid_device *hid;
	uint8_t type;

	if (chan == NULL) {
		LOG_ERR("HID: invalid channel");
		return -EINVAL;
	}

	hid = HID_DEVICE_BY_INTR_CHAN(chan);

	if ((buf->len < sizeof(*hdr)) || (buf->len - sizeof(*hdr) > BT_HID_RX_MTU)) {
		LOG_ERR("HID: INTR buf len %u invalid", buf->len);
		return 0;
	}

	if (HID_CHAN_TYPE(chan) != BT_HID_CHANNEL_INTR) {
		LOG_ERR("HID: invalid role for INTR channel");
		return -EIO;
	}

	if (hid->state != BT_HID_STATE_CONNECTED) {
		LOG_WRN("HID: INTR recv in state %d, ignoring", hid->state);
		return 0;
	}

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	type = BT_HID_GET_TRANS_FROM_HDR(hdr->header);

	LOG_DBG("HID: INTR recv type 0x%x param 0x%x", type,
		BT_HID_GET_PARAM_FROM_HDR(hdr->header));

	if (type != BT_HID_TRANS_DATA) {
		LOG_ERR("HID: INTR type %u not supported", type);
		return 0;
	}

	return hid_intr_handle(chan, buf);
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

static void bt_hid_session_init(struct bt_hid_device *hid, struct bt_conn *conn,
				enum bt_hid_role role)
{
	hid->ctrl_session.br_chan.chan.ops = &ctrl_ops;
	hid->ctrl_session.br_chan.rx.mtu = BT_HID_RX_MTU;
	hid->ctrl_session.type = BT_HID_CHANNEL_CTRL;

	hid->intr_session.br_chan.chan.ops = &intr_ops;
	hid->intr_session.br_chan.rx.mtu = BT_HID_RX_MTU;
	hid->intr_session.type = BT_HID_CHANNEL_INTR;

	hid->role = role;
	hid->conn = bt_conn_ref(conn);
	hid->boot_mode = false;
	hid->has_report_id = false;
	hid->boot_device = false;

	/* Do NOT reset hid->suspended here: DRE/BV-09-C requires suspend
	 * state to persist across disconnect/reconnect (HID spec v1.1.2
	 * Section 3.1.2.2.2). It is cleared by VC unplug and init-time
	 * zero-initialization only.
	 */

	k_work_init_delayable(&hid->intr_timeout, hid_intr_timeout_handler);
}

static void bt_hid_device_cleanup(struct bt_hid_device *hid)
{
	k_work_cancel_delayable(&hid->intr_timeout);

	if (hid->conn != NULL) {
		bt_conn_unref(hid->conn);
		hid->conn = NULL;
	}

	/* HID spec v1.1.2 Section 2.1.2: default protocol mode is Report
	 * Protocol Mode. Reset on disconnect for next connection.
	 */
	hid->boot_mode = false;
	hid->has_report_id = false;

	/* HID spec v1.1.2 Section 3.1.2.2.2 (DRE/BV-09-C): suspend state
	 * persists across disconnect/reconnect when Virtual Cable is
	 * established.  Do NOT reset hid->suspended here.
	 */

	hid->state = BT_HID_STATE_DISCONNECTED;
}

static int hid_l2cap_ctrl_accept(struct bt_conn *conn, struct bt_l2cap_server *server,
				 struct bt_l2cap_chan **chan)
{
	struct bt_hid_device *hid;

	hid = hid_get_connection(conn);
	if (hid == NULL) {
		return -EBUSY;
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
		return -EBUSY;
	}

	if (hid->ctrl_session.br_chan.chan.conn != conn) {
		LOG_ERR("HID: CTRL channel not connected");
		return -ENOTCONN;
	}

	/* HID spec v1.1.2 Section 5.2.1: The control channel must be
	 * fully established before the interrupt channel connection.
	 */
	if (hid->state != BT_HID_STATE_CTRL_CONNECTED) {
		LOG_ERR("HID: unexpected state %d for INTR accept", hid->state);
		return -ENOTCONN;
	}

	hid->state = BT_HID_STATE_INTR_CONNECTING;

	*chan = &hid->intr_session.br_chan.chan;
	return 0;
}

int bt_hid_device_register(const struct bt_hid_device_cb *cb)
{
	int err;

	LOG_DBG("HID: register");

	if (cb == NULL) {
		return -EINVAL;
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

void bt_hid_device_set_report_id(struct bt_hid_device *hid, bool has_id)
{
	__ASSERT_NO_MSG(hid);
	hid->has_report_id = has_id;
}

void bt_hid_device_set_boot_device(struct bt_hid_device *hid, bool boot_device)
{
	__ASSERT_NO_MSG(hid);
	hid->boot_device = boot_device;
}

int bt_hid_device_unregister(void)
{
	LOG_DBG("HID: unregister");

	if (hid_device_conn.state != BT_HID_STATE_DISCONNECTED) {
		if (hid_device_conn.intr_session.br_chan.chan.conn) {
			bt_l2cap_chan_disconnect(&hid_device_conn.intr_session.br_chan.chan);
		}

		if (hid_device_conn.ctrl_session.br_chan.chan.conn) {
			bt_l2cap_chan_disconnect(&hid_device_conn.ctrl_session.br_chan.chan);
		}

		hid_device_conn.state = BT_HID_STATE_DISCONNECTING;
	}

	hid_cb = NULL;

	if (hid_l2cap_registered) {
		bt_hid_device_deinit();
		hid_l2cap_registered = false;
	}

	return 0;
}

int bt_hid_device_connect(struct bt_conn *conn, struct bt_hid_device **hid_out)
{
	struct bt_hid_device *hid;
	int err;

	hid = hid_get_connection(conn);
	if (hid == NULL) {
		return (conn == NULL) ? -EINVAL : -EBUSY;
	}

	if (hid->state != BT_HID_STATE_DISCONNECTED) {
		LOG_ERR("HID: busy (state %d)", hid->state);
		return -EBUSY;
	}

	if ((hid->ctrl_session.br_chan.chan.conn != NULL) &&
	    (hid->ctrl_session.br_chan.chan.conn != conn)) {
		LOG_ERR("HID: CTRL channel bound to another connection");
		return -EALREADY;
	}

	if (hid->intr_session.br_chan.chan.conn != NULL) {
		LOG_ERR("HID: INTR channel already connected");
		return -EALREADY;
	}

	bt_hid_session_init(hid, conn, BT_HID_ROLE_INITIATOR);

	err = bt_l2cap_chan_connect(conn, &hid->ctrl_session.br_chan.chan, BT_L2CAP_PSM_HID_CTL);
	if (err != 0) {
		LOG_WRN("HID: connect failed (%d)", err);
		bt_hid_device_cleanup(hid);
		return err;
	}

	hid->state = BT_HID_STATE_CTRL_CONNECTING;
	*hid_out = hid;
	return 0;
}

int bt_hid_device_reconnect(struct bt_conn *conn, struct bt_hid_device **hid)
{
	/* TODO: HID spec v1.1.2 Appendix A.2 describes a full reconnect
	 * procedure including HIDReconnectInitiate SDP attribute validation,
	 * role switch handling, and reconnect timeout. Currently this reuses
	 * the connect path which only re-establishes L2CAP channels on an
	 * existing ACL connection.
	 */
	return bt_hid_device_connect(conn, hid);
}

int bt_hid_device_disconnect(struct bt_hid_device *hid)
{
	__ASSERT_NO_MSG(hid);

	if (hid->state == BT_HID_STATE_DISCONNECTED ||
	    hid->state == BT_HID_STATE_DISCONNECTING) {
		LOG_ERR("HID: not connected (state %d)", hid->state);
		return -ENOTCONN;
	}

	hid->state = BT_HID_STATE_DISCONNECTING;

	if (hid->intr_session.br_chan.chan.conn) {
		bt_l2cap_chan_disconnect(&hid->intr_session.br_chan.chan);
	} else if (hid->ctrl_session.br_chan.chan.conn) {
		bt_l2cap_chan_disconnect(&hid->ctrl_session.br_chan.chan);
	}

	return 0;
}

int bt_hid_device_get_report_response(struct bt_hid_device *hid, uint8_t type, struct net_buf *buf)
{
	struct bt_hid_hdr *hdr;
	int err;

	__ASSERT_NO_MSG(hid);

	if (hid->state != BT_HID_STATE_CONNECTED) {
		LOG_ERR("HID: not connected (state %d)", hid->state);
		return -ENOTCONN;
	}

	if (buf->len > BT_HID_TX_MTU) {
		LOG_ERR("HID: ctrl payload %u > MTU %u", buf->len, BT_HID_TX_MTU);
		return -EMSGSIZE;
	}

	if (net_buf_headroom(buf) < sizeof(struct bt_hid_hdr)) {
		return -ENOMEM;
	}

	hdr = net_buf_push(buf, sizeof(struct bt_hid_hdr));
	memset(hdr, 0, sizeof(struct bt_hid_hdr));
	hdr->header = BT_HID_BUILD_HDR(BT_HID_TRANS_DATA, type);

	err = bt_l2cap_chan_send(&hid->ctrl_session.br_chan.chan, buf);
	if (err < 0) {
		LOG_ERR("HID: L2CAP send failed (%d)", err);
		return err;
	}

	return 0;
}

int bt_hid_device_get_protocol_response(struct bt_hid_device *hid, uint8_t protocol)
{
	struct net_buf *buf;
	struct bt_hid_hdr *hdr;
	int err;

	__ASSERT_NO_MSG(hid);

	if (hid->state != BT_HID_STATE_CONNECTED) {
		LOG_ERR("HID: not connected (state %d)", hid->state);
		return -ENOTCONN;
	}

	buf = bt_hid_device_create_pdu(NULL);
	if (buf == NULL) {
		return -ENOMEM;
	}

	if (net_buf_tailroom(buf) < (sizeof(struct bt_hid_hdr) + sizeof(uint8_t))) {
		net_buf_unref(buf);
		return -ENOMEM;
	}

	hdr = net_buf_add(buf, sizeof(struct bt_hid_hdr));
	memset(hdr, 0, sizeof(struct bt_hid_hdr));
	hdr->header = BT_HID_BUILD_HDR(BT_HID_TRANS_DATA, BT_HID_REPORT_TYPE_OTHER);

	net_buf_add_u8(buf, protocol);

	err = bt_l2cap_chan_send(&hid->ctrl_session.br_chan.chan, buf);
	if (err < 0) {
		LOG_ERR("HID: L2CAP send failed (%d)", err);
		net_buf_unref(buf);
		return err;
	}

	return 0;
}

int bt_hid_device_send(struct bt_hid_device *hid, struct net_buf *buf)
{
	struct bt_hid_hdr *hdr;
	int err;

	__ASSERT_NO_MSG(hid);

	if (hid->state != BT_HID_STATE_CONNECTED) {
		LOG_ERR("HID: not connected (state %d)", hid->state);
		return -ENOTCONN;
	}

	if (buf->len > BT_HID_TX_MTU) {
		LOG_ERR("HID: intr payload %u > MTU %u", buf->len, BT_HID_TX_MTU);
		return -EMSGSIZE;
	}

	if (net_buf_headroom(buf) < sizeof(struct bt_hid_hdr)) {
		return -ENOMEM;
	}

	hdr = net_buf_push(buf, sizeof(struct bt_hid_hdr));
	memset(hdr, 0, sizeof(struct bt_hid_hdr));
	hdr->header = BT_HID_BUILD_HDR(BT_HID_TRANS_DATA, BT_HID_REPORT_TYPE_INPUT);

	err = bt_l2cap_chan_send(&hid->intr_session.br_chan.chan, buf);
	if (err < 0) {
		LOG_ERR("HID: L2CAP send failed (%d)", err);
		return err;
	}

	return 0;
}

int bt_hid_device_report_error(struct bt_hid_device *hid, uint8_t error)
{
	__ASSERT_NO_MSG(hid);

	if (hid->state != BT_HID_STATE_CONNECTED) {
		LOG_ERR("HID: not connected (state %d)", hid->state);
		return -ENOTCONN;
	}

	return hid_send_handshake(&hid->ctrl_session, error);
}

int bt_hid_device_virtual_cable_unplug(struct bt_hid_device *hid)
{
	struct net_buf *buf;
	struct bt_hid_hdr *hdr;
	int err;

	__ASSERT_NO_MSG(hid);

	if (hid->state != BT_HID_STATE_CONNECTED) {
		return -ENOTCONN;
	}

	buf = bt_hid_device_create_pdu(NULL);
	if (buf == NULL) {
		return -ENOMEM;
	}

	hdr = net_buf_add(buf, sizeof(struct bt_hid_hdr));
	hdr->header = BT_HID_BUILD_HDR(BT_HID_TRANS_CONTROL,
					BT_HID_PAR_CONTROL_VIRTUAL_CABLE_UNPLUG);

	err = bt_l2cap_chan_send(&hid->ctrl_session.br_chan.chan, buf);
	if (err < 0) {
		net_buf_unref(buf);
		return err;
	}

	/* HID spec v1.1.2 Section 3.1.2.2.3: initiator of VCU shall also
	 * destroy all bonding and virtual cable information.
	 *
	 * TODO: ideally a short delay before disconnect would allow the
	 * remote side to process the VCU message first.
	 */
	hid->suspended = false;
	bt_br_unpair(bt_conn_get_dst_br(hid->conn));

	return bt_hid_device_disconnect(hid);
}

static int bt_hid_device_init(void)
{
	int err;

	LOG_DBG("HID: init");

	/* Register HID CTRL PSM with L2CAP */
	err = bt_l2cap_br_server_register(&hid_server_ctrl);
	if (err < 0) {
		LOG_ERR("HID: CTRL L2CAP register failed (%d)", err);
		return err;
	}

	/* Register HID INTR PSM with L2CAP */
	err = bt_l2cap_br_server_register(&hid_server_intr);
	if (err < 0) {
		LOG_ERR("HID: INTR L2CAP register failed (%d)", err);
		return err;
	}

	return 0;
}

static int bt_hid_device_deinit(void)
{
	int err;

	LOG_DBG("HID: deinit");

	err = bt_l2cap_br_server_unregister(&hid_server_ctrl);
	if (err < 0) {
		LOG_ERR("HID: CTRL L2CAP unregister failed (%d)", err);
		return err;
	}

	err = bt_l2cap_br_server_unregister(&hid_server_intr);
	if (err < 0) {
		LOG_ERR("HID: INTR L2CAP unregister failed (%d)", err);
		return err;
	}

	return 0;
}
