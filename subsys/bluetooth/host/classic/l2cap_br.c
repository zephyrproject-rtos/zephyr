/* l2cap_br.c - L2CAP BREDR oriented handling */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>

#include "host/buf_view.h"
#include "host/hci_core.h"
#include "host/conn_internal.h"
#include "l2cap_br_internal.h"
#include "avdtp_internal.h"
#include "a2dp_internal.h"
#include "avctp_internal.h"
#include "avrcp_internal.h"
#include "rfcomm_internal.h"
#include "sdp_internal.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_l2cap_br, CONFIG_BT_L2CAP_LOG_LEVEL);

#define BR_CHAN_RTX(_w) CONTAINER_OF(k_work_delayable_from_work(_w), \
				     struct bt_l2cap_br_chan, rtx_work)

#define L2CAP_BR_PSM_START	0x0001
#define L2CAP_BR_PSM_END	0xffff
#define L2CAP_BR_PSM_DYN_START	0x1000
#define L2CAP_BR_PSM_DYN_END	L2CAP_BR_PSM_END

#define L2CAP_BR_CID_DYN_START	0x0040
#define L2CAP_BR_CID_DYN_END	0xffff
#define L2CAP_BR_CID_IS_DYN(_cid) \
	(_cid >= L2CAP_BR_CID_DYN_START && _cid <= L2CAP_BR_CID_DYN_END)

#define L2CAP_BR_MIN_MTU	48
#define L2CAP_BR_DEFAULT_MTU	672

#define L2CAP_BR_PSM_SDP	0x0001

#define L2CAP_BR_INFO_TIMEOUT		K_SECONDS(4)
#define L2CAP_BR_CFG_TIMEOUT		K_SECONDS(4)
#define L2CAP_BR_DISCONN_TIMEOUT	K_SECONDS(1)
#define L2CAP_BR_CONN_TIMEOUT		K_SECONDS(40)

/*
 * L2CAP extended feature mask:
 * BR/EDR fixed channel support enabled
 */
#define L2CAP_FEAT_FIXED_CHAN_MASK	0x00000080

enum {
	/* Connection oriented channels flags */
	L2CAP_FLAG_CONN_LCONF_DONE,	/* local config accepted by remote */
	L2CAP_FLAG_CONN_RCONF_DONE,	/* remote config accepted by local */
	L2CAP_FLAG_CONN_ACCEPTOR,	/* getting incoming connection req */
	L2CAP_FLAG_CONN_PENDING,	/* remote sent pending result in rsp */

	/* Signaling channel flags */
	L2CAP_FLAG_SIG_INFO_PENDING,	/* retrieving remote l2cap info */
	L2CAP_FLAG_SIG_INFO_DONE,	/* remote l2cap info is done */

	/* fixed channels flags */
	L2CAP_FLAG_FIXED_CONNECTED,		/* fixed connected */
};

static sys_slist_t br_servers;


/* Pool for outgoing BR/EDR signaling packets, min MTU is 48 */
NET_BUF_POOL_FIXED_DEFINE(br_sig_pool, CONFIG_BT_MAX_CONN,
			  BT_L2CAP_BUF_SIZE(L2CAP_BR_MIN_MTU), 8, NULL);

/* BR/EDR L2CAP signalling channel specific context */
struct bt_l2cap_br {
	/* The channel this context is associated with */
	struct bt_l2cap_br_chan	chan;
	uint8_t			info_ident;
	/*
	 * 2.1 CHANNEL IDENTIFIERS in
	 * BLUETOOTH CORE SPECIFICATION Version 5.4 | Vol 3, Part A.
	 * The range of fixed L2CAP CID is 0x0001 ~ 0x0007 both for LE and BR.
	 * So use one octet buffer to keep the `Fixed channels supported`
	 * of peer device.
	 */
	uint8_t			info_fixed_chan;
	uint32_t			info_feat_mask;
};

static struct bt_l2cap_br bt_l2cap_br_pool[CONFIG_BT_MAX_CONN];

struct bt_l2cap_chan *bt_l2cap_br_lookup_rx_cid(struct bt_conn *conn,
						uint16_t cid)
{
	struct bt_l2cap_chan *chan;

	SYS_SLIST_FOR_EACH_CONTAINER(&conn->channels, chan, node) {
		if (BR_CHAN(chan)->rx.cid == cid) {
			return chan;
		}
	}

	return NULL;
}

struct bt_l2cap_chan *bt_l2cap_br_lookup_tx_cid(struct bt_conn *conn,
						uint16_t cid)
{
	struct bt_l2cap_chan *chan;

	SYS_SLIST_FOR_EACH_CONTAINER(&conn->channels, chan, node) {
		if (BR_CHAN(chan)->tx.cid == cid) {
			return chan;
		}
	}

	return NULL;
}

uint8_t bt_l2cap_br_get_remote_fixed_chan(struct bt_conn *conn)
{
	struct bt_l2cap_chan *chan_sig;
	struct bt_l2cap_br *br_chan_sig;

	chan_sig = bt_l2cap_br_lookup_rx_cid(conn, BT_L2CAP_CID_BR_SIG);
	if (!chan_sig) {
		return (uint8_t)0U;
	}

	br_chan_sig = CONTAINER_OF(chan_sig, struct bt_l2cap_br, chan.chan);

	return br_chan_sig->info_fixed_chan;
}

static struct bt_l2cap_br_chan*
l2cap_br_chan_alloc_cid(struct bt_conn *conn, struct bt_l2cap_chan *chan)
{
	struct bt_l2cap_br_chan *br_chan = BR_CHAN(chan);
	uint16_t cid;

	/*
	 * No action needed if there's already a CID allocated, e.g. in
	 * the case of a fixed channel.
	 */
	if (br_chan->rx.cid > 0) {
		return br_chan;
	}

	/*
	 * L2CAP_BR_CID_DYN_END is 0xffff so we don't check against it since
	 * cid is uint16_t, just check against uint16_t overflow
	 */
	for (cid = L2CAP_BR_CID_DYN_START; cid; cid++) {
		if (!bt_l2cap_br_lookup_rx_cid(conn, cid)) {
			br_chan->rx.cid = cid;
			return br_chan;
		}
	}

	return NULL;
}

static void l2cap_br_chan_cleanup(struct bt_l2cap_chan *chan)
{
	bt_l2cap_chan_remove(chan->conn, chan);
	bt_l2cap_br_chan_del(chan);
}

static void l2cap_br_chan_destroy(struct bt_l2cap_chan *chan)
{
	struct bt_l2cap_br_chan *br_chan = BR_CHAN(chan);

	LOG_DBG("chan %p cid 0x%04x", br_chan, br_chan->rx.cid);

	/* Cancel ongoing work. Since the channel can be re-used after this
	 * we need to sync to make sure that the kernel does not have it
	 * in its queue anymore.
	 *
	 * In the case where we are in the context of executing the rtx_work
	 * item, we don't sync as it will deadlock the workqueue.
	 */
	struct k_work_q *rtx_work_queue = br_chan->rtx_work.queue;

	if (rtx_work_queue == NULL || k_current_get() != &rtx_work_queue->thread) {
		k_work_cancel_delayable_sync(&br_chan->rtx_work, &br_chan->rtx_sync);
	} else {
		k_work_cancel_delayable(&br_chan->rtx_work);
	}

	atomic_clear(BR_CHAN(chan)->flags);
}

static void l2cap_br_rtx_timeout(struct k_work *work)
{
	struct bt_l2cap_br_chan *chan = BR_CHAN_RTX(work);

	LOG_WRN("chan %p timeout", chan);

	if (chan->rx.cid == BT_L2CAP_CID_BR_SIG) {
		LOG_DBG("Skip BR/EDR signalling channel ");
		atomic_clear_bit(chan->flags, L2CAP_FLAG_SIG_INFO_PENDING);
		return;
	}

	LOG_DBG("chan %p %s scid 0x%04x", chan, bt_l2cap_chan_state_str(chan->state), chan->rx.cid);

	switch (chan->state) {
	case BT_L2CAP_CONFIG:
		bt_l2cap_br_chan_disconnect(&chan->chan);
		break;
	case BT_L2CAP_DISCONNECTING:
	case BT_L2CAP_CONNECTING:
		l2cap_br_chan_cleanup(&chan->chan);
		break;
	default:
		break;
	}
}

static bool l2cap_br_chan_add(struct bt_conn *conn, struct bt_l2cap_chan *chan,
			      bt_l2cap_chan_destroy_t destroy)
{
	struct bt_l2cap_br_chan *ch = l2cap_br_chan_alloc_cid(conn, chan);

	if (!ch) {
		LOG_DBG("Unable to allocate L2CAP CID");
		return false;
	}

	k_fifo_init(&ch->_pdu_tx_queue);

	/* All dynamic channels have the destroy handler which makes sure that
	 * the RTX work structure is properly released with a cancel sync.
	 * The fixed signal channel is only removed when disconnected and the
	 * disconnected handler is always called from the workqueue itself so
	 * canceling from there should always succeed.
	 */
	k_work_init_delayable(&ch->rtx_work, l2cap_br_rtx_timeout);
	bt_l2cap_chan_add(conn, chan, destroy);

	return true;
}

static uint8_t l2cap_br_get_ident(void)
{
	static uint8_t ident;

	ident++;
	/* handle integer overflow (0 is not valid) */
	if (!ident) {
		ident++;
	}

	return ident;
}

