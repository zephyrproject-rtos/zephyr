/* l2cap.c - L2CAP handling */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <string.h>
#include <errno.h>
#include <atomic.h>
#include <misc/byteorder.h>
#include <misc/util.h>

#include <bluetooth/hci.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/hci_driver.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_L2CAP)
#include "common/log.h"

#include "hci_core.h"
#include "conn_internal.h"
#include "l2cap_internal.h"

#define LE_CHAN_RTX(_w) CONTAINER_OF(_w, struct bt_l2cap_le_chan, chan.rtx_work)

#define L2CAP_LE_MIN_MTU		23

#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
#define L2CAP_LE_MAX_CREDITS		(CONFIG_BT_ACL_RX_COUNT - 1)
#else
#define L2CAP_LE_MAX_CREDITS		(CONFIG_BT_RX_BUF_COUNT - 1)
#endif

#define L2CAP_LE_CREDITS_THRESHOLD(_creds) (_creds / 2)

#define L2CAP_LE_CID_DYN_START	0x0040
#define L2CAP_LE_CID_DYN_END	0x007f
#define L2CAP_LE_CID_IS_DYN(_cid) \
	(_cid >= L2CAP_LE_CID_DYN_START && _cid <= L2CAP_LE_CID_DYN_END)

#define L2CAP_LE_PSM_FIXED_START 0x0001
#define L2CAP_LE_PSM_FIXED_END   0x007f
#define L2CAP_LE_PSM_DYN_START   0x0080
#define L2CAP_LE_PSM_DYN_END     0x00ff

#define L2CAP_CONN_TIMEOUT	K_SECONDS(40)
#define L2CAP_DISC_TIMEOUT	K_SECONDS(2)

static sys_slist_t le_channels;

#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
/* Size of MTU is based on the maximum amount of data the buffer can hold
 * excluding ACL and driver headers.
 */
#define L2CAP_MAX_LE_MPS	BT_L2CAP_RX_MTU
/* For now use MPS - SDU length to disable segmentation */
#define L2CAP_MAX_LE_MTU	(L2CAP_MAX_LE_MPS - 2)

#define l2cap_lookup_ident(conn, ident) __l2cap_lookup_ident(conn, ident, false)
#define l2cap_remove_ident(conn, ident) __l2cap_lookup_ident(conn, ident, true)

static sys_slist_t servers;

#endif /* CONFIG_BT_L2CAP_DYNAMIC_CHANNEL */

/* L2CAP signalling channel specific context */
struct bt_l2cap {
	/* The channel this context is associated with */
	struct bt_l2cap_le_chan	chan;
};

static struct bt_l2cap bt_l2cap_pool[CONFIG_BT_MAX_CONN];

static u8_t get_ident(void)
{
	static u8_t ident;

	ident++;
	/* handle integer overflow (0 is not valid) */
	if (!ident) {
		ident++;
	}

	return ident;
}

void bt_l2cap_le_fixed_chan_register(struct bt_l2cap_fixed_chan *chan)
{
	BT_DBG("CID 0x%04x", chan->cid);

	sys_slist_append(&le_channels, &chan->node);
}

#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
static struct bt_l2cap_le_chan *l2cap_chan_alloc_cid(struct bt_conn *conn,
						     struct bt_l2cap_chan *chan)
{
	struct bt_l2cap_le_chan *ch = BT_L2CAP_LE_CHAN(chan);
	u16_t cid;

	/*
	 * No action needed if there's already a CID allocated, e.g. in
	 * the case of a fixed channel.
	 */
	if (ch && ch->rx.cid > 0) {
		return ch;
	}

	for (cid = L2CAP_LE_CID_DYN_START; cid <= L2CAP_LE_CID_DYN_END; cid++) {
		if (ch && !bt_l2cap_le_lookup_rx_cid(conn, cid)) {
			ch->rx.cid = cid;
			return ch;
		}
	}

	return NULL;
}

static struct bt_l2cap_le_chan *
__l2cap_lookup_ident(struct bt_conn *conn, u16_t ident, bool remove)
{
	struct bt_l2cap_chan *chan;
	sys_snode_t *prev = NULL;

	SYS_SLIST_FOR_EACH_CONTAINER(&conn->channels, chan, node) {
		if (chan->ident == ident) {
			if (remove) {
				sys_slist_remove(&conn->channels, prev,
						 &chan->node);
			}
			return BT_L2CAP_LE_CHAN(chan);
		}

		prev = &chan->node;
	}

	return NULL;
}
#endif /* CONFIG_BT_L2CAP_DYNAMIC_CHANNEL */

void bt_l2cap_chan_remove(struct bt_conn *conn, struct bt_l2cap_chan *ch)
{
	struct bt_l2cap_chan *chan;
	sys_snode_t *prev = NULL;

	SYS_SLIST_FOR_EACH_CONTAINER(&conn->channels, chan, node) {
		if (chan == ch) {
			sys_slist_remove(&conn->channels, prev, &chan->node);
			return;
		}

		prev = &chan->node;
	}
}

#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
#if defined(CONFIG_BT_DEBUG_L2CAP)
const char *bt_l2cap_chan_state_str(bt_l2cap_chan_state_t state)
{
	switch (state) {
	case BT_L2CAP_DISCONNECTED:
		return "disconnected";
	case BT_L2CAP_CONNECT:
		return "connect";
	case BT_L2CAP_CONFIG:
		return "config";
	case BT_L2CAP_CONNECTED:
		return "connected";
	case BT_L2CAP_DISCONNECT:
		return "disconnect";
	default:
		return "unknown";
	}
}

void bt_l2cap_chan_set_state_debug(struct bt_l2cap_chan *chan,
				   bt_l2cap_chan_state_t state,
				   const char *func, int line)
{
	BT_DBG("chan %p psm 0x%04x %s -> %s", chan, chan->psm,
	       bt_l2cap_chan_state_str(chan->state),
	       bt_l2cap_chan_state_str(state));

	/* check transitions validness */
	switch (state) {
	case BT_L2CAP_DISCONNECTED:
		/* regardless of old state always allows this state */
		break;
	case BT_L2CAP_CONNECT:
		if (chan->state != BT_L2CAP_DISCONNECTED) {
			BT_WARN("%s()%d: invalid transition", func, line);
		}
		break;
	case BT_L2CAP_CONFIG:
		if (chan->state != BT_L2CAP_CONNECT) {
			BT_WARN("%s()%d: invalid transition", func, line);
		}
		break;
	case BT_L2CAP_CONNECTED:
		if (chan->state != BT_L2CAP_CONFIG &&
		    chan->state != BT_L2CAP_CONNECT) {
			BT_WARN("%s()%d: invalid transition", func, line);
		}
		break;
	case BT_L2CAP_DISCONNECT:
		if (chan->state != BT_L2CAP_CONFIG &&
		    chan->state != BT_L2CAP_CONNECTED) {
			BT_WARN("%s()%d: invalid transition", func, line);
		}
		break;
	default:
		BT_ERR("%s()%d: unknown (%u) state was set", func, line, state);
		return;
	}

	chan->state = state;
}
#else
void bt_l2cap_chan_set_state(struct bt_l2cap_chan *chan,
			     bt_l2cap_chan_state_t state)
{
	chan->state = state;
}
#endif /* CONFIG_BT_DEBUG_L2CAP */
#endif /* CONFIG_BT_L2CAP_DYNAMIC_CHANNEL */

