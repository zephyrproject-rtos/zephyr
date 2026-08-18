/* hid_host.c - HID Profile - HID Host side handling */

/*
 * Copyright 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>

#include "common/assert.h"

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/classic/classic.h>
#include <zephyr/bluetooth/classic/hid_host.h>
#include <zephyr/bluetooth/l2cap.h>
#include "host/hci_core.h"
#include "host/conn_internal.h"
#include "host/l2cap_internal.h"

#include "hid_internal.h"

#define LOG_LEVEL CONFIG_BT_HID_HOST_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_hid_host);

/* Get the HID host from CTRL L2CAP channel */
#define HID_HOST_BY_CTRL_CHAN(ch) \
	CONTAINER_OF(ch, struct bt_hid_host, ctrl_session.br_chan.chan)

/* Get the HID host from INTR L2CAP channel */
#define HID_HOST_BY_INTR_CHAN(ch) \
	CONTAINER_OF(ch, struct bt_hid_host, intr_session.br_chan.chan)

/* Get the HID session from L2CAP channel */
#define HID_SESSION_BY_CHAN(ch) \
	CONTAINER_OF(ch, struct bt_hid_host_session, br_chan.chan)

#define HID_CHAN_TYPE(ch) \
	(((ch) == NULL) ? BT_HID_CHANNEL_UNKNOWN : HID_SESSION_BY_CHAN((ch))->type)

/* Timeout for INTR channel connection after CTRL is established (seconds) */
#define HID_INTR_CONN_TIMEOUT K_SECONDS(30)

/* Timeout for a Control channel transaction to be answered by the device.
 * HID spec v1.1.2 Section 5.2.6 notes that default supervisory timeouts are
 * typically 30 seconds, after which the connection to the device is considered
 * lost, so the same value is used to bound a pending transaction.
 */
#define HID_TRANS_TIMEOUT K_SECONDS(30)

/* HID host callback. Non-NULL exactly while the L2CAP servers are registered. */
static const struct bt_hid_host_cb *hid_cb;

/* HID host connections, indexed by ACL connection index */
static struct bt_hid_host hid_conn_pool[CONFIG_BT_MAX_CONN];

static const char *hid_state_str(uint8_t state)
{
	switch (state) {
	case BT_HID_STATE_DISCONNECTED:
		return "disconnected";
	case BT_HID_STATE_CTRL_CONNECTING:
		return "ctrl-connecting";
	case BT_HID_STATE_CTRL_CONNECTED:
		return "ctrl-connected";
	case BT_HID_STATE_INTR_CONNECTING:
		return "intr-connecting";
	case BT_HID_STATE_CONNECTED:
		return "connected";
	case BT_HID_STATE_DISCONNECTING:
		return "disconnecting";
	default:
		return "unknown";
	}
}

#if defined(CONFIG_BT_HID_HOST_LOG_LEVEL_DBG)
/* Single mutation point for the connection state. The transition is logged and
 * checked only in debug builds, and an unexpected one is reported but still
 * applied so a peer that deviates cannot wedge the state machine. HID spec
 * v1.1.2 Section 5.2.2 defines the ordering the table below encodes.
 */
static void hid_set_state_debug(struct bt_hid_host *hid, enum bt_hid_state state,
				const char *func, int line)
{
	LOG_DBG("hid %p %s -> %s", hid, hid_state_str(hid->state), hid_state_str(state));

	/* check transitions validness */
	switch (state) {
	case BT_HID_STATE_DISCONNECTED:
		/* Cleanup after teardown or link loss, allowed from any state. */
		break;
	case BT_HID_STATE_CTRL_CONNECTING:
		if (hid->state != BT_HID_STATE_DISCONNECTED) {
			LOG_WRN("%s()%d: invalid transition", func, line);
		}
		break;
	case BT_HID_STATE_CTRL_CONNECTED:
		if (hid->state != BT_HID_STATE_CTRL_CONNECTING) {
			LOG_WRN("%s()%d: invalid transition", func, line);
		}
		break;
	case BT_HID_STATE_INTR_CONNECTING:
		if (hid->state != BT_HID_STATE_CTRL_CONNECTED) {
			LOG_WRN("%s()%d: invalid transition", func, line);
		}
		break;
	case BT_HID_STATE_CONNECTED:
		if (hid->state != BT_HID_STATE_INTR_CONNECTING) {
			LOG_WRN("%s()%d: invalid transition", func, line);
		}
		break;
	case BT_HID_STATE_DISCONNECTING:
		/* A teardown spans both channels and re-enters this state once per
		 * channel, so only starting one on a released association is wrong.
		 */
		if (hid->state == BT_HID_STATE_DISCONNECTED) {
			LOG_WRN("%s()%d: invalid transition", func, line);
		}
		break;
	default:
		LOG_ERR("%s()%d: unknown (%u) state was set", func, line, state);
		return;
	}

	hid->state = state;
}
#define hid_set_state(_hid, _state) hid_set_state_debug(_hid, _state, __func__, __LINE__)
#else
static void hid_set_state(struct bt_hid_host *hid, enum bt_hid_state state)
{
	hid->state = state;
}
#endif /* CONFIG_BT_HID_HOST_LOG_LEVEL_DBG */

static struct bt_hid_host *hid_allocate(const struct bt_conn *conn)
{
	struct bt_hid_host *hid;
	size_t index;

	index = (size_t)bt_conn_index(conn);
	__ASSERT(index < ARRAY_SIZE(hid_conn_pool), "Index is out of bounds");

	hid = &hid_conn_pool[index];

	if (hid->state != BT_HID_STATE_DISCONNECTED) {
		LOG_ERR("busy (state %s)", hid_state_str(hid->state));
		return NULL;
	}

	return hid;
}

static struct bt_hid_host *hid_find(const struct bt_conn *conn)
{
	struct bt_hid_host *hid;
	size_t index;

	if (conn == NULL) {
		return NULL;
	}

	index = (size_t)bt_conn_index(conn);
	__ASSERT(index < ARRAY_SIZE(hid_conn_pool), "Index is out of bounds");

	hid = &hid_conn_pool[index];

	if (hid->state == BT_HID_STATE_DISCONNECTED) {
		return NULL;
	}

	return hid;
}

struct bt_conn *bt_hid_host_get_conn(struct bt_hid_host *hid)
{
	struct bt_conn *conn;