static void raise_data_ready(struct bt_l2cap_br_chan *br_chan)
{
	if (!atomic_set(&br_chan->_pdu_ready_lock, 1)) {
		sys_slist_append(&br_chan->chan.conn->l2cap_data_ready,
				 &br_chan->_pdu_ready);
		LOG_DBG("data ready raised");
	} else {
		LOG_DBG("data ready already");
	}

	bt_conn_data_ready(br_chan->chan.conn);
}

static void lower_data_ready(struct bt_l2cap_br_chan *br_chan)
{
	struct bt_conn *conn = br_chan->chan.conn;
	__maybe_unused sys_snode_t *s = sys_slist_get(&conn->l2cap_data_ready);

	__ASSERT_NO_MSG(s == &br_chan->_pdu_ready);

	__maybe_unused atomic_t old = atomic_set(&br_chan->_pdu_ready_lock, 0);

	__ASSERT_NO_MSG(old);
}

static void cancel_data_ready(struct bt_l2cap_br_chan *br_chan)
{
	struct bt_conn *conn = br_chan->chan.conn;

	sys_slist_find_and_remove(&conn->l2cap_data_ready,
				  &br_chan->_pdu_ready);

	atomic_set(&br_chan->_pdu_ready_lock, 0);
}

int bt_l2cap_br_send_cb(struct bt_conn *conn, uint16_t cid, struct net_buf *buf,
			bt_conn_tx_cb_t cb, void *user_data)
{
	struct bt_l2cap_hdr *hdr;
	struct bt_l2cap_chan *ch = bt_l2cap_br_lookup_tx_cid(conn, cid);
	struct bt_l2cap_br_chan *br_chan = CONTAINER_OF(ch, struct bt_l2cap_br_chan, chan);

	LOG_DBG("chan %p buf %p len %zu", br_chan, buf, buf->len);

	hdr = net_buf_push(buf, sizeof(*hdr));
	hdr->len = sys_cpu_to_le16(buf->len - sizeof(*hdr));
	hdr->cid = sys_cpu_to_le16(cid);

	if (buf->user_data_size < sizeof(struct closure)) {
		LOG_WRN("not enough room in user_data %d < %d pool %u",
			buf->user_data_size,
			CONFIG_BT_CONN_TX_USER_DATA_SIZE,
			buf->pool_id);
		return -EINVAL;
	}

	LOG_DBG("push PDU: cb %p userdata %p", cb, user_data);

	make_closure(buf->user_data, cb, user_data);
	k_fifo_put(&br_chan->_pdu_tx_queue, buf);
	raise_data_ready(br_chan);

	return 0;
}

/* Send the buffer and release it in case of failure.
 * Any other cleanup in failure to send should be handled by the disconnected
 * handler.
 */
static inline void l2cap_send(struct bt_conn *conn, uint16_t cid,
			      struct net_buf *buf)
{
	if (bt_l2cap_br_send_cb(conn, cid, buf, NULL, NULL)) {
		net_buf_unref(buf);
	}
}

static void l2cap_br_chan_send_req(struct bt_l2cap_br_chan *chan,
				   struct net_buf *buf, k_timeout_t timeout)
{

	if (bt_l2cap_br_send_cb(chan->chan.conn, BT_L2CAP_CID_BR_SIG, buf,
			     NULL, NULL)) {
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
	k_work_reschedule(&chan->rtx_work, timeout);
}

/* L2CAP channel wants to send a PDU */
static bool chan_has_data(struct bt_l2cap_br_chan *br_chan)
{
	return !k_fifo_is_empty(&br_chan->_pdu_tx_queue);
}

struct net_buf *l2cap_br_data_pull(struct bt_conn *conn,
				   size_t amount,
				   size_t *length)
{
	const sys_snode_t *pdu_ready = sys_slist_peek_head(&conn->l2cap_data_ready);

	if (!pdu_ready) {
		LOG_DBG("nothing to send on this conn");
		return NULL;
	}

	struct bt_l2cap_br_chan *br_chan = CONTAINER_OF(pdu_ready,
							struct bt_l2cap_br_chan,
							_pdu_ready);

	/* Leave the PDU buffer in the queue until we have sent all its
	 * fragments.
	 */
	struct net_buf *pdu = k_fifo_peek_head(&br_chan->_pdu_tx_queue);

	__ASSERT(pdu, "signaled ready but no PDUs in the TX queue");

	if (bt_buf_has_view(pdu)) {
		LOG_ERR("already have view on %p", pdu);
		return NULL;
	}

	/* We can't interleave ACL fragments from different channels for the
	 * same ACL conn -> we have to wait until a full L2 PDU is transferred
	 * before switching channels.
	 */
	bool last_frag = amount >= pdu->len;

	if (last_frag) {
		LOG_DBG("last frag, removing %p", pdu);
		__maybe_unused struct net_buf *b = k_fifo_get(&br_chan->_pdu_tx_queue, K_NO_WAIT);

		__ASSERT_NO_MSG(b == pdu);

		LOG_DBG("chan %p done", br_chan);
		lower_data_ready(br_chan);

		/* Append channel to list if it still has data */
		if (chan_has_data(br_chan)) {
			LOG_DBG("chan %p ready", br_chan);
			raise_data_ready(br_chan);
		}
	}

	*length = pdu->len;

	return pdu;
}

static void l2cap_br_get_info(struct bt_l2cap_br *l2cap, uint16_t info_type)
{
	struct bt_l2cap_info_req *info;
	struct net_buf *buf;
	struct bt_l2cap_sig_hdr *hdr;

	LOG_DBG("info type %u", info_type);

	if (atomic_test_bit(l2cap->chan.flags, L2CAP_FLAG_SIG_INFO_PENDING)) {
		return;
	}

	switch (info_type) {
	case BT_L2CAP_INFO_FEAT_MASK:
	case BT_L2CAP_INFO_FIXED_CHAN:
		break;
	default:
		LOG_WRN("Unsupported info type %u", info_type);
		return;
	}

	buf = bt_l2cap_create_pdu(&br_sig_pool, 0);

	atomic_set_bit(l2cap->chan.flags, L2CAP_FLAG_SIG_INFO_PENDING);
	l2cap->info_ident = l2cap_br_get_ident();

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->code = BT_L2CAP_INFO_REQ;
	hdr->ident = l2cap->info_ident;
	hdr->len = sys_cpu_to_le16(sizeof(*info));

	info = net_buf_add(buf, sizeof(*info));
	info->type = sys_cpu_to_le16(info_type);

	l2cap_br_chan_send_req(&l2cap->chan, buf, L2CAP_BR_INFO_TIMEOUT);
}

static void connect_fixed_channel(struct bt_l2cap_br_chan *chan)
{
	if (atomic_test_and_set_bit(chan->flags, L2CAP_FLAG_FIXED_CONNECTED)) {
		return;
	}

	if (chan->chan.ops && chan->chan.ops->connected) {
		chan->chan.ops->connected(&chan->chan);
	}
}

static void connect_optional_fixed_channels(struct bt_l2cap_br *l2cap)
{
	/* can be change to loop if more BR/EDR fixed channels are added */
	if (l2cap->info_fixed_chan & BIT(BT_L2CAP_CID_BR_SMP)) {
		struct bt_l2cap_chan *chan;

		chan = bt_l2cap_br_lookup_rx_cid(l2cap->chan.chan.conn,
						 BT_L2CAP_CID_BR_SMP);
		if (chan) {
			connect_fixed_channel(BR_CHAN(chan));
		}
	}
}

static int l2cap_br_info_rsp(struct bt_l2cap_br *l2cap, uint8_t ident,
			     struct net_buf *buf)
{
	struct bt_l2cap_info_rsp *rsp;
	uint16_t type, result;
	int err = 0;

	if (atomic_test_bit(l2cap->chan.flags, L2CAP_FLAG_SIG_INFO_DONE)) {
		return 0;
	}

	if (atomic_test_and_clear_bit(l2cap->chan.flags,
				      L2CAP_FLAG_SIG_INFO_PENDING)) {
		/*
		 * Release RTX timer since got the response & there's pending
		 * command request.
		 */
		k_work_cancel_delayable(&l2cap->chan.rtx_work);
	}

	if (buf->len < sizeof(*rsp)) {
		LOG_ERR("Too small info rsp packet size");
		err = -EINVAL;
		goto done;
	}

	if (ident != l2cap->info_ident) {
		LOG_WRN("Idents mismatch");
		err = -EINVAL;
		goto done;
	}

	rsp = net_buf_pull_mem(buf, sizeof(*rsp));
	result = sys_le16_to_cpu(rsp->result);
	if (result != BT_L2CAP_INFO_SUCCESS) {
		LOG_WRN("Result unsuccessful");
		err = -EINVAL;
		goto done;
	}

	type = sys_le16_to_cpu(rsp->type);