void bt_l2cap_chan_del(struct bt_l2cap_chan *chan)
{
	BT_DBG("conn %p chan %p", chan->conn, chan);

	if (!chan->conn) {
		goto destroy;
	}

	if (chan->ops->disconnected) {
		chan->ops->disconnected(chan);
	}

	chan->conn = NULL;

destroy:
#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
	/* Reset internal members of common channel */
	bt_l2cap_chan_set_state(chan, BT_L2CAP_DISCONNECTED);
	chan->psm = 0;
#endif

	if (chan->destroy) {
		chan->destroy(chan);
	}
}

static void l2cap_rtx_timeout(struct k_work *work)
{
	struct bt_l2cap_le_chan *chan = LE_CHAN_RTX(work);

	BT_ERR("chan %p timeout", chan);

	bt_l2cap_chan_remove(chan->chan.conn, &chan->chan);
	bt_l2cap_chan_del(&chan->chan);
}

void bt_l2cap_chan_add(struct bt_conn *conn, struct bt_l2cap_chan *chan,
		       bt_l2cap_chan_destroy_t destroy)
{
	/* Attach channel to the connection */
	sys_slist_append(&conn->channels, &chan->node);
	chan->conn = conn;
	chan->destroy = destroy;

	BT_DBG("conn %p chan %p", conn, chan);
}

static bool l2cap_chan_add(struct bt_conn *conn, struct bt_l2cap_chan *chan,
			   bt_l2cap_chan_destroy_t destroy)
{
	struct bt_l2cap_le_chan *ch;

#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
	ch = l2cap_chan_alloc_cid(conn, chan);
#else
	ch = BT_L2CAP_LE_CHAN(chan);
#endif

	if (!ch) {
		BT_ERR("Unable to allocate L2CAP CID");
		return false;
	}

	k_delayed_work_init(&chan->rtx_work, l2cap_rtx_timeout);

	bt_l2cap_chan_add(conn, chan, destroy);

	if (IS_ENABLED(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL) &&
	    L2CAP_LE_CID_IS_DYN(ch->rx.cid)) {
		bt_l2cap_chan_set_state(chan, BT_L2CAP_CONNECT);
	}

	return true;
}

void bt_l2cap_connected(struct bt_conn *conn)
{
	struct bt_l2cap_fixed_chan *fchan;
	struct bt_l2cap_chan *chan;

	if (IS_ENABLED(CONFIG_BT_BREDR) &&
	    conn->type == BT_CONN_TYPE_BR) {
		bt_l2cap_br_connected(conn);
		return;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&le_channels, fchan, node) {
		struct bt_l2cap_le_chan *ch;

		if (fchan->accept(conn, &chan) < 0) {
			continue;
		}

		ch = BT_L2CAP_LE_CHAN(chan);

		/* Fill up remaining fixed channel context attached in
		 * fchan->accept()
		 */
		ch->rx.cid = fchan->cid;
		ch->tx.cid = fchan->cid;

		if (!l2cap_chan_add(conn, chan, NULL)) {
			return;
		}

		if (chan->ops->connected) {
			chan->ops->connected(chan);
		}
	}
}

void bt_l2cap_disconnected(struct bt_conn *conn)
{
	struct bt_l2cap_chan *chan, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&conn->channels, chan, next, node) {
		bt_l2cap_chan_del(chan);
	}
}

static struct net_buf *l2cap_create_le_sig_pdu(struct net_buf *buf,
					       u8_t code, u8_t ident,
					       u16_t len)
{
	struct bt_l2cap_sig_hdr *hdr;

	buf = bt_l2cap_create_pdu(NULL, 0);

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->code = code;
	hdr->ident = ident;
	hdr->len = sys_cpu_to_le16(len);

	return buf;
}

#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
static void l2cap_chan_send_req(struct bt_l2cap_le_chan *chan,
				struct net_buf *buf, s32_t timeout)
{
	/* BLUETOOTH SPECIFICATION Version 4.2 [Vol 3, Part A] page 126:
	 *
	 * The value of this timer is implementation-dependent but the minimum
	 * initial value is 1 second and the maximum initial value is 60
	 * seconds. One RTX timer shall exist for each outstanding signaling
	 * request, including each Echo Request. The timer disappears on the
	 * final expiration, when the response is received, or the physical
	 * link is lost.
	 */
	if (timeout) {
		k_delayed_work_submit(&chan->chan.rtx_work, timeout);
	} else {
		k_delayed_work_cancel(&chan->chan.rtx_work);
	}

	bt_l2cap_send(chan->chan.conn, BT_L2CAP_CID_LE_SIG, buf);
}

static int l2cap_le_conn_req(struct bt_l2cap_le_chan *ch)
{
	struct net_buf *buf;
	struct bt_l2cap_le_conn_req *req;

	ch->chan.ident = get_ident();

	buf = l2cap_create_le_sig_pdu(NULL, BT_L2CAP_LE_CONN_REQ,
				      ch->chan.ident, sizeof(*req));

	req = net_buf_add(buf, sizeof(*req));
	req->psm = sys_cpu_to_le16(ch->chan.psm);
	req->scid = sys_cpu_to_le16(ch->rx.cid);
	req->mtu = sys_cpu_to_le16(ch->rx.mtu);
	req->mps = sys_cpu_to_le16(ch->rx.mps);
	req->credits = sys_cpu_to_le16(ch->rx.init_credits);

	l2cap_chan_send_req(ch, buf, L2CAP_CONN_TIMEOUT);

	return 0;
}

static void l2cap_le_encrypt_change(struct bt_l2cap_chan *chan, u8_t status)
{
	/* Skip channels already connected or with a pending request */
	if (chan->state != BT_L2CAP_CONNECT || chan->ident) {
		return;
	}

	if (status) {
		bt_l2cap_chan_remove(chan->conn, chan);
		bt_l2cap_chan_del(chan);
		return;
	}

	/* Retry to connect */
	l2cap_le_conn_req(BT_L2CAP_LE_CHAN(chan));
}
#endif /* CONFIG_BT_L2CAP_DYNAMIC_CHANNEL */

void bt_l2cap_encrypt_change(struct bt_conn *conn, u8_t hci_status)
{
	struct bt_l2cap_chan *chan;

	if (IS_ENABLED(CONFIG_BT_BREDR) &&
	    conn->type == BT_CONN_TYPE_BR) {
		l2cap_br_encrypt_change(conn, hci_status);
		return;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&conn->channels, chan, node) {
#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
		l2cap_le_encrypt_change(chan, hci_status);
#endif

		if (chan->ops->encrypt_change) {
			chan->ops->encrypt_change(chan, hci_status);
		}
	}
}

