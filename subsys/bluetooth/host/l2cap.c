/* l2cap.c - L2CAP handling */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/drivers/bluetooth/hci_driver.h>

#define LOG_DBG_ENABLED IS_ENABLED(CONFIG_BT_L2CAP_LOG_LEVEL_DBG)

#include "hci_core.h"
#include "conn_internal.h"
#include "l2cap_internal.h"
#include "keys.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_l2cap, CONFIG_BT_L2CAP_LOG_LEVEL);

#define LE_CHAN_RTX(_w) CONTAINER_OF(_w, struct bt_l2cap_le_chan, rtx_work)
#define CHAN_RX(_w) CONTAINER_OF(_w, struct bt_l2cap_le_chan, rx_work)

#define L2CAP_LE_MIN_MTU		23
#define L2CAP_ECRED_MIN_MTU		64

#define L2CAP_LE_MAX_CREDITS		(CONFIG_BT_BUF_ACL_RX_COUNT - 1)

#define L2CAP_LE_CID_DYN_START	0x0040
#define L2CAP_LE_CID_DYN_END	0x007f
#define L2CAP_LE_CID_IS_DYN(_cid) \
	(_cid >= L2CAP_LE_CID_DYN_START && _cid <= L2CAP_LE_CID_DYN_END)

#define L2CAP_LE_PSM_FIXED_START 0x0001
#define L2CAP_LE_PSM_FIXED_END   0x007f
#define L2CAP_LE_PSM_DYN_START   0x0080
#define L2CAP_LE_PSM_DYN_END     0x00ff
#define L2CAP_LE_PSM_IS_DYN(_psm) \
	(_psm >= L2CAP_LE_PSM_DYN_START && _psm <= L2CAP_LE_PSM_DYN_END)

#define L2CAP_CONN_TIMEOUT	K_SECONDS(40)
#define L2CAP_DISC_TIMEOUT	K_SECONDS(2)
#define L2CAP_RTX_TIMEOUT	K_SECONDS(2)

#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
/* Dedicated pool for disconnect buffers so they are guaranteed to be send
 * even in case of data congestion due to flooding.
 */
NET_BUF_POOL_FIXED_DEFINE(disc_pool, 1,
			  BT_L2CAP_BUF_SIZE(
				sizeof(struct bt_l2cap_sig_hdr) +
				sizeof(struct bt_l2cap_disconn_req)),
			  8, NULL);

#define l2cap_lookup_ident(conn, ident) __l2cap_lookup_ident(conn, ident, false)
#define l2cap_remove_ident(conn, ident) __l2cap_lookup_ident(conn, ident, true)

struct l2cap_tx_meta_data {
	int sent;
	uint16_t cid;
	bt_conn_tx_cb_t cb;
	void *user_data;
};

struct l2cap_tx_meta {
	struct l2cap_tx_meta_data *data;
};

static struct l2cap_tx_meta_data l2cap_tx_meta_data_storage[CONFIG_BT_CONN_TX_MAX];
K_FIFO_DEFINE(free_l2cap_tx_meta_data);

static struct l2cap_tx_meta_data *alloc_tx_meta_data(void)
{
	return k_fifo_get(&free_l2cap_tx_meta_data, K_NO_WAIT);
}

static void free_tx_meta_data(struct l2cap_tx_meta_data *data)
{
	(void)memset(data, 0, sizeof(*data));
	k_fifo_put(&free_l2cap_tx_meta_data, data);
}

#define l2cap_tx_meta_data(buf) (((struct l2cap_tx_meta *)net_buf_user_data(buf))->data)

static sys_slist_t servers;

#endif /* CONFIG_BT_L2CAP_DYNAMIC_CHANNEL */

/* L2CAP signalling channel specific context */
struct bt_l2cap {
	/* The channel this context is associated with */
	struct bt_l2cap_le_chan	chan;
};

static const struct bt_l2cap_ecred_cb *ecred_cb;
static struct bt_l2cap bt_l2cap_pool[CONFIG_BT_MAX_CONN];

void bt_l2cap_register_ecred_cb(const struct bt_l2cap_ecred_cb *cb)
{
	ecred_cb = cb;
}

static uint8_t get_ident(void)
{
	static uint8_t ident;

	ident++;
	/* handle integer overflow (0 is not valid) */
	if (!ident) {
		ident++;
	}

	return ident;
}

#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
static struct bt_l2cap_le_chan *l2cap_chan_alloc_cid(struct bt_conn *conn,
						     struct bt_l2cap_chan *chan)
{
	struct bt_l2cap_le_chan *le_chan = BT_L2CAP_LE_CHAN(chan);
	uint16_t cid;

	/*
	 * No action needed if there's already a CID allocated, e.g. in
	 * the case of a fixed channel.
	 */
	if (le_chan->rx.cid > 0) {
		return le_chan;
	}

	for (cid = L2CAP_LE_CID_DYN_START; cid <= L2CAP_LE_CID_DYN_END; cid++) {
		if (!bt_l2cap_le_lookup_rx_cid(conn, cid)) {
			le_chan->rx.cid = cid;
			return le_chan;
		}
	}

	return NULL;
}