	/* A released instance still points at the connection it was last attached
	 * to, so the association has to be up before a reference is handed out.
	 */
	if ((hid == NULL) || (hid->state == BT_HID_STATE_DISCONNECTED)) {
		return NULL;
	}

	conn = hid->ctrl_session.br_chan.chan.conn;
	if (conn == NULL) {
		return NULL;
	}

	return bt_conn_ref(conn);
}

struct net_buf *bt_hid_host_create_pdu(struct net_buf_pool *pool)
{
	return bt_l2cap_create_pdu(pool, sizeof(struct bt_hid_hdr));
}

static const char *hid_w4_str(uint8_t type)
{
	switch (type) {
	case BT_HID_W4_NONE:
		return "none";
	case BT_HID_W4_GET_REPORT:
		return "get-report";
	case BT_HID_W4_SET_REPORT:
		return "set-report";
	case BT_HID_W4_GET_PROTOCOL:
		return "get-protocol";
	case BT_HID_W4_SET_PROTOCOL:
		return "set-protocol";
	default:
		return "unknown";
	}
}

/* Published while a claim is being filled in.
 *
 * A reply cannot arrive while this is the token because the PDU has not been
 * sent yet, so a Control channel PDU seen with it is unsolicited and dropped.
 * Same idea as the &cancel sentinel used by att.c.
 */
static struct bt_hid_req hid_req_claiming;

/* Claim the Control channel for a request.
 *
 * HID spec v1.1.2 Section 3.2.1 allows one outstanding transaction, so the claim
 * is also the serialization point. It is taken before the PDU goes out because
 * the reply is handled on the RX thread and may run as soon as the send returns,
 * and it is taken atomically because requests come from application threads
 * while the RX thread and the timeout release them.
 */
static int hid_req_claim(struct bt_hid_host *hid, const struct bt_hid_req *req)
{
	if (!atomic_ptr_cas(&hid->_req, NULL, &hid_req_claiming)) {
		LOG_ERR("transaction %s pending", hid_w4_str(hid->req.type));
		return -EBUSY;
	}

	hid->req = *req;

	/* The timer is armed only once the contents are in place, so the timeout
	 * can never observe a half-published transaction. The transaction and its
	 * timer are always updated together: the timer is the only thing that
	 * releases a transaction the device never answers, and a stale timer would
	 * fire during the next one.
	 */
	k_work_schedule(&hid->trans_work, HID_TRANS_TIMEOUT);
	(void)atomic_ptr_set(&hid->_req, &hid->req);

	LOG_DBG("transaction %s started", hid_w4_str(req->type));

	return 0;
}

/* Release a claim whose PDU never made it out, so a failed request does not
 * block the Control channel.
 */
static void hid_req_abort(struct bt_hid_host *hid)
{
	if (atomic_ptr_cas(&hid->_req, &hid->req, NULL)) {
		k_work_cancel_delayable(&hid->trans_work);
	}
}

/* Consume the outstanding transaction.
 *
 * Returns false when there is nothing to consume or when another context got
 * there first, in which case the caller must do nothing: the reply and the
 * timeout race for the same transaction and only one of them may act on it.
 */
static bool hid_req_take(struct bt_hid_host *hid, struct bt_hid_req *req)
{
	if (atomic_ptr_get(&hid->_req) != &hid->req) {
		return false;
	}

	*req = hid->req;

	if (!atomic_ptr_cas(&hid->_req, &hid->req, NULL)) {
		return false;
	}

	LOG_DBG("transaction %s done", hid_w4_str(req->type));
	k_work_cancel_delayable(&hid->trans_work);

	return true;
}

static void hid_intr_timeout_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct bt_hid_host *hid = CONTAINER_OF(dwork, struct bt_hid_host, intr_work);
	int err;

	/* HID spec v1.1.2 Section 5.2.2 requires both channels to be open: CTRL
	 * came up but the device never opened INTR, so the association cannot make
	 * progress and whatever is up is torn down.
	 */
	LOG_WRN("INTR connect timeout in state %s", hid_state_str(hid->state));

	err = bt_hid_host_disconnect(hid);
	if (err != 0) {
		LOG_WRN("timeout: disconnect failed (%d)", err);
	}
}

static void hid_trans_timeout_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct bt_hid_host *hid = CONTAINER_OF(dwork, struct bt_hid_host, trans_work);
	struct bt_hid_req req;
	int err;

	/* The transaction is taken here only to settle a race:
	 * k_work_cancel_delayable() cannot stop this work once it is queued, so it
	 * can still run after the reply was consumed or after the association was
	 * released. Losing the transaction means one of those happened and nothing
	 * timed out.
	 */
	if (!hid_req_take(hid, &req)) {
		return;
	}

	/* HID spec v1.1.2 Section 5.2.6: a transaction that is never answered means
	 * the connection to the device is lost, so the association is torn down and
	 * the outcome is reported through the disconnected callback rather than
	 * through the result callback of the request. Only one transaction can be
	 * outstanding (Section 3.2.1), so the application can attribute the loss
	 * without being told which request it was.
	 */
	LOG_WRN("transaction %s timed out in state %s", hid_w4_str(req.type),
		hid_state_str(hid->state));

	err = bt_hid_host_disconnect(hid);
	if (err != 0) {
		LOG_WRN("timeout: disconnect failed (%d)", err);
	}
}

static void hid_vcu_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct bt_hid_host *hid = CONTAINER_OF(dwork, struct bt_hid_host, vcu_work);
	int err;

	LOG_DBG("VCU disconnect");

	err = bt_hid_host_disconnect(hid);
	if (err != 0) {
		LOG_WRN("VCU disconnect failed (%d)", err);
	}
}