struct net_buf *bt_l2cap_create_pdu(struct net_buf_pool *pool, size_t reserve)
{
	return bt_conn_create_pdu(pool, sizeof(struct bt_l2cap_hdr) + reserve);
}

void bt_l2cap_send_cb(struct bt_conn *conn, u16_t cid, struct net_buf *buf,
		      bt_conn_tx_cb_t cb)
{
	struct bt_l2cap_hdr *hdr;

	BT_DBG("conn %p cid %u len %zu", conn, cid, net_buf_frags_len(buf));

	hdr = net_buf_push(buf, sizeof(*hdr));
	hdr->len = sys_cpu_to_le16(buf->len - sizeof(*hdr));
	hdr->cid = sys_cpu_to_le16(cid);

	bt_conn_send_cb(conn, buf, cb);
}

static void l2cap_send_reject(struct bt_conn *conn, u8_t ident,
			      u16_t reason, void *data, u8_t data_len)
{
	struct bt_l2cap_cmd_reject *rej;
	struct net_buf *buf;

	buf = l2cap_create_le_sig_pdu(NULL, BT_L2CAP_CMD_REJECT, ident,
				      sizeof(*rej) + data_len);

	rej = net_buf_add(buf, sizeof(*rej));
	rej->reason = sys_cpu_to_le16(reason);

	if (data) {
		net_buf_add_mem(buf, data, data_len);
	}

	bt_l2cap_send(conn, BT_L2CAP_CID_LE_SIG, buf);
}

static void le_conn_param_rsp(struct bt_l2cap *l2cap, struct net_buf *buf)
{
	struct bt_l2cap_conn_param_rsp *rsp = (void *)buf->data;

	if (buf->len < sizeof(*rsp)) {
		BT_ERR("Too small LE conn param rsp");
		return;
	}

	BT_DBG("LE conn param rsp result %u", sys_le16_to_cpu(rsp->result));
}

#if defined(CONFIG_BT_CENTRAL)
static void le_conn_param_update_req(struct bt_l2cap *l2cap, u8_t ident,
				     struct net_buf *buf)
{
	struct bt_conn *conn = l2cap->chan.chan.conn;
	struct bt_le_conn_param param;
	struct bt_l2cap_conn_param_rsp *rsp;
	struct bt_l2cap_conn_param_req *req = (void *)buf->data;
	bool accepted;

	if (buf->len < sizeof(*req)) {
		BT_ERR("Too small LE conn update param req");
		return;
	}

	if (conn->role != BT_HCI_ROLE_MASTER) {
		l2cap_send_reject(conn, ident, BT_L2CAP_REJ_NOT_UNDERSTOOD,
				  NULL, 0);
		return;
	}

	param.interval_min = sys_le16_to_cpu(req->min_interval);
	param.interval_max = sys_le16_to_cpu(req->max_interval);
	param.latency = sys_le16_to_cpu(req->latency);
	param.timeout = sys_le16_to_cpu(req->timeout);

	BT_DBG("min 0x%04x max 0x%04x latency: 0x%04x timeout: 0x%04x",
	       param.interval_min, param.interval_max, param.latency,
	       param.timeout);

	buf = l2cap_create_le_sig_pdu(buf, BT_L2CAP_CONN_PARAM_RSP, ident,
				      sizeof(*rsp));

	accepted = le_param_req(conn, &param);

	rsp = net_buf_add(buf, sizeof(*rsp));
	if (accepted) {
		rsp->result = sys_cpu_to_le16(BT_L2CAP_CONN_PARAM_ACCEPTED);
	} else {
		rsp->result = sys_cpu_to_le16(BT_L2CAP_CONN_PARAM_REJECTED);
	}

	bt_l2cap_send(conn, BT_L2CAP_CID_LE_SIG, buf);

	if (accepted) {
		bt_conn_le_conn_update(conn, &param);
	}
}
#endif /* CONFIG_BT_CENTRAL */

struct bt_l2cap_chan *bt_l2cap_le_lookup_tx_cid(struct bt_conn *conn,
						u16_t cid)
{
	struct bt_l2cap_chan *chan;

	SYS_SLIST_FOR_EACH_CONTAINER(&conn->channels, chan, node) {
		if (BT_L2CAP_LE_CHAN(chan)->tx.cid == cid) {
			return chan;
		}
	}

	return NULL;
}

struct bt_l2cap_chan *bt_l2cap_le_lookup_rx_cid(struct bt_conn *conn,
						u16_t cid)
{
	struct bt_l2cap_chan *chan;

	SYS_SLIST_FOR_EACH_CONTAINER(&conn->channels, chan, node) {
		if (BT_L2CAP_LE_CHAN(chan)->rx.cid == cid) {
			return chan;
		}
	}

	return NULL;
}

#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
static struct bt_l2cap_server *l2cap_server_lookup_psm(u16_t psm)
{
	struct bt_l2cap_server *server;

	SYS_SLIST_FOR_EACH_CONTAINER(&servers, server, node) {
		if (server->psm == psm) {
			return server;
		}
	}

	return NULL;
}

int bt_l2cap_server_register(struct bt_l2cap_server *server)
{
	if (!server->accept) {
		return -EINVAL;
	}

	if (server->psm) {
		if (server->psm < L2CAP_LE_PSM_FIXED_START ||
		    server->psm > L2CAP_LE_PSM_DYN_END) {
			return -EINVAL;
		}

		/* Check if given PSM is already in use */
		if (l2cap_server_lookup_psm(server->psm)) {
			BT_DBG("PSM already registered");
			return -EADDRINUSE;
		}
	} else {
		u16_t psm;

		for (psm = L2CAP_LE_PSM_DYN_START;
		     psm <= L2CAP_LE_PSM_DYN_END; psm++) {
			if (!l2cap_server_lookup_psm(psm)) {
				break;
			}
		}

		if (psm > L2CAP_LE_PSM_DYN_END) {
			BT_WARN("No free dynamic PSMs available");
			return -EADDRNOTAVAIL;
		}

		BT_DBG("Allocated PSM 0x%04x for new server", psm);
		server->psm = psm;
	}

	if (server->sec_level > BT_SECURITY_FIPS) {
		return -EINVAL;
	} else if (server->sec_level < BT_SECURITY_LOW) {
		/* Level 0 is only applicable for BR/EDR */
		server->sec_level = BT_SECURITY_LOW;
	}

	BT_DBG("PSM 0x%04x", server->psm);

	sys_slist_append(&servers, &server->node);

	return 0;
}

static void l2cap_chan_rx_init(struct bt_l2cap_le_chan *chan)
{
	BT_DBG("chan %p", chan);

	/* Use existing MTU if defined */
	if (!chan->rx.mtu) {
		chan->rx.mtu = L2CAP_MAX_LE_MTU;
	}

	/* Use existing credits if defined */
	if (!chan->rx.init_credits) {
		if (chan->chan.ops->alloc_buf) {
			/* Auto tune credits to receive a full packet */
			chan->rx.init_credits = (chan->rx.mtu +
						 (L2CAP_MAX_LE_MPS - 1)) /
						L2CAP_MAX_LE_MPS;
		} else {
			chan->rx.init_credits = L2CAP_LE_MAX_CREDITS;
		}
	}

	/* MPS shall not be bigger than MTU + 2 as the remaining bytes cannot
	 * be used.
	 */
	chan->rx.mps = min(chan->rx.mtu + 2, L2CAP_MAX_LE_MPS);
	k_sem_init(&chan->rx.credits, 0, UINT_MAX);
}