static struct bt_l2cap_le_chan *
__l2cap_lookup_ident(struct bt_conn *conn, uint16_t ident, bool remove)
{
	struct bt_l2cap_chan *chan;
	sys_snode_t *prev = NULL;

	SYS_SLIST_FOR_EACH_CONTAINER(&conn->channels, chan, node) {
		if (BT_L2CAP_LE_CHAN(chan)->ident == ident) {
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

const char *bt_l2cap_chan_state_str(bt_l2cap_chan_state_t state)
{
	switch (state) {
	case BT_L2CAP_DISCONNECTED:
		return "disconnected";
	case BT_L2CAP_CONNECTING:
		return "connecting";
	case BT_L2CAP_CONFIG:
		return "config";
	case BT_L2CAP_CONNECTED:
		return "connected";
	case BT_L2CAP_DISCONNECTING:
		return "disconnecting";
	default:
		return "unknown";
	}
}

#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
#if defined(CONFIG_BT_L2CAP_LOG_LEVEL_DBG)
void bt_l2cap_chan_set_state_debug(struct bt_l2cap_chan *chan,
				   bt_l2cap_chan_state_t state,
				   const char *func, int line)
{
	struct bt_l2cap_le_chan *le_chan = BT_L2CAP_LE_CHAN(chan);

	LOG_DBG("chan %p psm 0x%04x %s -> %s", chan, le_chan->psm,
		bt_l2cap_chan_state_str(le_chan->state), bt_l2cap_chan_state_str(state));

	/* check transitions validness */
	switch (state) {
	case BT_L2CAP_DISCONNECTED:
		/* regardless of old state always allows this state */
		break;
	case BT_L2CAP_CONNECTING:
		if (le_chan->state != BT_L2CAP_DISCONNECTED) {
			LOG_WRN("%s()%d: invalid transition", func, line);
		}
		break;
	case BT_L2CAP_CONFIG:
		if (le_chan->state != BT_L2CAP_CONNECTING) {
			LOG_WRN("%s()%d: invalid transition", func, line);
		}
		break;
	case BT_L2CAP_CONNECTED:
		if (le_chan->state != BT_L2CAP_CONFIG &&
		    le_chan->state != BT_L2CAP_CONNECTING) {
			LOG_WRN("%s()%d: invalid transition", func, line);
		}
		break;
	case BT_L2CAP_DISCONNECTING:
		if (le_chan->state != BT_L2CAP_CONFIG &&
		    le_chan->state != BT_L2CAP_CONNECTED) {
			LOG_WRN("%s()%d: invalid transition", func, line);
		}
		break;
	default:
		LOG_ERR("%s()%d: unknown (%u) state was set", func, line, state);
		return;
	}

	le_chan->state = state;
}
#else
void bt_l2cap_chan_set_state(struct bt_l2cap_chan *chan,
			     bt_l2cap_chan_state_t state)
{
	BT_L2CAP_LE_CHAN(chan)->state = state;
}
#endif /* CONFIG_BT_L2CAP_LOG_LEVEL_DBG */
#endif /* CONFIG_BT_L2CAP_DYNAMIC_CHANNEL */

void bt_l2cap_chan_del(struct bt_l2cap_chan *chan)
{
	const struct bt_l2cap_chan_ops *ops = chan->ops;

	LOG_DBG("conn %p chan %p", chan->conn, chan);

	if (!chan->conn) {
		goto destroy;
	}

	if (ops->disconnected) {
		ops->disconnected(chan);
	}

	chan->conn = NULL;

destroy:
#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
	/* Reset internal members of common channel */
	bt_l2cap_chan_set_state(chan, BT_L2CAP_DISCONNECTED);
	BT_L2CAP_LE_CHAN(chan)->psm = 0U;
#endif
	if (chan->destroy) {
		chan->destroy(chan);
	}

	if (ops->released) {
		ops->released(chan);
	}
}

#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
static void l2cap_rtx_timeout(struct k_work *work)
{
	struct bt_l2cap_le_chan *chan = LE_CHAN_RTX(work);
	struct bt_conn *conn = chan->chan.conn;

	LOG_ERR("chan %p timeout", chan);

	bt_l2cap_chan_remove(conn, &chan->chan);
	bt_l2cap_chan_del(&chan->chan);

	/* Remove other channels if pending on the same ident */
	while ((chan = l2cap_remove_ident(conn, chan->ident))) {
		bt_l2cap_chan_del(&chan->chan);
	}
}

static void l2cap_chan_le_recv(struct bt_l2cap_le_chan *chan,
			       struct net_buf *buf);

static void l2cap_rx_process(struct k_work *work)
{
	struct bt_l2cap_le_chan *ch = CHAN_RX(work);
	struct net_buf *buf;

	while ((buf = net_buf_get(&ch->rx_queue, K_NO_WAIT))) {
		LOG_DBG("ch %p buf %p", ch, buf);
		l2cap_chan_le_recv(ch, buf);
		net_buf_unref(buf);
	}
}
#endif /* CONFIG_BT_L2CAP_DYNAMIC_CHANNEL */

void bt_l2cap_chan_add(struct bt_conn *conn, struct bt_l2cap_chan *chan,
		       bt_l2cap_chan_destroy_t destroy)
{
	/* Attach channel to the connection */
	sys_slist_append(&conn->channels, &chan->node);
	chan->conn = conn;
	chan->destroy = destroy;

	LOG_DBG("conn %p chan %p", conn, chan);
}

static bool l2cap_chan_add(struct bt_conn *conn, struct bt_l2cap_chan *chan,
			   bt_l2cap_chan_destroy_t destroy)
{
	struct bt_l2cap_le_chan *le_chan;

#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
	le_chan = l2cap_chan_alloc_cid(conn, chan);
#else
	le_chan = BT_L2CAP_LE_CHAN(chan);
#endif

	if (!le_chan) {
		LOG_ERR("Unable to allocate L2CAP channel ID");
		return false;
	}

	atomic_clear(chan->status);

	bt_l2cap_chan_add(conn, chan, destroy);

#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
	/* All dynamic channels have the destroy handler which makes sure that
	 * the RTX work structure is properly released with a cancel sync.
	 * The fixed signal channel is only removed when disconnected and the
	 * disconnected handler is always called from the workqueue itself so
	 * canceling from there should always succeed.
	 */
	k_work_init_delayable(&le_chan->rtx_work, l2cap_rtx_timeout);

	if (L2CAP_LE_CID_IS_DYN(le_chan->rx.cid)) {
		k_work_init(&le_chan->rx_work, l2cap_rx_process);
		k_fifo_init(&le_chan->rx_queue);
		bt_l2cap_chan_set_state(chan, BT_L2CAP_CONNECTING);
	}
#endif /* CONFIG_BT_L2CAP_DYNAMIC_CHANNEL */

	return true;
}

void bt_l2cap_connected(struct bt_conn *conn)
{
	struct bt_l2cap_chan *chan;

	if (IS_ENABLED(CONFIG_BT_BREDR) &&
	    conn->type == BT_CONN_TYPE_BR) {
		bt_l2cap_br_connected(conn);
		return;
	}

	STRUCT_SECTION_FOREACH(bt_l2cap_fixed_chan, fchan) {
		struct bt_l2cap_le_chan *le_chan;

		if (fchan->accept(conn, &chan) < 0) {
			continue;
		}

		le_chan = BT_L2CAP_LE_CHAN(chan);

		/* Fill up remaining fixed channel context attached in
		 * fchan->accept()
		 */
		le_chan->rx.cid = fchan->cid;
		le_chan->tx.cid = fchan->cid;

		if (!l2cap_chan_add(conn, chan, fchan->destroy)) {
			return;
		}

		if (chan->ops->connected) {
			chan->ops->connected(chan);
		}

		/* Always set output status to fixed channels */
		atomic_set_bit(chan->status, BT_L2CAP_STATUS_OUT);

		if (chan->ops->status) {
			chan->ops->status(chan, chan->status);
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
					       uint8_t code, uint8_t ident,
					       uint16_t len)
{
	struct bt_l2cap_sig_hdr *hdr;
	struct net_buf_pool *pool = NULL;

#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
	if (code == BT_L2CAP_DISCONN_REQ) {
		pool = &disc_pool;
	}
#endif
	/* Don't wait more than the minimum RTX timeout of 2 seconds */
	buf = bt_l2cap_create_pdu_timeout(pool, 0, L2CAP_RTX_TIMEOUT);
	if (!buf) {
		/* If it was not possible to allocate a buffer within the
		 * timeout return NULL.
		 */
		LOG_ERR("Unable to allocate buffer for op 0x%02x", code);
		return NULL;
	}

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->code = code;
	hdr->ident = ident;
	hdr->len = sys_cpu_to_le16(len);

	return buf;
}

/* Send the buffer and release it in case of failure.
 * Any other cleanup in failure to send should be handled by the disconnected
 * handler.
 */
static inline void l2cap_send(struct bt_conn *conn, uint16_t cid,
			      struct net_buf *buf)
{
	if (bt_l2cap_send(conn, cid, buf)) {
		net_buf_unref(buf);
	}
}

#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
static void l2cap_chan_send_req(struct bt_l2cap_chan *chan,
				struct net_buf *buf, k_timeout_t timeout)
{
	if (bt_l2cap_send(chan->conn, BT_L2CAP_CID_LE_SIG, buf)) {
		net_buf_unref(buf);
		return;
	}

	/* BLUETOOTH SPECIFICATION Version 4.2 [Vol 3, Part A] page 126:
	 *
	 * The value of this timer is implementation-dependent but the minimum
	 * initial value is 1 second and the maximum initial value is 60
	 * seconds. One RTX timer shall exist for each outstanding signaling
	 * request, including each Echo Request. The timer disappears on the
	 * final expiration, when the response is received, or the physical
	 * link is lost.
	 */
	k_work_reschedule(&(BT_L2CAP_LE_CHAN(chan)->rtx_work), timeout);
}

static int l2cap_le_conn_req(struct bt_l2cap_le_chan *ch)
{
	struct net_buf *buf;
	struct bt_l2cap_le_conn_req *req;

	ch->ident = get_ident();

	buf = l2cap_create_le_sig_pdu(NULL, BT_L2CAP_LE_CONN_REQ,
				      ch->ident, sizeof(*req));
	if (!buf) {
		return -ENOMEM;
	}

	req = net_buf_add(buf, sizeof(*req));
	req->psm = sys_cpu_to_le16(ch->psm);
	req->scid = sys_cpu_to_le16(ch->rx.cid);
	req->mtu = sys_cpu_to_le16(ch->rx.mtu);
	req->mps = sys_cpu_to_le16(ch->rx.mps);
	req->credits = sys_cpu_to_le16(1);
	atomic_set(&ch->rx.credits, 1);

	l2cap_chan_send_req(&ch->chan, buf, L2CAP_CONN_TIMEOUT);

	return 0;
}

#if defined(CONFIG_BT_L2CAP_ECRED)
static int l2cap_ecred_conn_req(struct bt_l2cap_chan **chan, int channels)
{
	struct net_buf *buf;
	struct bt_l2cap_ecred_conn_req *req;
	struct bt_l2cap_le_chan *ch;
	int i;
	uint8_t ident;
	uint16_t req_psm;

	if (!chan || !channels) {
		return -EINVAL;
	}

	ident = get_ident();

	buf = l2cap_create_le_sig_pdu(NULL, BT_L2CAP_ECRED_CONN_REQ, ident,
				      sizeof(*req) +
				      (channels * sizeof(uint16_t)));

	req = net_buf_add(buf, sizeof(*req));

	ch = BT_L2CAP_LE_CHAN(chan[0]);

	/* Init common parameters */
	req->psm = sys_cpu_to_le16(ch->psm);
	req->mtu = sys_cpu_to_le16(ch->rx.mtu);
	req->mps = sys_cpu_to_le16(ch->rx.mps);
	req->credits = sys_cpu_to_le16(1);
	req_psm = ch->psm;

	for (i = 0; i < channels; i++) {
		ch = BT_L2CAP_LE_CHAN(chan[i]);

		__ASSERT(ch->psm == req_psm,
			 "The PSM shall be the same for channels in the same request.");

		ch->ident = ident;

		net_buf_add_le16(buf, ch->rx.cid);
	}

	l2cap_chan_send_req(*chan, buf, L2CAP_CONN_TIMEOUT);

	return 0;
}
#endif /* defined(CONFIG_BT_L2CAP_ECRED) */

static void l2cap_le_encrypt_change(struct bt_l2cap_chan *chan, uint8_t status)
{
	int err;
	struct bt_l2cap_le_chan *le = BT_L2CAP_LE_CHAN(chan);

	/* Skip channels that are not pending waiting for encryption */
	if (!atomic_test_and_clear_bit(chan->status,
				       BT_L2CAP_STATUS_ENCRYPT_PENDING)) {
		return;
	}

	if (status) {
		goto fail;
	}

#if defined(CONFIG_BT_L2CAP_ECRED)
	if (le->ident) {
		struct bt_l2cap_chan *echan[L2CAP_ECRED_CHAN_MAX_PER_REQ];
		struct bt_l2cap_chan *ch;
		int i = 0;

		SYS_SLIST_FOR_EACH_CONTAINER(&chan->conn->channels, ch, node) {
			if (le->ident == BT_L2CAP_LE_CHAN(ch)->ident) {
				__ASSERT(i < L2CAP_ECRED_CHAN_MAX_PER_REQ,
					 "There can only be L2CAP_ECRED_CHAN_MAX_PER_REQ channels "
					 "from the same request.");
				atomic_clear_bit(ch->status, BT_L2CAP_STATUS_ENCRYPT_PENDING);
				echan[i++] = ch;
			}
		}

		/* Retry ecred connect */
		l2cap_ecred_conn_req(echan, i);
		return;
	}
#endif /* defined(CONFIG_BT_L2CAP_ECRED) */

	/* Retry to connect */
	err = l2cap_le_conn_req(le);
	if (err) {
		goto fail;
	}

	return;
fail:
	bt_l2cap_chan_remove(chan->conn, chan);
	bt_l2cap_chan_del(chan);
}
#endif /* CONFIG_BT_L2CAP_DYNAMIC_CHANNEL */

void bt_l2cap_security_changed(struct bt_conn *conn, uint8_t hci_status)
{
	struct bt_l2cap_chan *chan, *next;

	if (IS_ENABLED(CONFIG_BT_BREDR) &&
	    conn->type == BT_CONN_TYPE_BR) {
		l2cap_br_encrypt_change(conn, hci_status);
		return;
	}

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&conn->channels, chan, next, node) {
#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
		l2cap_le_encrypt_change(chan, hci_status);
#endif

		if (chan->ops->encrypt_change) {
			chan->ops->encrypt_change(chan, hci_status);
		}
	}
}

struct net_buf *bt_l2cap_create_pdu_timeout(struct net_buf_pool *pool,
					    size_t reserve,
					    k_timeout_t timeout)
{
	return bt_conn_create_pdu_timeout(pool,
					  sizeof(struct bt_l2cap_hdr) + reserve,
					  timeout);
}

int bt_l2cap_send_cb(struct bt_conn *conn, uint16_t cid, struct net_buf *buf,
		     bt_conn_tx_cb_t cb, void *user_data)
{
	struct bt_l2cap_hdr *hdr;

	LOG_DBG("conn %p cid %u len %zu", conn, cid, net_buf_frags_len(buf));

	hdr = net_buf_push(buf, sizeof(*hdr));
	hdr->len = sys_cpu_to_le16(buf->len - sizeof(*hdr));
	hdr->cid = sys_cpu_to_le16(cid);

	return bt_conn_send_cb(conn, buf, cb, user_data);
}

static void l2cap_send_reject(struct bt_conn *conn, uint8_t ident,
			      uint16_t reason, void *data, uint8_t data_len)
{
	struct bt_l2cap_cmd_reject *rej;
	struct net_buf *buf;

	buf = l2cap_create_le_sig_pdu(NULL, BT_L2CAP_CMD_REJECT, ident,
				      sizeof(*rej) + data_len);
	if (!buf) {
		return;
	}

	rej = net_buf_add(buf, sizeof(*rej));
	rej->reason = sys_cpu_to_le16(reason);

	if (data) {
		net_buf_add_mem(buf, data, data_len);
	}

	l2cap_send(conn, BT_L2CAP_CID_LE_SIG, buf);
}

static void le_conn_param_rsp(struct bt_l2cap *l2cap, struct net_buf *buf)
{
	struct bt_l2cap_conn_param_rsp *rsp = (void *)buf->data;

	if (buf->len < sizeof(*rsp)) {
		LOG_ERR("Too small LE conn param rsp");
		return;
	}

	LOG_DBG("LE conn param rsp result %u", sys_le16_to_cpu(rsp->result));
}

static void le_conn_param_update_req(struct bt_l2cap *l2cap, uint8_t ident,
				     struct net_buf *buf)
{
	struct bt_conn *conn = l2cap->chan.chan.conn;
	struct bt_le_conn_param param;
	struct bt_l2cap_conn_param_rsp *rsp;
	struct bt_l2cap_conn_param_req *req = (void *)buf->data;
	bool accepted;

	if (buf->len < sizeof(*req)) {
		LOG_ERR("Too small LE conn update param req");
		return;
	}

	if (conn->state != BT_CONN_CONNECTED) {
		LOG_WRN("Not connected");
		return;
	}

	if (conn->role != BT_HCI_ROLE_CENTRAL) {
		l2cap_send_reject(conn, ident, BT_L2CAP_REJ_NOT_UNDERSTOOD,
				  NULL, 0);
		return;
	}

	param.interval_min = sys_le16_to_cpu(req->min_interval);
	param.interval_max = sys_le16_to_cpu(req->max_interval);
	param.latency = sys_le16_to_cpu(req->latency);
	param.timeout = sys_le16_to_cpu(req->timeout);

	LOG_DBG("min 0x%04x max 0x%04x latency: 0x%04x timeout: 0x%04x", param.interval_min,
		param.interval_max, param.latency, param.timeout);

	buf = l2cap_create_le_sig_pdu(buf, BT_L2CAP_CONN_PARAM_RSP, ident,
				      sizeof(*rsp));
	if (!buf) {
		return;
	}

	accepted = le_param_req(conn, &param);

	rsp = net_buf_add(buf, sizeof(*rsp));
	if (accepted) {
		rsp->result = sys_cpu_to_le16(BT_L2CAP_CONN_PARAM_ACCEPTED);
	} else {
		rsp->result = sys_cpu_to_le16(BT_L2CAP_CONN_PARAM_REJECTED);
	}

	l2cap_send(conn, BT_L2CAP_CID_LE_SIG, buf);

	if (accepted) {
		bt_conn_le_conn_update(conn, &param);
	}
}

struct bt_l2cap_chan *bt_l2cap_le_lookup_tx_cid(struct bt_conn *conn,
						uint16_t cid)
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
						uint16_t cid)
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
struct bt_l2cap_server *bt_l2cap_server_lookup_psm(uint16_t psm)
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
		if (bt_l2cap_server_lookup_psm(server->psm)) {
			LOG_DBG("PSM already registered");
			return -EADDRINUSE;
		}
	} else {
		uint16_t psm;

		for (psm = L2CAP_LE_PSM_DYN_START;
		     psm <= L2CAP_LE_PSM_DYN_END; psm++) {
			if (!bt_l2cap_server_lookup_psm(psm)) {
				break;
			}
		}

		if (psm > L2CAP_LE_PSM_DYN_END) {
			LOG_WRN("No free dynamic PSMs available");
			return -EADDRNOTAVAIL;
		}

		LOG_DBG("Allocated PSM 0x%04x for new server", psm);
		server->psm = psm;
	}

	if (server->sec_level > BT_SECURITY_L4) {
		return -EINVAL;
	} else if (server->sec_level < BT_SECURITY_L1) {
		/* Level 0 is only applicable for BR/EDR */
		server->sec_level = BT_SECURITY_L1;
	}

	LOG_DBG("PSM 0x%04x", server->psm);

	sys_slist_append(&servers, &server->node);

	return 0;
}

static void l2cap_chan_rx_init(struct bt_l2cap_le_chan *chan)
{
	LOG_DBG("chan %p", chan);

	/* Use existing MTU if defined */
	if (!chan->rx.mtu) {
		/* If application has not provide the incoming L2CAP SDU MTU use
		 * an MTU that does not require segmentation.
		 */
		chan->rx.mtu = BT_L2CAP_SDU_RX_MTU;
	}

	/* MPS shall not be bigger than MTU + BT_L2CAP_SDU_HDR_SIZE as the
	 * remaining bytes cannot be used.
	 */
	chan->rx.mps = MIN(chan->rx.mtu + BT_L2CAP_SDU_HDR_SIZE,
			   BT_L2CAP_RX_MTU);

	/* Truncate MTU if channel have disabled segmentation but still have
	 * set an MTU which requires it.
	 */
	if (!chan->chan.ops->alloc_buf &&
	    (chan->rx.mps < chan->rx.mtu + BT_L2CAP_SDU_HDR_SIZE)) {
		LOG_WRN("Segmentation disabled but MTU > MPS, truncating MTU");
		chan->rx.mtu = chan->rx.mps - BT_L2CAP_SDU_HDR_SIZE;
	}

	atomic_set(&chan->rx.credits, 1);
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

static int l2cap_chan_le_send_sdu(struct bt_l2cap_le_chan *ch,
				  struct net_buf **buf, uint16_t sent);

static void l2cap_chan_tx_process(struct k_work *work)
{
	struct bt_l2cap_le_chan *ch;
	struct net_buf *buf;

	ch = CONTAINER_OF(work, struct bt_l2cap_le_chan, tx_work);

	/* Resume tx in case there are buffers in the queue */
	while ((buf = l2cap_chan_le_get_tx_buf(ch))) {
		int sent = l2cap_tx_meta_data(buf)->sent;

		LOG_DBG("buf %p sent %u", buf, sent);

		sent = l2cap_chan_le_send_sdu(ch, &buf, sent);
		if (sent < 0) {
			if (sent == -EAGAIN) {
				ch->tx_buf = buf;
				/* If we don't reschedule, and the app doesn't nudge l2cap (e.g. by
				 * sending another SDU), the channel will be stuck in limbo. To
				 * prevent this, we reschedule with a configurable delay.
				 */
				k_work_schedule(&ch->tx_work, K_MSEC(CONFIG_BT_L2CAP_RESCHED_MS));
			} else {
				net_buf_unref(buf);
			}
			break;
		}
	}
}

static void l2cap_chan_tx_init(struct bt_l2cap_le_chan *chan)
{
	LOG_DBG("chan %p", chan);

	(void)memset(&chan->tx, 0, sizeof(chan->tx));
	atomic_set(&chan->tx.credits, 0);
	k_fifo_init(&chan->tx_queue);
	k_work_init_delayable(&chan->tx_work, l2cap_chan_tx_process);
}

static void l2cap_chan_tx_give_credits(struct bt_l2cap_le_chan *chan,
				       uint16_t credits)
{
	LOG_DBG("chan %p credits %u", chan, credits);

	atomic_add(&chan->tx.credits, credits);

	if (!atomic_test_and_set_bit(chan->chan.status, BT_L2CAP_STATUS_OUT) &&
	    chan->chan.ops->status) {
		chan->chan.ops->status(&chan->chan, chan->chan.status);
	}
}

static void l2cap_chan_destroy(struct bt_l2cap_chan *chan)
{
	struct bt_l2cap_le_chan *le_chan = BT_L2CAP_LE_CHAN(chan);
	struct net_buf *buf;

	LOG_DBG("chan %p cid 0x%04x", le_chan, le_chan->rx.cid);

	/* Cancel ongoing work. Since the channel can be re-used after this
	 * we need to sync to make sure that the kernel does not have it
	 * in its queue anymore.
	 *
	 * In the case where we are in the context of executing the rtx_work
	 * item, we don't sync as it will deadlock the workqueue.
	 */
	if (k_current_get() != &le_chan->rtx_work.queue->thread) {
		k_work_cancel_delayable_sync(&le_chan->rtx_work, &le_chan->rtx_sync);
	} else {
		k_work_cancel_delayable(&le_chan->rtx_work);
	}

	if (le_chan->tx_buf) {
		net_buf_unref(le_chan->tx_buf);
		le_chan->tx_buf = NULL;
	}

	/* Remove buffers on the TX queue */
	while ((buf = net_buf_get(&le_chan->tx_queue, K_NO_WAIT))) {
		net_buf_unref(buf);
	}

	/* Remove buffers on the RX queue */
	while ((buf = net_buf_get(&le_chan->rx_queue, K_NO_WAIT))) {
		net_buf_unref(buf);
	}

	/* Destroy segmented SDU if it exists */
	if (le_chan->_sdu) {
		net_buf_unref(le_chan->_sdu);
		le_chan->_sdu = NULL;
		le_chan->_sdu_len = 0U;
	}
}

static uint16_t le_err_to_result(int err)
{
	switch (err) {
	case -ENOMEM:
		return BT_L2CAP_LE_ERR_NO_RESOURCES;
	case -EACCES:
		return BT_L2CAP_LE_ERR_AUTHORIZATION;
	case -EPERM:
		return BT_L2CAP_LE_ERR_KEY_SIZE;
	case -ENOTSUP:
		/* This handle the cases where a fixed channel is registered but
		 * for some reason (e.g. controller not suporting a feature)
		 * cannot be used.
		 */
		return BT_L2CAP_LE_ERR_PSM_NOT_SUPP;
	default:
		return BT_L2CAP_LE_ERR_UNACCEPT_PARAMS;
	}
}

static uint16_t l2cap_chan_accept(struct bt_conn *conn,
			       struct bt_l2cap_server *server, uint16_t scid,
			       uint16_t mtu, uint16_t mps, uint16_t credits,
			       struct bt_l2cap_chan **chan)
{
	struct bt_l2cap_le_chan *le_chan;
	int err;

	LOG_DBG("conn %p scid 0x%04x chan %p", conn, scid, chan);

	if (!L2CAP_LE_CID_IS_DYN(scid)) {
		return BT_L2CAP_LE_ERR_INVALID_SCID;
	}

	*chan = bt_l2cap_le_lookup_tx_cid(conn, scid);
	if (*chan) {
		return BT_L2CAP_LE_ERR_SCID_IN_USE;
	}

	/* Request server to accept the new connection and allocate the
	 * channel.
	 */
	err = server->accept(conn, chan);
	if (err < 0) {
		return le_err_to_result(err);
	}

	if (!(*chan)->ops->recv) {
		LOG_ERR("Mandatory callback 'recv' missing");
		return BT_L2CAP_LE_ERR_UNACCEPT_PARAMS;
	}

	le_chan = BT_L2CAP_LE_CHAN(*chan);

	le_chan->required_sec_level = server->sec_level;

	if (!l2cap_chan_add(conn, *chan, l2cap_chan_destroy)) {
		return BT_L2CAP_LE_ERR_NO_RESOURCES;
	}

	/* Init TX parameters */
	l2cap_chan_tx_init(le_chan);
	le_chan->tx.cid = scid;
	le_chan->tx.mps = mps;
	le_chan->tx.mtu = mtu;
	l2cap_chan_tx_give_credits(le_chan, credits);

	/* Init RX parameters */
	l2cap_chan_rx_init(le_chan);
	atomic_set(&le_chan->rx.credits, 1);

	/* Set channel PSM */
	le_chan->psm = server->psm;

	/* Update state */
	bt_l2cap_chan_set_state(*chan, BT_L2CAP_CONNECTED);

	if ((*chan)->ops->connected) {
		(*chan)->ops->connected(*chan);
	}

	return BT_L2CAP_LE_SUCCESS;
}

static uint16_t l2cap_check_security(struct bt_conn *conn,
				 struct bt_l2cap_server *server)
{
	if (IS_ENABLED(CONFIG_BT_CONN_DISABLE_SECURITY)) {
		return BT_L2CAP_LE_SUCCESS;
	}

	if (conn->sec_level >= server->sec_level) {
		return BT_L2CAP_LE_SUCCESS;
	}

	if (conn->sec_level > BT_SECURITY_L1) {
		return BT_L2CAP_LE_ERR_AUTHENTICATION;
	}

	/* If an LTK or an STK is available and encryption is required
	 * (LE security mode 1) but encryption is not enabled, the
	 * service request shall be rejected with the error code
	 * "Insufficient Encryption".
	 */
	if (bt_conn_ltk_present(conn)) {
		return BT_L2CAP_LE_ERR_ENCRYPTION;
	}

	return BT_L2CAP_LE_ERR_AUTHENTICATION;
}

static void le_conn_req(struct bt_l2cap *l2cap, uint8_t ident,
			struct net_buf *buf)
{
	struct bt_conn *conn = l2cap->chan.chan.conn;
	struct bt_l2cap_chan *chan;
	struct bt_l2cap_le_chan *le_chan;
	struct bt_l2cap_server *server;
	struct bt_l2cap_le_conn_req *req = (void *)buf->data;
	struct bt_l2cap_le_conn_rsp *rsp;
	uint16_t psm, scid, mtu, mps, credits;
	uint16_t result;

	if (buf->len < sizeof(*req)) {
		LOG_ERR("Too small LE conn req packet size");
		return;
	}

	psm = sys_le16_to_cpu(req->psm);
	scid = sys_le16_to_cpu(req->scid);
	mtu = sys_le16_to_cpu(req->mtu);
	mps = sys_le16_to_cpu(req->mps);
	credits = sys_le16_to_cpu(req->credits);

	LOG_DBG("psm 0x%02x scid 0x%04x mtu %u mps %u credits %u", psm, scid, mtu, mps, credits);

	if (mtu < L2CAP_LE_MIN_MTU || mps < L2CAP_LE_MIN_MTU) {
		LOG_ERR("Invalid LE-Conn Req params");
		return;
	}

	buf = l2cap_create_le_sig_pdu(buf, BT_L2CAP_LE_CONN_RSP, ident,
				      sizeof(*rsp));
	if (!buf) {
		return;
	}

	rsp = net_buf_add(buf, sizeof(*rsp));
	(void)memset(rsp, 0, sizeof(*rsp));

	/* Check if there is a server registered */
	server = bt_l2cap_server_lookup_psm(psm);
	if (!server) {
		rsp->result = sys_cpu_to_le16(BT_L2CAP_LE_ERR_PSM_NOT_SUPP);
		goto rsp;
	}

	/* Check if connection has minimum required security level */
	result = l2cap_check_security(conn, server);
	if (result != BT_L2CAP_LE_SUCCESS) {
		rsp->result = sys_cpu_to_le16(result);
		goto rsp;
	}

	result = l2cap_chan_accept(conn, server, scid, mtu, mps, credits,
				   &chan);
	if (result != BT_L2CAP_LE_SUCCESS) {
		rsp->result = sys_cpu_to_le16(result);
		goto rsp;
	}

	le_chan = BT_L2CAP_LE_CHAN(chan);

	/* Prepare response protocol data */
	rsp->dcid = sys_cpu_to_le16(le_chan->rx.cid);
	rsp->mps = sys_cpu_to_le16(le_chan->rx.mps);
	rsp->mtu = sys_cpu_to_le16(le_chan->rx.mtu);
	rsp->credits = sys_cpu_to_le16(1);
	atomic_set(&le_chan->rx.credits, 1);

	rsp->result = BT_L2CAP_LE_SUCCESS;

rsp:
	l2cap_send(conn, BT_L2CAP_CID_LE_SIG, buf);
}

#if defined(CONFIG_BT_L2CAP_ECRED)
static void le_ecred_conn_req(struct bt_l2cap *l2cap, uint8_t ident,
			      struct net_buf *buf)
{
	struct bt_conn *conn = l2cap->chan.chan.conn;
	struct bt_l2cap_chan *chan[L2CAP_ECRED_CHAN_MAX_PER_REQ];
	struct bt_l2cap_le_chan *ch = NULL;
	struct bt_l2cap_server *server;
	struct bt_l2cap_ecred_conn_req *req;
	struct bt_l2cap_ecred_conn_rsp *rsp;
	uint16_t mtu, mps, credits, result = BT_L2CAP_LE_SUCCESS;
	uint16_t psm = 0x0000;
	uint16_t scid, dcid[L2CAP_ECRED_CHAN_MAX_PER_REQ];
	int i = 0;
	uint8_t req_cid_count;

	/* set dcid to zeros here, in case of all connections refused error */
	memset(dcid, 0, sizeof(dcid));
	if (buf->len < sizeof(*req)) {
		LOG_ERR("Too small LE conn req packet size");
		result = BT_L2CAP_LE_ERR_INVALID_PARAMS;
		req_cid_count = 0;
		goto response;
	}

	req = net_buf_pull_mem(buf, sizeof(*req));
	req_cid_count = buf->len / sizeof(scid);

	if (buf->len > sizeof(dcid)) {
		LOG_ERR("Too large LE conn req packet size");
		req_cid_count = L2CAP_ECRED_CHAN_MAX_PER_REQ;
		result = BT_L2CAP_LE_ERR_INVALID_PARAMS;
		goto response;
	}

	psm = sys_le16_to_cpu(req->psm);
	mtu = sys_le16_to_cpu(req->mtu);
	mps = sys_le16_to_cpu(req->mps);
	credits = sys_le16_to_cpu(req->credits);

	LOG_DBG("psm 0x%02x mtu %u mps %u credits %u", psm, mtu, mps, credits);

	if (mtu < L2CAP_ECRED_MIN_MTU || mps < L2CAP_ECRED_MIN_MTU) {
		LOG_ERR("Invalid ecred conn req params");
		result = BT_L2CAP_LE_ERR_INVALID_PARAMS;
		goto response;
	}

	/* Check if there is a server registered */
	server = bt_l2cap_server_lookup_psm(psm);
	if (!server) {
		result = BT_L2CAP_LE_ERR_PSM_NOT_SUPP;
		goto response;
	}

	/* Check if connection has minimum required security level */
	result = l2cap_check_security(conn, server);
	if (result != BT_L2CAP_LE_SUCCESS) {
		goto response;
	}

	while (buf->len >= sizeof(scid)) {
		uint16_t rc;
		scid = net_buf_pull_le16(buf);

		rc = l2cap_chan_accept(conn, server, scid, mtu, mps,
				credits, &chan[i]);
		if (rc != BT_L2CAP_LE_SUCCESS) {
			result = rc;
		}
		switch (rc) {
		case BT_L2CAP_LE_SUCCESS:
			ch = BT_L2CAP_LE_CHAN(chan[i]);
			dcid[i++] = sys_cpu_to_le16(ch->rx.cid);
			continue;
		/* Some connections refused – invalid Source CID */
		/* Some connections refused – Source CID already allocated */
		/* Some connections refused – not enough resources
		 * available.
		 */
		default:
			/* If a Destination CID is 0x0000, the channel was not
			 * established.
			 */
			dcid[i++] = 0x0000;
			continue;
		}
	}

response:
	buf = l2cap_create_le_sig_pdu(buf, BT_L2CAP_ECRED_CONN_RSP, ident,
				      sizeof(*rsp) +
				      (sizeof(scid) * req_cid_count));
	if (!buf) {
		goto callback;
	}

	rsp = net_buf_add(buf, sizeof(*rsp));
	(void)memset(rsp, 0, sizeof(*rsp));
	if (ch) {
		rsp->mps = sys_cpu_to_le16(ch->rx.mps);
		rsp->mtu = sys_cpu_to_le16(ch->rx.mtu);
		rsp->credits = sys_cpu_to_le16(1);
	}
	rsp->result = sys_cpu_to_le16(result);

	net_buf_add_mem(buf, dcid, sizeof(scid) * req_cid_count);

	l2cap_send(conn, BT_L2CAP_CID_LE_SIG, buf);

callback:
	if (ecred_cb && ecred_cb->ecred_conn_req) {
		ecred_cb->ecred_conn_req(conn, result, psm);
	}
}

static void le_ecred_reconf_req(struct bt_l2cap *l2cap, uint8_t ident,
				struct net_buf *buf)
{
	struct bt_conn *conn = l2cap->chan.chan.conn;
	struct bt_l2cap_chan *chans[L2CAP_ECRED_CHAN_MAX_PER_REQ];
	struct bt_l2cap_ecred_reconf_req *req;
	struct bt_l2cap_ecred_reconf_rsp *rsp;
	uint16_t mtu, mps;
	uint16_t scid, result = BT_L2CAP_RECONF_SUCCESS;
	int chan_count = 0;
	bool mps_reduced = false;

	if (buf->len < sizeof(*req)) {
		LOG_ERR("Too small ecred reconf req packet size");
		return;
	}

	req = net_buf_pull_mem(buf, sizeof(*req));

	mtu = sys_le16_to_cpu(req->mtu);
	mps = sys_le16_to_cpu(req->mps);

	if (mps < L2CAP_ECRED_MIN_MTU) {
		result = BT_L2CAP_RECONF_OTHER_UNACCEPT;
		goto response;
	}

	if (mtu < L2CAP_ECRED_MIN_MTU) {
		result = BT_L2CAP_RECONF_INVALID_MTU;
		goto response;
	}

	while (buf->len >= sizeof(scid)) {
		struct bt_l2cap_chan *chan;
		scid = net_buf_pull_le16(buf);
		chan = bt_l2cap_le_lookup_tx_cid(conn, scid);
		if (!chan) {
			result = BT_L2CAP_RECONF_INVALID_CID;
			goto response;
		}

		if (BT_L2CAP_LE_CHAN(chan)->tx.mtu > mtu) {
			LOG_ERR("chan %p decreased MTU %u -> %u", chan,
				BT_L2CAP_LE_CHAN(chan)->tx.mtu, mtu);
			result = BT_L2CAP_RECONF_INVALID_MTU;
			goto response;
		}

		if (BT_L2CAP_LE_CHAN(chan)->tx.mps > mps) {
			mps_reduced = true;
		}

		chans[chan_count] = chan;
		chan_count++;
	}

	/* As per BT Core Spec V5.2 Vol. 3, Part A, section 7.11
	 * The request (...) shall not decrease the MPS of a channel
	 * if more than one channel is specified.
	 */
	if (mps_reduced && chan_count > 1) {
		result = BT_L2CAP_RECONF_INVALID_MPS;
		goto response;
	}

	for (int i = 0; i < chan_count; i++) {
		BT_L2CAP_LE_CHAN(chans[i])->tx.mtu = mtu;
		BT_L2CAP_LE_CHAN(chans[i])->tx.mps = mps;

		if (chans[i]->ops->reconfigured) {
			chans[i]->ops->reconfigured(chans[i]);
		}
	}

	LOG_DBG("mtu %u mps %u", mtu, mps);

response:
	buf = l2cap_create_le_sig_pdu(buf, BT_L2CAP_ECRED_RECONF_RSP, ident,
				      sizeof(*rsp));

	rsp = net_buf_add(buf, sizeof(*rsp));
	rsp->result = sys_cpu_to_le16(result);

	l2cap_send(conn, BT_L2CAP_CID_LE_SIG, buf);
}

static void le_ecred_reconf_rsp(struct bt_l2cap *l2cap, uint8_t ident,
				struct net_buf *buf)
{
	struct bt_conn *conn = l2cap->chan.chan.conn;
	struct bt_l2cap_ecred_reconf_rsp *rsp;
	struct bt_l2cap_le_chan *ch;
	uint16_t result;

	if (buf->len < sizeof(*rsp)) {
		LOG_ERR("Too small ecred reconf rsp packet size");
		return;
	}

	rsp = net_buf_pull_mem(buf, sizeof(*rsp));
	result = sys_le16_to_cpu(rsp->result);

	while ((ch = l2cap_lookup_ident(conn, ident))) {
		/* Stop timer started on REQ send. The timer is only set on one
		 * of the channels, but we don't want to make assumptions on
		 * which one it is.
		 */
		k_work_cancel_delayable(&ch->rtx_work);

		if (result == BT_L2CAP_LE_SUCCESS) {
			ch->rx.mtu = ch->pending_rx_mtu;
		}

		ch->pending_rx_mtu = 0;
		ch->ident = 0U;

		if (ch->chan.ops->reconfigured) {
			ch->chan.ops->reconfigured(&ch->chan);
		}
	}
}
#endif /* defined(CONFIG_BT_L2CAP_ECRED) */

static struct bt_l2cap_le_chan *l2cap_remove_rx_cid(struct bt_conn *conn,
						    uint16_t cid)
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

static void le_disconn_req(struct bt_l2cap *l2cap, uint8_t ident,
			   struct net_buf *buf)
{
	struct bt_conn *conn = l2cap->chan.chan.conn;
	struct bt_l2cap_le_chan *chan;
	struct bt_l2cap_disconn_req *req = (void *)buf->data;
	struct bt_l2cap_disconn_rsp *rsp;
	uint16_t dcid;

	if (buf->len < sizeof(*req)) {
		LOG_ERR("Too small LE conn req packet size");
		return;
	}

	dcid = sys_le16_to_cpu(req->dcid);

	LOG_DBG("dcid 0x%04x scid 0x%04x", dcid, sys_le16_to_cpu(req->scid));

	chan = l2cap_remove_rx_cid(conn, dcid);
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
	if (!buf) {
		return;
	}

	rsp = net_buf_add(buf, sizeof(*rsp));
	rsp->dcid = sys_cpu_to_le16(chan->rx.cid);
	rsp->scid = sys_cpu_to_le16(chan->tx.cid);

	bt_l2cap_chan_del(&chan->chan);

	l2cap_send(conn, BT_L2CAP_CID_LE_SIG, buf);
}

static int l2cap_change_security(struct bt_l2cap_le_chan *chan, uint16_t err)
{
	struct bt_conn *conn = chan->chan.conn;
	bt_security_t sec;
	int ret;

	if (atomic_test_bit(chan->chan.status,
			    BT_L2CAP_STATUS_ENCRYPT_PENDING)) {
		return -EINPROGRESS;
	}

	switch (err) {
	case BT_L2CAP_LE_ERR_ENCRYPTION:
		if (conn->sec_level >= BT_SECURITY_L2) {
			return -EALREADY;
		}

		sec = BT_SECURITY_L2;
		break;
	case BT_L2CAP_LE_ERR_AUTHENTICATION:
		if (conn->sec_level < BT_SECURITY_L2) {
			sec = BT_SECURITY_L2;
		} else if (conn->sec_level < BT_SECURITY_L3) {
			sec = BT_SECURITY_L3;
		} else if (conn->sec_level < BT_SECURITY_L4) {
			sec = BT_SECURITY_L4;
		} else {
			return -EALREADY;
		}
		break;
	default:
		return -EINVAL;
	}

	ret = bt_conn_set_security(chan->chan.conn, sec);
	if (ret < 0) {
		return ret;
	}

	atomic_set_bit(chan->chan.status, BT_L2CAP_STATUS_ENCRYPT_PENDING);

	return 0;
}

#if defined(CONFIG_BT_L2CAP_ECRED)
static void le_ecred_conn_rsp(struct bt_l2cap *l2cap, uint8_t ident,
			      struct net_buf *buf)
{
	struct bt_conn *conn = l2cap->chan.chan.conn;
	struct bt_l2cap_le_chan *chan;
	struct bt_l2cap_ecred_conn_rsp *rsp;
	uint16_t dcid, mtu, mps, credits, result, psm;
	uint8_t attempted = 0;
	uint8_t succeeded = 0;

	if (buf->len < sizeof(*rsp)) {
		LOG_ERR("Too small ecred conn rsp packet size");
		return;
	}

	rsp = net_buf_pull_mem(buf, sizeof(*rsp));
	mtu = sys_le16_to_cpu(rsp->mtu);
	mps = sys_le16_to_cpu(rsp->mps);
	credits = sys_le16_to_cpu(rsp->credits);
	result = sys_le16_to_cpu(rsp->result);

	LOG_DBG("mtu 0x%04x mps 0x%04x credits 0x%04x result %u", mtu, mps, credits, result);

	chan = l2cap_lookup_ident(conn, ident);
	if (chan) {
		psm = chan->psm;
	} else {
		psm = 0x0000;
	}

	switch (result) {
	case BT_L2CAP_LE_ERR_AUTHENTICATION:
	case BT_L2CAP_LE_ERR_ENCRYPTION:
		while ((chan = l2cap_lookup_ident(conn, ident))) {

			/* Cancel RTX work */
			k_work_cancel_delayable(&chan->rtx_work);

			/* If security needs changing wait it to be completed */
			if (!l2cap_change_security(chan, result)) {
				return;
			}
			bt_l2cap_chan_remove(conn, &chan->chan);
			bt_l2cap_chan_del(&chan->chan);
		}
		break;
	case BT_L2CAP_LE_SUCCESS:
	/* Some connections refused – invalid Source CID */
	case BT_L2CAP_LE_ERR_INVALID_SCID:
	/* Some connections refused – Source CID already allocated */
	case BT_L2CAP_LE_ERR_SCID_IN_USE:
	/* Some connections refused – not enough resources available */
	case BT_L2CAP_LE_ERR_NO_RESOURCES:
		while ((chan = l2cap_lookup_ident(conn, ident))) {
			struct bt_l2cap_chan *c;

			/* Cancel RTX work */
			k_work_cancel_delayable(&chan->rtx_work);

			if (buf->len < sizeof(dcid)) {
				LOG_ERR("Fewer dcid values than expected");
				bt_l2cap_chan_remove(conn, &chan->chan);
				bt_l2cap_chan_del(&chan->chan);
				continue;
			}

			dcid = net_buf_pull_le16(buf);
			attempted++;

			LOG_DBG("dcid 0x%04x", dcid);

			/* If a Destination CID is 0x0000, the channel was not
			 * established.
			 */
			if (!dcid) {
				bt_l2cap_chan_remove(conn, &chan->chan);
				bt_l2cap_chan_del(&chan->chan);
				continue;
			}

			c = bt_l2cap_le_lookup_tx_cid(conn, dcid);
			if (c) {
				/* If a device receives a
				 * L2CAP_CREDIT_BASED_CONNECTION_RSP packet
				 * with an already assigned Destination CID,
				 * then both the original channel and the new
				 * channel shall be immediately discarded and
				 * not used.
				 */
				bt_l2cap_chan_remove(conn, &chan->chan);
				bt_l2cap_chan_del(&chan->chan);
				bt_l2cap_chan_disconnect(c);
				continue;
			}

			chan->tx.cid = dcid;

			chan->ident = 0U;

			chan->tx.mtu = mtu;
			chan->tx.mps = mps;

			/* Update state */
			bt_l2cap_chan_set_state(&chan->chan,
						BT_L2CAP_CONNECTED);

			if (chan->chan.ops->connected) {
				chan->chan.ops->connected(&chan->chan);
			}

			/* Give credits */
			l2cap_chan_tx_give_credits(chan, credits);
			atomic_set(&chan->rx.credits, 1);

			succeeded++;
		}
		break;
	case BT_L2CAP_LE_ERR_PSM_NOT_SUPP:
	default:
		while ((chan = l2cap_remove_ident(conn, ident))) {
			bt_l2cap_chan_del(&chan->chan);
		}
		break;
	}

	if (ecred_cb && ecred_cb->ecred_conn_rsp) {
		ecred_cb->ecred_conn_rsp(conn, result, attempted, succeeded, psm);
	}
}
#endif /* CONFIG_BT_L2CAP_ECRED */

static void le_conn_rsp(struct bt_l2cap *l2cap, uint8_t ident,
			struct net_buf *buf)
{
	struct bt_conn *conn = l2cap->chan.chan.conn;
	struct bt_l2cap_le_chan *chan;
	struct bt_l2cap_le_conn_rsp *rsp = (void *)buf->data;
	uint16_t dcid, mtu, mps, credits, result;

	if (buf->len < sizeof(*rsp)) {
		LOG_ERR("Too small LE conn rsp packet size");
		return;
	}

	dcid = sys_le16_to_cpu(rsp->dcid);
	mtu = sys_le16_to_cpu(rsp->mtu);
	mps = sys_le16_to_cpu(rsp->mps);
	credits = sys_le16_to_cpu(rsp->credits);
	result = sys_le16_to_cpu(rsp->result);

	LOG_DBG("dcid 0x%04x mtu %u mps %u credits %u result 0x%04x", dcid, mtu, mps, credits,
		result);

	/* Keep the channel in case of security errors */
	if (result == BT_L2CAP_LE_SUCCESS ||
	    result == BT_L2CAP_LE_ERR_AUTHENTICATION ||
	    result == BT_L2CAP_LE_ERR_ENCRYPTION) {
		chan = l2cap_lookup_ident(conn, ident);
	} else {
		chan = l2cap_remove_ident(conn, ident);
	}

	if (!chan) {
		LOG_ERR("Cannot find channel for ident %u", ident);
		return;
	}

	/* Cancel RTX work */
	k_work_cancel_delayable(&chan->rtx_work);

	/* Reset ident since it got a response */
	chan->ident = 0U;

	switch (result) {
	case BT_L2CAP_LE_SUCCESS:
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
		atomic_set(&chan->rx.credits, 1);

		break;
	case BT_L2CAP_LE_ERR_AUTHENTICATION:
	case BT_L2CAP_LE_ERR_ENCRYPTION:
		/* If security needs changing wait it to be completed */
		if (l2cap_change_security(chan, result) == 0) {
			return;
		}
		bt_l2cap_chan_remove(conn, &chan->chan);
		__fallthrough;
	default:
		bt_l2cap_chan_del(&chan->chan);
	}
}

static void le_disconn_rsp(struct bt_l2cap *l2cap, uint8_t ident,
			   struct net_buf *buf)
{
	struct bt_conn *conn = l2cap->chan.chan.conn;
	struct bt_l2cap_le_chan *chan;
	struct bt_l2cap_disconn_rsp *rsp = (void *)buf->data;
	uint16_t scid;

	if (buf->len < sizeof(*rsp)) {
		LOG_ERR("Too small LE disconn rsp packet size");
		return;
	}

	scid = sys_le16_to_cpu(rsp->scid);

	LOG_DBG("dcid 0x%04x scid 0x%04x", sys_le16_to_cpu(rsp->dcid), scid);

	chan = l2cap_remove_rx_cid(conn, scid);
	if (!chan) {
		return;
	}

	bt_l2cap_chan_del(&chan->chan);
}

static inline struct net_buf *l2cap_alloc_seg(struct net_buf *buf, struct bt_l2cap_le_chan *ch)
{
	struct net_buf_pool *pool = net_buf_pool_get(buf->pool_id);
	struct net_buf *seg;

	/* Use the dedicated segment callback if registered */
	if (ch->chan.ops->alloc_seg) {
		seg = ch->chan.ops->alloc_seg(&ch->chan);
		__ASSERT_NO_MSG(seg);
	} else {
		/* Try to use original pool if possible */
		seg = net_buf_alloc(pool, K_NO_WAIT);
	}

	if (seg) {
		net_buf_reserve(seg, BT_L2CAP_CHAN_SEND_RESERVE);
		return seg;
	}

	/* Fallback to using global connection tx pool */
	return bt_l2cap_create_pdu_timeout(NULL, 0, K_NO_WAIT);
}

static struct net_buf *l2cap_chan_create_seg(struct bt_l2cap_le_chan *ch,
					     struct net_buf *buf,
					     size_t sdu_hdr_len)
{
	struct net_buf *seg;
	uint16_t headroom;
	uint16_t len;

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
	seg = l2cap_alloc_seg(buf, ch);

	if (!seg) {
		return NULL;
	}

	if (sdu_hdr_len) {
		net_buf_add_le16(seg, net_buf_frags_len(buf));
	}

	/* Don't send more that TX MPS including SDU length */
	len = MIN(net_buf_tailroom(seg), ch->tx.mps - sdu_hdr_len);
	/* Limit if original buffer is smaller than the segment */
	len = MIN(buf->len, len);
	net_buf_add_mem(seg, buf->data, len);
	net_buf_pull(buf, len);

	LOG_DBG("ch %p seg %p len %u", ch, seg, seg->len);

	return seg;
}

static void l2cap_chan_tx_resume(struct bt_l2cap_le_chan *ch)
{
	if (!atomic_get(&ch->tx.credits) ||
	    (k_fifo_is_empty(&ch->tx_queue) && !ch->tx_buf)) {
		return;
	}

	k_work_reschedule(&ch->tx_work, K_NO_WAIT);
}

static void l2cap_chan_sdu_sent(struct bt_conn *conn, void *user_data, int err)
{
	struct l2cap_tx_meta_data *data = user_data;
	struct bt_l2cap_chan *chan;
	bt_conn_tx_cb_t cb = data->cb;
	void *cb_user_data = data->user_data;
	uint16_t cid = data->cid;

	LOG_DBG("conn %p CID 0x%04x err %d", conn, cid, err);

	free_tx_meta_data(data);

	if (err) {
		if (cb) {
			cb(conn, cb_user_data, err);
		}

		return;
	}

	chan = bt_l2cap_le_lookup_tx_cid(conn, cid);
	if (!chan) {
		/* Received SDU sent callback for disconnected channel */
		return;
	}

	if (chan->ops->sent) {
		chan->ops->sent(chan);
	}

	if (cb) {
		cb(conn, cb_user_data, 0);
	}

	/* Resume the current channel */
	l2cap_chan_tx_resume(BT_L2CAP_LE_CHAN(chan));
}

static void l2cap_chan_seg_sent(struct bt_conn *conn, void *user_data, int err)
{
	struct l2cap_tx_meta_data *data = user_data;
	struct bt_l2cap_chan *chan;

	LOG_DBG("conn %p CID 0x%04x err %d", conn, data->cid, err);

	if (err) {
		return;
	}

	chan = bt_l2cap_le_lookup_tx_cid(conn, data->cid);
	if (!chan) {
		/* Received segment sent callback for disconnected channel */
		return;
	}

	l2cap_chan_tx_resume(BT_L2CAP_LE_CHAN(chan));
}

static bool test_and_dec(atomic_t *target)
{
	atomic_t old_value, new_value;

	do {
		old_value = atomic_get(target);
		if (!old_value) {
			return false;
		}

		new_value = old_value - 1;
	} while (atomic_cas(target, old_value, new_value) == 0);

	return true;
}

/* This returns -EAGAIN whenever a segment cannot be send immediately which can
 * happen under the following circuntances:
 *
 * 1. There are no credits
 * 2. There are no buffers
 * 3. There are no TX contexts
 *
 * In all cases the original buffer is unaffected so it can be pushed back to
 * be sent later.
 */
static int l2cap_chan_le_send(struct bt_l2cap_le_chan *ch,
			      struct net_buf *buf, uint16_t sdu_hdr_len)
{
	struct net_buf *seg;
	struct net_buf_simple_state state;
	int len, err;

	if (!test_and_dec(&ch->tx.credits)) {
		LOG_DBG("No credits to transmit packet");
		return -EAGAIN;
	}

	/* Save state so it can be restored if we failed to send */
	net_buf_simple_save(&buf->b, &state);

	seg = l2cap_chan_create_seg(ch, buf, sdu_hdr_len);
	if (!seg) {
		atomic_inc(&ch->tx.credits);
		return -EAGAIN;
	}

	LOG_DBG("ch %p cid 0x%04x len %u credits %lu", ch, ch->tx.cid, seg->len,
		atomic_get(&ch->tx.credits));

	len = seg->len - sdu_hdr_len;

	/* Set a callback if there is no data left in the buffer */
	if (buf == seg || !buf->len) {
		err = bt_l2cap_send_cb(ch->chan.conn, ch->tx.cid, seg,
				       l2cap_chan_sdu_sent,
				       l2cap_tx_meta_data(buf));
	} else {
		err = bt_l2cap_send_cb(ch->chan.conn, ch->tx.cid, seg,
				       l2cap_chan_seg_sent,
				       l2cap_tx_meta_data(buf));
	}

	if (err) {
		LOG_DBG("Unable to send seg %d", err);
		atomic_inc(&ch->tx.credits);

		/* The host takes ownership of the reference in seg when
		 * bt_l2cap_send_cb is successful. The call returned an error,
		 * so we must get rid of the reference that was taken in
		 * l2cap_chan_create_seg.
		 */
		net_buf_unref(seg);

		if (err == -ENOBUFS) {
			/* Restore state since segment could not be sent */
			net_buf_simple_restore(&buf->b, &state);
			return -EAGAIN;
		}

		return err;
	}

	/* Check if there is no credits left clear output status and notify its
	 * change.
	 */
	if (!atomic_get(&ch->tx.credits)) {
		atomic_clear_bit(ch->chan.status, BT_L2CAP_STATUS_OUT);
		if (ch->chan.ops->status) {
			ch->chan.ops->status(&ch->chan, ch->chan.status);
		}
	}

	return len;
}

static int l2cap_chan_le_send_sdu(struct bt_l2cap_le_chan *ch,
				  struct net_buf **buf, uint16_t sent)
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
		ret = l2cap_chan_le_send(ch, frag, BT_L2CAP_SDU_HDR_SIZE);
		if (ret < 0) {
			if (ret == -EAGAIN) {
				/* Store sent data into user_data */
				l2cap_tx_meta_data(frag)->sent = sent;
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
				l2cap_tx_meta_data(frag)->sent = sent;
			}
			*buf = frag;
			return ret;
		}
	}

	LOG_DBG("ch %p cid 0x%04x sent %u total_len %u", ch, ch->tx.cid, sent, total_len);

	net_buf_unref(frag);

	return ret;
}