static void hid_l2cap_ctrl_connected(struct bt_l2cap_chan *chan)
{
	struct bt_hid_host *hid = HID_HOST_BY_CTRL_CHAN(chan);
	__maybe_unused enum bt_hid_channel_type chtype;
	int err;

	chtype = HID_CHAN_TYPE(chan);

	LOG_DBG("session %d connected (state %s)", chtype, hid_state_str(hid->state));

	__ASSERT_NO_MSG(chtype == BT_HID_CHANNEL_CTRL);

	hid_set_state(hid, BT_HID_STATE_CTRL_CONNECTED);
	hid->ctrl_connected = true;

	if (hid->role == BT_HID_ROLE_ACCEPTOR) {
		/* HID spec v1.1.2 Section 5.2.2: the device opens the Interrupt channel
		 * next, so the association is not reported to the application until that
		 * channel is up.
		 */
		LOG_DBG("wait for INTR connection");
		k_work_schedule(&hid->intr_work, HID_INTR_CONN_TIMEOUT);
		return;
	}

	err = bt_l2cap_chan_connect(chan->conn, &hid->intr_session.br_chan.chan,
				    BT_L2CAP_PSM_HID_INTR);
	if (err != 0) {
		LOG_ERR("INTR connect failed (%d)", err);
		hid_set_state(hid, BT_HID_STATE_DISCONNECTING);
		bt_l2cap_chan_disconnect(&hid->ctrl_session.br_chan.chan);
		return;
	}

	hid_set_state(hid, BT_HID_STATE_INTR_CONNECTING);
}

static void hid_l2cap_intr_connected(struct bt_l2cap_chan *chan)
{
	struct bt_hid_host *hid = HID_HOST_BY_INTR_CHAN(chan);
	__maybe_unused enum bt_hid_channel_type chtype;

	chtype = HID_CHAN_TYPE(chan);

	LOG_DBG("session %d connected (state %s)", chtype, hid_state_str(hid->state));

	__ASSERT_NO_MSG(chtype == BT_HID_CHANNEL_INTR);

	/* HID spec v1.1.2 Section 5.2.2: both CTRL and INTR channels
	 * must be established for the HID connection to be valid.
	 */
	if (!hid->ctrl_connected) {
		LOG_WRN("CTRL not connected, reject INTR");
		bt_l2cap_chan_disconnect(chan);
		return;
	}

	hid_set_state(hid, BT_HID_STATE_CONNECTED);
	hid->intr_connected = true;
	k_work_cancel_delayable(&hid->intr_work);

	if ((hid_cb != NULL) && (hid_cb->connected != NULL)) {
		hid_cb->connected(hid);
	}
}

static void hid_cleanup(struct bt_hid_host *hid)
{
	/* Releasing the instance only detaches it from the connection, so every
	 * caller has to have closed both channels first; otherwise L2CAP would keep
	 * them open with nothing left to dispatch to.
	 */
	__ASSERT(!hid->ctrl_connected && !hid->intr_connected,
		 "release with channels still open");

	k_work_cancel_delayable(&hid->intr_work);
	k_work_cancel_delayable(&hid->trans_work);
	k_work_cancel_delayable(&hid->vcu_work);

	hid->boot_mode = false;
	hid->suspended = false;
	hid->ctrl_connected = false;
	hid->intr_connected = false;
	atomic_ptr_clear(&hid->_req);
	hid_set_state(hid, BT_HID_STATE_DISCONNECTED);
}

static void hid_l2cap_ctrl_disconnected(struct bt_l2cap_chan *chan)
{
	struct bt_hid_host *hid = HID_HOST_BY_CTRL_CHAN(chan);
	__maybe_unused enum bt_hid_channel_type chtype;

	chtype = HID_CHAN_TYPE(chan);

	LOG_DBG("session %d disconnected (state %s)", chtype, hid_state_str(hid->state));

	__ASSERT_NO_MSG(chtype == BT_HID_CHANNEL_CTRL);

	hid->ctrl_connected = false;

	/* HID spec v1.1.2 Section 5.2.2: INTR shall be disconnected before CTRL, so
	 * the peer closing CTRL first leaves INTR to be cleaned up here.
	 */
	if (hid->intr_connected) {
		int err;

		hid_set_state(hid, BT_HID_STATE_DISCONNECTING);

		err = bt_l2cap_chan_disconnect(&hid->intr_session.br_chan.chan);
		if ((err != 0) && (err != -EALREADY)) {
			LOG_WRN("INTR disconnect failed (%d)", err);
		}

		return;
	}

	if ((hid_cb != NULL) && (hid_cb->disconnected != NULL)) {
		hid_cb->disconnected(hid);
	}

	hid_cleanup(hid);
}

static void hid_l2cap_intr_disconnected(struct bt_l2cap_chan *chan)
{
	struct bt_hid_host *hid = HID_HOST_BY_INTR_CHAN(chan);
	__maybe_unused enum bt_hid_channel_type chtype;

	chtype = HID_CHAN_TYPE(chan);

	LOG_DBG("session %d disconnected (state %s)", chtype, hid_state_str(hid->state));

	__ASSERT_NO_MSG(chtype == BT_HID_CHANNEL_INTR);

	hid->intr_connected = false;

	/* HID spec v1.1.2 Section 5.2.2: the Interrupt channel closes before the
	 * Control channel. Whichever side closed Interrupt, the association is over,
	 * so Control is closed from here instead of waiting for the peer to do it,
	 * which would leave the association unusable for an unbounded time.
	 */
	if (hid->ctrl_connected) {
		int err;

		hid_set_state(hid, BT_HID_STATE_DISCONNECTING);

		err = bt_l2cap_chan_disconnect(&hid->ctrl_session.br_chan.chan);
		if ((err != 0) && (err != -EALREADY)) {
			LOG_WRN("CTRL disconnect failed (%d)", err);
		}

		return;
	}

	if ((hid_cb != NULL) && (hid_cb->disconnected != NULL)) {
		hid_cb->disconnected(hid);
	}

	hid_cleanup(hid);
}