static void l2cap_chan_tx_init(struct bt_l2cap_le_chan *chan)
{
	BT_DBG("chan %p", chan);

	(void)memset(&chan->tx, 0, sizeof(chan->tx));
	k_sem_init(&chan->tx.credits, 0, UINT_MAX);
	k_fifo_init(&chan->tx_queue);
}

static void l2cap_chan_tx_give_credits(struct bt_l2cap_le_chan *chan,
				       u16_t credits)
{
	BT_DBG("chan %p credits %u", chan, credits);

	while (credits--) {
		k_sem_give(&chan->tx.credits);
	}
}

static void l2cap_chan_rx_give_credits(struct bt_l2cap_le_chan *chan,
				       u16_t credits)
{
	BT_DBG("chan %p credits %u", chan, credits);

	while (credits--) {
		k_sem_give(&chan->rx.credits);
	}
}

static void l2cap_chan_destroy(struct bt_l2cap_chan *chan)
{
	struct bt_l2cap_le_chan *ch = BT_L2CAP_LE_CHAN(chan);
	struct net_buf *buf;

	BT_DBG("chan %p cid 0x%04x", ch, ch->rx.cid);

	/* Cancel ongoing work */
	k_delayed_work_cancel(&chan->rtx_work);

	/* Remove buffers on the TX queue */
	while ((buf = net_buf_get(&ch->tx_queue, K_NO_WAIT))) {
		net_buf_unref(buf);
	}

	/* Destroy segmented SDU if it exists */
	if (ch->_sdu) {
		net_buf_unref(ch->_sdu);
		ch->_sdu = NULL;
		ch->_sdu_len = 0;
	}
}

static void le_conn_req(struct bt_l2cap *l2cap, u8_t ident,
			struct net_buf *buf)
{
	struct bt_conn *conn = l2cap->chan.chan.conn;
	struct bt_l2cap_chan *chan;
	struct bt_l2cap_server *server;
	struct bt_l2cap_le_conn_req *req = (void *)buf->data;
	struct bt_l2cap_le_conn_rsp *rsp;
	u16_t psm, scid, mtu, mps, credits;

	if (buf->len < sizeof(*req)) {
		BT_ERR("Too small LE conn req packet size");
		return;
	}

	psm = sys_le16_to_cpu(req->psm);
	scid = sys_le16_to_cpu(req->scid);
	mtu = sys_le16_to_cpu(req->mtu);
	mps = sys_le16_to_cpu(req->mps);
	credits = sys_le16_to_cpu(req->credits);

	BT_DBG("psm 0x%02x scid 0x%04x mtu %u mps %u credits %u", psm, scid,
	       mtu, mps, credits);

	if (mtu < L2CAP_LE_MIN_MTU || mps < L2CAP_LE_MIN_MTU) {
		BT_ERR("Invalid LE-Conn Req params");
		return;
	}

	buf = l2cap_create_le_sig_pdu(buf, BT_L2CAP_LE_CONN_RSP, ident,
				      sizeof(*rsp));

	rsp = net_buf_add(buf, sizeof(*rsp));
	(void)memset(rsp, 0, sizeof(*rsp));

	/* Check if there is a server registered */
	server = l2cap_server_lookup_psm(psm);
	if (!server) {
		rsp->result = sys_cpu_to_le16(BT_L2CAP_ERR_PSM_NOT_SUPP);
		goto rsp;
	}

	/* Check if connection has minimum required security level */
	if (conn->sec_level < server->sec_level) {
		rsp->result = sys_cpu_to_le16(BT_L2CAP_ERR_AUTHENTICATION);
		goto rsp;
	}

	if (!L2CAP_LE_CID_IS_DYN(scid)) {
		rsp->result = sys_cpu_to_le16(BT_L2CAP_ERR_INVALID_SCID);
		goto rsp;
	}

	chan = bt_l2cap_le_lookup_tx_cid(conn, scid);
	if (chan) {
		rsp->result = sys_cpu_to_le16(BT_L2CAP_ERR_SCID_IN_USE);
		goto rsp;
	}

	/* Request server to accept the new connection and allocate the
	 * channel.
	 *
	 * TODO: Handle different errors, it may be required to respond async.
	 */
	if (server->accept(conn, &chan) < 0) {
		rsp->result = sys_cpu_to_le16(BT_L2CAP_ERR_NO_RESOURCES);
		goto rsp;
	}

	chan->required_sec_level = server->sec_level;

	if (l2cap_chan_add(conn, chan, l2cap_chan_destroy)) {
		struct bt_l2cap_le_chan *ch = BT_L2CAP_LE_CHAN(chan);

		/* Init TX parameters */
		l2cap_chan_tx_init(ch);
		ch->tx.cid = scid;
		ch->tx.mps = mps;
		ch->tx.mtu = mtu;
		ch->tx.init_credits = credits;
		l2cap_chan_tx_give_credits(ch, credits);

		/* Init RX parameters */
		l2cap_chan_rx_init(ch);
		l2cap_chan_rx_give_credits(ch, ch->rx.init_credits);

		/* Set channel PSM */
		chan->psm = server->psm;

		/* Update state */
		bt_l2cap_chan_set_state(chan, BT_L2CAP_CONNECTED);

		if (chan->ops->connected) {
			chan->ops->connected(chan);
		}

		/* Prepare response protocol data */
		rsp->dcid = sys_cpu_to_le16(ch->rx.cid);
		rsp->mps = sys_cpu_to_le16(ch->rx.mps);
		rsp->mtu = sys_cpu_to_le16(ch->rx.mtu);
		rsp->credits = sys_cpu_to_le16(ch->rx.init_credits);
		rsp->result = BT_L2CAP_SUCCESS;
	} else {
		rsp->result = sys_cpu_to_le16(BT_L2CAP_ERR_NO_RESOURCES);
	}
rsp:
	bt_l2cap_send(conn, BT_L2CAP_CID_LE_SIG, buf);
}

static struct bt_l2cap_le_chan *l2cap_remove_tx_cid(struct bt_conn *conn,
						    u16_t cid)
{
	struct bt_l2cap_chan *chan;
	sys_snode_t *prev = NULL;

	/* Protect fixed channels against accidental removal */
	if (!L2CAP_LE_CID_IS_DYN(cid)) {
		return NULL;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&conn->channels, chan, node) {
		if (BT_L2CAP_LE_CHAN(chan)->rx.cid == cid) {
			sys_slist_remove(&conn->channels, prev, &chan->node);
			return BT_L2CAP_LE_CHAN(chan);
		}

		prev = &chan->node;
	}

	return NULL;
}