	switch (type) {
	case BT_L2CAP_INFO_FEAT_MASK:
		if (buf->len < sizeof(uint32_t)) {
			LOG_ERR("Invalid remote info feat mask");
			err = -EINVAL;
			break;
		}
		l2cap->info_feat_mask = net_buf_pull_le32(buf);
		LOG_DBG("remote info mask 0x%08x", l2cap->info_feat_mask);

		if (!(l2cap->info_feat_mask & L2CAP_FEAT_FIXED_CHAN_MASK)) {
			break;
		}

		l2cap_br_get_info(l2cap, BT_L2CAP_INFO_FIXED_CHAN);
		return 0;
	case BT_L2CAP_INFO_FIXED_CHAN:
		if (buf->len < sizeof(uint8_t)) {
			LOG_ERR("Invalid remote info fixed chan");
			err = -EINVAL;
			break;
		}
		/*
		 * 2.1 CHANNEL IDENTIFIERS in
		 * BLUETOOTH CORE SPECIFICATION Version 5.4 | Vol 3, Part A.
		 * The info length of `Fixed channels supported` is 8 octets.
		 * Then the range of fixed L2CAP CID is 0x0001 ~ 0x0007 both for LE and BR.
		 * So use one octet buffer to keep the `Fixed channels supported`
		 * of peer device.
		 */
		l2cap->info_fixed_chan = net_buf_pull_u8(buf);
		LOG_DBG("remote fixed channel mask 0x%02x", l2cap->info_fixed_chan);

		connect_optional_fixed_channels(l2cap);

		break;
	default:
		LOG_WRN("type 0x%04x unsupported", type);
		err = -EINVAL;
		break;
	}
done:
	atomic_set_bit(l2cap->chan.flags, L2CAP_FLAG_SIG_INFO_DONE);
	l2cap->info_ident = 0U;
	return err;
}

static uint8_t get_fixed_channels_mask(void)
{
	uint8_t mask = 0U;

	/* this needs to be enhanced if AMP Test Manager support is added */
	STRUCT_SECTION_FOREACH(bt_l2cap_br_fixed_chan, fchan) {
		mask |= BIT(fchan->cid);
	}

	return mask;
}

static int l2cap_br_info_req(struct bt_l2cap_br *l2cap, uint8_t ident,
			     struct net_buf *buf)
{
	struct bt_conn *conn = l2cap->chan.chan.conn;
	struct bt_l2cap_info_req *req = (void *)buf->data;
	struct bt_l2cap_info_rsp *rsp;
	struct net_buf *rsp_buf;
	struct bt_l2cap_sig_hdr *hdr_info;
	uint16_t type;

	if (buf->len < sizeof(*req)) {
		LOG_ERR("Too small info req packet size");
		return -EINVAL;
	}

	rsp_buf = bt_l2cap_create_pdu(&br_sig_pool, 0);

	type = sys_le16_to_cpu(req->type);
	LOG_DBG("type 0x%04x", type);

	hdr_info = net_buf_add(rsp_buf, sizeof(*hdr_info));
	hdr_info->code = BT_L2CAP_INFO_RSP;
	hdr_info->ident = ident;

	rsp = net_buf_add(rsp_buf, sizeof(*rsp));

	switch (type) {
	case BT_L2CAP_INFO_FEAT_MASK:
		rsp->type = sys_cpu_to_le16(BT_L2CAP_INFO_FEAT_MASK);
		rsp->result = sys_cpu_to_le16(BT_L2CAP_INFO_SUCCESS);
		net_buf_add_le32(rsp_buf, L2CAP_FEAT_FIXED_CHAN_MASK);
		hdr_info->len = sys_cpu_to_le16(sizeof(*rsp) + sizeof(uint32_t));
		break;
	case BT_L2CAP_INFO_FIXED_CHAN:
		rsp->type = sys_cpu_to_le16(BT_L2CAP_INFO_FIXED_CHAN);
		rsp->result = sys_cpu_to_le16(BT_L2CAP_INFO_SUCCESS);
		/* fixed channel mask protocol data is 8 octets wide */
		(void)memset(net_buf_add(rsp_buf, 8), 0, 8);
		rsp->data[0] = get_fixed_channels_mask();

		hdr_info->len = sys_cpu_to_le16(sizeof(*rsp) + 8);
		break;
	default:
		rsp->type = req->type;
		rsp->result = sys_cpu_to_le16(BT_L2CAP_INFO_NOTSUPP);
		hdr_info->len = sys_cpu_to_le16(sizeof(*rsp));
		break;
	}

	l2cap_send(conn, BT_L2CAP_CID_BR_SIG, rsp_buf);

	return 0;
}

void bt_l2cap_br_connected(struct bt_conn *conn)
{
	struct bt_l2cap_chan *chan;

	STRUCT_SECTION_FOREACH(bt_l2cap_br_fixed_chan, fchan) {
		struct bt_l2cap_br_chan *br_chan;

		if (!fchan->accept) {
			continue;
		}

		if (fchan->accept(conn, &chan) < 0) {
			continue;
		}

		br_chan = BR_CHAN(chan);

		br_chan->rx.cid = fchan->cid;
		br_chan->tx.cid = fchan->cid;

		if (!l2cap_br_chan_add(conn, chan, NULL)) {
			return;
		}

		/*
		 * other fixed channels will be connected after Information
		 * Response is received
		 */
		if (fchan->cid == BT_L2CAP_CID_BR_SIG) {
			struct bt_l2cap_br *sig_ch;

			connect_fixed_channel(br_chan);

			sig_ch = CONTAINER_OF(br_chan, struct bt_l2cap_br, chan);
			l2cap_br_get_info(sig_ch, BT_L2CAP_INFO_FEAT_MASK);
		}
	}
}

void bt_l2cap_br_disconnected(struct bt_conn *conn)
{
	struct bt_l2cap_chan *chan, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&conn->channels, chan, next, node) {
		bt_l2cap_br_chan_del(chan);
	}
}

static struct bt_l2cap_server *l2cap_br_server_lookup_psm(uint16_t psm)
{
	struct bt_l2cap_server *server;

	SYS_SLIST_FOR_EACH_CONTAINER(&br_servers, server, node) {
		if (server->psm == psm) {
			return server;
		}
	}

	return NULL;
}

static void l2cap_br_conf_add_mtu(struct net_buf *buf, const uint16_t mtu)
{
	net_buf_add_u8(buf, BT_L2CAP_CONF_OPT_MTU);
	net_buf_add_u8(buf, sizeof(mtu));
	net_buf_add_le16(buf, mtu);
}

static void l2cap_br_conf_add_opt(struct net_buf *buf, const struct bt_l2cap_conf_opt *opt)
{
	net_buf_add_u8(buf, opt->type & BT_L2CAP_CONF_MASK);
	net_buf_add_u8(buf, opt->len);
	net_buf_add_mem(buf, opt->data, opt->len);
}

static void l2cap_br_conf(struct bt_l2cap_chan *chan)
{
	struct bt_l2cap_sig_hdr *hdr;
	struct bt_l2cap_conf_req *conf;
	struct net_buf *buf;

	buf = bt_l2cap_create_pdu(&br_sig_pool, 0);

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->code = BT_L2CAP_CONF_REQ;
	hdr->ident = l2cap_br_get_ident();
	conf = net_buf_add(buf, sizeof(*conf));
	(void)memset(conf, 0, sizeof(*conf));

	conf->dcid = sys_cpu_to_le16(BR_CHAN(chan)->tx.cid);
	/*
	 * Add MTU option if app set non default BR/EDR L2CAP MTU,
	 * otherwise sent empty configuration data meaning default MTU
	 * to be used.
	 */
	if (BR_CHAN(chan)->rx.mtu != L2CAP_BR_DEFAULT_MTU) {
		l2cap_br_conf_add_mtu(buf, BR_CHAN(chan)->rx.mtu);
	}

	hdr->len = sys_cpu_to_le16(buf->len - sizeof(*hdr));

	/*
	 * TODO:
	 * might be needed to start tracking number of configuration iterations
	 * on both directions
	 */
	l2cap_br_chan_send_req(BR_CHAN(chan), buf, L2CAP_BR_CFG_TIMEOUT);
}

enum l2cap_br_conn_security_result {
	L2CAP_CONN_SECURITY_PASSED,
	L2CAP_CONN_SECURITY_REJECT,
	L2CAP_CONN_SECURITY_PENDING
};

/*
 * Security helper against channel connection.
 * Returns L2CAP_CONN_SECURITY_PASSED if:
 * - existing security on link is applicable for requested PSM in connection,
 * - legacy (non SSP) devices connecting with low security requirements,
 * Returns L2CAP_CONN_SECURITY_PENDING if:
 * - channel connection process is on hold since there were valid security
 *   conditions triggering authentication indirectly in subcall.
 * Returns L2CAP_CONN_SECURITY_REJECT if:
 * - bt_conn_set_security API returns < 0.
 */

static enum l2cap_br_conn_security_result
l2cap_br_conn_security(struct bt_l2cap_chan *chan, const uint16_t psm)
{
	int check;
	struct bt_l2cap_br_chan *br_chan = BR_CHAN(chan);

	/* For SDP PSM there's no need to change existing security on link */
	if (br_chan->required_sec_level == BT_SECURITY_L0) {
		return L2CAP_CONN_SECURITY_PASSED;
	}

	/*
	 * No link key needed for legacy devices (pre 2.1) and when low security
	 * level is required.
	 */
	if (br_chan->required_sec_level == BT_SECURITY_L1 &&
	    !BT_FEAT_HOST_SSP(chan->conn->br.features)) {
		return L2CAP_CONN_SECURITY_PASSED;
	}

	switch (br_chan->required_sec_level) {
	case BT_SECURITY_L4:
	case BT_SECURITY_L3:
	case BT_SECURITY_L2:
		break;
	default:
		/*
		 * For non SDP PSM connections GAP's Security Mode 4 requires at
		 * least unauthenticated link key and enabled encryption if
		 * remote supports SSP before any L2CAP CoC traffic. So preset
		 * local to MEDIUM security to trigger it if needed.
		 */
		if (BT_FEAT_HOST_SSP(chan->conn->br.features)) {
			br_chan->required_sec_level = BT_SECURITY_L2;
		}
		break;
	}

	check = bt_conn_set_security(chan->conn, br_chan->required_sec_level);

	/*
	 * Check case when on existing connection security level already covers
	 * channel (service) security requirements against link security and
	 * bt_conn_set_security API returns 0 what implies also there was no
	 * need to trigger authentication.
	 */
	if (check == 0 &&
	    chan->conn->sec_level >= br_chan->required_sec_level) {
		return L2CAP_CONN_SECURITY_PASSED;
	}

	/*
	 * If 'check' still holds 0, it means local host just sent HCI
	 * authentication command to start procedure to increase link security
	 * since service/profile requires that.
	 */
	if (check == 0) {
		/*
		 * General Bonding refers to the process of performing bonding
		 * during connection setup or channel establishment procedures
		 * as a precursor to accessing a service.
		 * For current case, it is dedicated bonding.
		 */
		atomic_set_bit(chan->conn->flags, BT_CONN_BR_GENERAL_BONDING);
		return L2CAP_CONN_SECURITY_PENDING;
	}

	/*
	 * For any other values in 'check' it means there was internal
	 * validation condition forbidding to start authentication at this
	 * moment.
	 */
	return L2CAP_CONN_SECURITY_REJECT;
}