static void le_credits(struct bt_l2cap *l2cap, uint8_t ident,
		       struct net_buf *buf)
{
	struct bt_conn *conn = l2cap->chan.chan.conn;
	struct bt_l2cap_chan *chan;
	struct bt_l2cap_le_credits *ev = (void *)buf->data;
	struct bt_l2cap_le_chan *le_chan;
	uint16_t credits, cid;

	if (buf->len < sizeof(*ev)) {
		LOG_ERR("Too small LE Credits packet size");
		return;
	}

	cid = sys_le16_to_cpu(ev->cid);
	credits = sys_le16_to_cpu(ev->credits);

	LOG_DBG("cid 0x%04x credits %u", cid, credits);

	chan = bt_l2cap_le_lookup_tx_cid(conn, cid);
	if (!chan) {
		LOG_ERR("Unable to find channel of LE Credits packet");
		return;
	}

	le_chan = BT_L2CAP_LE_CHAN(chan);

	if (atomic_get(&le_chan->tx.credits) + credits > UINT16_MAX) {
		LOG_ERR("Credits overflow");
		bt_l2cap_chan_disconnect(chan);
		return;
	}

	l2cap_chan_tx_give_credits(le_chan, credits);

	LOG_DBG("chan %p total credits %lu", le_chan, atomic_get(&le_chan->tx.credits));