static void le_disconn_req(struct bt_l2cap *l2cap, u8_t ident,
			   struct net_buf *buf)
{
	struct bt_conn *conn = l2cap->chan.chan.conn;
	struct bt_l2cap_le_chan *chan;
	struct bt_l2cap_disconn_req *req = (void *)buf->data;
	struct bt_l2cap_disconn_rsp *rsp;
	u16_t scid;

	if (buf->len < sizeof(*req)) {
		BT_ERR("Too small LE conn req packet size");
		return;
	}

	scid = sys_le16_to_cpu(req->scid);

	BT_DBG("scid 0x%04x dcid 0x%04x", scid, sys_le16_to_cpu(req->dcid));

	chan = l2cap_remove_tx_cid(conn, scid);
	if (!chan) {
		struct bt_l2cap_cmd_reject_cid_data data;

		data.scid = req->scid;
		data.dcid = req->dcid;

		l2cap_send_reject(conn, ident, BT_L2CAP_REJ_INVALID_CID, &data,
				  sizeof(data));
		return;
	}

	buf = l2cap_create_le_sig_pdu(buf, BT_L2CAP_DISCONN_RSP, ident,
				      sizeof(*rsp));

	rsp = net_buf_add(buf, sizeof(*rsp));
	rsp->dcid = sys_cpu_to_le16(chan->rx.cid);
	rsp->scid = sys_cpu_to_le16(chan->tx.cid);

	bt_l2cap_chan_del(&chan->chan);

	bt_l2cap_send(conn, BT_L2CAP_CID_LE_SIG, buf);
}

static int l2cap_change_security(struct bt_l2cap_le_chan *chan, u16_t err)
{
	switch (err) {
	case BT_L2CAP_ERR_ENCRYPTION:
		if (chan->chan.required_sec_level >= BT_SECURITY_MEDIUM) {
			return -EALREADY;
		}
		chan->chan.required_sec_level = BT_SECURITY_MEDIUM;
		break;
	case BT_L2CAP_ERR_AUTHENTICATION:
		if (chan->chan.required_sec_level < BT_SECURITY_MEDIUM) {
			chan->chan.required_sec_level = BT_SECURITY_MEDIUM;
		} else if (chan->chan.required_sec_level < BT_SECURITY_HIGH) {
			chan->chan.required_sec_level = BT_SECURITY_HIGH;
		} else if (chan->chan.required_sec_level < BT_SECURITY_FIPS) {
			chan->chan.required_sec_level = BT_SECURITY_FIPS;
		} else {
			return -EALREADY;
		}
		break;
	default:
		return -EINVAL;
	}

	return bt_conn_security(chan->chan.conn, chan->chan.required_sec_level);
}

static void le_conn_rsp(struct bt_l2cap *l2cap, u8_t ident,
			struct net_buf *buf)
{
	struct bt_conn *conn = l2cap->chan.chan.conn;
	struct bt_l2cap_le_chan *chan;
	struct bt_l2cap_le_conn_rsp *rsp = (void *)buf->data;
	u16_t dcid, mtu, mps, credits, result;

	if (buf->len < sizeof(*rsp)) {
		BT_ERR("Too small LE conn rsp packet size");
		return;
	}

	dcid = sys_le16_to_cpu(rsp->dcid);
	mtu = sys_le16_to_cpu(rsp->mtu);
	mps = sys_le16_to_cpu(rsp->mps);
	credits = sys_le16_to_cpu(rsp->credits);
	result = sys_le16_to_cpu(rsp->result);

	BT_DBG("dcid 0x%04x mtu %u mps %u credits %u result 0x%04x", dcid,
	       mtu, mps, credits, result);

	/* Keep the channel in case of security errors */
	if (result == BT_L2CAP_SUCCESS ||
	    result == BT_L2CAP_ERR_AUTHENTICATION ||
	    result == BT_L2CAP_ERR_ENCRYPTION) {
		chan = l2cap_lookup_ident(conn, ident);
	} else {
		chan = l2cap_remove_ident(conn, ident);
	}

	if (!chan) {
		BT_ERR("Cannot find channel for ident %u", ident);
		return;
	}

	/* Cancel RTX work */
	k_delayed_work_cancel(&chan->chan.rtx_work);

	/* Reset ident since it got a response */
	chan->chan.ident = 0;

	switch (result) {
	case BT_L2CAP_SUCCESS:
		chan->tx.cid = dcid;
		chan->tx.mtu = mtu;
		chan->tx.mps = mps;

		/* Update state */
		bt_l2cap_chan_set_state(&chan->chan, BT_L2CAP_CONNECTED);

		if (chan->chan.ops->connected) {
			chan->chan.ops->connected(&chan->chan);
		}

		/* Give credits */
		l2cap_chan_tx_give_credits(chan, credits);
		l2cap_chan_rx_give_credits(chan, chan->rx.init_credits);

		break;
	case BT_L2CAP_ERR_AUTHENTICATION:
	case BT_L2CAP_ERR_ENCRYPTION:
		/* If security needs changing wait it to be completed */
		if (l2cap_change_security(chan, result) == 0) {
			return;
		}
		bt_l2cap_chan_remove(conn, &chan->chan);
	default:
		bt_l2cap_chan_del(&chan->chan);
	}
}

static void le_disconn_rsp(struct bt_l2cap *l2cap, u8_t ident,
			   struct net_buf *buf)
{
	struct bt_conn *conn = l2cap->chan.chan.conn;
	struct bt_l2cap_le_chan *chan;
	struct bt_l2cap_disconn_rsp *rsp = (void *)buf->data;
	u16_t dcid;

	if (buf->len < sizeof(*rsp)) {
		BT_ERR("Too small LE disconn rsp packet size");
		return;
	}

	dcid = sys_le16_to_cpu(rsp->dcid);

	BT_DBG("dcid 0x%04x scid 0x%04x", dcid, sys_le16_to_cpu(rsp->scid));

	chan = l2cap_remove_tx_cid(conn, dcid);
	if (!chan) {
		return;
	}

	bt_l2cap_chan_del(&chan->chan);
}

static inline struct net_buf *l2cap_alloc_seg(struct net_buf *buf)
{
	struct net_buf_pool *pool = net_buf_pool_get(buf->pool_id);
	struct net_buf *seg;

	/* Try to use original pool if possible */
	seg = net_buf_alloc(pool, K_NO_WAIT);
	if (seg) {
		net_buf_reserve(seg, BT_L2CAP_CHAN_SEND_RESERVE);
		return seg;
	}

	/* Fallback to using global connection tx pool */
	return bt_l2cap_create_pdu(NULL, 0);
}