static void l2cap_br_send_conn_rsp(struct bt_conn *conn, uint16_t scid,
				  uint16_t dcid, uint8_t ident, uint16_t result)
{
	struct net_buf *buf;
	struct bt_l2cap_conn_rsp *rsp;
	struct bt_l2cap_sig_hdr *hdr;

	buf = bt_l2cap_create_pdu(&br_sig_pool, 0);

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->code = BT_L2CAP_CONN_RSP;
	hdr->ident = ident;
	hdr->len = sys_cpu_to_le16(sizeof(*rsp));

	rsp = net_buf_add(buf, sizeof(*rsp));
	rsp->dcid = sys_cpu_to_le16(dcid);
	rsp->scid = sys_cpu_to_le16(scid);
	rsp->result = sys_cpu_to_le16(result);

	if (result == BT_L2CAP_BR_PENDING) {
		rsp->status = sys_cpu_to_le16(BT_L2CAP_CS_AUTHEN_PEND);
	} else {
		rsp->status = sys_cpu_to_le16(BT_L2CAP_CS_NO_INFO);
	}

	l2cap_send(conn, BT_L2CAP_CID_BR_SIG, buf);
}

static int l2cap_br_conn_req_reply(struct bt_l2cap_chan *chan, uint16_t result)
{
	/* Send response to connection request only when in acceptor role */
	if (!atomic_test_bit(BR_CHAN(chan)->flags, L2CAP_FLAG_CONN_ACCEPTOR)) {
		return -ESRCH;
	}

	l2cap_br_send_conn_rsp(chan->conn, BR_CHAN(chan)->tx.cid,
			       BR_CHAN(chan)->rx.cid, BR_CHAN(chan)->ident, result);
	BR_CHAN(chan)->ident = 0U;

	return 0;
}

#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
#if defined(CONFIG_BT_L2CAP_LOG_LEVEL_DBG)
void bt_l2cap_br_chan_set_state_debug(struct bt_l2cap_chan *chan,
				   bt_l2cap_chan_state_t state,
				   const char *func, int line)
{
	struct bt_l2cap_br_chan *br_chan;

	br_chan = BR_CHAN(chan);

	LOG_DBG("chan %p psm 0x%04x %s -> %s", chan, br_chan->psm,
		bt_l2cap_chan_state_str(br_chan->state), bt_l2cap_chan_state_str(state));

	/* check transitions validness */
	switch (state) {
	case BT_L2CAP_DISCONNECTED:
		/* regardless of old state always allows this state */
		break;
	case BT_L2CAP_CONNECTING:
		if (br_chan->state != BT_L2CAP_DISCONNECTED) {
			LOG_WRN("%s()%d: invalid transition", func, line);
		}
		break;
	case BT_L2CAP_CONFIG:
		if (br_chan->state != BT_L2CAP_CONNECTING) {
			LOG_WRN("%s()%d: invalid transition", func, line);
		}
		break;
	case BT_L2CAP_CONNECTED:
		if (br_chan->state != BT_L2CAP_CONFIG &&
		    br_chan->state != BT_L2CAP_CONNECTING) {
			LOG_WRN("%s()%d: invalid transition", func, line);
		}
		break;
	case BT_L2CAP_DISCONNECTING:
		if (br_chan->state != BT_L2CAP_CONFIG &&
		    br_chan->state != BT_L2CAP_CONNECTED) {
			LOG_WRN("%s()%d: invalid transition", func, line);
		}
		break;
	default:
		LOG_ERR("%s()%d: unknown (%u) state was set", func, line, state);
		return;
	}

	br_chan->state = state;
}
#else
void bt_l2cap_br_chan_set_state(struct bt_l2cap_chan *chan,
			     bt_l2cap_chan_state_t state)
{
	BR_CHAN(chan)->state = state;
}
#endif /* CONFIG_BT_L2CAP_LOG_LEVEL_DBG */
#endif /* CONFIG_BT_L2CAP_DYNAMIC_CHANNEL */

void bt_l2cap_br_chan_del(struct bt_l2cap_chan *chan)
{
	const struct bt_l2cap_chan_ops *ops = chan->ops;
	struct bt_l2cap_br_chan *br_chan = CONTAINER_OF(chan, struct bt_l2cap_br_chan, chan);

	LOG_DBG("conn %p chan %p", chan->conn, chan);

	if (!chan->conn) {
		goto destroy;
	}

	cancel_data_ready(br_chan);

	/* Remove buffers on the PDU TX queue. */
	while (chan_has_data(br_chan)) {
		struct net_buf *buf = k_fifo_get(&br_chan->_pdu_tx_queue, K_NO_WAIT);

		net_buf_unref(buf);
	}

	if (ops->disconnected) {
		ops->disconnected(chan);
	}

	chan->conn = NULL;

destroy:
#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
	/* Reset internal members of common channel */
	bt_l2cap_br_chan_set_state(chan, BT_L2CAP_DISCONNECTED);
	BR_CHAN(chan)->psm = 0U;
#endif
	if (chan->destroy) {
		chan->destroy(chan);
	}

	if (ops->released) {
		ops->released(chan);
	}
}

static void l2cap_br_conn_req(struct bt_l2cap_br *l2cap, uint8_t ident,
			      struct net_buf *buf)
{
	struct bt_conn *conn = l2cap->chan.chan.conn;
	struct bt_l2cap_chan *chan;
	struct bt_l2cap_server *server;
	struct bt_l2cap_conn_req *req = (void *)buf->data;
	uint16_t psm, scid, result;
	struct bt_l2cap_br_chan *br_chan;

	if (buf->len < sizeof(*req)) {
		LOG_ERR("Too small L2CAP conn req packet size");
		return;
	}

	psm = sys_le16_to_cpu(req->psm);
	scid = sys_le16_to_cpu(req->scid);

	LOG_DBG("psm 0x%02x scid 0x%04x", psm, scid);

	/* Check if there is a server registered */
	server = l2cap_br_server_lookup_psm(psm);
	if (!server) {
		result = BT_L2CAP_BR_ERR_PSM_NOT_SUPP;
		goto no_chan;
	}

	/*
	 * Report security violation for non SDP channel without encryption when
	 * remote supports SSP.
	 */
	if (server->sec_level != BT_SECURITY_L0 &&
	    BT_FEAT_HOST_SSP(conn->br.features) && !conn->encrypt) {
		result = BT_L2CAP_BR_ERR_SEC_BLOCK;
		goto no_chan;
	}

	if (!L2CAP_BR_CID_IS_DYN(scid)) {
		result = BT_L2CAP_BR_ERR_INVALID_SCID;
		goto no_chan;
	}

	chan = bt_l2cap_br_lookup_tx_cid(conn, scid);
	if (chan) {
		/*
		 * we have a chan here but this is due to SCID being already in
		 * use so it is not channel we are suppose to pass to
		 * l2cap_br_conn_req_reply as wrong DCID would be used
		 */
		result = BT_L2CAP_BR_ERR_SCID_IN_USE;
		goto no_chan;
	}

	/*
	 * Request server to accept the new connection and allocate the
	 * channel. If no free channels available for PSM server reply with
	 * proper result and quit since chan pointer is uninitialized then.
	 */
	if (server->accept(conn, server, &chan) < 0) {
		result = BT_L2CAP_BR_ERR_NO_RESOURCES;
		goto no_chan;
	}

	br_chan = BR_CHAN(chan);
	br_chan->required_sec_level = server->sec_level;

	l2cap_br_chan_add(conn, chan, l2cap_br_chan_destroy);
	BR_CHAN(chan)->tx.cid = scid;
	br_chan->ident = ident;
	bt_l2cap_br_chan_set_state(chan, BT_L2CAP_CONNECTING);
	atomic_set_bit(BR_CHAN(chan)->flags, L2CAP_FLAG_CONN_ACCEPTOR);

	/* Disable fragmentation of l2cap rx pdu */
	BR_CHAN(chan)->rx.mtu = MIN(BR_CHAN(chan)->rx.mtu, BT_L2CAP_RX_MTU);

	switch (l2cap_br_conn_security(chan, psm)) {
	case L2CAP_CONN_SECURITY_PENDING:
		result = BT_L2CAP_BR_PENDING;
		/* TODO: auth timeout */
		break;
	case L2CAP_CONN_SECURITY_PASSED:
		result = BT_L2CAP_BR_SUCCESS;
		break;
	case L2CAP_CONN_SECURITY_REJECT:
	default:
		result = BT_L2CAP_BR_ERR_SEC_BLOCK;
		break;
	}
	/* Reply on connection request as acceptor */
	l2cap_br_conn_req_reply(chan, result);