static int hid_handshake_handle(struct bt_hid_host *hid, struct net_buf *buf, uint8_t param)
{
	struct bt_hid_req req;

	ARG_UNUSED(buf);

	/* HID spec v1.1.2 Section 3.2.1: a HANDSHAKE only ever acknowledges a
	 * request issued by this host, so one that arrives without an outstanding
	 * transaction is dropped.
	 */
	if (!hid_req_take(hid, &req)) {
		LOG_DBG("Drop unexpected HANDSHAKE 0x%x", param);
		return 0;
	}

	switch (req.type) {
	case BT_HID_W4_GET_REPORT:
		/* HID spec v1.1.2 Section 3.2.1.1: a GET_REPORT the device refuses is
		 * answered with a HANDSHAKE, so there is no payload to hand up.
		 */
		if ((hid_cb != NULL) && (hid_cb->get_report != NULL)) {
			hid_cb->get_report(hid, param, req.report_type, NULL);
		}
		break;

	case BT_HID_W4_SET_REPORT:
		if ((hid_cb != NULL) && (hid_cb->set_report != NULL)) {
			hid_cb->set_report(hid, param);
		}
		break;

	case BT_HID_W4_GET_PROTOCOL:
		if ((hid_cb != NULL) && (hid_cb->get_protocol != NULL)) {
			hid_cb->get_protocol(hid, param,
					     hid->boot_mode ? BT_HID_PROTOCOL_BOOT_MODE
							    : BT_HID_PROTOCOL_REPORT_MODE);
		}
		break;

	case BT_HID_W4_SET_PROTOCOL:
		/* HID spec v1.1.2 Section 3.1.2.6: the device acknowledges SET_PROTOCOL
		 * with a HANDSHAKE, so the new mode is only adopted once it has been
		 * accepted. Applications need it because Boot Protocol Mode always
		 * carries a Report ID (Section 3.3.1).
		 */
		if (param == BT_HID_HS_RSP_SUCCESS) {
			hid->boot_mode = (req.protocol == BT_HID_PROTOCOL_BOOT_MODE);
			LOG_DBG("protocol mode %s", hid->boot_mode ? "boot" : "report");
		}

		if ((hid_cb != NULL) && (hid_cb->set_protocol != NULL)) {
			hid_cb->set_protocol(hid, param);
		}
		break;

	default:
		LOG_WRN("HANDSHAKE for unknown transaction %u", req.type);
		break;
	}

	return 0;
}

static int hid_data_handle(struct bt_hid_host *hid, struct net_buf *buf, uint8_t param)
{
	const struct bt_hid_req *pending = atomic_ptr_get(&hid->_req);
	uint8_t report_type = FIELD_GET(BT_HID_PARAM_REPORT_TYPE_MASK, param);
	struct bt_hid_req req;

	if (pending != &hid->req) {
		/* Nothing was requested on the Control channel, or the request is not
		 * on the air yet, so there is no transaction this DATA can belong to.
		 */
		LOG_DBG("Drop unexpected CTRL DATA");
		return 0;
	}

	switch (pending->type) {
	case BT_HID_W4_GET_PROTOCOL:
		/* HID spec v1.1.2 Section 3.1.2.5: the reply to GET_PROTOCOL is a
		 * DATA message carrying a single protocol mode octet.
		 */
		if (buf->len < sizeof(uint8_t)) {
			return -EINVAL;
		}

		if (!hid_req_take(hid, &req)) {
			return 0;
		}

		hid->boot_mode = (FIELD_GET(BT_HID_PROTOCOL_MASK, net_buf_pull_u8(buf)) ==
				  BT_HID_PROTOCOL_BOOT_MODE);

		if ((hid_cb != NULL) && (hid_cb->get_protocol != NULL)) {
			hid_cb->get_protocol(hid, BT_HID_HS_RSP_SUCCESS,
					     hid->boot_mode ? BT_HID_PROTOCOL_BOOT_MODE
							    : BT_HID_PROTOCOL_REPORT_MODE);
		}
		break;

	case BT_HID_W4_GET_REPORT:
		/* HID spec v1.1.2 Section 3.1.2.4: the DATA message carries the report
		 * type of the request it answers, so a different one does not belong to
		 * this transaction and the transaction is left outstanding.
		 */
		if (report_type != pending->report_type) {
			LOG_WRN("CTRL DATA report type %u, requested %u, ignoring", report_type,
				pending->report_type);
			return 0;
		}

		if (!hid_req_take(hid, &req)) {
			return 0;
		}

		if ((hid_cb != NULL) && (hid_cb->get_report != NULL)) {
			hid_cb->get_report(hid, BT_HID_HS_RSP_SUCCESS, report_type, buf);
		}
		break;

	default:
		/* Nothing was requested on the Control channel, so there is no
		 * transaction this DATA can belong to.
		 */
		LOG_DBG("Drop unexpected CTRL DATA");
		break;
	}

	return 0;
}

static int hid_control_handle(struct bt_hid_host *hid, struct net_buf *buf, uint8_t control)
{
	struct bt_hid_req req;

	ARG_UNUSED(buf);

	/* HID spec v1.1.2 Section 3.1.2.2.3: VIRTUAL_CABLE_UNPLUG is the only
	 * HID_CONTROL message a device may send to a host, and a host shall ignore
	 * all other HID_CONTROL messages sent by devices.
	 */
	if (control != BT_HID_CONTROL_VIRTUAL_CABLE_UNPLUG) {
		LOG_WRN("Control operation %u not handled, ignoring", control);
		return 0;
	}

	LOG_DBG("VC unplug");

	/* HID spec v1.1.2 Section 3.1.2.2.3: the recipient should discard any
	 * control transfer that is pending when the unplug arrives, so the result
	 * of a request issued before it is never reported afterwards.
	 */
	if (hid_req_take(hid, &req)) {
		LOG_DBG("transaction %s discarded on VC unplug", hid_w4_str(req.type));
	}

	if ((hid_cb != NULL) && (hid_cb->vc_unplug != NULL)) {
		hid_cb->vc_unplug(hid);
	}

	/* The recipient shall not send a HANDSHAKE but shall acknowledge the
	 * request by disconnecting INTR followed by CTRL, discarding any pending
	 * control transfer. The teardown is deferred to the system workqueue so
	 * the channels are not disconnected from inside the receive callback.
	 * Clearing the bonding is left to the application through the vc_unplug
	 * callback above, for the same reason as on the device side: the only host
	 * API, bt_br_unpair(), unconditionally drops the whole ACL, which would
	 * tear down any co-located profile sharing it, while the spec only allows
	 * the ACL to be dropped "if there are no other profiles using the ACL".
	 */
	hid->suspended = false;

	k_work_schedule(&hid->vcu_work, K_NO_WAIT);

	return 0;
}

static int hid_intr_handle(struct bt_hid_host *hid, struct net_buf *buf)
{
	/* HID spec v1.1.2 Section 3.1.2.9: the payload is a report and is handed up
	 * as received. Whether it starts with a Report ID follows from the report
	 * descriptor and from the protocol mode, which the application owns.
	 */
	if ((hid_cb != NULL) && (hid_cb->input_report != NULL)) {
		hid_cb->input_report(hid, buf);
	}

	return 0;
}