static struct net_buf *l2cap_chan_create_seg(struct bt_l2cap_le_chan *ch,
					     struct net_buf *buf,
					     size_t sdu_hdr_len)
{
	struct net_buf *seg;
	u16_t headroom;
	u16_t len;

	/* Segment if data (+ data headroom) is bigger than MPS */
	if (buf->len + sdu_hdr_len > ch->tx.mps) {
		goto segment;
	}

	headroom = BT_L2CAP_CHAN_SEND_RESERVE + sdu_hdr_len;

	/* Check if original buffer has enough headroom and don't have any
	 * fragments.
	 */
	if (net_buf_headroom(buf) >= headroom && !buf->frags) {
		if (sdu_hdr_len) {
			/* Push SDU length if set */
			net_buf_push_le16(buf, net_buf_frags_len(buf));
		}
		return net_buf_ref(buf);
	}

segment:
	seg = l2cap_alloc_seg(buf);

	if (sdu_hdr_len) {
		net_buf_add_le16(seg, net_buf_frags_len(buf));
	}

	/* Don't send more that TX MPS including SDU length */
	len = min(net_buf_tailroom(seg), ch->tx.mps - sdu_hdr_len);
	/* Limit if original buffer is smaller than the segment */
	len = min(buf->len, len);
	net_buf_add_mem(seg, buf->data, len);
	net_buf_pull(buf, len);

	BT_DBG("ch %p seg %p len %u", ch, seg, seg->len);

	return seg;
}

static int l2cap_chan_le_send(struct bt_l2cap_le_chan *ch, struct net_buf *buf,
			      u16_t sdu_hdr_len)
{
	int len;

	/* Wait for credits */
	if (k_sem_take(&ch->tx.credits, K_NO_WAIT)) {
		BT_DBG("No credits to transmit packet");
		return -EAGAIN;
	}

	buf = l2cap_chan_create_seg(ch, buf, sdu_hdr_len);

	/* Channel may have been disconnected while waiting for a buffer */
	if (!ch->chan.conn) {
		net_buf_unref(buf);
		return -ECONNRESET;
	}

	BT_DBG("ch %p cid 0x%04x len %u credits %u", ch, ch->tx.cid,
	       buf->len, k_sem_count_get(&ch->tx.credits));

	len = buf->len - sdu_hdr_len;

	bt_l2cap_send(ch->chan.conn, ch->tx.cid, buf);

	return len;
}

static int l2cap_chan_le_send_sdu(struct bt_l2cap_le_chan *ch,
				  struct net_buf **buf, int sent)
{
	int ret, total_len;
	struct net_buf *frag;

	total_len = net_buf_frags_len(*buf) + sent;

	if (total_len > ch->tx.mtu) {
		return -EMSGSIZE;
	}

	frag = *buf;
	if (!frag->len && frag->frags) {
		frag = frag->frags;
	}

	if (!sent) {
		/* Add SDU length for the first segment */
		ret = l2cap_chan_le_send(ch, frag, BT_L2CAP_SDU_HDR_LEN);
		if (ret < 0) {
			if (ret == -EAGAIN) {
				/* Store sent data into user_data */
				memcpy(net_buf_user_data(frag), &sent,
				       sizeof(sent));
			}
			*buf = frag;
			return ret;
		}
		sent = ret;
	}

	/* Send remaining segments */
	for (ret = 0; sent < total_len; sent += ret) {
		/* Proceed to next fragment */
		if (!frag->len) {
			frag = net_buf_frag_del(NULL, frag);
		}

		ret = l2cap_chan_le_send(ch, frag, 0);
		if (ret < 0) {
			if (ret == -EAGAIN) {
				/* Store sent data into user_data */
				memcpy(net_buf_user_data(frag), &sent,
				       sizeof(sent));
			}
			*buf = frag;
			return ret;
		}
	}

	BT_DBG("ch %p cid 0x%04x sent %u total_len %u", ch, ch->tx.cid, sent,
	       total_len);

	net_buf_unref(frag);

	return ret;
}

static struct net_buf *l2cap_chan_le_get_tx_buf(struct bt_l2cap_le_chan *ch)
{
	struct net_buf *buf;

	/* Return current buffer */
	if (ch->tx_buf) {
		buf = ch->tx_buf;
		ch->tx_buf = NULL;
		return buf;
	}

	return net_buf_get(&ch->tx_queue, K_NO_WAIT);
}

static void l2cap_chan_le_send_resume(struct bt_l2cap_le_chan *ch)
{
	struct net_buf *buf;

	/* Resume tx in case there are buffers in the queue */
	while ((buf = l2cap_chan_le_get_tx_buf(ch))) {
		int sent = *((int *)net_buf_user_data(buf));

		BT_DBG("buf %p sent %u", buf, sent);

		sent = l2cap_chan_le_send_sdu(ch, &buf, sent);
		if (sent < 0) {
			if (sent == -EAGAIN) {
				ch->tx_buf = buf;
			}
			break;
		}
	}
}

static void le_credits(struct bt_l2cap *l2cap, u8_t ident,
		       struct net_buf *buf)
{
	struct bt_conn *conn = l2cap->chan.chan.conn;
	struct bt_l2cap_chan *chan;
	struct bt_l2cap_le_credits *ev = (void *)buf->data;
	struct bt_l2cap_le_chan *ch;
	u16_t credits, cid;

	if (buf->len < sizeof(*ev)) {
		BT_ERR("Too small LE Credits packet size");
		return;
	}

	cid = sys_le16_to_cpu(ev->cid);
	credits = sys_le16_to_cpu(ev->credits);

	BT_DBG("cid 0x%04x credits %u", cid, credits);

	chan = bt_l2cap_le_lookup_tx_cid(conn, cid);
	if (!chan) {
		BT_ERR("Unable to find channel of LE Credits packet");
		return;
	}

	ch = BT_L2CAP_LE_CHAN(chan);

	if (k_sem_count_get(&ch->tx.credits) + credits > UINT16_MAX) {
		BT_ERR("Credits overflow");
		bt_l2cap_chan_disconnect(chan);
		return;
	}

	l2cap_chan_tx_give_credits(ch, credits);

	BT_DBG("chan %p total credits %u", ch,
	       k_sem_count_get(&ch->tx.credits));

	l2cap_chan_le_send_resume(ch);
}

static void reject_cmd(struct bt_l2cap *l2cap, u8_t ident,
		       struct net_buf *buf)
{
	struct bt_conn *conn = l2cap->chan.chan.conn;
	struct bt_l2cap_le_chan *chan;

	/* Check if there is a outstanding channel */
	chan = l2cap_remove_ident(conn, ident);
	if (!chan) {
		return;
	}

	bt_l2cap_chan_del(&chan->chan);
}
#endif /* CONFIG_BT_L2CAP_DYNAMIC_CHANNEL */