	l2cap_chan_tx_resume(le_chan);
}

static void reject_cmd(struct bt_l2cap *l2cap, uint8_t ident,
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

static int l2cap_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_l2cap *l2cap = CONTAINER_OF(chan, struct bt_l2cap, chan);
	struct bt_l2cap_sig_hdr *hdr;
	uint16_t len;

	if (buf->len < sizeof(*hdr)) {
		LOG_ERR("Too small L2CAP signaling PDU");
		return 0;
	}

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	len = sys_le16_to_cpu(hdr->len);

	LOG_DBG("Signaling code 0x%02x ident %u len %u", hdr->code, hdr->ident, len);

	if (buf->len != len) {
		LOG_ERR("L2CAP length mismatch (%u != %u)", buf->len, len);
		return 0;
	}

	if (!hdr->ident) {
		LOG_ERR("Invalid ident value in L2CAP PDU");
		return 0;
	}

	switch (hdr->code) {
	case BT_L2CAP_CONN_PARAM_RSP:
		le_conn_param_rsp(l2cap, buf);
		break;
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
#if defined(CONFIG_BT_L2CAP_ECRED)
	case BT_L2CAP_ECRED_CONN_REQ:
		le_ecred_conn_req(l2cap, hdr->ident, buf);
		break;
	case BT_L2CAP_ECRED_CONN_RSP:
		le_ecred_conn_rsp(l2cap, hdr->ident, buf);
		break;
	case BT_L2CAP_ECRED_RECONF_REQ:
		le_ecred_reconf_req(l2cap, hdr->ident, buf);
		break;
	case BT_L2CAP_ECRED_RECONF_RSP:
		le_ecred_reconf_rsp(l2cap, hdr->ident, buf);
		break;
#endif /* defined(CONFIG_BT_L2CAP_ECRED) */
#else
	case BT_L2CAP_CMD_REJECT:
		/* Ignored */
		break;
#endif /* CONFIG_BT_L2CAP_DYNAMIC_CHANNEL */
	case BT_L2CAP_CONN_PARAM_REQ:
		if (IS_ENABLED(CONFIG_BT_CENTRAL)) {
			le_conn_param_update_req(l2cap, hdr->ident, buf);
			break;
		}
		__fallthrough;
	default:
		LOG_WRN("Rejecting unknown L2CAP PDU code 0x%02x", hdr->code);
		l2cap_send_reject(chan->conn, hdr->ident,
				  BT_L2CAP_REJ_NOT_UNDERSTOOD, NULL, 0);
		break;
	}

	return 0;
}