typedef int (*hid_ctrl_handler_t)(struct bt_hid_host *hid, struct net_buf *buf, uint8_t param);

static const struct {
	uint8_t type;
	hid_ctrl_handler_t handler;
} hid_l2cap_ctrl_handlers[] = {
	{BT_HID_MSG_TYPE_HANDSHAKE, hid_handshake_handle},
	{BT_HID_MSG_TYPE_DATA, hid_data_handle},
	{BT_HID_MSG_TYPE_CONTROL, hid_control_handle},
};

static int hid_l2cap_ctrl_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_hid_hdr *hdr;
	struct bt_hid_host *hid = HID_HOST_BY_CTRL_CHAN(chan);
	uint8_t type;
	uint8_t param;

	__ASSERT_NO_MSG(HID_CHAN_TYPE(chan) == BT_HID_CHANNEL_CTRL);

	/* HID spec v1.1.2 Section 5.2.2: the Control channel carries traffic as
	 * soon as it is open, independently of the Interrupt channel. Drop anything
	 * received before the Control channel itself is connected.
	 */
	if (hid->state < BT_HID_STATE_CTRL_CONNECTED) {
		LOG_WRN("CTRL recv in state %s, ignoring", hid_state_str(hid->state));
		return 0;
	}

	if (buf->len < sizeof(*hdr)) {
		LOG_ERR("CTRL buf len %u invalid", buf->len);
		return 0;
	}

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	param = BT_HID_HDR_GET_PARAM(hdr->header);
	type = BT_HID_HDR_GET_TYPE(hdr->header);

	LOG_DBG("CTRL recv type 0x%x param 0x%x", type, param);

	ARRAY_FOR_EACH(hid_l2cap_ctrl_handlers, i) {
		if (hid_l2cap_ctrl_handlers[i].type == type) {
			int err = hid_l2cap_ctrl_handlers[i].handler(hid, buf, param);

			/* HID spec v1.1.2 Section 3.1.2.1: HANDSHAKE shall not be
			 * sent by a host, so unlike the device side there is no way
			 * to report a malformed message to the peer and the error is
			 * only logged.
			 */
			if (err != 0) {
				LOG_WRN("type %u handling failed (%d)", type, err);
			}

			return 0;
		}
	}

	LOG_WRN("type %u not supported", type);

	return 0;
}

static int hid_l2cap_intr_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_hid_hdr *hdr;
	struct bt_hid_host *hid = HID_HOST_BY_INTR_CHAN(chan);
	uint8_t type;
	uint8_t param;
	uint8_t report_type;

	if (buf->len < sizeof(*hdr)) {
		LOG_ERR("INTR buf len %u invalid", buf->len);
		return 0;
	}

	__ASSERT_NO_MSG(HID_CHAN_TYPE(chan) == BT_HID_CHANNEL_INTR);

	if (hid->state != BT_HID_STATE_CONNECTED) {
		LOG_WRN("INTR recv in state %s, ignoring", hid_state_str(hid->state));
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

	/* HID spec v1.1.2 Section 3.2.2.1: an Interrupt IN transfer is a DATA
	 * message carrying an Input report from the device. Section 3.2.2 gives the
	 * Interrupt channel no acknowledgment mechanism, so anything else is
	 * silently dropped.
	 */
	report_type = FIELD_GET(BT_HID_PARAM_REPORT_TYPE_MASK, param);
	if (report_type != BT_HID_REPORT_TYPE_INPUT) {
		LOG_WRN("INTR DATA report type %u not Input, ignoring", report_type);
		return 0;
	}

	return hid_intr_handle(hid, buf);
}

static const struct bt_l2cap_chan_ops ctrl_ops = {
	.connected = hid_l2cap_ctrl_connected,
	.disconnected = hid_l2cap_ctrl_disconnected,
	.recv = hid_l2cap_ctrl_recv,
};

static const struct bt_l2cap_chan_ops intr_ops = {
	.connected = hid_l2cap_intr_connected,
	.disconnected = hid_l2cap_intr_disconnected,
	.recv = hid_l2cap_intr_recv,
};

static void hid_session_init(struct bt_hid_host *hid, enum bt_hid_role role)
{
	hid->ctrl_session.br_chan.chan.ops = &ctrl_ops;
	/* HID spec v1.1.2 Section 5.2.3.1: a host in Report Protocol Mode shall
	 * use an L2CAP MTU of 672 octets or greater on both channels, and in all
	 * cases at least the L2CAP default minimum of 48 octets.
	 */
	hid->ctrl_session.br_chan.rx.mtu = BT_L2CAP_RX_MTU;
	hid->ctrl_session.type = BT_HID_CHANNEL_CTRL;

	hid->intr_session.br_chan.chan.ops = &intr_ops;
	hid->intr_session.br_chan.rx.mtu = BT_L2CAP_RX_MTU;
	hid->intr_session.type = BT_HID_CHANNEL_INTR;

	/* HID spec v1.1.2 Section 5.4.3.4.2: a HID Host shall use Security Mode 4,
	 * and Section 5.4.3.4.4 recommends Just Works unless active attacks are a
	 * concern, so encryption without authentication. An initiator sets the level
	 * itself; on the acceptor path L2CAP takes it from the server.
	 */
	hid->ctrl_session.br_chan.required_sec_level = BT_SECURITY_L2;
	hid->intr_session.br_chan.required_sec_level = BT_SECURITY_L2;

	hid->role = role;
	/* HID spec v1.1.2 Section 2.1.2: the default protocol mode is Report
	 * Protocol Mode.
	 */
	hid->boot_mode = false;
	hid->suspended = false;
	hid->ctrl_connected = false;
	hid->intr_connected = false;
	atomic_ptr_clear(&hid->_req);

	/* Nothing is carried over from an earlier connection: discovery runs for
	 * every connection.
	 */
	k_work_init_delayable(&hid->intr_work, hid_intr_timeout_handler);
	k_work_init_delayable(&hid->trans_work, hid_trans_timeout_handler);
	k_work_init_delayable(&hid->vcu_work, hid_vcu_work_handler);
}

static int hid_l2cap_ctrl_accept(struct bt_conn *conn, struct bt_l2cap_server *server,
				      struct bt_l2cap_chan **chan)
{
	struct bt_hid_host *hid;