static void l2cap_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_l2cap *l2cap = CONTAINER_OF(chan, struct bt_l2cap, chan);
	struct bt_l2cap_sig_hdr *hdr = (void *)buf->data;
	u16_t len;

	if (buf->len < sizeof(*hdr)) {
		BT_ERR("Too small L2CAP signaling PDU");
		return;
	}

	len = sys_le16_to_cpu(hdr->len);
	net_buf_pull(buf, sizeof(*hdr));

	BT_DBG("Signaling code 0x%02x ident %u len %u", hdr->code,
	       hdr->ident, len);

	if (buf->len != len) {
		BT_ERR("L2CAP length mismatch (%u != %u)", buf->len, len);
		return;
	}

	if (!hdr->ident) {
		BT_ERR("Invalid ident value in L2CAP PDU");
		return;
	}

	switch (hdr->code) {
	case BT_L2CAP_CONN_PARAM_RSP:
		le_conn_param_rsp(l2cap, buf);
		break;
#if defined(CONFIG_BT_CENTRAL)
	case BT_L2CAP_CONN_PARAM_REQ:
		le_conn_param_update_req(l2cap, hdr->ident, buf);
		break;
#endif /* CONFIG_BT_CENTRAL */
#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
	case BT_L2CAP_LE_CONN_REQ:
		le_conn_req(l2cap, hdr->ident, buf);
		break;
	case BT_L2CAP_LE_CONN_RSP:
		le_conn_rsp(l2cap, hdr->ident, buf);
		break;
	case BT_L2CAP_DISCONN_REQ:
		le_disconn_req(l2cap, hdr->ident, buf);
		break;
	case BT_L2CAP_DISCONN_RSP:
		le_disconn_rsp(l2cap, hdr->ident, buf);
		break;
	case BT_L2CAP_LE_CREDITS:
		le_credits(l2cap, hdr->ident, buf);
		break;
	case BT_L2CAP_CMD_REJECT:
		reject_cmd(l2cap, hdr->ident, buf);
		break;
#else
	case BT_L2CAP_CMD_REJECT:
		/* Ignored */
		break;
#endif /* CONFIG_BT_L2CAP_DYNAMIC_CHANNEL */
	default:
		BT_WARN("Unknown L2CAP PDU code 0x%02x", hdr->code);
		l2cap_send_reject(chan->conn, hdr->ident,
				  BT_L2CAP_REJ_NOT_UNDERSTOOD, NULL, 0);
		break;
	}
}

#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
static void l2cap_chan_update_credits(struct bt_l2cap_le_chan *chan,
				      struct net_buf *buf)
{
	struct bt_l2cap_le_credits *ev;
	u16_t credits;

	/* Only give more credits if it went bellow the defined threshold */
	if (k_sem_count_get(&chan->rx.credits) >
	    L2CAP_LE_CREDITS_THRESHOLD(chan->rx.init_credits)) {
		goto done;
	}

	/* Restore credits */
	credits = chan->rx.init_credits - k_sem_count_get(&chan->rx.credits);
	l2cap_chan_rx_give_credits(chan, credits);

	buf = l2cap_create_le_sig_pdu(buf, BT_L2CAP_LE_CREDITS, get_ident(),
				      sizeof(*ev));

	ev = net_buf_add(buf, sizeof(*ev));
	ev->cid = sys_cpu_to_le16(chan->rx.cid);
	ev->credits = sys_cpu_to_le16(credits);

	bt_l2cap_send(chan->chan.conn, BT_L2CAP_CID_LE_SIG, buf);

done:
	BT_DBG("chan %p credits %u", chan, k_sem_count_get(&chan->rx.credits));
}

static struct net_buf *l2cap_alloc_frag(struct bt_l2cap_le_chan *chan)
{
	struct net_buf *frag = NULL;

	frag = chan->chan.ops->alloc_buf(&chan->chan);
	if (!frag) {
		return NULL;
	}

	BT_DBG("frag %p tailroom %zu", frag, net_buf_tailroom(frag));

	net_buf_frag_add(chan->_sdu, frag);

	return frag;
}

static void l2cap_chan_le_recv_sdu(struct bt_l2cap_le_chan *chan,
				   struct net_buf *buf)
{
	struct net_buf *frag;
	u16_t len;

	BT_DBG("chan %p len %u sdu %zu", chan, buf->len,
	       net_buf_frags_len(chan->_sdu));

	if (net_buf_frags_len(chan->_sdu) + buf->len > chan->_sdu_len) {
		BT_ERR("SDU length mismatch");
		bt_l2cap_chan_disconnect(&chan->chan);
		return;
	}

	/* Jump to last fragment */
	frag = net_buf_frag_last(chan->_sdu);

	while (buf->len) {
		/* Check if there is any space left in the current fragment */
		if (!net_buf_tailroom(frag)) {
			frag = l2cap_alloc_frag(chan);
			if (!frag) {
				BT_ERR("Unable to store SDU");
				bt_l2cap_chan_disconnect(&chan->chan);
				return;
			}
		}

		len = min(net_buf_tailroom(frag), buf->len);
		net_buf_add_mem(frag, buf->data, len);
		net_buf_pull(buf, len);

		BT_DBG("frag %p len %u", frag, frag->len);
	}

	if (net_buf_frags_len(chan->_sdu) == chan->_sdu_len) {
		/* Receiving complete SDU, notify channel and reset SDU buf */
		chan->chan.ops->recv(&chan->chan, chan->_sdu);
		net_buf_unref(chan->_sdu);
		chan->_sdu = NULL;
		chan->_sdu_len = 0;
	}

	l2cap_chan_update_credits(chan, buf);
}

static void l2cap_chan_le_recv(struct bt_l2cap_le_chan *chan,
			       struct net_buf *buf)
{
	u16_t sdu_len;

	if (k_sem_take(&chan->rx.credits, K_NO_WAIT)) {
		BT_ERR("No credits to receive packet");
		bt_l2cap_chan_disconnect(&chan->chan);
		return;
	}

	/* Check if segments already exist */
	if (chan->_sdu) {
		l2cap_chan_le_recv_sdu(chan, buf);
		return;
	}

	sdu_len = net_buf_pull_le16(buf);

	BT_DBG("chan %p len %u sdu_len %u", chan, buf->len, sdu_len);

	if (sdu_len > chan->rx.mtu) {
		BT_ERR("Invalid SDU length");
		bt_l2cap_chan_disconnect(&chan->chan);
		return;
	}

	/* Always allocate buffer from the channel if supported. */
	if (chan->chan.ops->alloc_buf) {
		chan->_sdu = chan->chan.ops->alloc_buf(&chan->chan);
		if (!chan->_sdu) {
			BT_ERR("Unable to allocate buffer for SDU");
			bt_l2cap_chan_disconnect(&chan->chan);
			return;
		}
		chan->_sdu_len = sdu_len;
		l2cap_chan_le_recv_sdu(chan, buf);
		return;
	}

	chan->chan.ops->recv(&chan->chan, buf);

	l2cap_chan_update_credits(chan, buf);
}
#endif /* CONFIG_BT_L2CAP_DYNAMIC_CHANNEL */

static void l2cap_chan_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
	struct bt_l2cap_le_chan *ch = BT_L2CAP_LE_CHAN(chan);

	if (L2CAP_LE_CID_IS_DYN(ch->rx.cid)) {
		l2cap_chan_le_recv(ch, buf);
		return;
	}
#endif /* CONFIG_BT_L2CAP_DYNAMIC_CHANNEL */

	BT_DBG("chan %p len %u", chan, buf->len);

	chan->ops->recv(chan, buf);
}