#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
static void l2cap_chan_shutdown(struct bt_l2cap_chan *chan)
{
	struct bt_l2cap_le_chan *le_chan = BT_L2CAP_LE_CHAN(chan);
	struct net_buf *buf;

	LOG_DBG("chan %p", chan);

	atomic_set_bit(chan->status, BT_L2CAP_STATUS_SHUTDOWN);

	/* Destroy segmented SDU if it exists */
	if (le_chan->_sdu) {
		net_buf_unref(le_chan->_sdu);
		le_chan->_sdu = NULL;
		le_chan->_sdu_len = 0U;
	}

	/* Cleanup outstanding request */
	if (le_chan->tx_buf) {
		net_buf_unref(le_chan->tx_buf);
		le_chan->tx_buf = NULL;
	}

	/* Remove buffers on the TX queue */
	while ((buf = net_buf_get(&le_chan->tx_queue, K_NO_WAIT))) {
		net_buf_unref(buf);
	}

	/* Remove buffers on the RX queue */
	while ((buf = net_buf_get(&le_chan->rx_queue, K_NO_WAIT))) {
		net_buf_unref(buf);
	}

	/* Update status */
	if (chan->ops->status) {
		chan->ops->status(chan, chan->status);
	}
}

/** @brief Get @c chan->state.
 *
 * This field does not exist when @kconfig{CONFIG_BT_L2CAP_DYNAMIC_CHANNEL} is
 * disabled. In that case, this function returns @ref BT_L2CAP_CONNECTED since
 * the struct can only represent static channels in that case and static
 * channels are always connected.
 */