	LOG_DBG("CTRL accept conn %p", conn);

	if (hid_cb == NULL) {
		LOG_ERR("HID Host not registered");
		return -ENOTSUP;
	}

	hid = hid_allocate(conn);
	if (hid == NULL) {
		return -EBUSY;
	}

	hid_session_init(hid, BT_HID_ROLE_ACCEPTOR);
	hid_set_state(hid, BT_HID_STATE_CTRL_CONNECTING);

	*chan = &hid->ctrl_session.br_chan.chan;
	return 0;
}

static int hid_l2cap_intr_accept(struct bt_conn *conn, struct bt_l2cap_server *server,
				      struct bt_l2cap_chan **chan)
{
	struct bt_hid_host *hid;

	LOG_DBG("INTR accept conn %p", conn);

	/* HID spec v1.1.2 Section 5.2.2: the control channel is opened first and
	 * has to be established before the interrupt channel.
	 */
	hid = hid_find(conn);
	if ((hid == NULL) || (hid->state != BT_HID_STATE_CTRL_CONNECTED)) {
		LOG_ERR("CTRL channel not connected");
		return -ENOTCONN;
	}

	if (hid->intr_connected) {
		LOG_ERR("INTR channel already connected");
		return -EBUSY;
	}

	hid_set_state(hid, BT_HID_STATE_INTR_CONNECTING);

	*chan = &hid->intr_session.br_chan.chan;
	return 0;
}

/* Security Mode 4 on both PSMs, see hid_session_init() for the level. */
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

static int hid_init(void)
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

static void hid_deinit(void)
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

int bt_hid_host_register(const struct bt_hid_host_cb *cb)
{
	int err;

	LOG_DBG("register");

	if (cb == NULL) {
		return -EINVAL;
	}

	if (hid_cb != NULL) {
		return -EALREADY;
	}

	err = hid_init();
	if (err < 0) {
		return err;
	}

	hid_cb = cb;
	return 0;
}

int bt_hid_host_unregister(void)
{
	LOG_DBG("unregister");

	if (hid_cb == NULL) {
		return -EALREADY;
	}

	ARRAY_FOR_EACH(hid_conn_pool, i) {
		if (hid_conn_pool[i].state != BT_HID_STATE_DISCONNECTED) {
			LOG_ERR("cannot unregister while connected (state %s)",
				hid_state_str(hid_conn_pool[i].state));
			return -EBUSY;
		}
	}

	hid_deinit();
	hid_cb = NULL;

	return 0;
}

int bt_hid_host_connect(struct bt_conn *conn, struct bt_hid_host **hid)
{
	struct bt_hid_host *inst;
	int err;

	LOG_DBG("connect");

	if ((conn == NULL) || (hid == NULL)) {
		return -EINVAL;
	}

	if (hid_cb == NULL) {
		return -ENOTSUP;
	}

	inst = hid_allocate(conn);
	if (inst == NULL) {
		return -EBUSY;
	}

	hid_session_init(inst, BT_HID_ROLE_INITIATOR);
	hid_set_state(inst, BT_HID_STATE_CTRL_CONNECTING);

	/* HID spec v1.1.2 Section 5.2.2: the Control channel is opened first and the
	 * Interrupt channel follows from hid_l2cap_ctrl_connected().
	 */
	err = bt_l2cap_chan_connect(conn, &inst->ctrl_session.br_chan.chan,
				    BT_L2CAP_PSM_HID_CTRL);
	if (err != 0) {
		LOG_ERR("CTRL connect failed (%d)", err);
		hid_cleanup(inst);
		return err;
	}

	*hid = inst;
	return 0;
}

int bt_hid_host_disconnect(struct bt_hid_host *hid)
{
	int err;

	if (hid == NULL) {
		return -EINVAL;
	}

	LOG_DBG("disconnect");

	if ((hid->state == BT_HID_STATE_DISCONNECTED) ||
	    (hid->state == BT_HID_STATE_DISCONNECTING)) {
		LOG_ERR("not connected (state %s)", hid_state_str(hid->state));
		return -ENOTCONN;
	}

	/* HID spec v1.1.2 Section 5.2.2: the channels are closed in reverse
	 * order, INTR first and CTRL once INTR has closed.
	 */
	if (hid->intr_connected) {
		err = bt_l2cap_chan_disconnect(&hid->intr_session.br_chan.chan);
	} else if (hid->ctrl_connected) {
		err = bt_l2cap_chan_disconnect(&hid->ctrl_session.br_chan.chan);
	} else {
		/* The Control channel connect is still outstanding, so there is no
		 * channel to close yet.
		 */
		hid_cleanup(hid);
		return 0;
	}

	/* -EALREADY means the channel is already tearing down, which matches
	 * the intent of this call; only update state once the request is
	 * accepted so a failed disconnect leaves the state untouched.
	 */
	if ((err != 0) && (err != -EALREADY)) {
		LOG_ERR("chan disconnect failed (%d)", err);
		return err;
	}

	hid_set_state(hid, BT_HID_STATE_DISCONNECTING);

	return 0;
}

/* Common pre-flight validation for everything sent on the Control channel.
 *
 * The message header depends on the report type and on whether a ReportID is in
 * use, so the PDU has to be built by the caller. Validating first avoids
 * allocating and formatting one that would then be rejected. Whether the Control
 * channel is free is decided by the claim in hid_req_claim(), which is the only
 * place that can decide it without racing another request.
 */
static int hid_check_req(const struct bt_hid_host *hid)
{
	if (hid == NULL) {
		return -EINVAL;
	}

	/* HID spec v1.1.2 Section 5.2.2: Control channel commands may be sent once
	 * the Control channel is open, the Interrupt channel need not be
	 * established. A teardown in progress is still refused, and the Control
	 * channel outlives the Interrupt one there, hence the state check.
	 */
	if (!hid->ctrl_connected || (hid->state == BT_HID_STATE_DISCONNECTING)) {
		LOG_ERR("CTRL not ready (state %s)", hid_state_str(hid->state));
		return -ENOTCONN;
	}

	return 0;
}

/* Build a Control channel PDU holding only the HIDP header. */
static struct net_buf *hid_create_ctrl_pdu(uint8_t type, uint8_t param)
{
	struct net_buf *buf;
	struct bt_hid_hdr *hdr;

