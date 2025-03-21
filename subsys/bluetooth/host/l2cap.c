/* l2cap.c - L2CAP handling */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/sys/util.h>
#include <zephyr/net_buf.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>

#define LOG_DBG_ENABLED IS_ENABLED(CONFIG_BT_L2CAP_LOG_LEVEL_DBG)

#include "buf_view.h"
#include "hci_core.h"
#include "conn_internal.h"
#include "l2cap_internal.h"
#include "keys.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_l2cap, CONFIG_BT_L2CAP_LOG_LEVEL);

#define LE_CHAN_RTX(_w) CONTAINER_OF(k_work_delayable_from_work(_w), \
				     struct bt_l2cap_le_chan, rtx_work)
#define CHAN_RX(_w) CONTAINER_OF(_w, struct bt_l2cap_le_chan, rx_work)

#define L2CAP_LE_MIN_MTU		23

#define L2CAP_LE_MAX_CREDITS		(BT_BUF_ACL_RX_COUNT - 1)

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
/** @brief Local L2CAP RTX (Response Timeout eXpired)
 *
 *  Specification-allowed range for the value of RTX is 1 to 60 seconds.
 */
#define L2CAP_RTX_TIMEOUT	K_SECONDS(2)

#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
/* Dedicated pool for disconnect buffers so they are guaranteed to be send
 * even in case of data congestion due to flooding.
 */