static inline bt_l2cap_chan_state_t bt_l2cap_chan_get_state(struct bt_l2cap_chan *chan)
{
#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
	return BT_L2CAP_LE_CHAN(chan)->state;
#else
	return BT_L2CAP_CONNECTED;
#endif
}

static void l2cap_chan_send_credits(struct bt_l2cap_le_chan *chan,
				    uint16_t credits)
{
	struct bt_l2cap_le_credits *ev;
	struct net_buf *buf;

	__ASSERT_NO_MSG(bt_l2cap_chan_get_state(&chan->chan) == BT_L2CAP_CONNECTED);

	buf = l2cap_create_le_sig_pdu(NULL, BT_L2CAP_LE_CREDITS, get_ident(),
				      sizeof(*ev));
	if (!buf) {
		LOG_ERR("Unable to send credits update");
		/* Disconnect would probably not work either so the only
		 * option left is to shutdown the channel.
		 */
		l2cap_chan_shutdown(&chan->chan);
		return;
	}

	__ASSERT_NO_MSG(atomic_get(&chan->rx.credits) == 0);
	atomic_set(&chan->rx.credits, credits);

	ev = net_buf_add(buf, sizeof(*ev));
	ev->cid = sys_cpu_to_le16(chan->rx.cid);
	ev->credits = sys_cpu_to_le16(credits);

	l2cap_send(chan->chan.conn, BT_L2CAP_CID_LE_SIG, buf);

	LOG_DBG("chan %p credits %lu", chan, atomic_get(&chan->rx.credits));
}