	buf = bt_hid_host_create_pdu(NULL);
	if (buf == NULL) {
		return NULL;
	}

	hdr = net_buf_add(buf, sizeof(struct bt_hid_hdr));
	hdr->header = BT_HID_BUILD_HDR(type, param);

	return buf;
}

/* Send a Control channel PDU and, when it is a request, claim the transaction
 * and arm its timer first.
 *
 * Claiming and sending are kept in one function so that a request can never
 * become outstanding without a timer, a timer can never outlive its request, and
 * every path that fails to reach the air releases the claim again.
 *
 * @param req NULL for HID_CONTROL, which HID spec v1.1.2 Section 3.1.2.2 leaves
 *            unacknowledged.
 */
static int hid_send_ctrl(struct bt_hid_host *hid, struct net_buf *buf,
			 const struct bt_hid_req *req)
{
	int err;

	if (req != NULL) {
		err = hid_req_claim(hid, req);
		if (err != 0) {
			return err;
		}
	}

	err = bt_l2cap_chan_send(&hid->ctrl_session.br_chan.chan, buf);
	if (err < 0) {
		if (req != NULL) {
			hid_req_abort(hid);
		}

		return err;
	}

	return 0;
}

int bt_hid_host_get_report(struct bt_hid_host *hid, uint8_t type, uint8_t report_id,
			   uint16_t buffer_size)
{
	struct bt_hid_req req = {
		.type = BT_HID_W4_GET_REPORT,
		.report_type = type,
		.buffer_size = buffer_size,
	};
	struct net_buf *buf;
	uint8_t param;
	int err;

	LOG_DBG("get report type %u size %u", type, buffer_size);

	err = hid_check_req(hid);
	if (err != 0) {
		return err;
	}

	/* HID spec v1.1.2 Table 3.4: report type 0 is reserved. */
	if ((type != BT_HID_REPORT_TYPE_INPUT) && (type != BT_HID_REPORT_TYPE_OUTPUT) &&
	    (type != BT_HID_REPORT_TYPE_FEATURE)) {
		return -EINVAL;
	}

	/* HID spec v1.1.2 Section 3.1.2.3: BufferSize is the maximum length of the
	 * DATA message the device may answer with, so asking for more than the
	 * Control channel can receive would only guarantee the reply is lost.
	 */
	if (buffer_size > (hid->ctrl_session.br_chan.rx.mtu - BT_HID_HDR_SIZE)) {
		LOG_ERR("buffer size %u exceeds CTRL RX MTU %u", buffer_size,
			hid->ctrl_session.br_chan.rx.mtu);
		return -EMSGSIZE;
	}

	param = FIELD_PREP(BT_HID_PARAM_REPORT_TYPE_MASK, type);
	if (buffer_size > 0U) {
		param |= BT_HID_PARAM_REPORT_SIZE_MASK;
	}

	buf = hid_create_ctrl_pdu(BT_HID_MSG_TYPE_GET_REPORT, param);
	if (buf == NULL) {
		return -ENOBUFS;
	}

	/* HID spec v1.1.2 Section 3.1.2.3: the ReportID field, when present,
	 * immediately follows the request octet and the BufferSize field follows the
	 * ReportID. Report ID 0 is reserved by the USB HID specification, so it
	 * stands for "no Report ID in use" here, the same way a buffer size of 0
	 * leaves out the BufferSize field. Only the application knows the report
	 * descriptor, so it decides whether the field is sent.
	 */
	if (report_id != 0U) {
		net_buf_add_u8(buf, report_id);
	}

	if (buffer_size > 0U) {
		net_buf_add_le16(buf, buffer_size);
	}

	err = hid_send_ctrl(hid, buf, &req);
	if (err != 0) {
		LOG_ERR("Get_Report send failed (%d)", err);
		net_buf_unref(buf);
		return err;
	}

	return 0;
}

int bt_hid_host_set_report(struct bt_hid_host *hid, uint8_t type, struct net_buf *buf)
{
	struct bt_hid_req req = {
		.type = BT_HID_W4_SET_REPORT,
		.report_type = type,
	};
	struct bt_hid_hdr *hdr;
	int err;

	LOG_DBG("set report type %u len %u", type, buf != NULL ? buf->len : 0U);

	err = hid_check_req(hid);
	if (err != 0) {
		return err;
	}

	if (buf == NULL) {
		return -EINVAL;
	}

	/* HID spec v1.1.2 Table 3.6: report type 0 is reserved. */
	if ((type != BT_HID_REPORT_TYPE_INPUT) && (type != BT_HID_REPORT_TYPE_OUTPUT) &&
	    (type != BT_HID_REPORT_TYPE_FEATURE)) {
		return -EINVAL;
	}

	if (net_buf_headroom(buf) < sizeof(struct bt_hid_hdr)) {
		return -ENOMEM;
	}

	hdr = net_buf_push(buf, sizeof(struct bt_hid_hdr));
	hdr->header = BT_HID_BUILD_HDR(BT_HID_MSG_TYPE_SET_REPORT,
				       FIELD_PREP(BT_HID_PARAM_REPORT_TYPE_MASK, type));

	err = hid_send_ctrl(hid, buf, &req);
	if (err != 0) {
		LOG_ERR("Set_Report send failed (%d)", err);
		/* The caller keeps the buffer when the request fails, so the header
		 * added above is removed again to hand it back unchanged. Otherwise a
		 * retry would prepend a second header.
		 */
		net_buf_pull(buf, sizeof(struct bt_hid_hdr));
		return err;
	}

	return 0;
}

int bt_hid_host_get_protocol(struct bt_hid_host *hid)
{
	struct bt_hid_req req = {
		.type = BT_HID_W4_GET_PROTOCOL,
	};
	struct net_buf *buf;
	int err;

	LOG_DBG("get protocol");

	err = hid_check_req(hid);
	if (err != 0) {
		return err;
	}

	buf = hid_create_ctrl_pdu(BT_HID_MSG_TYPE_GET_PROTOCOL, 0U);
	if (buf == NULL) {
		return -ENOBUFS;
	}

	err = hid_send_ctrl(hid, buf, &req);
	if (err != 0) {
		LOG_ERR("Get_Protocol send failed (%d)", err);
		net_buf_unref(buf);
		return err;
	}

	return 0;
}