	if (result != BT_L2CAP_BR_SUCCESS) {
		/* Disconnect link when security rules were violated */
		if (result == BT_L2CAP_BR_ERR_SEC_BLOCK) {
			bt_conn_disconnect(conn, BT_HCI_ERR_AUTH_FAIL);
		} else if (result == BT_L2CAP_BR_PENDING) {
			/* Recover the ident when conn is pending */
			br_chan->ident = ident;
		}

		return;
	}

	bt_l2cap_br_chan_set_state(chan, BT_L2CAP_CONFIG);
	l2cap_br_conf(chan);
	return;

no_chan:
	l2cap_br_send_conn_rsp(conn, scid, 0, ident, result);
}

static void l2cap_br_conf_rsp(struct bt_l2cap_br *l2cap, uint8_t ident,
			      uint16_t len, struct net_buf *buf)
{
	struct bt_conn *conn = l2cap->chan.chan.conn;
	struct bt_l2cap_chan *chan;
	struct bt_l2cap_conf_rsp *rsp = (void *)buf->data;
	uint16_t flags, scid, result, opt_len;
	struct bt_l2cap_br_chan *br_chan;

	if (buf->len < sizeof(*rsp)) {
		LOG_ERR("Too small L2CAP conf rsp packet size");
		return;
	}

	flags = sys_le16_to_cpu(rsp->flags);
	scid = sys_le16_to_cpu(rsp->scid);
	result = sys_le16_to_cpu(rsp->result);
	opt_len = len - sizeof(*rsp);

	LOG_DBG("scid 0x%04x flags 0x%02x result 0x%02x len %u", scid, flags, result, opt_len);

	chan = bt_l2cap_br_lookup_rx_cid(conn, scid);
	if (!chan) {
		LOG_ERR("channel mismatch!");
		return;
	}

	br_chan = BR_CHAN(chan);

	/* Release RTX work since got the response */
	k_work_cancel_delayable(&br_chan->rtx_work);

	/*
	 * TODO: handle other results than success and parse response data if
	 * available
	 */
	switch (result) {
	case BT_L2CAP_CONF_SUCCESS:
		atomic_set_bit(br_chan->flags, L2CAP_FLAG_CONN_LCONF_DONE);

		if (br_chan->state == BT_L2CAP_CONFIG &&
		    atomic_test_bit(br_chan->flags,
				    L2CAP_FLAG_CONN_RCONF_DONE)) {
			LOG_DBG("scid 0x%04x rx MTU %u dcid 0x%04x tx MTU %u", br_chan->rx.cid,
				br_chan->rx.mtu, br_chan->tx.cid, br_chan->tx.mtu);

			bt_l2cap_br_chan_set_state(chan, BT_L2CAP_CONNECTED);
			if (chan->ops && chan->ops->connected) {
				chan->ops->connected(chan);
			}
		}
		break;
	default:
		/* currently disconnect channel on non success result */
		bt_l2cap_chan_disconnect(chan);
		break;
	}
}

static int bt_l2cap_br_allocate_psm(uint16_t *psm)
{
	/* DYN_END is UINT16_MAX, so to be able to do a psm <= DYN_END comparison
	 * we need to use uint32_t as the type.
	 */
	static uint32_t allocated_psm = L2CAP_BR_PSM_DYN_START;

	if (allocated_psm < L2CAP_BR_PSM_DYN_END) {
		allocated_psm = allocated_psm + 1;
	} else {
		goto failed;
	}

	for (; allocated_psm <= L2CAP_BR_PSM_DYN_END; allocated_psm++) {
		/* Bluetooth Core Specification Version 6.0 | Vol 3, Part A, section 4.2
		 *
		 * The PSM field is at least two octets in length. All PSM values shall have
		 * the least significant bit of the most significant octet equal to 0 and the
		 * least significant bit of all other octets equal to 1.
		 */
		if ((allocated_psm & 0x0101) != 0x0001) {
			continue;
		}

		if (l2cap_br_server_lookup_psm((uint16_t)allocated_psm)) {
			LOG_DBG("PSM 0x%04x has been used", allocated_psm);
			continue;
		}

		LOG_DBG("Allocated PSM 0x%04x for new server", allocated_psm);
		*psm = (uint16_t)allocated_psm;
		return 0;
	}

failed:
	LOG_WRN("No free dynamic PSMs available");
	return -EADDRNOTAVAIL;
}

int bt_l2cap_br_server_register(struct bt_l2cap_server *server)
{
	int err;

	if (!server->accept) {
		return -EINVAL;
	}

	if (server->sec_level > BT_SECURITY_L4) {
		return -EINVAL;
	} else if (server->sec_level == BT_SECURITY_L0 &&
		   server->psm != L2CAP_BR_PSM_SDP) {
		server->sec_level = BT_SECURITY_L1;
	}

	if (!server->psm) {
		err = bt_l2cap_br_allocate_psm(&server->psm);
		if (err) {
			return err;
		}
	} else {
		/* PSM must be odd and lsb of upper byte must be 0 */
		if ((server->psm & 0x0101) != 0x0001) {
			LOG_WRN("PSM must be odd and lsb of upper byte must be 0");
			return -EINVAL;
		}

		/* Check if given PSM is already in use */
		if (l2cap_br_server_lookup_psm(server->psm)) {
			LOG_WRN("PSM already registered");
			return -EADDRINUSE;
		}
	}

	LOG_DBG("PSM 0x%04x", server->psm);

	sys_slist_append(&br_servers, &server->node);

	return 0;
}

static void l2cap_br_send_reject(struct bt_conn *conn, uint8_t ident,
				 uint16_t reason, void *data, uint8_t data_len)
{
	struct bt_l2cap_cmd_reject *rej;
	struct bt_l2cap_sig_hdr *hdr;
	struct net_buf *buf;

	buf = bt_l2cap_create_pdu(&br_sig_pool, 0);

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->code = BT_L2CAP_CMD_REJECT;
	hdr->ident = ident;
	hdr->len = sys_cpu_to_le16(sizeof(*rej) + data_len);

	rej = net_buf_add(buf, sizeof(*rej));
	rej->reason = sys_cpu_to_le16(reason);

	/*
	 * optional data if available must be already in little-endian format
	 * made by caller.and be compliant with Core 4.2 [Vol 3, Part A, 4.1,
	 * table 4.4]
	 */
	if (data) {
		net_buf_add_mem(buf, data, data_len);
	}

	l2cap_send(conn, BT_L2CAP_CID_BR_SIG, buf);
}

static uint16_t l2cap_br_conf_opt_mtu(struct bt_l2cap_chan *chan,
				   struct net_buf *buf, size_t len)
{
	uint16_t mtu, result = BT_L2CAP_CONF_SUCCESS;
	struct bt_l2cap_conf_opt_mtu *opt_mtu;

	/* Core 4.2 [Vol 3, Part A, 5.1] MTU payload length */
	if (len != sizeof(*opt_mtu)) {
		LOG_ERR("tx MTU length %zu invalid", len);
		result = BT_L2CAP_CONF_REJECT;
		goto done;
	}

	opt_mtu = (struct bt_l2cap_conf_opt_mtu *)buf->data;

	mtu = sys_le16_to_cpu(opt_mtu->mtu);
	if (mtu < L2CAP_BR_MIN_MTU) {
		result = BT_L2CAP_CONF_UNACCEPT;
		BR_CHAN(chan)->tx.mtu = L2CAP_BR_MIN_MTU;
		opt_mtu->mtu = sys_cpu_to_le16(L2CAP_BR_MIN_MTU);
		LOG_DBG("tx MTU %u invalid", mtu);
		goto done;
	}

	BR_CHAN(chan)->tx.mtu = mtu;
	LOG_DBG("tx MTU %u", mtu);
done:
	return result;
}

static uint16_t l2cap_br_conf_opt_flush_timeout(struct bt_l2cap_chan *chan,
						struct net_buf *buf, size_t len)
{
	uint16_t result = BT_L2CAP_CONF_SUCCESS;
	struct bt_l2cap_conf_opt_flush_timeout *opt_to;

	if (len != sizeof(*opt_to)) {
		LOG_ERR("qos frame length %zu invalid", len);
		result = BT_L2CAP_CONF_REJECT;
		goto done;
	}

	opt_to = (struct bt_l2cap_conf_opt_flush_timeout *)buf->data;

	LOG_DBG("Flush timeout %u", opt_to->timeout);

	opt_to->timeout = sys_cpu_to_le16(0xFFFF);
	result = BT_L2CAP_CONF_UNACCEPT;
done:
	return result;
}

static uint16_t l2cap_br_conf_opt_qos(struct bt_l2cap_chan *chan,
				      struct net_buf *buf, size_t len)
{
	uint16_t result = BT_L2CAP_CONF_SUCCESS;
	struct bt_l2cap_conf_opt_qos *opt_qos;

	if (len != sizeof(*opt_qos)) {
		LOG_ERR("qos frame length %zu invalid", len);
		result = BT_L2CAP_CONF_REJECT;
		goto done;
	}

	opt_qos = (struct bt_l2cap_conf_opt_qos *)buf->data;

	LOG_DBG("QOS Type %u", opt_qos->service_type);

	if (opt_qos->service_type == BT_L2CAP_QOS_TYPE_GUARANTEED) {
		/* Set to default value */
		result = BT_L2CAP_CONF_UNACCEPT;
		opt_qos->flags = 0x00;
		/* do not care */
		opt_qos->token_rate = sys_cpu_to_le32(0x00000000);
		/* no token bucket is needed */
		opt_qos->token_bucket_size = sys_cpu_to_le32(0x00000000);
		/* do not care */
		opt_qos->peak_bandwidth = sys_cpu_to_le32(0x00000000);
		/* do not care */
		opt_qos->latency = sys_cpu_to_le32(0xFFFFFFFF);
		/* do not care */
		opt_qos->delay_variation = sys_cpu_to_le32(0xFFFFFFFF);
	}

done:
	return result;
}