int bt_l2cap_chan_recv_complete(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_l2cap_le_chan *le_chan = BT_L2CAP_LE_CHAN(chan);
	struct bt_conn *conn = chan->conn;

	__ASSERT_NO_MSG(chan);
	__ASSERT_NO_MSG(buf);

	net_buf_unref(buf);

	if (!conn) {
		return -ENOTCONN;
	}

	if (conn->type != BT_CONN_TYPE_LE) {
		return -ENOTSUP;
	}

	LOG_DBG("chan %p buf %p", chan, buf);

	if (bt_l2cap_chan_get_state(&le_chan->chan) == BT_L2CAP_CONNECTED) {
		l2cap_chan_send_credits(le_chan, 1);
	}

	return 0;
}

static struct net_buf *l2cap_alloc_frag(k_timeout_t timeout, void *user_data)
{
	struct bt_l2cap_le_chan *chan = user_data;
	struct net_buf *frag = NULL;

	frag = chan->chan.ops->alloc_buf(&chan->chan);
	if (!frag) {
		return NULL;
	}

	LOG_DBG("frag %p tailroom %zu", frag, net_buf_tailroom(frag));

	return frag;
}

static void l2cap_chan_le_recv_sdu(struct bt_l2cap_le_chan *chan,
				   struct net_buf *buf, uint16_t seg)
{
	int err;

	LOG_DBG("chan %p len %zu", chan, net_buf_frags_len(buf));

	__ASSERT_NO_MSG(bt_l2cap_chan_get_state(&chan->chan) == BT_L2CAP_CONNECTED);
	__ASSERT_NO_MSG(atomic_get(&chan->rx.credits) == 0);

	/* Receiving complete SDU, notify channel and reset SDU buf */
	err = chan->chan.ops->recv(&chan->chan, buf);
	if (err < 0) {
		if (err != -EINPROGRESS) {
			LOG_ERR("err %d", err);
			bt_l2cap_chan_disconnect(&chan->chan);
			net_buf_unref(buf);
		}
		return;
	} else if (bt_l2cap_chan_get_state(&chan->chan) == BT_L2CAP_CONNECTED) {
		l2cap_chan_send_credits(chan, 1);
	}

	net_buf_unref(buf);
}

static void l2cap_chan_le_recv_seg(struct bt_l2cap_le_chan *chan,
				   struct net_buf *buf)
{
	uint16_t len;
	uint16_t seg = 0U;

	len = net_buf_frags_len(chan->_sdu);
	if (len) {
		memcpy(&seg, net_buf_user_data(chan->_sdu), sizeof(seg));
	}

	if (len + buf->len > chan->_sdu_len) {
		LOG_ERR("SDU length mismatch");
		bt_l2cap_chan_disconnect(&chan->chan);
		return;
	}

	seg++;
	/* Store received segments in user_data */
	memcpy(net_buf_user_data(chan->_sdu), &seg, sizeof(seg));

	LOG_DBG("chan %p seg %d len %zu", chan, seg, net_buf_frags_len(buf));

	/* Append received segment to SDU */
	len = net_buf_append_bytes(chan->_sdu, buf->len, buf->data, K_NO_WAIT,
				   l2cap_alloc_frag, chan);
	if (len != buf->len) {
		LOG_ERR("Unable to store SDU");
		bt_l2cap_chan_disconnect(&chan->chan);
		return;
	}

	if (net_buf_frags_len(chan->_sdu) < chan->_sdu_len) {
		/* Give more credits if remote has run out of them, this
		 * should only happen if the remote cannot fully utilize the
		 * MPS for some reason.
		 *
		 * We can't send more than one credit, because if the remote
		 * decides to start fully utilizing the MPS for the remainder of
		 * the SDU, then the remote will end up with more credits than
		 * the app has buffers.
		 */
		if (atomic_get(&chan->rx.credits) == 0) {
			LOG_DBG("remote is not fully utilizing MPS");
			l2cap_chan_send_credits(chan, 1);
		}

		return;
	}

	buf = chan->_sdu;
	chan->_sdu = NULL;
	chan->_sdu_len = 0U;

	l2cap_chan_le_recv_sdu(chan, buf, seg);
}

static void l2cap_chan_le_recv(struct bt_l2cap_le_chan *chan,
			       struct net_buf *buf)
{
	uint16_t sdu_len;
	int err;

	if (!test_and_dec(&chan->rx.credits)) {
		LOG_ERR("No credits to receive packet");
		bt_l2cap_chan_disconnect(&chan->chan);
		return;
	}

	if (buf->len > chan->rx.mps) {
		LOG_WRN("PDU size > MPS (%u > %u)", buf->len, chan->rx.mps);
		bt_l2cap_chan_disconnect(&chan->chan);
		return;
	}

	/* Check if segments already exist */
	if (chan->_sdu) {
		l2cap_chan_le_recv_seg(chan, buf);
		return;
	}

	if (buf->len < 2) {
		LOG_WRN("Too short data packet");
		bt_l2cap_chan_disconnect(&chan->chan);
		return;
	}

	sdu_len = net_buf_pull_le16(buf);

	LOG_DBG("chan %p len %u sdu_len %u", chan, buf->len, sdu_len);

	if (sdu_len > chan->rx.mtu) {
		LOG_ERR("Invalid SDU length");
		bt_l2cap_chan_disconnect(&chan->chan);
		return;
	}

	/* Always allocate buffer from the channel if supported. */
	if (chan->chan.ops->alloc_buf) {
		chan->_sdu = chan->chan.ops->alloc_buf(&chan->chan);
		if (!chan->_sdu) {
			LOG_ERR("Unable to allocate buffer for SDU");
			bt_l2cap_chan_disconnect(&chan->chan);
			return;
		}
		chan->_sdu_len = sdu_len;

		/* Send sdu_len/mps worth of credits */
		uint16_t credits = DIV_ROUND_UP(
			MIN(sdu_len - buf->len, net_buf_tailroom(chan->_sdu)),
			chan->rx.mps);

		if (credits) {
			LOG_DBG("sending %d extra credits (sdu_len %d buf_len %d mps %d)",
				credits,
				sdu_len,
				buf->len,
				chan->rx.mps);
			l2cap_chan_send_credits(chan, credits);
		}

		l2cap_chan_le_recv_seg(chan, buf);
		return;
	}

	err = chan->chan.ops->recv(&chan->chan, buf);
	if (err < 0) {
		if (err != -EINPROGRESS) {
			LOG_ERR("err %d", err);
			bt_l2cap_chan_disconnect(&chan->chan);
		}
		return;
	}

	l2cap_chan_send_credits(chan, 1);
}

static void l2cap_chan_recv_queue(struct bt_l2cap_le_chan *chan,
				  struct net_buf *buf)
{
	if (chan->state == BT_L2CAP_DISCONNECTING) {
		LOG_WRN("Ignoring data received while disconnecting");
		net_buf_unref(buf);
		return;
	}

	if (atomic_test_bit(chan->chan.status, BT_L2CAP_STATUS_SHUTDOWN)) {
		LOG_WRN("Ignoring data received while channel has shutdown");
		net_buf_unref(buf);
		return;
	}

	if (!L2CAP_LE_PSM_IS_DYN(chan->psm)) {
		l2cap_chan_le_recv(chan, buf);
		net_buf_unref(buf);
		return;
	}

	net_buf_put(&chan->rx_queue, buf);
	k_work_submit(&chan->rx_work);
}
#endif /* CONFIG_BT_L2CAP_DYNAMIC_CHANNEL */

static void l2cap_chan_recv(struct bt_l2cap_chan *chan, struct net_buf *buf,
			    bool complete)
{
#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
	struct bt_l2cap_le_chan *le_chan = BT_L2CAP_LE_CHAN(chan);

	if (L2CAP_LE_CID_IS_DYN(le_chan->rx.cid)) {
		if (complete) {
			l2cap_chan_recv_queue(le_chan, buf);
		} else {
			/* if packet was not complete this means peer device
			 * overflowed our RX and channel shall be disconnected
			 */
			bt_l2cap_chan_disconnect(chan);
			net_buf_unref(buf);
		}

		return;
	}
#endif /* CONFIG_BT_L2CAP_DYNAMIC_CHANNEL */

	LOG_DBG("chan %p len %u", chan, buf->len);

	chan->ops->recv(chan, buf);
	net_buf_unref(buf);
}

void bt_l2cap_recv(struct bt_conn *conn, struct net_buf *buf, bool complete)
{
	struct bt_l2cap_hdr *hdr;
	struct bt_l2cap_chan *chan;
	uint16_t cid;

	if (IS_ENABLED(CONFIG_BT_BREDR) &&
	    conn->type == BT_CONN_TYPE_BR) {
		bt_l2cap_br_recv(conn, buf);
		return;
	}

	if (buf->len < sizeof(*hdr)) {
		LOG_ERR("Too small L2CAP PDU received");
		net_buf_unref(buf);
		return;
	}

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	cid = sys_le16_to_cpu(hdr->cid);

	LOG_DBG("Packet for CID %u len %u", cid, buf->len);

	chan = bt_l2cap_le_lookup_rx_cid(conn, cid);
	if (!chan) {
		LOG_WRN("Ignoring data for unknown channel ID 0x%04x", cid);
		net_buf_unref(buf);
		return;
	}

	l2cap_chan_recv(chan, buf, complete);
}

int bt_l2cap_update_conn_param(struct bt_conn *conn,
			       const struct bt_le_conn_param *param)
{
	struct bt_l2cap_conn_param_req *req;
	struct net_buf *buf;
	int err;

	buf = l2cap_create_le_sig_pdu(NULL, BT_L2CAP_CONN_PARAM_REQ,
				      get_ident(), sizeof(*req));
	if (!buf) {
		return -ENOMEM;
	}

	req = net_buf_add(buf, sizeof(*req));
	req->min_interval = sys_cpu_to_le16(param->interval_min);
	req->max_interval = sys_cpu_to_le16(param->interval_max);
	req->latency = sys_cpu_to_le16(param->latency);
	req->timeout = sys_cpu_to_le16(param->timeout);

	err = bt_l2cap_send(conn, BT_L2CAP_CID_LE_SIG, buf);
	if (err) {
		net_buf_unref(buf);
		return err;
	}

	return 0;
}

static void l2cap_connected(struct bt_l2cap_chan *chan)
{
	LOG_DBG("ch %p cid 0x%04x", BT_L2CAP_LE_CHAN(chan), BT_L2CAP_LE_CHAN(chan)->rx.cid);
}

static void l2cap_disconnected(struct bt_l2cap_chan *chan)
{
	struct bt_l2cap_le_chan *le_chan = BT_L2CAP_LE_CHAN(chan);

	LOG_DBG("ch %p cid 0x%04x", le_chan, le_chan->rx.cid);

#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
	/* Cancel RTX work on signal channel.
	 * Disconnected callback is always called from system workqueue
	 * so this should always succeed.
	 */
	(void)k_work_cancel_delayable(&le_chan->rtx_work);
#endif /* CONFIG_BT_L2CAP_DYNAMIC_CHANNEL */
}

static int l2cap_accept(struct bt_conn *conn, struct bt_l2cap_chan **chan)
{
	int i;
	static const struct bt_l2cap_chan_ops ops = {
		.connected = l2cap_connected,
		.disconnected = l2cap_disconnected,
		.recv = l2cap_recv,
	};

	LOG_DBG("conn %p handle %u", conn, conn->handle);

	for (i = 0; i < ARRAY_SIZE(bt_l2cap_pool); i++) {
		struct bt_l2cap *l2cap = &bt_l2cap_pool[i];

		if (l2cap->chan.chan.conn) {
			continue;
		}

		l2cap->chan.chan.ops = &ops;
		*chan = &l2cap->chan.chan;

		return 0;
	}

	LOG_ERR("No available L2CAP context for conn %p", conn);

	return -ENOMEM;
}