int bt_hid_host_set_protocol(struct bt_hid_host *hid, uint8_t protocol)
{
	struct bt_hid_req req = {
		.type = BT_HID_W4_SET_PROTOCOL,
		.protocol = protocol,
	};
	struct net_buf *buf;
	int err;

	LOG_DBG("set protocol %u", protocol);

	err = hid_check_req(hid);
	if (err != 0) {
		return err;
	}

	if ((protocol != BT_HID_PROTOCOL_BOOT_MODE) &&
	    (protocol != BT_HID_PROTOCOL_REPORT_MODE)) {
		return -EINVAL;
	}

	buf = hid_create_ctrl_pdu(BT_HID_MSG_TYPE_SET_PROTOCOL,
				  FIELD_PREP(BT_HID_PROTOCOL_MASK, protocol));
	if (buf == NULL) {
		return -ENOBUFS;
	}

	/* The mode is adopted only when the device acknowledges the request with a
	 * successful HANDSHAKE, see hid_handshake_handle(). It travels with the
	 * transaction, which is claimed before the PDU is sent, so the RX thread
	 * always sees it.
	 */
	err = hid_send_ctrl(hid, buf, &req);
	if (err != 0) {
		LOG_ERR("Set_Protocol send failed (%d)", err);
		net_buf_unref(buf);
		return err;
	}

	return 0;
}

int bt_hid_host_output_report(struct bt_hid_host *hid, struct net_buf *buf)
{
	struct bt_hid_hdr *hdr;
	int err;

	LOG_DBG("output report len %u", buf != NULL ? buf->len : 0U);

	if ((hid == NULL) || (buf == NULL)) {
		return -EINVAL;
	}

	if (hid->state != BT_HID_STATE_CONNECTED) {
		LOG_ERR("not connected (state %s)", hid_state_str(hid->state));
		return -ENOTCONN;
	}

	if (net_buf_headroom(buf) < sizeof(struct bt_hid_hdr)) {
		return -ENOMEM;
	}

	hdr = net_buf_push(buf, sizeof(struct bt_hid_hdr));
	hdr->header = BT_HID_BUILD_HDR(BT_HID_MSG_TYPE_DATA, BT_HID_REPORT_TYPE_OUTPUT);

	err = bt_l2cap_chan_send(&hid->intr_session.br_chan.chan, buf);
	if (err < 0) {
		LOG_ERR("Output_Report send failed (%d)", err);
		return err;
	}

	return 0;
}

int bt_hid_host_suspend(struct bt_hid_host *hid)
{
	struct net_buf *buf;
	int err;

	LOG_DBG("suspend");

	err = hid_check_req(hid);
	if (err != 0) {
		return err;
	}

	/* HID spec v1.1.2 Section 3.1.2.2: HID_CONTROL has no HANDSHAKE
	 * response, so no transaction state is tracked for it.
	 */
	buf = hid_create_ctrl_pdu(BT_HID_MSG_TYPE_CONTROL, BT_HID_CONTROL_SUSPEND);
	if (buf == NULL) {
		return -ENOBUFS;
	}

	err = hid_send_ctrl(hid, buf, NULL);
	if (err != 0) {
		LOG_ERR("Suspend send failed (%d)", err);
		net_buf_unref(buf);
		return err;
	}

	hid->suspended = true;

	return 0;
}

int bt_hid_host_exit_suspend(struct bt_hid_host *hid)
{
	struct net_buf *buf;
	int err;

	LOG_DBG("exit suspend");

	err = hid_check_req(hid);
	if (err != 0) {
		return err;
	}

	buf = hid_create_ctrl_pdu(BT_HID_MSG_TYPE_CONTROL, BT_HID_CONTROL_EXIT_SUSPEND);
	if (buf == NULL) {
		return -ENOBUFS;
	}

	err = hid_send_ctrl(hid, buf, NULL);
	if (err != 0) {
		LOG_ERR("Exit_Suspend send failed (%d)", err);
		net_buf_unref(buf);
		return err;
	}

	hid->suspended = false;

	return 0;
}

static void hid_vcu_tx_cb(struct bt_conn *conn, void *user_data, int err)
{
	struct bt_hid_host *hid;

	if ((conn == NULL) || (user_data == NULL)) {
		return;
	}

	hid = (struct bt_hid_host *)user_data;

	/* HID spec v1.1.2 Section 3.1.2.2.3: the recipient acknowledges the unplug
	 * by disconnecting the Interrupt channel and then the Control channel, so
	 * the sender waits for that instead of closing the channels itself. The
	 * teardown scheduled here only covers a peer that never acknowledges; one
	 * that does closes the association first, which cancels this work. It runs
	 * on the system workqueue because this callback runs in the TX context.
	 */
	if (err != 0) {
		/* The PDU never reached the controller, so the device cannot have
		 * seen the unplug and no acknowledgment is coming.
		 */
		LOG_WRN("VCU control message send failed (%d)", err);
		k_work_schedule(&hid->vcu_work, K_NO_WAIT);
		return;
	}

	k_work_schedule(&hid->vcu_work, HID_TRANS_TIMEOUT);
}

int bt_hid_host_virtual_cable_unplug(struct bt_hid_host *hid)
{
	struct net_buf *buf;
	int err;

	LOG_DBG("virtual cable unplug");

	err = hid_check_req(hid);
	if (err != 0) {
		return err;
	}

	buf = hid_create_ctrl_pdu(BT_HID_MSG_TYPE_CONTROL, BT_HID_CONTROL_VIRTUAL_CABLE_UNPLUG);
	if (buf == NULL) {
		return -ENOBUFS;
	}

	/* The tx callback arms the fallback teardown only once the PDU is on its
	 * way, so a peer that acknowledges the unplug by closing the channels is
	 * never raced by a local disconnect.
	 */
	err = bt_l2cap_br_chan_send_cb(&hid->ctrl_session.br_chan.chan, buf,
				       hid_vcu_tx_cb, hid);
	if (err < 0) {
		LOG_ERR("VCU send failed (%d)", err);
		net_buf_unref(buf);
		return err;
	}

	/* HID spec v1.1.2 Section 3.1.2.2.3 also requires the initiator to destroy
	 * the bonding; this is left to the application for the same reason as on the
	 * device side, since bt_br_unpair() would drop the shared ACL.
	 */

	return 0;
}