void bt_l2cap_recv(struct bt_conn *conn, struct net_buf *buf)
{
	struct bt_l2cap_hdr *hdr = (void *)buf->data;
	struct bt_l2cap_chan *chan;
	u16_t cid;

	if (IS_ENABLED(CONFIG_BT_BREDR) &&
	    conn->type == BT_CONN_TYPE_BR) {
		bt_l2cap_br_recv(conn, buf);
		return;
	}

	if (buf->len < sizeof(*hdr)) {
		BT_ERR("Too small L2CAP PDU received");
		net_buf_unref(buf);
		return;
	}

	cid = sys_le16_to_cpu(hdr->cid);
	net_buf_pull(buf, sizeof(*hdr));

	BT_DBG("Packet for CID %u len %u", cid, buf->len);

	chan = bt_l2cap_le_lookup_rx_cid(conn, cid);
	if (!chan) {
		BT_WARN("Ignoring data for unknown CID 0x%04x", cid);
		net_buf_unref(buf);
		return;
	}

	l2cap_chan_recv(chan, buf);
	net_buf_unref(buf);
}

int bt_l2cap_update_conn_param(struct bt_conn *conn,
			       const struct bt_le_conn_param *param)
{
	struct bt_l2cap_conn_param_req *req;
	struct net_buf *buf;

	buf = l2cap_create_le_sig_pdu(NULL, BT_L2CAP_CONN_PARAM_REQ,
				      get_ident(), sizeof(*req));

	req = net_buf_add(buf, sizeof(*req));
	req->min_interval = sys_cpu_to_le16(param->interval_min);
	req->max_interval = sys_cpu_to_le16(param->interval_max);
	req->latency = sys_cpu_to_le16(param->latency);
	req->timeout = sys_cpu_to_le16(param->timeout);

	bt_l2cap_send(conn, BT_L2CAP_CID_LE_SIG, buf);

	return 0;
}

static void l2cap_connected(struct bt_l2cap_chan *chan)
{
	BT_DBG("ch %p cid 0x%04x", BT_L2CAP_LE_CHAN(chan),
	       BT_L2CAP_LE_CHAN(chan)->rx.cid);
}

static void l2cap_disconnected(struct bt_l2cap_chan *chan)
{
	BT_DBG("ch %p cid 0x%04x", BT_L2CAP_LE_CHAN(chan),
	       BT_L2CAP_LE_CHAN(chan)->rx.cid);
}

static int l2cap_accept(struct bt_conn *conn, struct bt_l2cap_chan **chan)
{
	int i;
	static struct bt_l2cap_chan_ops ops = {
		.connected = l2cap_connected,
		.disconnected = l2cap_disconnected,
		.recv = l2cap_recv,
	};

	BT_DBG("conn %p handle %u", conn, conn->handle);

	for (i = 0; i < ARRAY_SIZE(bt_l2cap_pool); i++) {
		struct bt_l2cap *l2cap = &bt_l2cap_pool[i];

		if (l2cap->chan.chan.conn) {
			continue;
		}

		l2cap->chan.chan.ops = &ops;
		*chan = &l2cap->chan.chan;

		return 0;
	}

	BT_ERR("No available L2CAP context for conn %p", conn);

	return -ENOMEM;
}

void bt_l2cap_init(void)
{
	static struct bt_l2cap_fixed_chan chan = {
		.cid	= BT_L2CAP_CID_LE_SIG,
		.accept	= l2cap_accept,
	};

	bt_l2cap_le_fixed_chan_register(&chan);

	if (IS_ENABLED(CONFIG_BT_BREDR)) {
		bt_l2cap_br_init();
	}
}

#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
static int l2cap_le_connect(struct bt_conn *conn, struct bt_l2cap_le_chan *ch,
			    u16_t psm)
{
	if (psm < L2CAP_LE_PSM_FIXED_START || psm > L2CAP_LE_PSM_DYN_END) {
		return -EINVAL;
	}

	l2cap_chan_tx_init(ch);
	l2cap_chan_rx_init(ch);

	if (!l2cap_chan_add(conn, &ch->chan, l2cap_chan_destroy)) {
		return -ENOMEM;
	}

	ch->chan.psm = psm;

	return l2cap_le_conn_req(ch);
}

int bt_l2cap_chan_connect(struct bt_conn *conn, struct bt_l2cap_chan *chan,
			  u16_t psm)
{
	BT_DBG("conn %p chan %p psm 0x%04x", conn, chan, psm);

	if (!conn || conn->state != BT_CONN_CONNECTED) {
		return -ENOTCONN;
	}

	if (!chan) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_BREDR) &&
	    conn->type == BT_CONN_TYPE_BR) {
		return bt_l2cap_br_chan_connect(conn, chan, psm);
	}

	if (chan->required_sec_level > BT_SECURITY_FIPS) {
		return -EINVAL;
	} else if (chan->required_sec_level == BT_SECURITY_NONE) {
		chan->required_sec_level = BT_SECURITY_LOW;
	}

	return l2cap_le_connect(conn, BT_L2CAP_LE_CHAN(chan), psm);
}

int bt_l2cap_chan_disconnect(struct bt_l2cap_chan *chan)
{
	struct bt_conn *conn = chan->conn;
	struct net_buf *buf;
	struct bt_l2cap_disconn_req *req;
	struct bt_l2cap_le_chan *ch;

	if (!conn) {
		return -ENOTCONN;
	}

	if (IS_ENABLED(CONFIG_BT_BREDR) &&
	    conn->type == BT_CONN_TYPE_BR) {
		return bt_l2cap_br_chan_disconnect(chan);
	}

	ch = BT_L2CAP_LE_CHAN(chan);

	BT_DBG("chan %p scid 0x%04x dcid 0x%04x", chan, ch->rx.cid,
	       ch->tx.cid);

	ch->chan.ident = get_ident();

	buf = l2cap_create_le_sig_pdu(NULL, BT_L2CAP_DISCONN_REQ,
				      ch->chan.ident, sizeof(*req));

	req = net_buf_add(buf, sizeof(*req));
	req->dcid = sys_cpu_to_le16(ch->tx.cid);
	req->scid = sys_cpu_to_le16(ch->rx.cid);

	l2cap_chan_send_req(ch, buf, L2CAP_DISC_TIMEOUT);
	bt_l2cap_chan_set_state(chan, BT_L2CAP_DISCONNECT);

	return 0;
}

int bt_l2cap_chan_send(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	int err;

	if (!buf) {
		return -EINVAL;
	}

	BT_DBG("chan %p buf %p len %zu", chan, buf, net_buf_frags_len(buf));

	if (!chan->conn || chan->conn->state != BT_CONN_CONNECTED) {
		return -ENOTCONN;
	}

	if (IS_ENABLED(CONFIG_BT_BREDR) &&
	    chan->conn->type == BT_CONN_TYPE_BR) {
		return bt_l2cap_br_chan_send(chan, buf);
	}

	err = l2cap_chan_le_send_sdu(BT_L2CAP_LE_CHAN(chan), &buf, 0);
	if (err < 0) {
		if (err == -EAGAIN) {
			/* Queue buffer to be sent later */
			net_buf_put(&(BT_L2CAP_LE_CHAN(chan))->tx_queue, buf);
			return *((int *)net_buf_user_data(buf));
		}
		BT_ERR("failed to send message %d", err);
	}

	return err;
}
#endif /* CONFIG_BT_L2CAP_DYNAMIC_CHANNEL */