NET_BUF_POOL_FIXED_DEFINE(disc_pool, 1,
			  BT_L2CAP_BUF_SIZE(
				sizeof(struct bt_l2cap_sig_hdr) +
				sizeof(struct bt_l2cap_disconn_req)),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

#define l2cap_lookup_ident(conn, ident) __l2cap_lookup_ident(conn, ident, false)
#define l2cap_remove_ident(conn, ident) __l2cap_lookup_ident(conn, ident, true)

static sys_slist_t servers = SYS_SLIST_STATIC_INIT(&servers);

static void l2cap_tx_buf_destroy(struct bt_conn *conn, struct net_buf *buf, int err)
{
	net_buf_unref(buf);
}
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

static void cancel_data_ready(struct bt_l2cap_le_chan *lechan);
static bool chan_has_data(struct bt_l2cap_le_chan *lechan);
void bt_l2cap_chan_del(struct bt_l2cap_chan *chan)
{
	const struct bt_l2cap_chan_ops *ops = chan->ops;
	struct bt_l2cap_le_chan *le_chan = BT_L2CAP_LE_CHAN(chan);

	LOG_DBG("conn %p chan %p", chan->conn, chan);

	if (!chan->conn) {
		goto destroy;
	}

	cancel_data_ready(le_chan);

	/* Remove buffers on the PDU TX queue. We can't do that in
	 * `l2cap_chan_destroy()` as it is not called for fixed channels.
	 */
	while (chan_has_data(le_chan)) {
		struct net_buf *buf = k_fifo_get(&le_chan->tx_queue, K_NO_WAIT);

		net_buf_unref(buf);
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

	while ((buf = k_fifo_get(&ch->rx_queue, K_NO_WAIT))) {
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

static void init_le_chan_private(struct bt_l2cap_le_chan *le_chan)
{
	/* Initialize private members of the struct. We can't "just memset" as
	 * some members are used as application parameters.
	 */
#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
	le_chan->_sdu = NULL;
	le_chan->_sdu_len = 0;
#if defined(CONFIG_BT_L2CAP_SEG_RECV)
	le_chan->_sdu_len_done = 0;
#endif /* CONFIG_BT_L2CAP_SEG_RECV */
#endif /* CONFIG_BT_L2CAP_DYNAMIC_CHANNEL */
	memset(&le_chan->_pdu_ready, 0, sizeof(le_chan->_pdu_ready));
	le_chan->_pdu_ready_lock = 0;
	le_chan->_pdu_remaining = 0;
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
	init_le_chan_private(le_chan);

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

	if (IS_ENABLED(CONFIG_BT_CLASSIC) &&
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

		k_fifo_init(&le_chan->tx_queue);

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

	if (IS_ENABLED(CONFIG_BT_CLASSIC) &&
	    conn->type == BT_CONN_TYPE_BR) {
		bt_l2cap_br_disconnected(conn);
		return;
	}

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&conn->channels, chan, next, node) {
		bt_l2cap_chan_del(chan);
	}
}

static struct net_buf *l2cap_create_le_sig_pdu(uint8_t code, uint8_t ident,
					       uint16_t len)
{
	struct bt_l2cap_sig_hdr *hdr;
	struct net_buf_pool *pool = NULL;
	struct net_buf *buf;

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

/* Send the buffer over the signalling channel. Release it in case of failure.
 * Any other cleanup in failure to send should be handled by the disconnected
 * handler.
 */
static int l2cap_send_sig(struct bt_conn *conn, struct net_buf *buf)
{
	struct bt_l2cap_chan *ch = bt_l2cap_le_lookup_tx_cid(conn, BT_L2CAP_CID_LE_SIG);
	struct bt_l2cap_le_chan *chan = BT_L2CAP_LE_CHAN(ch);

	int err = bt_l2cap_send_pdu(chan, buf, NULL, NULL);

	if (err) {
		net_buf_unref(buf);
	}

	return err;
}

#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
static void l2cap_chan_send_req(struct bt_l2cap_chan *chan,
				struct net_buf *buf, k_timeout_t timeout)
{
	if (l2cap_send_sig(chan->conn, buf)) {
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

	buf = l2cap_create_le_sig_pdu(BT_L2CAP_LE_CONN_REQ,
				      ch->ident, sizeof(*req));
	if (!buf) {
		return -ENOMEM;
	}

	req = net_buf_add(buf, sizeof(*req));
	req->psm = sys_cpu_to_le16(ch->psm);
	req->scid = sys_cpu_to_le16(ch->rx.cid);
	req->mtu = sys_cpu_to_le16(ch->rx.mtu);
	req->mps = sys_cpu_to_le16(ch->rx.mps);
	req->credits = sys_cpu_to_le16(ch->rx.credits);

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
	uint16_t req_mtu;

	if (!chan || !channels) {
		return -EINVAL;
	}

	ident = get_ident();

	buf = l2cap_create_le_sig_pdu(BT_L2CAP_ECRED_CONN_REQ, ident,
				      sizeof(*req) +
				      (channels * sizeof(uint16_t)));

	if (!buf) {
		return -ENOMEM;
	}

	req = net_buf_add(buf, sizeof(*req));

	ch = BT_L2CAP_LE_CHAN(chan[0]);

	/* Init common parameters */
	req->psm = sys_cpu_to_le16(ch->psm);
	req->mtu = sys_cpu_to_le16(ch->rx.mtu);
	req->mps = sys_cpu_to_le16(ch->rx.mps);
	req->credits = sys_cpu_to_le16(ch->rx.credits);
	req_psm = ch->psm;
	req_mtu = ch->tx.mtu;

	for (i = 0; i < channels; i++) {
		ch = BT_L2CAP_LE_CHAN(chan[i]);

		__ASSERT(ch->psm == req_psm,
			 "The PSM shall be the same for channels in the same request.");
		__ASSERT(ch->tx.mtu == req_mtu,
			 "The MTU shall be the same for channels in the same request.");

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
		struct bt_l2cap_chan *echan[BT_L2CAP_ECRED_CHAN_MAX_PER_REQ];
		struct bt_l2cap_chan *ch;
		int i = 0;

		SYS_SLIST_FOR_EACH_CONTAINER(&chan->conn->channels, ch, node) {
			if (le->ident == BT_L2CAP_LE_CHAN(ch)->ident) {
				__ASSERT(i < BT_L2CAP_ECRED_CHAN_MAX_PER_REQ,
					 "There can only be BT_L2CAP_ECRED_CHAN_MAX_PER_REQ "
					 "channels from the same request.");
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

	if (IS_ENABLED(CONFIG_BT_CLASSIC) &&
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
	if (!K_TIMEOUT_EQ(timeout, K_NO_WAIT) &&
	    k_current_get() == k_work_queue_thread_get(&k_sys_work_q)) {
		timeout = K_NO_WAIT;
	}

	return bt_conn_create_pdu_timeout(pool,
					  sizeof(struct bt_l2cap_hdr) + reserve,
					  timeout);
}

static void raise_data_ready(struct bt_l2cap_le_chan *le_chan)
{
	if (!atomic_set(&le_chan->_pdu_ready_lock, 1)) {
		sys_slist_append(&le_chan->chan.conn->l2cap_data_ready,
				 &le_chan->_pdu_ready);
		LOG_DBG("data ready raised %p", le_chan);
	} else {
		LOG_DBG("data ready already %p", le_chan);
	}

	bt_conn_data_ready(le_chan->chan.conn);
}

static void lower_data_ready(struct bt_l2cap_le_chan *le_chan)
{
	struct bt_conn *conn = le_chan->chan.conn;
	__maybe_unused sys_snode_t *s = sys_slist_get(&conn->l2cap_data_ready);

	LOG_DBG("%p", le_chan);

	__ASSERT_NO_MSG(s == &le_chan->_pdu_ready);

	__maybe_unused atomic_t old = atomic_set(&le_chan->_pdu_ready_lock, 0);

	__ASSERT_NO_MSG(old);
}

static void cancel_data_ready(struct bt_l2cap_le_chan *le_chan)
{
	struct bt_conn *conn = le_chan->chan.conn;

	LOG_DBG("%p", le_chan);

	sys_slist_find_and_remove(&conn->l2cap_data_ready,
				  &le_chan->_pdu_ready);
	atomic_set(&le_chan->_pdu_ready_lock, 0);
}

int bt_l2cap_send_pdu(struct bt_l2cap_le_chan *le_chan, struct net_buf *pdu,
		      bt_conn_tx_cb_t cb, void *user_data)
{
	if (!le_chan->chan.conn || le_chan->chan.conn->state != BT_CONN_CONNECTED) {
		return -ENOTCONN;
	}

	if (pdu->ref != 1) {
		/* The host may alter the buf contents when fragmenting. Higher
		 * layers cannot expect the buf contents to stay intact. Extra
		 * refs suggests a silent data corruption would occur if not for
		 * this error.
		 */
		LOG_ERR("Expecting 1 ref, got %d", pdu->ref);
		return -EINVAL;
	}

	if (pdu->user_data_size < sizeof(struct closure)) {
		LOG_DBG("not enough room in user_data %d < %d pool %u",
			pdu->user_data_size,
			CONFIG_BT_CONN_TX_USER_DATA_SIZE,
			pdu->pool_id);
		return -EINVAL;
	}

	make_closure(pdu->user_data, cb, user_data);
	LOG_DBG("push: pdu %p len %d cb %p userdata %p", pdu, pdu->len, cb, user_data);

	k_fifo_put(&le_chan->tx_queue, pdu);

	raise_data_ready(le_chan); /* tis just a flag */

	return 0;		/* look ma, no failures */
}

/* L2CAP channel wants to send a PDU */
static bool chan_has_data(struct bt_l2cap_le_chan *lechan)
{
	return !k_fifo_is_empty(&lechan->tx_queue);
}

#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
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
#endif

/* Just like in group projects :p */
static void chan_take_credit(struct bt_l2cap_le_chan *lechan)
{
#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
	if (!L2CAP_LE_CID_IS_DYN(lechan->tx.cid)) {
		return;
	}

	if (!test_and_dec(&lechan->tx.credits)) {
		/* Always ensure you have credits before calling this fn */
		__ASSERT_NO_MSG(0);
	}

	/* Notify channel user that it can't send anymore on this channel. */
	if (!atomic_get(&lechan->tx.credits)) {
		LOG_DBG("chan %p paused", lechan);
		atomic_clear_bit(lechan->chan.status, BT_L2CAP_STATUS_OUT);

		if (lechan->chan.ops->status) {
			lechan->chan.ops->status(&lechan->chan, lechan->chan.status);
		}
	}
#endif
}

static struct bt_l2cap_le_chan *get_ready_chan(struct bt_conn *conn)
{
	struct bt_l2cap_le_chan *lechan;

	sys_snode_t *pdu_ready = sys_slist_peek_head(&conn->l2cap_data_ready);

	if (!pdu_ready) {
		LOG_DBG("nothing to send on this conn");
		return NULL;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&conn->l2cap_data_ready, lechan, _pdu_ready) {
		if (chan_has_data(lechan)) {
			LOG_DBG("sending from chan %p (%s) data %d", lechan,
				L2CAP_LE_CID_IS_DYN(lechan->tx.cid) ? "dynamic" : "static",
				chan_has_data(lechan));
			return lechan;
		}

		LOG_DBG("chan %p has no data", lechan);
		lower_data_ready(lechan);
	}

	return NULL;
}

static void l2cap_chan_sdu_sent(struct bt_conn *conn, void *user_data, int err)
{
	struct bt_l2cap_chan *chan;
	uint16_t cid = POINTER_TO_UINT(user_data);

	LOG_DBG("conn %p CID 0x%04x err %d", conn, cid, err);

	if (err) {
		LOG_DBG("error %d when sending SDU", err);

		return;
	}

	chan = bt_l2cap_le_lookup_tx_cid(conn, cid);
	if (!chan) {
		LOG_DBG("got SDU sent cb for disconnected chan (CID %u)", cid);

		return;
	}

	if (chan->ops->sent) {
		chan->ops->sent(chan);
	}
}

static uint16_t get_pdu_len(struct bt_l2cap_le_chan *lechan,
			    struct net_buf *buf)
{
	if (!L2CAP_LE_CID_IS_DYN(lechan->tx.cid)) {
		/* No segmentation shenanigans on static channels */
		return buf->len;
	}

	return MIN(buf->len, lechan->tx.mps);
}

static bool chan_has_credits(struct bt_l2cap_le_chan *lechan)
{
#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
	if (!L2CAP_LE_CID_IS_DYN(lechan->tx.cid)) {
		return true;
	}

	LOG_DBG("chan %p credits %ld", lechan, atomic_get(&lechan->tx.credits));

	return atomic_get(&lechan->tx.credits) >= 1;
#else
	return true;
#endif
}

__weak void bt_test_l2cap_data_pull_spy(struct bt_conn *conn,
					struct bt_l2cap_le_chan *lechan,
					size_t amount,
					size_t *length)
{
}

struct net_buf *l2cap_data_pull(struct bt_conn *conn,
				size_t amount,
				size_t *length)
{
	struct bt_l2cap_le_chan *lechan = get_ready_chan(conn);

	if (IS_ENABLED(CONFIG_BT_TESTING)) {
		/* Allow tests to snoop in */
		bt_test_l2cap_data_pull_spy(conn, lechan, amount, length);
	}

	if (!lechan) {
		LOG_DBG("no channel conn %p", conn);
		bt_tx_irq_raise();
		return NULL;
	}

	/* Leave the PDU buffer in the queue until we have sent all its
	 * fragments.
	 *
	 * For SDUs we do the same, we keep it in the queue until all the
	 * segments have been sent, adding the PDU headers just-in-time.
	 */
	struct net_buf *pdu = k_fifo_peek_head(&lechan->tx_queue);

	/* We don't have anything to send for the current channel. We could
	 * however have something to send on another channel that is attached to
	 * the same ACL connection. Re-trigger the TX processor: it will call us
	 * again and this time we will select another channel to pull data from.
	 */
	if (!pdu) {
		bt_tx_irq_raise();
		return NULL;
	}

	if (bt_buf_has_view(pdu)) {
		LOG_ERR("already have view on %p", pdu);
		return NULL;
	}

	if (lechan->_pdu_remaining == 0 && !chan_has_credits(lechan)) {
		/* We don't have credits to send a new K-frame PDU. Remove the
		 * channel from the ready-list, it will be added back later when
		 * we get more credits.
		 */
		LOG_DBG("no credits for new K-frame on %p", lechan);
		lower_data_ready(lechan);
		return NULL;
	}

	/* Add PDU header */
	if (lechan->_pdu_remaining == 0) {
		struct bt_l2cap_hdr *hdr;
		uint16_t pdu_len = get_pdu_len(lechan, pdu);

		LOG_DBG("Adding L2CAP PDU header: buf %p chan %p len %u / %u",
			pdu, lechan, pdu_len, pdu->len);

		LOG_HEXDUMP_DBG(pdu->data, pdu->len, "PDU payload");

		hdr = net_buf_push(pdu, sizeof(*hdr));
		hdr->len = sys_cpu_to_le16(pdu_len);
		hdr->cid = sys_cpu_to_le16(lechan->tx.cid);

		lechan->_pdu_remaining = pdu_len + sizeof(*hdr);
		chan_take_credit(lechan);
	}

	/* Whether the data to be pulled is the last ACL fragment */
	bool last_frag = amount >= lechan->_pdu_remaining;

	/* Whether the data to be pulled is part of the last L2CAP segment. For
	 * static channels, this variable will always be true, even though
	 * static channels don't have the concept of L2CAP segments.
	 */
	bool last_seg = lechan->_pdu_remaining == pdu->len;

	if (last_frag && last_seg) {
		LOG_DBG("last frag of last seg, dequeuing %p", pdu);
		__maybe_unused struct net_buf *b = k_fifo_get(&lechan->tx_queue, K_NO_WAIT);

		__ASSERT_NO_MSG(b == pdu);
	}

	if (last_frag && L2CAP_LE_CID_IS_DYN(lechan->tx.cid)) {
		bool sdu_end = last_frag && last_seg;

		LOG_DBG("adding %s callback", sdu_end ? "`sdu_sent`" : "NULL");
		/* No user callbacks for SDUs */
		make_closure(pdu->user_data,
			     sdu_end ? l2cap_chan_sdu_sent : NULL,
			     sdu_end ? UINT_TO_POINTER(lechan->tx.cid) : NULL);
	}

	if (last_frag) {
		LOG_DBG("done sending PDU");

		/* Lowering the "request to send" and raising it again allows
		 * fair scheduling of channels on an ACL link: the channel is
		 * marked as "ready to send" by adding a reference to it on a
		 * FIFO on `conn`. Adding it again will send it to the back of
		 * the queue.
		 *
		 * TODO: add a user-controlled QoS function.
		 */
		LOG_DBG("chan %p done", lechan);
		lower_data_ready(lechan);

		/* Append channel to list if it still has data */
		if (chan_has_data(lechan)) {
			LOG_DBG("chan %p ready", lechan);
			raise_data_ready(lechan);
		}
	}

	/* This is used by `conn.c` to figure out if the PDU is done sending. */
	*length = lechan->_pdu_remaining;

	if (lechan->_pdu_remaining > amount) {
		lechan->_pdu_remaining -= amount;
	} else {
		lechan->_pdu_remaining = 0;
	}

	return pdu;
}

static void l2cap_send_reject(struct bt_conn *conn, uint8_t ident,
			      uint16_t reason, void *data, uint8_t data_len)
{
	struct bt_l2cap_cmd_reject *rej;
	struct net_buf *buf;

	buf = l2cap_create_le_sig_pdu(BT_L2CAP_CMD_REJECT, ident,
				      sizeof(*rej) + data_len);
	if (!buf) {
		return;
	}

	rej = net_buf_add(buf, sizeof(*rej));
	rej->reason = sys_cpu_to_le16(reason);

	if (data) {
		net_buf_add_mem(buf, data, data_len);
	}

	l2cap_send_sig(conn, buf);
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

	buf = l2cap_create_le_sig_pdu(BT_L2CAP_CONN_PARAM_RSP, ident,
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

	l2cap_send_sig(conn, buf);

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

#if defined(CONFIG_BT_L2CAP_SEG_RECV)
static void l2cap_chan_seg_recv_rx_init(struct bt_l2cap_le_chan *chan)
{
	if (chan->rx.mps > BT_L2CAP_RX_MTU) {
		LOG_ERR("Limiting RX MPS by stack buffer size.");
		chan->rx.mps = BT_L2CAP_RX_MTU;
	}

	chan->_sdu_len = 0;
	chan->_sdu_len_done = 0;
}
#endif /* CONFIG_BT_L2CAP_SEG_RECV */

static void l2cap_chan_rx_init(struct bt_l2cap_le_chan *chan)
{
	LOG_DBG("chan %p", chan);

	/* Redirect to experimental API. */
	IF_ENABLED(CONFIG_BT_L2CAP_SEG_RECV, ({
		if (chan->chan.ops->seg_recv) {
			l2cap_chan_seg_recv_rx_init(chan);
			return;
		}
	}))

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

/** @brief Get @c chan->state.
 *
 * This field does not exist when @kconfig{CONFIG_BT_L2CAP_DYNAMIC_CHANNEL} is
 * disabled. In that case, this function returns @ref BT_L2CAP_CONNECTED since
 * the struct can only represent static channels in that case and static
 * channels are always connected.
 */
static bt_l2cap_chan_state_t bt_l2cap_chan_get_state(struct bt_l2cap_chan *chan)
{
#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
	return BT_L2CAP_LE_CHAN(chan)->state;
#else
	return BT_L2CAP_CONNECTED;
#endif
}

static void l2cap_chan_tx_init(struct bt_l2cap_le_chan *chan)
{
	LOG_DBG("chan %p", chan);

	(void)memset(&chan->tx, 0, sizeof(chan->tx));
	atomic_set(&chan->tx.credits, 0);
	k_fifo_init(&chan->tx_queue);
}

static void l2cap_chan_tx_give_credits(struct bt_l2cap_le_chan *chan,
				       uint16_t credits)
{
	LOG_DBG("chan %p credits %u", chan, credits);

	atomic_add(&chan->tx.credits, credits);

	if (!atomic_test_and_set_bit(chan->chan.status, BT_L2CAP_STATUS_OUT)) {
		LOG_DBG("chan %p unpaused", chan);
		if (chan->chan.ops->status) {
			chan->chan.ops->status(&chan->chan, chan->chan.status);
		}
		if (chan_has_data(chan)) {
			raise_data_ready(chan);
		}
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
	struct k_work_q *rtx_work_queue = le_chan->rtx_work.queue;

	if (rtx_work_queue == NULL || k_current_get() != &rtx_work_queue->thread) {
		k_work_cancel_delayable_sync(&le_chan->rtx_work, &le_chan->rtx_sync);
	} else {
		k_work_cancel_delayable(&le_chan->rtx_work);
	}

	/* Remove buffers on the SDU RX queue */
	while ((buf = k_fifo_get(&le_chan->rx_queue, K_NO_WAIT))) {
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
		 * for some reason (e.g. controller not supporting a feature)
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
	err = server->accept(conn, server, chan);
	if (err < 0) {
		return le_err_to_result(err);
	}

#if defined(CONFIG_BT_L2CAP_SEG_RECV)
	if (!(*chan)->ops->recv == !(*chan)->ops->seg_recv) {
		LOG_ERR("Exactly one of 'recv' or 'seg_recv' must be set");
		return BT_L2CAP_LE_ERR_UNACCEPT_PARAMS;
	}
#else
	if (!(*chan)->ops->recv) {
		LOG_ERR("Mandatory callback 'recv' missing");
		return BT_L2CAP_LE_ERR_UNACCEPT_PARAMS;
	}
#endif

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

	/* Set channel PSM */
	le_chan->psm = server->psm;

	/* Update state */
	bt_l2cap_chan_set_state(*chan, BT_L2CAP_CONNECTED);

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
		LOG_ERR("Invalid LE-Conn Req params: mtu %u mps %u", mtu, mps);
		return;
	}

	buf = l2cap_create_le_sig_pdu(BT_L2CAP_LE_CONN_RSP, ident,
				      sizeof(*rsp));
	if (!buf) {
		return;
	}

	rsp = net_buf_add(buf, sizeof(*rsp));
	(void)memset(rsp, 0, sizeof(*rsp));

	/* Check if there is a server registered */
	server = bt_l2cap_server_lookup_psm(psm);
	if (!server) {
		result = BT_L2CAP_LE_ERR_PSM_NOT_SUPP;
		goto rsp;
	}

	/* Check if connection has minimum required security level */
	result = l2cap_check_security(conn, server);
	if (result != BT_L2CAP_LE_SUCCESS) {
		goto rsp;
	}

	result = l2cap_chan_accept(conn, server, scid, mtu, mps, credits,
				   &chan);
	if (result != BT_L2CAP_LE_SUCCESS) {
		goto rsp;
	}

	le_chan = BT_L2CAP_LE_CHAN(chan);

	/* Prepare response protocol data */
	rsp->dcid = sys_cpu_to_le16(le_chan->rx.cid);
	rsp->mps = sys_cpu_to_le16(le_chan->rx.mps);
	rsp->mtu = sys_cpu_to_le16(le_chan->rx.mtu);
	rsp->credits = sys_cpu_to_le16(le_chan->rx.credits);

	result = BT_L2CAP_LE_SUCCESS;

rsp:
	rsp->result = sys_cpu_to_le16(result);

	if (l2cap_send_sig(conn, buf)) {
		return;
	}

	/* Raise connected callback on success */
	if ((result == BT_L2CAP_LE_SUCCESS) && (chan->ops->connected != NULL)) {
		chan->ops->connected(chan);
	}
}

#if defined(CONFIG_BT_L2CAP_ECRED)
static void le_ecred_conn_req(struct bt_l2cap *l2cap, uint8_t ident,
			      struct net_buf *buf)
{
	struct bt_conn *conn = l2cap->chan.chan.conn;
	struct bt_l2cap_chan *chan[BT_L2CAP_ECRED_CHAN_MAX_PER_REQ];
	struct bt_l2cap_le_chan *ch = NULL;
	struct bt_l2cap_server *server;
	struct bt_l2cap_ecred_conn_req *req;
	struct bt_l2cap_ecred_conn_rsp *rsp;
	uint16_t mtu, mps, credits, result = BT_L2CAP_LE_SUCCESS;
	uint16_t psm = 0x0000;
	uint16_t scid, dcid[BT_L2CAP_ECRED_CHAN_MAX_PER_REQ];
	int i = 0;
	uint8_t req_cid_count;
	bool rsp_queued = false;

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
		req_cid_count = BT_L2CAP_ECRED_CHAN_MAX_PER_REQ;
		result = BT_L2CAP_LE_ERR_INVALID_PARAMS;
		goto response;
	}

	psm = sys_le16_to_cpu(req->psm);
	mtu = sys_le16_to_cpu(req->mtu);
	mps = sys_le16_to_cpu(req->mps);
	credits = sys_le16_to_cpu(req->credits);

	LOG_DBG("psm 0x%02x mtu %u mps %u credits %u", psm, mtu, mps, credits);

	if (mtu < BT_L2CAP_ECRED_MIN_MTU || mps < BT_L2CAP_ECRED_MIN_MTU) {
		LOG_ERR("Invalid ecred conn req params. mtu %u mps %u", mtu, mps);
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
	buf = l2cap_create_le_sig_pdu(BT_L2CAP_ECRED_CONN_RSP, ident,
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
		rsp->credits = sys_cpu_to_le16(ch->rx.credits);
	}
	rsp->result = sys_cpu_to_le16(result);

	net_buf_add_mem(buf, dcid, sizeof(scid) * req_cid_count);

	if (l2cap_send_sig(conn, buf)) {
		goto callback;
	}

	rsp_queued = true;

callback:
	if (ecred_cb && ecred_cb->ecred_conn_req) {
		ecred_cb->ecred_conn_req(conn, result, psm);
	}
	if (rsp_queued) {
		for (i = 0; i < req_cid_count; i++) {
			/* Raise connected callback for established channels */
			if ((dcid[i] != 0x00) && (chan[i]->ops->connected != NULL)) {
				chan[i]->ops->connected(chan[i]);
			}
		}
	}
}

static void le_ecred_reconf_req(struct bt_l2cap *l2cap, uint8_t ident,
				struct net_buf *buf)
{
	struct bt_conn *conn = l2cap->chan.chan.conn;
	struct bt_l2cap_chan *chans[BT_L2CAP_ECRED_CHAN_MAX_PER_REQ];
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

	if (mps < BT_L2CAP_ECRED_MIN_MTU) {
		result = BT_L2CAP_RECONF_OTHER_UNACCEPT;
		goto response;
	}

	if (mtu < BT_L2CAP_ECRED_MIN_MTU) {
		result = BT_L2CAP_RECONF_INVALID_MTU;
		goto response;
	}

	/* The specification only allows up to 5 CIDs in this packet */
	if (buf->len > (BT_L2CAP_ECRED_CHAN_MAX_PER_REQ * sizeof(scid))) {
		result = BT_L2CAP_RECONF_OTHER_UNACCEPT;
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
	buf = l2cap_create_le_sig_pdu(BT_L2CAP_ECRED_RECONF_RSP, ident,
				      sizeof(*rsp));
	if (!buf) {
		return;
	}

	rsp = net_buf_add(buf, sizeof(*rsp));
	rsp->result = sys_cpu_to_le16(result);

	l2cap_send_sig(conn, buf);
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

	buf = l2cap_create_le_sig_pdu(BT_L2CAP_DISCONN_RSP, ident,
				      sizeof(*rsp));
	if (!buf) {
		return;
	}

	rsp = net_buf_add(buf, sizeof(*rsp));
	rsp->dcid = sys_cpu_to_le16(chan->rx.cid);
	rsp->scid = sys_cpu_to_le16(chan->tx.cid);

	bt_l2cap_chan_del(&chan->chan);

	l2cap_send_sig(conn, buf);
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
}

static void reject_cmd(struct bt_l2cap *l2cap, uint8_t ident,
		       struct net_buf *buf)
{
	struct bt_conn *conn = l2cap->chan.chan.conn;
	struct bt_l2cap_le_chan *chan;

	while ((chan = l2cap_remove_ident(conn, ident))) {
		bt_l2cap_chan_del(&chan->chan);
	}
}
#endif /* CONFIG_BT_L2CAP_DYNAMIC_CHANNEL */

static int l2cap_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_l2cap_le_chan *l2chan = CONTAINER_OF(chan, struct bt_l2cap_le_chan, chan);
	struct bt_l2cap *l2cap = CONTAINER_OF(l2chan, struct bt_l2cap, chan);
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

	/* Remove buffers on the TX queue */
	while ((buf = k_fifo_get(&le_chan->tx_queue, K_NO_WAIT))) {
		l2cap_tx_buf_destroy(chan->conn, buf, -ESHUTDOWN);
	}

	/* Remove buffers on the RX queue */
	while ((buf = k_fifo_get(&le_chan->rx_queue, K_NO_WAIT))) {
		net_buf_unref(buf);
	}

	/* Update status */
	if (chan->ops->status) {
		chan->ops->status(chan, chan->status);
	}
}

static void l2cap_chan_send_credits(struct bt_l2cap_le_chan *chan,
				    uint16_t credits)
{
	struct bt_l2cap_le_credits *ev;
	struct net_buf *buf;

	__ASSERT_NO_MSG(bt_l2cap_chan_get_state(&chan->chan) == BT_L2CAP_CONNECTED);

	buf = l2cap_create_le_sig_pdu(BT_L2CAP_LE_CREDITS, get_ident(),
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

	l2cap_send_sig(chan->chan.conn, buf);

	LOG_DBG("chan %p credits %lu", chan, atomic_get(&chan->rx.credits));
}

#if defined(CONFIG_BT_L2CAP_SEG_RECV)
static int l2cap_chan_send_credits_pdu(struct bt_conn *conn, uint16_t cid, uint16_t credits)
{
	struct net_buf *buf;
	struct bt_l2cap_le_credits *ev;

	buf = l2cap_create_le_sig_pdu(BT_L2CAP_LE_CREDITS, get_ident(), sizeof(*ev));
	if (!buf) {
		return -ENOBUFS;
	}

	ev = net_buf_add(buf, sizeof(*ev));
	*ev = (struct bt_l2cap_le_credits){
		.cid = sys_cpu_to_le16(cid),
		.credits = sys_cpu_to_le16(credits),
	};

	return l2cap_send_sig(conn, buf);
}

/**
 * Combination of @ref atomic_add and @ref u16_add_overflow. Leaves @p
 * target unchanged if an overflow would occur. Assumes the current
 * value of @p target is representable by uint16_t.
 */
static bool atomic_add_safe_u16(atomic_t *target, uint16_t addition)
{
	uint16_t target_old, target_new;

	do {
		target_old = atomic_get(target);
		if (u16_add_overflow(target_old, addition, &target_new)) {
			return true;
		}
	} while (!atomic_cas(target, target_old, target_new));

	return false;
}

int bt_l2cap_chan_give_credits(struct bt_l2cap_chan *chan, uint16_t additional_credits)
{
	struct bt_l2cap_le_chan *le_chan = BT_L2CAP_LE_CHAN(chan);

	if (!chan || !chan->ops) {
		LOG_ERR("%s: Invalid chan object.", __func__);
		return -EINVAL;
	}

	if (!chan->ops->seg_recv) {
		LOG_ERR("%s: Available only with seg_recv.", __func__);
		return -EINVAL;
	}

	if (additional_credits == 0) {
		LOG_ERR("%s: Refusing to give 0.", __func__);
		return -EINVAL;
	}

	if (bt_l2cap_chan_get_state(chan) == BT_L2CAP_CONNECTING) {
		LOG_ERR("%s: Cannot give credits while connecting.", __func__);
		return -EBUSY;
	}

	if (atomic_add_safe_u16(&le_chan->rx.credits, additional_credits)) {
		LOG_ERR("%s: Overflow.", __func__);
		return -EOVERFLOW;
	}

	if (bt_l2cap_chan_get_state(chan) == BT_L2CAP_CONNECTED) {
		int err;

		err = l2cap_chan_send_credits_pdu(chan->conn, le_chan->rx.cid, additional_credits);
		if (err) {
			LOG_ERR("%s: PDU failed %d.", __func__, err);
			return err;
		}
	}

	return 0;
}
#endif /* CONFIG_BT_L2CAP_SEG_RECV */

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

	if (IS_ENABLED(CONFIG_BT_CLASSIC) && conn->type == BT_CONN_TYPE_BR) {
		return bt_l2cap_br_chan_recv_complete(chan);
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

	LOG_DBG("chan %p len %u", chan, buf->len);

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

	len = chan->_sdu->len;
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

	LOG_DBG("chan %p seg %d len %u", chan, seg, buf->len);

	/* Append received segment to SDU */
	len = net_buf_append_bytes(chan->_sdu, buf->len, buf->data, K_NO_WAIT,
				   l2cap_alloc_frag, chan);
	if (len != buf->len) {
		LOG_ERR("Unable to store SDU");
		bt_l2cap_chan_disconnect(&chan->chan);
		return;
	}

	if (chan->_sdu->len < chan->_sdu_len) {
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

#if defined(CONFIG_BT_L2CAP_SEG_RECV)
static void l2cap_chan_le_recv_seg_direct(struct bt_l2cap_le_chan *chan, struct net_buf *seg)
{
	uint16_t seg_offset;
	uint16_t sdu_remaining;

	if (chan->_sdu_len_done == chan->_sdu_len) {

		/* This is the first PDU in a SDU. */

		if (seg->len < 2) {
			LOG_WRN("Missing SDU header");
			bt_l2cap_chan_disconnect(&chan->chan);
			return;
		}

		/* Pop off the "SDU header". */
		chan->_sdu_len = net_buf_pull_le16(seg);
		chan->_sdu_len_done = 0;

		if (chan->_sdu_len > chan->rx.mtu) {
			LOG_WRN("SDU exceeds MTU");
			bt_l2cap_chan_disconnect(&chan->chan);
			return;
		}
	}

	seg_offset = chan->_sdu_len_done;
	sdu_remaining = chan->_sdu_len - chan->_sdu_len_done;

	if (seg->len > sdu_remaining) {
		LOG_WRN("L2CAP RX PDU total exceeds SDU");
		bt_l2cap_chan_disconnect(&chan->chan);
		return;
	}

	/* Commit receive. */
	chan->_sdu_len_done += seg->len;

	/* Tail call. */
	chan->chan.ops->seg_recv(&chan->chan, chan->_sdu_len, seg_offset, &seg->b);
}
#endif /* CONFIG_BT_L2CAP_SEG_RECV */

static void l2cap_chan_le_recv(struct bt_l2cap_le_chan *chan,
			       struct net_buf *buf)
{
	struct net_buf *owned_ref;
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

	/* Redirect to experimental API. */
	IF_ENABLED(CONFIG_BT_L2CAP_SEG_RECV, (
		if (chan->chan.ops->seg_recv) {
			l2cap_chan_le_recv_seg_direct(chan, buf);
			return;
		}
	))

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

	owned_ref = net_buf_ref(buf);
	err = chan->chan.ops->recv(&chan->chan, owned_ref);
	if (err != -EINPROGRESS) {
		net_buf_unref(owned_ref);
		owned_ref = NULL;
	}

	if (err < 0) {
		if (err != -EINPROGRESS) {
			LOG_ERR("err %d", err);
			bt_l2cap_chan_disconnect(&chan->chan);
		}
		return;
	}

	/* Only attempt to send credits if the channel wasn't disconnected
	 * in the recv() callback above
	 */
	if (bt_l2cap_chan_get_state(&chan->chan) == BT_L2CAP_CONNECTED) {
		l2cap_chan_send_credits(chan, 1);
	}
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

	k_fifo_put(&chan->rx_queue, buf);
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

	if (IS_ENABLED(CONFIG_BT_CLASSIC) &&
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

	buf = l2cap_create_le_sig_pdu(BT_L2CAP_CONN_PARAM_REQ,
				      get_ident(), sizeof(*req));
	if (!buf) {
		return -ENOMEM;
	}

	req = net_buf_add(buf, sizeof(*req));
	req->min_interval = sys_cpu_to_le16(param->interval_min);
	req->max_interval = sys_cpu_to_le16(param->interval_max);
	req->latency = sys_cpu_to_le16(param->latency);
	req->timeout = sys_cpu_to_le16(param->timeout);

	return l2cap_send_sig(conn, buf);
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
	if (IS_ENABLED(CONFIG_BT_CLASSIC)) {
		bt_l2cap_br_init();
	}
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
	for (i = 0; i < BT_L2CAP_ECRED_CHAN_MAX_PER_REQ; i++) {
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

	for (i = 0; i < BT_L2CAP_ECRED_CHAN_MAX_PER_REQ; i++) {
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

	buf = l2cap_create_le_sig_pdu(BT_L2CAP_ECRED_RECONF_REQ,
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

#if defined(CONFIG_BT_L2CAP_RECONFIGURE_EXPLICIT)
int bt_l2cap_ecred_chan_reconfigure_explicit(struct bt_l2cap_chan **chans, size_t chan_count,
					     uint16_t mtu, uint16_t mps)
{
	struct bt_l2cap_ecred_reconf_req *req;
	struct bt_conn *conn = NULL;
	struct net_buf *buf;
	uint8_t ident;

	LOG_DBG("chans %p chan_count %u mtu 0x%04x mps 0x%04x", chans, chan_count, mtu, mps);

	if (!chans || !IN_RANGE(chan_count, 1, BT_L2CAP_ECRED_CHAN_MAX_PER_REQ)) {
		return -EINVAL;
	}

	if (!IN_RANGE(mps, BT_L2CAP_ECRED_MIN_MPS, BT_L2CAP_RX_MTU)) {
		return -EINVAL;
	}

	for (size_t i = 0; i < chan_count; i++) {
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

		/* MPS is not allowed to decrease when reconfiguring multiple channels.
		 * Core Specification 3.A.4.27 v6.0
		 */
		if (chan_count > 1 && mps < BT_L2CAP_LE_CHAN(chans[i])->rx.mps) {
			return -EINVAL;
		}
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

	buf = l2cap_create_le_sig_pdu(BT_L2CAP_ECRED_RECONF_REQ, ident,
				      sizeof(*req) + (chan_count * sizeof(uint16_t)));
	if (!buf) {
		return -ENOMEM;
	}

	req = net_buf_add(buf, sizeof(*req));
	req->mtu = sys_cpu_to_le16(mtu);
	req->mps = sys_cpu_to_le16(mps);

	for (size_t i = 0; i < chan_count; i++) {
		struct bt_l2cap_le_chan *ch;

		ch = BT_L2CAP_LE_CHAN(chans[i]);

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
#endif /* defined(CONFIG_BT_L2CAP_RECONFIGURE_EXPLICIT) */

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

	if (IS_ENABLED(CONFIG_BT_CLASSIC) &&
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

	if (IS_ENABLED(CONFIG_BT_CLASSIC) &&
	    conn->type == BT_CONN_TYPE_BR) {
		return bt_l2cap_br_chan_disconnect(chan);
	}

	le_chan = BT_L2CAP_LE_CHAN(chan);

	LOG_DBG("chan %p scid 0x%04x dcid 0x%04x", chan, le_chan->rx.cid, le_chan->tx.cid);

	le_chan->ident = get_ident();

	buf = l2cap_create_le_sig_pdu(BT_L2CAP_DISCONN_REQ,
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

__maybe_unused static bool user_data_not_empty(const struct net_buf *buf)
{
	size_t ud_len = sizeof(struct closure);
	const uint8_t *ud = net_buf_user_data(buf);

	for (size_t i = 0; i < ud_len; i++) {
		if (ud[i] != 0) {
			return true;
		}
	}

	return false;
}

static int bt_l2cap_dyn_chan_send(struct bt_l2cap_le_chan *le_chan, struct net_buf *buf)
{
	uint16_t sdu_len = buf->len;

	LOG_DBG("chan %p buf %p", le_chan, buf);

	/* Frags are not supported. */
	__ASSERT_NO_MSG(buf->frags == NULL);

	if (sdu_len > le_chan->tx.mtu) {
		LOG_ERR("attempt to send %u bytes on %u MTU chan",
			sdu_len, le_chan->tx.mtu);
		return -EMSGSIZE;
	}

	if (buf->ref != 1) {
		/* The host may alter the buf contents when segmenting. Higher
		 * layers cannot expect the buf contents to stay intact. Extra
		 * refs suggests a silent data corruption would occur if not for
		 * this error.
		 */
		LOG_ERR("buf given to l2cap has other refs");
		return -EINVAL;
	}

	if (net_buf_headroom(buf) < BT_L2CAP_SDU_CHAN_SEND_RESERVE) {
		/* Call `net_buf_reserve(buf, BT_L2CAP_SDU_CHAN_SEND_RESERVE)`
		 * when allocating buffers intended for bt_l2cap_chan_send().
		 */
		LOG_ERR("Not enough headroom in buf %p", buf);
		return -EINVAL;
	}

	if (user_data_not_empty(buf)) {
		/* There may be issues if user_data is not empty. */
		LOG_WRN("user_data is not empty");
	}

	/* Prepend SDU length.
	 *
	 * L2CAP LE CoC SDUs are segmented and put into K-frames PDUs which have
	 * their own L2CAP header (i.e. PDU length, channel id).
	 *
	 * The SDU length is right before the data that will be segmented and is
	 * only present in the first PDU. Here's an example:
	 *
	 * Sent data payload of 50 bytes over channel 0x4040 with MPS of 30 bytes:
	 * First PDU (K-frame):
	 * | L2CAP K-frame header        | K-frame payload                 |
	 * | PDU length  | Channel ID    | SDU length   | SDU payload      |
	 * | 0x001e      | 0x4040        | 0x0032       | 28 bytes of data |
	 *
	 * Second and last PDU (K-frame):
	 * | L2CAP K-frame header        | K-frame payload     |
	 * | PDU length  | Channel ID    | rest of SDU payload |
	 * | 0x0016      | 0x4040        | 22 bytes of data    |
	 */
	net_buf_push_le16(buf, sdu_len);

	/* Put buffer on TX queue */
	k_fifo_put(&le_chan->tx_queue, buf);

	/* Always process the queue in the same context */
	raise_data_ready(le_chan);

	return 0;
}

int bt_l2cap_chan_send(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	if (!buf || !chan) {
		return -EINVAL;
	}

	LOG_DBG("chan %p buf %p len %u", chan, buf, buf->len);

	if (buf->ref != 1) {
		LOG_WRN("Expecting 1 ref, got %d", buf->ref);
		return -EINVAL;
	}

	if (!chan->conn || chan->conn->state != BT_CONN_CONNECTED) {
		return -ENOTCONN;
	}

	if (atomic_test_bit(chan->status, BT_L2CAP_STATUS_SHUTDOWN)) {
		return -ESHUTDOWN;
	}

	if (IS_ENABLED(CONFIG_BT_CLASSIC) &&
	    chan->conn->type == BT_CONN_TYPE_BR) {
		return bt_l2cap_br_chan_send_cb(chan, buf, NULL, NULL);
	}

	/* Sending over static channels is not supported by this fn. Use
	 * `bt_l2cap_send_pdu()` instead.
	 */
	if (IS_ENABLED(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)) {
		struct bt_l2cap_le_chan *le_chan = BT_L2CAP_LE_CHAN(chan);

		__ASSERT_NO_MSG(le_chan);
		__ASSERT_NO_MSG(L2CAP_LE_CID_IS_DYN(le_chan->tx.cid));

		return bt_l2cap_dyn_chan_send(le_chan, buf);
	}

	LOG_DBG("Invalid channel type (chan %p)", chan);

	return -EINVAL;
}
#endif /* CONFIG_BT_L2CAP_DYNAMIC_CHANNEL */