BT_L2CAP_CHANNEL_DEFINE(le_fixed_chan, BT_L2CAP_CID_LE_SIG, l2cap_accept, NULL);

void bt_l2cap_init(void)
{
	if (IS_ENABLED(CONFIG_BT_BREDR)) {
		bt_l2cap_br_init();
	}

#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
	k_fifo_init(&free_l2cap_tx_meta_data);
	for (size_t i = 0; i < ARRAY_SIZE(l2cap_tx_meta_data_storage); i++) {
		(void)memset(&l2cap_tx_meta_data_storage[i], 0,
					sizeof(l2cap_tx_meta_data_storage[i]));
		k_fifo_put(&free_l2cap_tx_meta_data, &l2cap_tx_meta_data_storage[i]);
	}
#endif /* CONFIG_BT_L2CAP_DYNAMIC_CHANNEL */
}

#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
static int l2cap_le_connect(struct bt_conn *conn, struct bt_l2cap_le_chan *ch,
			    uint16_t psm)
{
	int err;

	if (psm < L2CAP_LE_PSM_FIXED_START || psm > L2CAP_LE_PSM_DYN_END) {
		return -EINVAL;
	}

	l2cap_chan_tx_init(ch);
	l2cap_chan_rx_init(ch);

	if (!l2cap_chan_add(conn, &ch->chan, l2cap_chan_destroy)) {
		return -ENOMEM;
	}

	ch->psm = psm;

	if (conn->sec_level < ch->required_sec_level) {
		err = bt_conn_set_security(conn, ch->required_sec_level);
		if (err) {
			goto fail;
		}

		atomic_set_bit(ch->chan.status,
			       BT_L2CAP_STATUS_ENCRYPT_PENDING);

		return 0;
	}

	err = l2cap_le_conn_req(ch);
	if (err) {
		goto fail;
	}

	return 0;

fail:
	bt_l2cap_chan_remove(conn, &ch->chan);
	bt_l2cap_chan_del(&ch->chan);
	return err;
}

#if defined(CONFIG_BT_L2CAP_ECRED)
static int l2cap_ecred_init(struct bt_conn *conn,
			       struct bt_l2cap_le_chan *ch, uint16_t psm)
{

	if (psm < L2CAP_LE_PSM_FIXED_START || psm > L2CAP_LE_PSM_DYN_END) {
		return -EINVAL;
	}

	l2cap_chan_tx_init(ch);
	l2cap_chan_rx_init(ch);

	if (!l2cap_chan_add(conn, &ch->chan, l2cap_chan_destroy)) {
		return -ENOMEM;
	}

	ch->psm = psm;

	LOG_DBG("ch %p psm 0x%02x mtu %u mps %u credits 1", ch, ch->psm, ch->rx.mtu, ch->rx.mps);

	return 0;
}

int bt_l2cap_ecred_chan_connect(struct bt_conn *conn,
				struct bt_l2cap_chan **chan, uint16_t psm)
{
	int i, err;

	LOG_DBG("conn %p chan %p psm 0x%04x", conn, chan, psm);

	if (!conn || !chan) {
		return -EINVAL;
	}

	/* Init non-null channels */
	for (i = 0; i < L2CAP_ECRED_CHAN_MAX_PER_REQ; i++) {
		if (!chan[i]) {
			break;
		}

		err = l2cap_ecred_init(conn, BT_L2CAP_LE_CHAN(chan[i]), psm);
		if (err < 0) {
			i--;
			goto fail;
		}
	}

	return l2cap_ecred_conn_req(chan, i);
fail:
	/* Remove channels added */
	for (; i >= 0; i--) {
		if (!chan[i]) {
			continue;
		}

		bt_l2cap_chan_remove(conn, chan[i]);
	}

	return err;
}

static struct bt_l2cap_le_chan *l2cap_find_pending_reconf(struct bt_conn *conn)
{
	struct bt_l2cap_chan *chan;

	SYS_SLIST_FOR_EACH_CONTAINER(&conn->channels, chan, node) {
		if (BT_L2CAP_LE_CHAN(chan)->pending_rx_mtu) {
			return BT_L2CAP_LE_CHAN(chan);
		}
	}

	return NULL;
}

int bt_l2cap_ecred_chan_reconfigure(struct bt_l2cap_chan **chans, uint16_t mtu)
{
	struct bt_l2cap_ecred_reconf_req *req;
	struct bt_conn *conn = NULL;
	struct bt_l2cap_le_chan *ch;
	struct net_buf *buf;
	uint8_t ident;
	int i;

	LOG_DBG("chans %p mtu 0x%04x", chans, mtu);

	if (!chans) {
		return -EINVAL;
	}

	for (i = 0; i < L2CAP_ECRED_CHAN_MAX_PER_REQ; i++) {
		if (!chans[i]) {
			break;
		}

		/* validate that all channels are from same connection */
		if (conn) {
			if (conn != chans[i]->conn) {
				return -EINVAL;
			}
		} else {
			conn = chans[i]->conn;
		}

		/* validate MTU is not decreased */
		if (mtu < BT_L2CAP_LE_CHAN(chans[i])->rx.mtu) {
			return -EINVAL;
		}
	}

	if (i == 0) {
		return -EINVAL;
	}

	if (!conn) {
		return -ENOTCONN;
	}

	if (conn->type != BT_CONN_TYPE_LE) {
		return -EINVAL;
	}

	/* allow only 1 request at time */
	if (l2cap_find_pending_reconf(conn)) {
		return -EBUSY;
	}

	ident = get_ident();

	buf = l2cap_create_le_sig_pdu(NULL, BT_L2CAP_ECRED_RECONF_REQ,
				      ident,
				      sizeof(*req) + (i * sizeof(uint16_t)));
	if (!buf) {
		return -ENOMEM;
	}

	req = net_buf_add(buf, sizeof(*req));
	req->mtu = sys_cpu_to_le16(mtu);

	/* MPS shall not be bigger than MTU + BT_L2CAP_SDU_HDR_SIZE
	 * as the remaining bytes cannot be used.
	 */
	req->mps = sys_cpu_to_le16(MIN(mtu + BT_L2CAP_SDU_HDR_SIZE,
				       BT_L2CAP_RX_MTU));

	for (int j = 0; j < i; j++) {
		ch = BT_L2CAP_LE_CHAN(chans[j]);

		ch->ident = ident;
		ch->pending_rx_mtu = mtu;

		net_buf_add_le16(buf, ch->rx.cid);
	};

	/* We set the RTX timer on one of the supplied channels, but when the
	 * request resolves or times out we will act on all the channels in the
	 * supplied array, using the ident field to find them.
	 */
	l2cap_chan_send_req(chans[0], buf, L2CAP_CONN_TIMEOUT);

	return 0;
}

#endif /* defined(CONFIG_BT_L2CAP_ECRED) */

int bt_l2cap_chan_connect(struct bt_conn *conn, struct bt_l2cap_chan *chan,
			  uint16_t psm)
{
	struct bt_l2cap_le_chan *le_chan = BT_L2CAP_LE_CHAN(chan);

	LOG_DBG("conn %p chan %p psm 0x%04x", conn, chan, psm);

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

	if (le_chan->required_sec_level > BT_SECURITY_L4) {
		return -EINVAL;
	} else if (le_chan->required_sec_level == BT_SECURITY_L0) {
		le_chan->required_sec_level = BT_SECURITY_L1;
	}

	return l2cap_le_connect(conn, le_chan, psm);
}

int bt_l2cap_chan_disconnect(struct bt_l2cap_chan *chan)
{
	struct bt_conn *conn = chan->conn;
	struct net_buf *buf;
	struct bt_l2cap_disconn_req *req;
	struct bt_l2cap_le_chan *le_chan;

	if (!conn) {
		return -ENOTCONN;
	}

	if (IS_ENABLED(CONFIG_BT_BREDR) &&
	    conn->type == BT_CONN_TYPE_BR) {
		return bt_l2cap_br_chan_disconnect(chan);
	}

	le_chan = BT_L2CAP_LE_CHAN(chan);

	LOG_DBG("chan %p scid 0x%04x dcid 0x%04x", chan, le_chan->rx.cid, le_chan->tx.cid);

	le_chan->ident = get_ident();

	buf = l2cap_create_le_sig_pdu(NULL, BT_L2CAP_DISCONN_REQ,
				      le_chan->ident, sizeof(*req));
	if (!buf) {
		return -ENOMEM;
	}

	req = net_buf_add(buf, sizeof(*req));
	req->dcid = sys_cpu_to_le16(le_chan->tx.cid);
	req->scid = sys_cpu_to_le16(le_chan->rx.cid);

	l2cap_chan_send_req(chan, buf, L2CAP_DISC_TIMEOUT);
	bt_l2cap_chan_set_state(chan, BT_L2CAP_DISCONNECTING);

	return 0;
}

int bt_l2cap_chan_send_cb(struct bt_l2cap_chan *chan, struct net_buf *buf, bt_conn_tx_cb_t cb,
			  void *user_data)
{
	struct bt_l2cap_le_chan *le_chan = BT_L2CAP_LE_CHAN(chan);
	struct l2cap_tx_meta_data *data;
	void *old_user_data = l2cap_tx_meta_data(buf);
	int err;

	if (!buf) {
		return -EINVAL;
	}

	LOG_DBG("chan %p buf %p len %zu", chan, buf, net_buf_frags_len(buf));

	if (!chan->conn || chan->conn->state != BT_CONN_CONNECTED) {
		return -ENOTCONN;
	}

	if (atomic_test_bit(chan->status, BT_L2CAP_STATUS_SHUTDOWN)) {
		return -ESHUTDOWN;
	}

	if (IS_ENABLED(CONFIG_BT_BREDR) &&
	    chan->conn->type == BT_CONN_TYPE_BR) {
		return bt_l2cap_br_chan_send_cb(chan, buf, cb, user_data);
	}

	data = alloc_tx_meta_data();
	if (!data) {
		LOG_WRN("Unable to allocate TX context");
		return -ENOBUFS;
	}

	data->sent = 0;
	data->cid = le_chan->tx.cid;
	data->cb = cb;
	data->user_data = user_data;
	l2cap_tx_meta_data(buf) = data;

	/* Queue if there are pending segments left from previous packet or
	 * there are no credits available.
	 */
	if (le_chan->tx_buf || !k_fifo_is_empty(&le_chan->tx_queue) ||
	    !atomic_get(&le_chan->tx.credits)) {
		l2cap_tx_meta_data(buf)->sent = 0;
		net_buf_put(&le_chan->tx_queue, buf);
		k_work_reschedule(&le_chan->tx_work, K_NO_WAIT);
		return 0;
	}

	err = l2cap_chan_le_send_sdu(le_chan, &buf, 0);
	if (err < 0) {
		if (err == -EAGAIN && l2cap_tx_meta_data(buf)->sent) {
			/* Queue buffer if at least one segment could be sent */
			net_buf_put(&le_chan->tx_queue, buf);
			return l2cap_tx_meta_data(buf)->sent;
		}

		LOG_ERR("failed to send message %d", err);

		l2cap_tx_meta_data(buf) = old_user_data;
		free_tx_meta_data(data);
	}

	return err;
}

int bt_l2cap_chan_send(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	return bt_l2cap_chan_send_cb(chan, buf, NULL, NULL);
}
#endif /* CONFIG_BT_L2CAP_DYNAMIC_CHANNEL */