static uint16_t l2cap_br_conf_opt_ret_fc(struct bt_l2cap_chan *chan,
					 struct net_buf *buf, size_t len)
{
	uint16_t result = BT_L2CAP_CONF_SUCCESS;
	struct bt_l2cap_conf_opt_ret_fc *opt_ret_fc;

	if (len != sizeof(*opt_ret_fc)) {
		LOG_ERR("ret_fc frame length %zu invalid", len);
		result = BT_L2CAP_CONF_REJECT;
		goto done;
	}

	opt_ret_fc = (struct bt_l2cap_conf_opt_ret_fc *)buf->data;

	LOG_DBG("ret_fc mode %u", opt_ret_fc->mode);

	if (opt_ret_fc->mode != BT_L2CAP_RET_FC_MODE_BASIC) {
		/* Set to default value */
		result = BT_L2CAP_CONF_UNACCEPT;
		opt_ret_fc->mode = BT_L2CAP_RET_FC_MODE_BASIC;
	}

done:
	return result;
}

static uint16_t l2cap_br_conf_opt_fcs(struct bt_l2cap_chan *chan,
				      struct net_buf *buf, size_t len)
{
	uint16_t result = BT_L2CAP_CONF_SUCCESS;
	struct bt_l2cap_conf_opt_fcs *opt_fcs;

	if (len != sizeof(*opt_fcs)) {
		LOG_ERR("fcs frame length %zu invalid", len);
		result = BT_L2CAP_CONF_REJECT;
		goto done;
	}

	opt_fcs = (struct bt_l2cap_conf_opt_fcs *)buf->data;

	LOG_DBG("FCS type %u", opt_fcs->type);

	if (opt_fcs->type != BT_L2CAP_FCS_TYPE_NO) {
		/* Set to default value */
		result = BT_L2CAP_CONF_UNACCEPT;
		opt_fcs->type = BT_L2CAP_FCS_TYPE_NO;
	}

done:
	return result;
}

static void l2cap_br_conf_req(struct bt_l2cap_br *l2cap, uint8_t ident,
			      uint16_t len, struct net_buf *buf)
{
	struct bt_conn *conn = l2cap->chan.chan.conn;
	struct bt_l2cap_chan *chan;
	struct bt_l2cap_conf_req *req;
	struct bt_l2cap_sig_hdr *hdr;
	struct bt_l2cap_conf_rsp *rsp;
	struct bt_l2cap_conf_opt *opt = NULL;
	uint16_t flags, dcid, opt_len, hint, result = BT_L2CAP_CONF_SUCCESS;

	if (buf->len < sizeof(*req)) {
		LOG_ERR("Too small L2CAP conf req packet size");
		return;
	}

	req = net_buf_pull_mem(buf, sizeof(*req));
	flags = sys_le16_to_cpu(req->flags);
	dcid = sys_le16_to_cpu(req->dcid);
	opt_len = len - sizeof(*req);

	LOG_DBG("dcid 0x%04x flags 0x%02x len %u", dcid, flags, opt_len);

	chan = bt_l2cap_br_lookup_rx_cid(conn, dcid);
	if (!chan) {
		LOG_ERR("rx channel mismatch!");
		struct bt_l2cap_cmd_reject_cid_data data = {
			.scid = req->dcid,
			.dcid = 0,
		};

		l2cap_br_send_reject(conn, ident, BT_L2CAP_REJ_INVALID_CID,
				     &data, sizeof(data));
		return;
	}

	if (!opt_len) {
		LOG_DBG("tx default MTU %u", L2CAP_BR_DEFAULT_MTU);
		BR_CHAN(chan)->tx.mtu = L2CAP_BR_DEFAULT_MTU;
		goto send_rsp;
	}

	while (buf->len >= sizeof(*opt)) {
		opt = net_buf_pull_mem(buf, sizeof(*opt));

		/* make sure opt object can get safe dereference in iteration */
		if (buf->len < opt->len) {
			LOG_ERR("Received too short option data");
			result = BT_L2CAP_CONF_REJECT;
			break;
		}

		hint = opt->type & BT_L2CAP_CONF_HINT;

		switch (opt->type & BT_L2CAP_CONF_MASK) {
		case BT_L2CAP_CONF_OPT_MTU:
			/* getting MTU modifies buf internals */
			result = l2cap_br_conf_opt_mtu(chan, buf, opt->len);
			/*
			 * MTU is done. For now bailout the loop but later on
			 * there can be a need to continue checking next options
			 * that are after MTU value and then goto is not proper
			 * way out here.
			 */
			goto send_rsp;
		case BT_L2CAP_CONF_OPT_FLUSH_TIMEOUT:
			result = l2cap_br_conf_opt_flush_timeout(chan, buf, opt->len);
			if (result != BT_L2CAP_CONF_SUCCESS) {
				goto send_rsp;
			}
			break;
		case BT_L2CAP_CONF_OPT_QOS:
			result = l2cap_br_conf_opt_qos(chan, buf, opt->len);
			if (result != BT_L2CAP_CONF_SUCCESS) {
				goto send_rsp;
			}
			break;
		case BT_L2CAP_CONF_OPT_RET_FC:
			result = l2cap_br_conf_opt_ret_fc(chan, buf, opt->len);
			if (result != BT_L2CAP_CONF_SUCCESS) {
				goto send_rsp;
			}
			break;
		case BT_L2CAP_CONF_OPT_FCS:
			result = l2cap_br_conf_opt_fcs(chan, buf, opt->len);
			if (result != BT_L2CAP_CONF_SUCCESS) {
				goto send_rsp;
			}
			break;
		case BT_L2CAP_CONF_OPT_EXT_FLOW_SPEC:
			__fallthrough;
		case BT_L2CAP_CONF_OPT_EXT_WIN_SIZE:
			result = BT_L2CAP_CONF_REJECT;
			goto send_rsp;
		default:
			if (!hint) {
				LOG_DBG("option %u not handled", opt->type);
				result = BT_L2CAP_CONF_UNKNOWN_OPT;
				goto send_rsp;
			}
			break;
		}

		/* Update buffer to point at next option */
		net_buf_pull(buf, opt->len);
	}

send_rsp:
	buf = bt_l2cap_create_pdu(&br_sig_pool, 0);

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->code = BT_L2CAP_CONF_RSP;
	hdr->ident = ident;
	rsp = net_buf_add(buf, sizeof(*rsp));
	(void)memset(rsp, 0, sizeof(*rsp));

	rsp->result = sys_cpu_to_le16(result);
	rsp->scid = sys_cpu_to_le16(BR_CHAN(chan)->tx.cid);

	/*
	 * Core 5.4, Vol 3, Part A, section 4.5.
	 * When used in the L2CAP_CONFIGURATION_RSP packet,
	 * the continuation flag shall be set to one if the
	 * flag is set to one in the Request, except for
	 * those error conditions more appropriate for an
	 * L2CAP_COMMAND_REJECT_RSP packet.
	 */
	rsp->flags = sys_cpu_to_le16(flags & BT_L2CAP_CONF_FLAGS_MASK);

	/*
	 * TODO: If options other than MTU became meaningful then processing
	 * the options chain need to be modified and taken into account when
	 * sending back to peer.
	 */
	if ((result == BT_L2CAP_CONF_UNKNOWN_OPT) || (result == BT_L2CAP_CONF_UNACCEPT)) {
		if (opt) {
			l2cap_br_conf_add_opt(buf, opt);
		}
	}

	hdr->len = sys_cpu_to_le16(buf->len - sizeof(*hdr));

	l2cap_send(conn, BT_L2CAP_CID_BR_SIG, buf);

	if (result != BT_L2CAP_CONF_SUCCESS) {
		return;
	}

	atomic_set_bit(BR_CHAN(chan)->flags, L2CAP_FLAG_CONN_RCONF_DONE);

	if (atomic_test_bit(BR_CHAN(chan)->flags, L2CAP_FLAG_CONN_LCONF_DONE) &&
	    BR_CHAN(chan)->state == BT_L2CAP_CONFIG) {
		LOG_DBG("scid 0x%04x rx MTU %u dcid 0x%04x tx MTU %u", BR_CHAN(chan)->rx.cid,
			BR_CHAN(chan)->rx.mtu, BR_CHAN(chan)->tx.cid, BR_CHAN(chan)->tx.mtu);

		bt_l2cap_br_chan_set_state(chan, BT_L2CAP_CONNECTED);
		if (chan->ops && chan->ops->connected) {
			chan->ops->connected(chan);
		}
	}
}

static struct bt_l2cap_br_chan *l2cap_br_remove_tx_cid(struct bt_conn *conn,
						       uint16_t cid)
{
	struct bt_l2cap_chan *chan;
	sys_snode_t *prev = NULL;

	/* Protect fixed channels against accidental removal */
	if (!L2CAP_BR_CID_IS_DYN(cid)) {
		return NULL;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&conn->channels, chan, node) {
		if (BR_CHAN(chan)->tx.cid == cid) {
			sys_slist_remove(&conn->channels, prev, &chan->node);
			return BR_CHAN(chan);
		}

		prev = &chan->node;
	}

	return NULL;
}

static void l2cap_br_disconn_req(struct bt_l2cap_br *l2cap, uint8_t ident,
				 struct net_buf *buf)
{
	struct bt_conn *conn = l2cap->chan.chan.conn;
	struct bt_l2cap_br_chan *chan;
	struct bt_l2cap_disconn_req *req = (void *)buf->data;
	struct bt_l2cap_disconn_rsp *rsp;
	struct bt_l2cap_sig_hdr *hdr;
	uint16_t scid, dcid;

	if (buf->len < sizeof(*req)) {
		LOG_ERR("Too small disconn req packet size");
		return;
	}

	dcid = sys_le16_to_cpu(req->dcid);
	scid = sys_le16_to_cpu(req->scid);

	LOG_DBG("scid 0x%04x dcid 0x%04x", dcid, scid);

	chan = l2cap_br_remove_tx_cid(conn, scid);
	if (!chan) {
		struct bt_l2cap_cmd_reject_cid_data data;

		data.scid = req->scid;
		data.dcid = req->dcid;
		l2cap_br_send_reject(conn, ident, BT_L2CAP_REJ_INVALID_CID,
				     &data, sizeof(data));
		return;
	}

	buf = bt_l2cap_create_pdu(&br_sig_pool, 0);

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->code = BT_L2CAP_DISCONN_RSP;
	hdr->ident = ident;
	hdr->len = sys_cpu_to_le16(sizeof(*rsp));

	rsp = net_buf_add(buf, sizeof(*rsp));
	rsp->dcid = sys_cpu_to_le16(chan->rx.cid);
	rsp->scid = sys_cpu_to_le16(chan->tx.cid);

	bt_l2cap_br_chan_del(&chan->chan);

	l2cap_send(conn, BT_L2CAP_CID_BR_SIG, buf);
}

static void l2cap_br_connected(struct bt_l2cap_chan *chan)
{
	LOG_DBG("ch %p cid 0x%04x", BR_CHAN(chan), BR_CHAN(chan)->rx.cid);
}

static void l2cap_br_disconnected(struct bt_l2cap_chan *chan)
{
	struct bt_l2cap_br_chan *br_chan = BR_CHAN(chan);

	LOG_DBG("ch %p cid 0x%04x", br_chan, br_chan->rx.cid);

	if (atomic_test_and_clear_bit(br_chan->flags,
				      L2CAP_FLAG_SIG_INFO_PENDING)) {
		/* Cancel RTX work on signal channel.
		 * Disconnected callback is always called from system workqueue
		 * so this should always succeed.
		 */
		(void)k_work_cancel_delayable(&br_chan->rtx_work);
	}
}

int bt_l2cap_br_chan_disconnect(struct bt_l2cap_chan *chan)
{
	struct bt_conn *conn = chan->conn;
	struct net_buf *buf;
	struct bt_l2cap_disconn_req *req;
	struct bt_l2cap_sig_hdr *hdr;
	struct bt_l2cap_br_chan *br_chan;

	if (!conn) {
		return -ENOTCONN;
	}

	br_chan = BR_CHAN(chan);

	if (br_chan->state == BT_L2CAP_DISCONNECTING) {
		return -EALREADY;
	}

	LOG_DBG("chan %p scid 0x%04x dcid 0x%04x", chan, br_chan->rx.cid, br_chan->tx.cid);

	buf = bt_l2cap_create_pdu(&br_sig_pool, 0);

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->code = BT_L2CAP_DISCONN_REQ;
	hdr->ident = l2cap_br_get_ident();
	hdr->len = sys_cpu_to_le16(sizeof(*req));

	req = net_buf_add(buf, sizeof(*req));
	req->dcid = sys_cpu_to_le16(br_chan->tx.cid);
	req->scid = sys_cpu_to_le16(br_chan->rx.cid);

	l2cap_br_chan_send_req(br_chan, buf, L2CAP_BR_DISCONN_TIMEOUT);
	bt_l2cap_br_chan_set_state(chan, BT_L2CAP_DISCONNECTING);

	return 0;
}

static void l2cap_br_disconn_rsp(struct bt_l2cap_br *l2cap, uint8_t ident,
				 struct net_buf *buf)
{
	struct bt_conn *conn = l2cap->chan.chan.conn;
	struct bt_l2cap_br_chan *chan;
	struct bt_l2cap_disconn_rsp *rsp = (void *)buf->data;
	uint16_t dcid, scid;

	if (buf->len < sizeof(*rsp)) {
		LOG_ERR("Too small disconn rsp packet size");
		return;
	}

	dcid = sys_le16_to_cpu(rsp->dcid);
	scid = sys_le16_to_cpu(rsp->scid);

	LOG_DBG("dcid 0x%04x scid 0x%04x", dcid, scid);

	chan = l2cap_br_remove_tx_cid(conn, dcid);
	if (!chan) {
		LOG_WRN("No dcid 0x%04x channel found", dcid);
		return;
	}

	bt_l2cap_br_chan_del(&chan->chan);
}

int bt_l2cap_br_chan_connect(struct bt_conn *conn, struct bt_l2cap_chan *chan,
			     uint16_t psm)
{
	struct net_buf *buf;
	struct bt_l2cap_sig_hdr *hdr;
	struct bt_l2cap_conn_req *req;
	struct bt_l2cap_br_chan *br_chan = BR_CHAN(chan);

	if (!psm) {
		return -EINVAL;
	}

	if (br_chan->psm) {
		return -EEXIST;
	}

	/* PSM must be odd and lsb of upper byte must be 0 */
	if ((psm & 0x0101) != 0x0001) {
		return -EINVAL;
	}

	if (br_chan->required_sec_level > BT_SECURITY_L4) {
		return -EINVAL;
	} else if (br_chan->required_sec_level == BT_SECURITY_L0 &&
		   psm != L2CAP_BR_PSM_SDP) {
		br_chan->required_sec_level = BT_SECURITY_L1;
	}

	switch (br_chan->state) {
	case BT_L2CAP_CONNECTED:
		/* Already connected */
		return -EISCONN;
	case BT_L2CAP_DISCONNECTED:
		/* Can connect */
		break;
	case BT_L2CAP_CONFIG:
	case BT_L2CAP_DISCONNECTING:
	default:
		/* Bad context */
		return -EBUSY;
	}

	if (!l2cap_br_chan_add(conn, chan, l2cap_br_chan_destroy)) {
		return -ENOMEM;
	}

	br_chan->psm = psm;
	bt_l2cap_br_chan_set_state(chan, BT_L2CAP_CONNECTING);
	atomic_set_bit(BR_CHAN(chan)->flags, L2CAP_FLAG_CONN_PENDING);

	switch (l2cap_br_conn_security(chan, psm)) {
	case L2CAP_CONN_SECURITY_PENDING:
		/*
		 * Authentication was triggered, wait with sending request on
		 * connection security changed callback context.
		 */
		 return 0;
	case L2CAP_CONN_SECURITY_PASSED:
		break;
	case L2CAP_CONN_SECURITY_REJECT:
	default:
		l2cap_br_chan_cleanup(chan);
		return -EIO;
	}

	buf = bt_l2cap_create_pdu(&br_sig_pool, 0);

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->code = BT_L2CAP_CONN_REQ;
	hdr->ident = l2cap_br_get_ident();
	hdr->len = sys_cpu_to_le16(sizeof(*req));

	req = net_buf_add(buf, sizeof(*req));
	req->psm = sys_cpu_to_le16(psm);
	req->scid = sys_cpu_to_le16(BR_CHAN(chan)->rx.cid);

	l2cap_br_chan_send_req(BR_CHAN(chan), buf, L2CAP_BR_CONN_TIMEOUT);

	return 0;
}

static void l2cap_br_conn_rsp(struct bt_l2cap_br *l2cap, uint8_t ident,
			      struct net_buf *buf)
{
	struct bt_conn *conn = l2cap->chan.chan.conn;
	struct bt_l2cap_chan *chan;
	struct bt_l2cap_conn_rsp *rsp = (void *)buf->data;
	uint16_t dcid, scid, result, status;
	struct bt_l2cap_br_chan *br_chan;

	if (buf->len < sizeof(*rsp)) {
		LOG_ERR("Too small L2CAP conn rsp packet size");
		return;
	}

	dcid = sys_le16_to_cpu(rsp->dcid);
	scid = sys_le16_to_cpu(rsp->scid);
	result = sys_le16_to_cpu(rsp->result);
	status = sys_le16_to_cpu(rsp->status);

	LOG_DBG("dcid 0x%04x scid 0x%04x result %u status %u", dcid, scid, result, status);

	chan = bt_l2cap_br_lookup_rx_cid(conn, scid);
	if (!chan) {
		LOG_ERR("No scid 0x%04x channel found", scid);
		return;
	}

	br_chan = BR_CHAN(chan);

	/* Release RTX work since got the response */
	k_work_cancel_delayable(&br_chan->rtx_work);

	if (br_chan->state != BT_L2CAP_CONNECTING) {
		LOG_DBG("Invalid channel %p state %s", chan,
			bt_l2cap_chan_state_str(br_chan->state));
		return;
	}

	switch (result) {
	case BT_L2CAP_BR_SUCCESS:
		br_chan->ident = 0U;
		BR_CHAN(chan)->tx.cid = dcid;
		l2cap_br_conf(chan);
		bt_l2cap_br_chan_set_state(chan, BT_L2CAP_CONFIG);
		atomic_clear_bit(BR_CHAN(chan)->flags, L2CAP_FLAG_CONN_PENDING);
		break;
	case BT_L2CAP_BR_PENDING:
		k_work_reschedule(&br_chan->rtx_work, L2CAP_BR_CONN_TIMEOUT);
		break;
	default:
		l2cap_br_chan_cleanup(chan);
		break;
	}
}

int bt_l2cap_br_chan_send_cb(struct bt_l2cap_chan *chan, struct net_buf *buf, bt_conn_tx_cb_t cb,
			     void *user_data)
{
	struct bt_l2cap_br_chan *br_chan;

	if (!buf || !chan) {
		return -EINVAL;
	}

	br_chan = BR_CHAN(chan);

	LOG_DBG("chan %p buf %p len %zu", chan, buf, net_buf_frags_len(buf));

	if (!chan->conn || chan->conn->state != BT_CONN_CONNECTED) {
		return -ENOTCONN;
	}

	if (atomic_test_bit(chan->status, BT_L2CAP_STATUS_SHUTDOWN)) {
		return -ESHUTDOWN;
	}

	if (buf->len > br_chan->tx.mtu) {
		return -EMSGSIZE;
	}

	return bt_l2cap_br_send_cb(br_chan->chan.conn, br_chan->tx.cid, buf, cb, user_data);
}

int bt_l2cap_br_chan_send(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	return bt_l2cap_br_chan_send_cb(chan, buf, NULL, NULL);
}

static void l2cap_br_sig_handle(struct bt_l2cap_br *l2cap, struct bt_l2cap_sig_hdr *hdr,
				      struct net_buf *buf)
{
	uint16_t len;
	struct net_buf_simple_state state;

	len = sys_le16_to_cpu(hdr->len);

	net_buf_simple_save(&buf->b, &state);

	switch (hdr->code) {
	case BT_L2CAP_INFO_RSP:
		l2cap_br_info_rsp(l2cap, hdr->ident, buf);
		break;
	case BT_L2CAP_INFO_REQ:
		l2cap_br_info_req(l2cap, hdr->ident, buf);
		break;
	case BT_L2CAP_DISCONN_REQ:
		l2cap_br_disconn_req(l2cap, hdr->ident, buf);
		break;
	case BT_L2CAP_CONN_REQ:
		l2cap_br_conn_req(l2cap, hdr->ident, buf);
		break;
	case BT_L2CAP_CONF_RSP:
		l2cap_br_conf_rsp(l2cap, hdr->ident, len, buf);
		break;
	case BT_L2CAP_CONF_REQ:
		l2cap_br_conf_req(l2cap, hdr->ident, len, buf);
		break;
	case BT_L2CAP_DISCONN_RSP:
		l2cap_br_disconn_rsp(l2cap, hdr->ident, buf);
		break;
	case BT_L2CAP_CONN_RSP:
		l2cap_br_conn_rsp(l2cap, hdr->ident, buf);
		break;
	default:
		LOG_WRN("Unknown/Unsupported L2CAP PDU code 0x%02x", hdr->code);
		l2cap_br_send_reject(l2cap->chan.chan.conn, hdr->ident,
					BT_L2CAP_REJ_NOT_UNDERSTOOD, NULL, 0);
		break;
	}

	net_buf_simple_restore(&buf->b, &state);
	(void)net_buf_pull_mem(buf, len);
}

static int l2cap_br_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_l2cap_br *l2cap = CONTAINER_OF(chan, struct bt_l2cap_br, chan.chan);
	struct bt_l2cap_sig_hdr *hdr;
	uint16_t len;

	while (buf->len > 0) {
		if (buf->len < sizeof(*hdr)) {
			LOG_ERR("Too small L2CAP signaling PDU");
			return 0;
		}

		hdr = net_buf_pull_mem(buf, sizeof(*hdr));
		len = sys_le16_to_cpu(hdr->len);

		LOG_DBG("Signaling code 0x%02x ident %u len %u", hdr->code, hdr->ident, len);

		if (buf->len < len) {
			LOG_ERR("L2CAP length is short (%u < %u)", buf->len, len);
			return 0;
		}

		if (!hdr->ident) {
			LOG_ERR("Invalid ident value in L2CAP PDU");
			(void)net_buf_pull_mem(buf, len);
			continue;
		}

		l2cap_br_sig_handle(l2cap, hdr, buf);
	}

	return 0;
}

static void l2cap_br_conn_pend(struct bt_l2cap_chan *chan, uint8_t status)
{
	struct net_buf *buf;
	struct bt_l2cap_sig_hdr *hdr;
	struct bt_l2cap_conn_req *req;

	if (BR_CHAN(chan)->state != BT_L2CAP_CONNECTING) {
		return;
	}

	LOG_DBG("chan %p status 0x%02x encr 0x%02x", chan, status, chan->conn->encrypt);

	if (status) {
		/*
		 * Security procedure status is non-zero so respond with
		 * security violation only as channel acceptor.
		 */
		l2cap_br_conn_req_reply(chan, BT_L2CAP_BR_ERR_SEC_BLOCK);

		/* Release channel allocated to outgoing connection request */
		if (atomic_test_bit(BR_CHAN(chan)->flags,
				    L2CAP_FLAG_CONN_PENDING)) {
			l2cap_br_chan_cleanup(chan);
		}

		return;
	}

	if (!chan->conn->encrypt) {
		return;
	}

	/*
	 * For incoming connection state send confirming outstanding
	 * response and initiate configuration request.
	 */
	if (l2cap_br_conn_req_reply(chan, BT_L2CAP_BR_SUCCESS) == 0) {
		bt_l2cap_br_chan_set_state(chan, BT_L2CAP_CONFIG);
		/*
		 * Initialize config request since remote needs to know
		 * local MTU segmentation.
		 */
		l2cap_br_conf(chan);
	} else if (atomic_test_and_clear_bit(BR_CHAN(chan)->flags,
					     L2CAP_FLAG_CONN_PENDING)) {
		buf = bt_l2cap_create_pdu(&br_sig_pool, 0);

		hdr = net_buf_add(buf, sizeof(*hdr));
		hdr->code = BT_L2CAP_CONN_REQ;
		hdr->ident = l2cap_br_get_ident();
		hdr->len = sys_cpu_to_le16(sizeof(*req));

		req = net_buf_add(buf, sizeof(*req));
		req->psm = sys_cpu_to_le16(BR_CHAN(chan)->psm);
		req->scid = sys_cpu_to_le16(BR_CHAN(chan)->rx.cid);

		l2cap_br_chan_send_req(BR_CHAN(chan), buf,
				       L2CAP_BR_CONN_TIMEOUT);
	}
}

void l2cap_br_encrypt_change(struct bt_conn *conn, uint8_t hci_status)
{
	struct bt_l2cap_chan *chan;

	SYS_SLIST_FOR_EACH_CONTAINER(&conn->channels, chan, node) {
		l2cap_br_conn_pend(chan, hci_status);

		if (chan->ops && chan->ops->encrypt_change) {
			chan->ops->encrypt_change(chan, hci_status);
		}
	}
}

static void check_fixed_channel(struct bt_l2cap_chan *chan)
{
	struct bt_l2cap_br_chan *br_chan = BR_CHAN(chan);

	if (br_chan->rx.cid < L2CAP_BR_CID_DYN_START) {
		connect_fixed_channel(br_chan);
	}
}

void bt_l2cap_br_recv(struct bt_conn *conn, struct net_buf *buf)
{
	struct bt_l2cap_hdr *hdr;
	struct bt_l2cap_chan *chan;
	uint16_t cid;

	if (buf->len < sizeof(*hdr)) {
		LOG_ERR("Too small L2CAP PDU received");
		net_buf_unref(buf);
		return;
	}

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	cid = sys_le16_to_cpu(hdr->cid);

	chan = bt_l2cap_br_lookup_rx_cid(conn, cid);
	if (!chan) {
		LOG_WRN("Ignoring data for unknown channel ID 0x%04x", cid);
		net_buf_unref(buf);
		return;
	}

	/*
	 * if data was received for fixed channel before Information
	 * Response we connect channel here.
	 */
	check_fixed_channel(chan);

	chan->ops->recv(chan, buf);
	net_buf_unref(buf);
}

static int l2cap_br_accept(struct bt_conn *conn, struct bt_l2cap_chan **chan)
{
	int i;
	static const struct bt_l2cap_chan_ops ops = {
		.connected = l2cap_br_connected,
		.disconnected = l2cap_br_disconnected,
		.recv = l2cap_br_recv,
	};

	LOG_DBG("conn %p handle %u", conn, conn->handle);

	for (i = 0; i < ARRAY_SIZE(bt_l2cap_br_pool); i++) {
		struct bt_l2cap_br *l2cap = &bt_l2cap_br_pool[i];

		if (l2cap->chan.chan.conn) {
			continue;
		}

		l2cap->chan.chan.ops = &ops;
		*chan = &l2cap->chan.chan;
		atomic_set(l2cap->chan.flags, 0);
		return 0;
	}

	LOG_ERR("No available L2CAP context for conn %p", conn);

	return -ENOMEM;
}

BT_L2CAP_BR_CHANNEL_DEFINE(br_fixed_chan, BT_L2CAP_CID_BR_SIG, l2cap_br_accept);

void bt_l2cap_br_init(void)
{
	sys_slist_init(&br_servers);

	if (IS_ENABLED(CONFIG_BT_RFCOMM)) {
		bt_rfcomm_init();
	}

	if (IS_ENABLED(CONFIG_BT_AVDTP)) {
		bt_avdtp_init();
	}

	if (IS_ENABLED(CONFIG_BT_AVCTP)) {
		bt_avctp_init();
	}

	bt_sdp_init();

	if (IS_ENABLED(CONFIG_BT_A2DP)) {
		bt_a2dp_init();
	}

	if (IS_ENABLED(CONFIG_BT_AVRCP)) {
		bt_avrcp_init();
	}
}
