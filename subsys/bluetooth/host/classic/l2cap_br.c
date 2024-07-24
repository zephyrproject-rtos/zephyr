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
#include <zephyr/sys/crc.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/drivers/bluetooth/hci_driver.h>

#include "host/buf_view.h"
#include "host/hci_core.h"
#include "host/conn_internal.h"
#include "l2cap_br_internal.h"
#include "avdtp_internal.h"
#include "a2dp_internal.h"
#include "rfcomm_internal.h"
#include "sdp_internal.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_l2cap_br, CONFIG_BT_L2CAP_LOG_LEVEL);

#if defined(CONFIG_BT_L2CAP_RET) || defined(CONFIG_BT_L2CAP_FC) || \
	defined(CONFIG_BT_L2CAP_ENH_RET) || defined(CONFIG_BT_L2CAP_STREAM)
#define L2CAP_BR_RET_FC_ENABLE 1
#else
#define L2CAP_BR_RET_FC_ENABLE 0
#endif

#define BR_CHAN_RTX(_w) CONTAINER_OF(k_work_delayable_from_work(_w), \
				     struct bt_l2cap_br_chan, rtx_work)

#define BR_CHAN_RET(_w) CONTAINER_OF(k_work_delayable_from_work(_w), \
				     struct bt_l2cap_br_chan, ret_work)

#define BR_CHAN_MONITOR(_w) CONTAINER_OF(k_work_delayable_from_work(_w), \
				     struct bt_l2cap_br_chan, monitor_work)

#define L2CAP_BR_PSM_START	0x0001
#define L2CAP_BR_PSM_END	0xffff

#define L2CAP_BR_CID_DYN_START	0x0040
#define L2CAP_BR_CID_DYN_END	0xffff
#define L2CAP_BR_CID_IS_DYN(_cid) \
	(_cid >= L2CAP_BR_CID_DYN_START && _cid <= L2CAP_BR_CID_DYN_END)

#define L2CAP_BR_MIN_MTU	48
#define L2CAP_BR_DEFAULT_MTU	672

#define L2CAP_BR_PSM_SDP	0x0001

#define L2CAP_BR_S_FRAME_FLAG_MASK 0xFFFFFF00U
#define L2CAP_BR_S_FRAME_UD_FLAG (0xFFFFFF00)
#define L2CAP_BR_IS_S_FRAME(flag)                                                \
	((((uint32_t)flag) & L2CAP_BR_S_FRAME_FLAG_MASK) == L2CAP_BR_S_FRAME_UD_FLAG)

#define L2CAP_BR_GET_S_BIT(flag) (((uint32_t)flag) & ~L2CAP_BR_S_FRAME_FLAG_MASK)

#define L2CAP_BR_S_FRAME_UD_FLAG_SET(s) ((void *)(L2CAP_BR_S_FRAME_UD_FLAG | (s)))
#define L2CAP_BR_S_RR_FRAME             L2CAP_BR_S_FRAME_UD_FLAG_SET(BT_L2CAP_CONTROL_S_RR)
#define L2CAP_BR_S_REJ_FRAME            L2CAP_BR_S_FRAME_UD_FLAG_SET(BT_L2CAP_CONTROL_S_REJ)
#define L2CAP_BR_S_RNR_FRAME            L2CAP_BR_S_FRAME_UD_FLAG_SET(BT_L2CAP_CONTROL_S_RNR)
#define L2CAP_BR_S_SREJ_FRAME           L2CAP_BR_S_FRAME_UD_FLAG_SET(BT_L2CAP_CONTROL_S_SREJ)

#define L2CAP_BR_INFO_TIMEOUT    K_SECONDS(4)
#define L2CAP_BR_CFG_TIMEOUT     K_SECONDS(4)
#define L2CAP_BR_DISCONN_TIMEOUT K_SECONDS(1)
#define L2CAP_BR_CONN_TIMEOUT    K_SECONDS(40)

#define L2CAP_FEAT_FC_MASK           BIT(0)
#define L2CAP_FEAT_RET_MASK          BIT(1)
#define L2CAP_FEAT_QOS_MASK          BIT(2)
#define L2CAP_FEAT_ENH_RET_MASK      BIT(3)
#define L2CAP_FEAT_STREAM_MASK       BIT(4)
#define L2CAP_FEAT_FCS_MASK          BIT(5)
#define L2CAP_FEAT_EXT_FS_MASK       BIT(6)
#define L2CAP_FEAT_FIXED_CHAN_MASK   BIT(7)
#define L2CAP_FEAT_EXT_WIN_SIZE_MASK BIT(8)
#define L2CAP_FEAT_CLS_MASK          BIT(9)
#define L2CAP_FEAT_ECBFC_MASK        BIT(10)

#if defined(CONFIG_BT_L2CAP_CLS)
#define L2CAP_FEAT_CLS_ENABLE_MASK L2CAP_FEAT_CLS_MASK
#else
#define L2CAP_FEAT_CLS_ENABLE_MASK 0
#endif /* CONFIG_BT_L2CAP_CLS */

#if defined(CONFIG_BT_L2CAP_RET)
#define L2CAP_FEAT_RET_ENABLE_MASK L2CAP_FEAT_RET_MASK
#else
#define L2CAP_FEAT_RET_ENABLE_MASK 0
#endif /* CONFIG_BT_L2CAP_RET */

#if defined(CONFIG_BT_L2CAP_FC)
#define L2CAP_FEAT_FC_ENABLE_MASK L2CAP_FEAT_FC_MASK
#else
#define L2CAP_FEAT_FC_ENABLE_MASK 0
#endif /* CONFIG_BT_L2CAP_FC */

#if defined(CONFIG_BT_L2CAP_ENH_RET)
#define L2CAP_FEAT_ENH_RET_ENABLE_MASK L2CAP_FEAT_ENH_RET_MASK
#else
#define L2CAP_FEAT_ENH_RET_ENABLE_MASK 0
#endif /* CONFIG_BT_L2CAP_ENH_RET */

#if defined(CONFIG_BT_L2CAP_STREAM)
#define L2CAP_FEAT_STREAM_ENABLE_MASK L2CAP_FEAT_STREAM_MASK
#else
#define L2CAP_FEAT_STREAM_ENABLE_MASK 0
#endif /* CONFIG_BT_L2CAP_STREAM */

#if defined(CONFIG_BT_L2CAP_FCS)
#define L2CAP_FEAT_FCS_ENABLE_MASK L2CAP_FEAT_FCS_MASK
#else
#define L2CAP_FEAT_FCS_ENABLE_MASK 0
#endif /* CONFIG_BT_L2CAP_FCS */

#if defined(CONFIG_BT_L2CAP_EXT_WIN_SIZE)
#define L2CAP_FEAT_EXT_WIN_SIZE_ENABLE_MASK L2CAP_FEAT_EXT_WIN_SIZE_MASK
#else
#define L2CAP_FEAT_EXT_WIN_SIZE_ENABLE_MASK 0
#endif /* CONFIG_BT_L2CAP_EXT_WIN_SIZE */

/*
 * L2CAP extended feature mask:
 * BR/EDR fixed channel support enabled
 * Unicast Connectionless Data Reception depends on `config BT_L2CAP_CLS`
 */
#define L2CAP_EXTENDED_FEAT_MASK                                                               \
	(L2CAP_FEAT_FIXED_CHAN_MASK | L2CAP_FEAT_CLS_ENABLE_MASK | L2CAP_FEAT_RET_ENABLE_MASK |    \
	 L2CAP_FEAT_FC_ENABLE_MASK | L2CAP_FEAT_ENH_RET_ENABLE_MASK |                              \
	 L2CAP_FEAT_STREAM_ENABLE_MASK | L2CAP_FEAT_FCS_ENABLE_MASK |                              \
	 L2CAP_FEAT_EXT_WIN_SIZE_ENABLE_MASK)

enum {
	/* Connection oriented channels flags */
	L2CAP_FLAG_CONN_LCONF_DONE, /* local config accepted by remote */
	L2CAP_FLAG_CONN_RCONF_DONE, /* remote config accepted by local */
	L2CAP_FLAG_CONN_ACCEPTOR,   /* getting incoming connection req */
	L2CAP_FLAG_CONN_PENDING,    /* remote sent pending result in rsp */

	/* Signaling channel flags */
	L2CAP_FLAG_SIG_INFO_PENDING, /* retrieving remote l2cap info */
	L2CAP_FLAG_SIG_INFO_DONE,    /* remote l2cap info is done */

	/* fixed channels flags */
	L2CAP_FLAG_FIXED_CONNECTED, /* fixed connected */

	/* Retransmition and flow control flags*/
	L2CAP_FLAG_RET_TIMER,   /* Retransmission timer is working */
	L2CAP_FLAG_PDU_RETRANS, /* PDU retransmission */

	/* Receiving S-frame/I-frame flags */
	L2CAP_FLAG_RECV_FRAME_P,         /* Poll (P) flag of received frame */
	L2CAP_FLAG_RECV_FRAME_R,         /* Retransmission Disable (R) of received frame */
	L2CAP_FLAG_RECV_FRAME_R_CHANGED, /* Flag the R flag changed.
					  * After send received frame with R bit set,
					  * clear the flag.
					  */

	/* Sending S-frame/I-frame flags */
	L2CAP_FLAG_SEND_FRAME_REJ,         /* Report an REJ in received frame */
	L2CAP_FLAG_SEND_FRAME_REJ_CHANGED, /* Flag the REJ flag changed.
					    * After send received frame with REJ flag,
					    * clear the flag.
					    */
	L2CAP_FLAG_SEND_FRAME_P,           /* Poll (P) flag */
	L2CAP_FLAG_SEND_FRAME_P_CHANGED,   /* Poll (P) flag changed */
	L2CAP_FLAG_SEND_S_FRAME,           /* Need to send S-frame */

	/* Remote State */
	L2CAP_FLAG_REMOTE_BUSY, /* Remote Busy Flag */

	/* Local State */
	L2CAP_FLAG_LOCAL_BUSY,         /* Local Busy Flag */
	L2CAP_FLAG_LOCAL_BUSY_CHANGED, /* Local Busy Flag changed */
	L2CAP_FLAG_REJ_ACTIONED,       /* REJ Actioned */
	L2CAP_FLAG_SREJ_ACTIONED,      /* SREJ Actioned */
	L2CAP_FLAG_RET_I_FRAME,        /* Could Retransmit I-frames */
	L2CAP_FLAG_NEW_I_FRAME,        /* Could Send Pending I-frames */
	L2CAP_FLAG_RET_REQ_I_FRAME,    /* Could Retransmit Requested I-frames */
	L2CAP_FLAG_REQ_SEQ_UPDATED,    /* req_seq has been updated */
};

static sys_slist_t br_servers;

/* Pool for outgoing BR/EDR signaling packets, min MTU is 48 */
NET_BUF_POOL_FIXED_DEFINE(br_sig_pool, CONFIG_BT_MAX_CONN,
			  BT_L2CAP_BUF_SIZE(L2CAP_BR_MIN_MTU), 8, NULL);

#if L2CAP_BR_RET_FC_ENABLE
static void br_tx_buf_destroy(struct net_buf *buf)
{
	net_buf_destroy(buf);

	LOG_DBG("");

	/* Kick the TX processor to send the rest of the frags. */
	bt_tx_irq_raise();
}

/* Pool for outgoing BR/EDR signaling packets, min MTU is 48 */
NET_BUF_POOL_FIXED_DEFINE(br_tx_pool, CONFIG_BT_L2CAP_TX_BUF_COUNT,
		    BT_L2CAP_BUF_SIZE(CONFIG_BT_L2CAP_MPS),
		    CONFIG_BT_CONN_TX_USER_DATA_SIZE, br_tx_buf_destroy);
#endif /* L2CAP_BR_RET_FC_ENABLE */

#if defined(CONFIG_BT_L2CAP_CLS)
static sys_slist_t br_clses;

struct bt_l2cap_cls_chan {
	/* The channel this context is associated with */
	struct bt_l2cap_br_chan	chan;
};

static struct bt_l2cap_cls_chan bt_l2cap_cls_pool[CONFIG_BT_MAX_CONN];
#endif /* CONFIG_BT_L2CAP_CLS */

/* BR/EDR L2CAP signalling channel specific context */
struct bt_l2cap_br {
	/* The channel this context is associated with */
	struct bt_l2cap_br_chan	chan;
	uint8_t			info_ident;
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

#if L2CAP_BR_RET_FC_ENABLE
	k_work_cancel_delayable(&br_chan->ret_work);
	k_work_cancel_delayable(&br_chan->monitor_work);
#endif /* L2CAP_BR_RET_FC_ENABLE */

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

/* L2CAP channel wants to send a PDU */
static bool chan_has_data(struct bt_l2cap_br_chan *br_chan)
{
	return !sys_slist_is_empty(&br_chan->_pdu_tx_queue);
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

#if L2CAP_BR_RET_FC_ENABLE
static void l2cap_br_start_timer(struct bt_l2cap_br_chan *br_chan, bool ret, bool restart)
{
	if (ret) {
		if (!atomic_test_and_set_bit(br_chan->flags, L2CAP_FLAG_RET_TIMER)) {
			k_work_cancel_delayable(&br_chan->monitor_work);
			k_work_schedule(&br_chan->ret_work, K_MSEC(br_chan->tx.ret_timeout));
		} else {
			if (restart) {
				k_work_reschedule(&br_chan->ret_work,
						  K_MSEC(br_chan->tx.ret_timeout));
			}
		}
	} else {
		if (atomic_test_and_clear_bit(br_chan->flags, L2CAP_FLAG_RET_TIMER)) {
			k_work_cancel_delayable(&br_chan->ret_work);
			k_work_schedule(&br_chan->monitor_work,
					K_MSEC(br_chan->tx.monitor_timeout));
		} else {
			if (restart) {
				k_work_reschedule(&br_chan->monitor_work,
						  K_MSEC(br_chan->tx.monitor_timeout));
			}
		}
	}
}

static uint16_t bt_l2cap_br_get_outstanding_count(struct bt_l2cap_br_chan *br_chan)
{
	struct bt_l2cap_br_window *tx_win, *next;
	uint16_t count = 0;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&br_chan->_pdu_outstanding, tx_win, next, node) {
		count++;
	}

	return count;
}

static struct bt_l2cap_br_window *l2cap_br_find_window(struct bt_l2cap_br_chan *br_chan)
{
	struct bt_l2cap_br_window *win;
	uint16_t count;

	count = bt_l2cap_br_get_outstanding_count(br_chan);
	if (count >= br_chan->tx.window) {
		return NULL;
	}

	win = k_fifo_get(&br_chan->_free_tx_win, K_NO_WAIT);
	if (win) {
		memset(win, 0, sizeof(*win));
	}

	return win;
}

static void l2cap_br_free_window(struct bt_l2cap_br_chan *br_chan,
								 struct bt_l2cap_br_window *tx_win)
{
	if (tx_win) {
		memset(tx_win, 0, sizeof(*tx_win));
		k_fifo_put(&br_chan->_free_tx_win, tx_win);
	}
}

static uint16_t bt_l2cap_br_update_seq(struct bt_l2cap_br_chan *br_chan, uint16_t seq)
{
	if ((br_chan->tx.mode == BT_L2CAP_BR_LINK_MODE_ERET) ||
	    (br_chan->tx.mode == BT_L2CAP_BR_LINK_MODE_STREAM)) {
		if (br_chan->tx.extended_control) {
			seq = seq % BT_L2CAP_EXT_CONTROL_SEQ_MAX;
		} else {
			seq = seq % BT_L2CAP_CONTROL_SEQ_MAX;
		}
	} else if ((br_chan->tx.mode == BT_L2CAP_BR_LINK_MODE_RET) ||
		   (br_chan->tx.mode == BT_L2CAP_BR_LINK_MODE_FC)) {
		seq = seq % BT_L2CAP_CONTROL_SEQ_MAX;
	}

	return seq;
}

static struct bt_l2cap_br_window *l2cap_br_find_srej(struct bt_l2cap_br_chan *br_chan)
{
	struct bt_l2cap_br_window *tx_win, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&br_chan->_pdu_outstanding, tx_win, next, node) {
		if (tx_win->srej) {
			return tx_win;
		}
	}

	return tx_win;
}

static bool bt_l2cap_br_check_req_seq_valid(struct bt_l2cap_br_chan *br_chan,
			      uint16_t req_seq)
{
	uint16_t outstanding_frames;
	uint16_t ack_frames;
	uint16_t outstanding_count;

	outstanding_frames = (uint16_t)(br_chan->next_tx_seq - br_chan->expected_ack_seq);
	ack_frames = (uint16_t)(req_seq - br_chan->expected_ack_seq);

	if ((br_chan->tx.mode == BT_L2CAP_BR_LINK_MODE_ERET) ||
	    (br_chan->tx.mode == BT_L2CAP_BR_LINK_MODE_STREAM)) {
		if (br_chan->tx.extended_control) {
			outstanding_frames = outstanding_frames % BT_L2CAP_EXT_CONTROL_SEQ_MAX;
			ack_frames = ack_frames % BT_L2CAP_EXT_CONTROL_SEQ_MAX;
		} else {
			outstanding_frames = outstanding_frames % BT_L2CAP_CONTROL_SEQ_MAX;
			ack_frames = ack_frames % BT_L2CAP_CONTROL_SEQ_MAX;
		}
	} else if ((br_chan->tx.mode == BT_L2CAP_BR_LINK_MODE_RET) ||
		   (br_chan->tx.mode == BT_L2CAP_BR_LINK_MODE_FC)) {
		outstanding_frames = outstanding_frames % BT_L2CAP_CONTROL_SEQ_MAX;
		ack_frames = ack_frames % BT_L2CAP_CONTROL_SEQ_MAX;
	}

	outstanding_count = bt_l2cap_br_get_outstanding_count(br_chan);
	__ASSERT(outstanding_count == outstanding_frames,
		 "Mismatch between window and seq %zu - %zu", outstanding_count,
		 outstanding_frames);

	if (ack_frames <= outstanding_frames) {
		return true;
	}

	return false;
}

static void l2cap_br_sdu_is_done(struct bt_l2cap_br_chan *br_chan,
			      struct net_buf *sdu, int err)
{
	bt_conn_tx_cb_t cb;

	__ASSERT(sdu, "Invalid sdu buffer on chan %p", br_chan);

	/* The SDU is done */
	LOG_DBG("SDU is done, removing %p", sdu);

	if (!sys_slist_find_and_remove(&br_chan->_pdu_tx_queue, &sdu->node)) {
		LOG_WRN("SDU %p is not found", sdu);
	}

	cb = closure_cb(sdu->user_data);
	if (cb) {
		cb(br_chan->chan.conn, closure_data(sdu->user_data), err);
	}

	/* Remove the pdu */
	net_buf_unref(sdu);

	LOG_DBG("chan %p done", br_chan);

	/* Append channel to list if it still has data */
	if (chan_has_data(br_chan)) {
		LOG_DBG("chan %p ready", br_chan);
		raise_data_ready(br_chan);
	}
}

static int bt_l2cap_br_update_req_seq_direct(struct bt_l2cap_br_chan *br_chan,
			      uint16_t req_seq, bool rej)
{
	struct bt_l2cap_br_window *tx_win;
	struct net_buf *sdu;

	tx_win = (struct bt_l2cap_br_window *)sys_slist_peek_head(
		&br_chan->_pdu_outstanding);
	while (tx_win) {
		if (tx_win->tx_seq != req_seq) {
			sdu = tx_win->sdu;
			if ((tx_win->sar == BT_L2CAP_CONTROL_SAR_UNSEG) ||
				(tx_win->sar == BT_L2CAP_CONTROL_SAR_END)) {
				l2cap_br_sdu_is_done(br_chan, sdu, 0);
			}
			tx_win = (struct bt_l2cap_br_window *)sys_slist_get(
				&br_chan->_pdu_outstanding);
			l2cap_br_free_window(br_chan, tx_win);
			tx_win = (struct bt_l2cap_br_window *)sys_slist_peek_head(
				&br_chan->_pdu_outstanding);
		} else {
			break;
		}
	}
	br_chan->expected_ack_seq = req_seq;

	if (rej) {
		tx_win = (struct bt_l2cap_br_window *)sys_slist_peek_head(
			&br_chan->_pdu_outstanding);
		if (tx_win) {
			sdu = tx_win->sdu;
			__ASSERT(sdu, "Invalid sdu buffer on chan %p", br_chan);
			net_buf_simple_restore(&sdu->b, &tx_win->sdu_state);
			if ((tx_win->sar == BT_L2CAP_CONTROL_SAR_UNSEG) ||
				(tx_win->sar == BT_L2CAP_CONTROL_SAR_START)) {
				br_chan->_sdu_total_len = 0;
			} else {
				br_chan->_sdu_total_len = tx_win->sdu_total_len;
			}
			br_chan->next_tx_seq = tx_win->tx_seq;
		}

		while (tx_win) {
			tx_win = (struct bt_l2cap_br_window *)sys_slist_get(
				&br_chan->_pdu_outstanding);
			l2cap_br_free_window(br_chan, tx_win);
			tx_win = (struct bt_l2cap_br_window *)sys_slist_peek_head(
				&br_chan->_pdu_outstanding);
			if (tx_win) {
				sdu = tx_win->sdu;
				__ASSERT(sdu, "Invalid sdu buffer on chan %p", br_chan);
				if ((tx_win->sar == BT_L2CAP_CONTROL_SAR_UNSEG) ||
					(tx_win->sar == BT_L2CAP_CONTROL_SAR_START)) {
					net_buf_simple_restore(&sdu->b, &tx_win->sdu_state);
				}
			}
		}
	}

	if (!atomic_test_bit(br_chan->flags, L2CAP_FLAG_RECV_FRAME_R)) {
		if (bt_l2cap_br_get_outstanding_count(br_chan)) {
			l2cap_br_start_timer(br_chan, true, true);
		} else {
			l2cap_br_start_timer(br_chan, false, false);
		}
	}

	if (chan_has_data(br_chan)) {
		if (!atomic_test_bit(br_chan->flags, L2CAP_FLAG_RET_REQ_I_FRAME)) {
			/* If there any unack I-frame, set req_seq updated */
			atomic_set_bit(br_chan->flags, L2CAP_FLAG_REQ_SEQ_UPDATED);
		}

		LOG_DBG("chan %p ready", br_chan);
		raise_data_ready(br_chan);
	}

	return 0;
}

static int bt_l2cap_br_update_req_seq(struct bt_l2cap_br_chan *br_chan,
			      uint16_t req_seq, bool rej)
{
	int err = 0;

	if (bt_l2cap_br_check_req_seq_valid(br_chan, req_seq)) {
		err = bt_l2cap_br_update_req_seq_direct(br_chan, req_seq, rej);
	} else {
		LOG_ERR("Invalid req seq %d received on %p", req_seq, br_chan);
		/* The L2CAP entity shall close the channel as a consequence
		 * of an ReqSeq Sequence error.
		 */
		bt_l2cap_br_chan_disconnect(&br_chan->chan);
		err = -ESHUTDOWN;
	}

	return err;
}

static int l2cap_br_send_s_frame(struct bt_l2cap_br_chan *br_chan, uint8_t s, k_timeout_t timeout)
{
	struct net_buf *buf;
	int err;

	if (atomic_test_and_set_bit(br_chan->flags, L2CAP_FLAG_SEND_S_FRAME)) {
		LOG_WRN("S-frame is in pending on %p", br_chan);
	}

	if (chan_has_data(br_chan)) {
		/* There is PDU in pending. No Empty SDU needed to trigger S-frame sending. */
		raise_data_ready(br_chan);
		return 0;
	}

	buf = bt_l2cap_create_pdu_timeout(&br_tx_pool, 0, timeout);
	if (!buf) {
		return -ENOBUFS;
	}

	err = bt_l2cap_br_send_cb(br_chan->chan.conn, br_chan->tx.cid, buf, NULL,
				  L2CAP_BR_S_FRAME_UD_FLAG_SET(s));
	if (err) {
		LOG_ERR("Fail to send S-frame %d on %p", err, br_chan);
		net_buf_unref(buf);
	}

	return err;
}

static void l2cap_br_ret_timeout(struct k_work *work)
{
	struct bt_l2cap_br_chan *br_chan = BR_CHAN_RET(work);

	if (!br_chan->chan.conn || br_chan->chan.conn->state != BT_CONN_CONNECTED) {
		/* ACL connection is broken. */
		return;
	}

	if (br_chan->state != BT_L2CAP_CONNECTED) {
		/* The channel is not connected */
		return;
	}

	/* Restart the timer */
	if (atomic_test_bit(br_chan->flags, L2CAP_FLAG_RET_TIMER)) {
		k_work_schedule(&br_chan->ret_work, K_MSEC(br_chan->tx.ret_timeout));
	}

	LOG_WRN("chan %p retransmission timeout", br_chan);

	if (sys_slist_peek_head(&br_chan->_pdu_outstanding)) {
		if (br_chan->tx.mode == BT_L2CAP_BR_LINK_MODE_RET) {
			atomic_set_bit(br_chan->flags, L2CAP_FLAG_PDU_RETRANS);
			/* Append channel to list if it still has data */
			if (chan_has_data(br_chan)) {
				LOG_DBG("chan %p ready", br_chan);
				raise_data_ready(br_chan);
			}
		} else if (br_chan->tx.mode == BT_L2CAP_BR_LINK_MODE_FC) {
			uint16_t expected_ack_seq =
				bt_l2cap_br_update_seq(br_chan, br_chan->expected_ack_seq + 1);

			LOG_WRN("unacknowledged I-frame with sequence number %d",
				br_chan->expected_ack_seq);
			bt_l2cap_br_update_req_seq(br_chan, expected_ack_seq, false);
		} else if (br_chan->tx.mode == BT_L2CAP_BR_LINK_MODE_ERET) {
			if (!atomic_test_and_set_bit(br_chan->flags, L2CAP_FLAG_SEND_FRAME_P)) {
				int err;

				/* State: XMIT
				 * Retrans Timer expires Or Local Busy Clears
				 * Action: send RR(P=1) or RNR(P=1)
				 */
				br_chan->retry_count = 1;

				l2cap_br_start_timer(br_chan, false, true);

				atomic_set_bit(br_chan->flags, L2CAP_FLAG_SEND_FRAME_P_CHANGED);
				err = l2cap_br_send_s_frame(br_chan, BT_L2CAP_CONTROL_S_RR,
							    K_NO_WAIT);
				if (err) {
					bt_l2cap_chan_disconnect(&br_chan->chan);
				}
			}
		}
	}
}

static void l2cap_br_monitor_timeout(struct k_work *work)
{
	struct bt_l2cap_br_chan *br_chan = BR_CHAN_MONITOR(work);

	if (!br_chan->chan.conn || br_chan->chan.conn->state != BT_CONN_CONNECTED) {
		/* ACL connection is broken. */
		return;
	}

	if (br_chan->state != BT_L2CAP_CONNECTED) {
		/* The channel is not connected */
		return;
	}

	/* Restart the timer */
	if (!atomic_test_bit(br_chan->flags, L2CAP_FLAG_RET_TIMER)) {
		k_work_schedule(&br_chan->monitor_work, K_MSEC(br_chan->tx.monitor_timeout));
	}

	LOG_WRN("chan %p monitor timeout", br_chan);

	if (br_chan->tx.mode == BT_L2CAP_BR_LINK_MODE_ERET) {
		if (!atomic_test_bit(br_chan->flags, L2CAP_FLAG_SEND_FRAME_P_CHANGED) &&
		    atomic_test_bit(br_chan->flags, L2CAP_FLAG_SEND_FRAME_P)) {
			/* State: WAIT_F
			 * Monitor Timer expires
			 * Action: send RR(P=1) or RNR(P=1)
			 */
			int err;

			br_chan->retry_count++;
			if (br_chan->retry_count > br_chan->tx.transmit) {
				bt_l2cap_chan_disconnect(&br_chan->chan);
				return;
			}

			atomic_set_bit(br_chan->flags, L2CAP_FLAG_SEND_FRAME_P_CHANGED);
			l2cap_br_start_timer(br_chan, false, true);

			err = l2cap_br_send_s_frame(br_chan, BT_L2CAP_CONTROL_S_RR, K_NO_WAIT);
			if (err) {
				bt_l2cap_chan_disconnect(&br_chan->chan);
			}
		}
	} else if ((br_chan->tx.mode == BT_L2CAP_BR_LINK_MODE_FC) ||
		   (br_chan->tx.mode == BT_L2CAP_BR_LINK_MODE_RET)) {
		/* Send S-frame with R=0.
		 * Send S-Frame if there is not any pending I-frame
		 */
		int err;

		err = l2cap_br_send_s_frame(br_chan, BT_L2CAP_CONTROL_S_RR, K_NO_WAIT);
		if (!err) {
			atomic_clear_bit(br_chan->flags, L2CAP_FLAG_RECV_FRAME_R);
		} else {
			bt_l2cap_chan_disconnect(&br_chan->chan);
		}
	}
}
#endif /* L2CAP_BR_RET_FC_ENABLE */

static bool l2cap_br_chan_add(struct bt_conn *conn, struct bt_l2cap_chan *chan,
			      bt_l2cap_chan_destroy_t destroy)
{
	struct bt_l2cap_br_chan *ch = l2cap_br_chan_alloc_cid(conn, chan);

	if (!ch) {
		LOG_DBG("Unable to allocate L2CAP CID");
		return false;
	}

	sys_slist_init(&ch->_pdu_tx_queue);

	/* All dynamic channels have the destroy handler which makes sure that
	 * the RTX work structure is properly released with a cancel sync.
	 * The fixed signal channel is only removed when disconnected and the
	 * disconnected handler is always called from the workqueue itself so
	 * canceling from there should always succeed.
	 */
	k_work_init_delayable(&ch->rtx_work, l2cap_br_rtx_timeout);
#if L2CAP_BR_RET_FC_ENABLE
	k_work_init_delayable(&ch->ret_work, l2cap_br_ret_timeout);
	k_work_init_delayable(&ch->monitor_work, l2cap_br_monitor_timeout);
#endif /* L2CAP_BR_RET_FC_ENABLE */
	bt_l2cap_chan_add(conn, chan, destroy);

	return true;
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
		LOG_DBG("not enough room in user_data %d < %d pool %u",
			buf->user_data_size,
			CONFIG_BT_CONN_TX_USER_DATA_SIZE,
			buf->pool_id);
		return -EINVAL;
	}

	LOG_DBG("push PDU: cb %p userdata %p", cb, user_data);

	make_closure(buf->user_data, cb, user_data);
	sys_slist_append(&br_chan->_pdu_tx_queue, &buf->node);
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

#if L2CAP_BR_RET_FC_ENABLE
static uint16_t get_pdu_len(struct bt_l2cap_br_chan *br_chan, struct net_buf *buf,
					 bool start_seg)
{
	uint16_t pdu_len = buf->len;
	uint16_t actual_mps;

	if (br_chan->tx.mode == BT_L2CAP_BR_LINK_MODE_BASIC) {
		return pdu_len;
	}

	/* MPS is Max PDU Size.
	 * PDU of I-frame includes the Control, L2CAP SDU Length (when present),
	 * Information Payload, and frame check sequence (FCS) (when present) fields.
	 */
	actual_mps = br_chan->tx.mps - BT_L2CAP_RT_FC_SDU_HDR_SIZE(br_chan) -
		     BT_L2CAP_RT_FC_SDU_TAIL_SIZE(br_chan);

	/* To get the max available space,
	 * assume SDU field is no present.
	 */
	actual_mps += BT_L2CAP_RT_FC_SDU_LEN_SIZE;

	if (pdu_len <= actual_mps) {
		return pdu_len;
	}

	if (start_seg) {
		/* The max mps length cannot meet the requirement.
		 * Deduct the SDU length field for Max length could be used for seg.
		 */
		return actual_mps - BT_L2CAP_RT_FC_SDU_LEN_SIZE;
	}

	return actual_mps;
}

static void bt_l2cap_br_pack_s_frame_header(struct bt_l2cap_br_chan *br_chan,
					    struct net_buf *buf, uint8_t s)
{
	struct bt_l2cap_hdr *hdr;
	uint16_t std_control;
	uint16_t enh_control;
	uint32_t ext_control;
	uint8_t f_bit = 0;
	uint8_t r_bit = 0;
	uint8_t p_bit = 0;
	uint16_t pdu_len;

	pdu_len = BT_L2CAP_RT_FC_SDU_HDR_SIZE(br_chan) + BT_L2CAP_RT_FC_SDU_TAIL_SIZE(br_chan) -
		  BT_L2CAP_RT_FC_SDU_LEN_SIZE;

	hdr = net_buf_push(buf, sizeof(*hdr));

	hdr->len = sys_cpu_to_le16(pdu_len);
	hdr->cid = sys_cpu_to_le16(br_chan->tx.cid);

	if (s == BT_L2CAP_CONTROL_S_RR) {
		if (atomic_test_and_clear_bit(br_chan->flags, L2CAP_FLAG_SEND_FRAME_REJ_CHANGED) &&
		    atomic_test_bit(br_chan->flags, L2CAP_FLAG_SEND_FRAME_REJ)) {
			s = BT_L2CAP_CONTROL_S_REJ;
		}
	}

	if (s == BT_L2CAP_CONTROL_S_RR) {
		if (atomic_test_and_clear_bit(br_chan->flags, L2CAP_FLAG_RECV_FRAME_P)) {
			f_bit = 1;
		} else if (atomic_test_and_clear_bit(br_chan->flags,
						     L2CAP_FLAG_SEND_FRAME_P_CHANGED) &&
			   atomic_test_bit(br_chan->flags, L2CAP_FLAG_SEND_FRAME_P)) {
			p_bit = 1;
		} else if (atomic_test_and_clear_bit(br_chan->flags,
						     L2CAP_FLAG_LOCAL_BUSY_CHANGED) &&
			   atomic_test_bit(br_chan->flags, L2CAP_FLAG_LOCAL_BUSY)) {
			s = BT_L2CAP_CONTROL_S_RNR;
		}
	}

	if (atomic_test_and_clear_bit(br_chan->flags, L2CAP_FLAG_RECV_FRAME_R_CHANGED) &&
	    atomic_test_bit(br_chan->flags, L2CAP_FLAG_RECV_FRAME_R)) {
		r_bit = 1;
	}

	if (br_chan->tx.mode != BT_L2CAP_BR_LINK_MODE_STREAM) {
		br_chan->req_seq = br_chan->expected_tx_seq;
	} else {
		br_chan->req_seq = 0;
	}

	if ((br_chan->tx.mode == BT_L2CAP_BR_LINK_MODE_ERET) ||
	    (br_chan->tx.mode == BT_L2CAP_BR_LINK_MODE_STREAM)) {
		if (br_chan->tx.extended_control) {
			/* Space occupied for extended control field. */
			ext_control = sys_cpu_to_le32(BT_L2CAP_S_FRAME_EXT_CONTROL_SET(
				f_bit, br_chan->req_seq, s, p_bit));
			net_buf_add_le32(buf, ext_control);
		} else {
			/* Space occupied for enhanced control field. */
			enh_control = sys_cpu_to_le16(BT_L2CAP_S_FRAME_ENH_CONTROL_SET(
				s, p_bit, f_bit, br_chan->req_seq));
			net_buf_add_le16(buf, enh_control);
		}
	} else {
		/* Space occupied for standard control field. */
		std_control = sys_cpu_to_le16(
			BT_L2CAP_S_FRAME_STD_CONTROL_SET(s, r_bit, br_chan->req_seq));
		net_buf_add_le16(buf, std_control);
	}
}

static void bt_l2cap_br_pack_i_frame_header(struct bt_l2cap_br_chan *br_chan, struct net_buf *buf,
					    uint8_t pdu_len, uint8_t sar, uint8_t tx_seq)
{
	struct bt_l2cap_hdr *hdr;
	uint16_t std_control;
	uint16_t enh_control;
	uint32_t ext_control;
	uint8_t f_bit = 0;
	uint8_t r_bit = 0;

	/* HDR space has been reserved */
	hdr = net_buf_push(buf, sizeof(*hdr));

	hdr->len = sys_cpu_to_le16(pdu_len);
	hdr->cid = sys_cpu_to_le16(br_chan->tx.cid);

	if (atomic_test_and_clear_bit(br_chan->flags, L2CAP_FLAG_RECV_FRAME_P)) {
		f_bit = 1;
	}

	if (atomic_test_and_clear_bit(br_chan->flags, L2CAP_FLAG_RECV_FRAME_R_CHANGED) &&
	    atomic_test_bit(br_chan->flags, L2CAP_FLAG_RECV_FRAME_R)) {
		r_bit = 1;
	}

	if (br_chan->tx.mode != BT_L2CAP_BR_LINK_MODE_STREAM) {
		br_chan->req_seq = br_chan->expected_tx_seq;
	} else {
		br_chan->req_seq = 0;
	}

	if ((br_chan->tx.mode == BT_L2CAP_BR_LINK_MODE_ERET) ||
	    (br_chan->tx.mode == BT_L2CAP_BR_LINK_MODE_STREAM)) {
		if (br_chan->tx.extended_control) {
			/* Space occupied for extended control field. */
			ext_control = BT_L2CAP_I_FRAME_EXT_CONTROL_SET(f_bit, tx_seq, sar,
								       br_chan->req_seq);
			net_buf_add_le32(buf, ext_control);
		} else {
			/* Space occupied for enhanced control field. */
			enh_control = BT_L2CAP_I_FRAME_ENH_CONTROL_SET(tx_seq, f_bit,
								       br_chan->req_seq, sar);
			net_buf_add_le16(buf, enh_control);
		}
	} else {
		/* Space occupied for standard control field. */
		std_control =
			BT_L2CAP_I_FRAME_STD_CONTROL_SET(tx_seq, r_bit, br_chan->req_seq, sar);
		net_buf_add_le16(buf, std_control);
	}
}

static struct net_buf *l2cap_br_get_next_sdu(struct bt_l2cap_br_chan *br_chan)
{
	struct net_buf *sdu, *next;

	if (br_chan->_pdu_buf) {
		return br_chan->_pdu_buf;
	}

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&br_chan->_pdu_tx_queue, sdu, next, node) {
		if (sdu->len) {
			return sdu;
		}

		if (L2CAP_BR_IS_S_FRAME(closure_data(sdu->user_data))) {
			return sdu;
		}
	}

	return NULL;
}

static bool l2cap_br_send_i_frame(struct bt_l2cap_br_chan *br_chan, struct net_buf *sdu)
{
	if (atomic_test_bit(br_chan->flags, L2CAP_FLAG_RECV_FRAME_R)) {
		return false;
	}

	if (atomic_test_bit(br_chan->flags, L2CAP_FLAG_RET_REQ_I_FRAME)) {
		if (l2cap_br_find_srej(br_chan)) {
			return true;
		}
	}

	if (atomic_test_bit(br_chan->flags, L2CAP_FLAG_RET_I_FRAME) &&
	    (atomic_test_bit(br_chan->flags, L2CAP_FLAG_PDU_RETRANS) ||
	     atomic_test_bit(br_chan->flags, L2CAP_FLAG_REQ_SEQ_UPDATED))) {
		return true;
	}

	if (atomic_test_bit(br_chan->flags, L2CAP_FLAG_SEND_FRAME_P)) {
		return false;
	}

	if (atomic_test_bit(br_chan->flags, L2CAP_FLAG_REMOTE_BUSY)) {
		return false;
	}

	if (!atomic_test_bit(br_chan->flags, L2CAP_FLAG_NEW_I_FRAME)) {
		return false;
	}

	if (!sdu) {
		return false;
	}

	if (bt_l2cap_br_get_outstanding_count(br_chan) >= br_chan->tx.window) {
		return false;
	}

	if (k_fifo_peek_head(&br_chan->_free_tx_win)) {
		return true;
	}

	return false;
}

static struct net_buf *l2cap_br_ret_fc_data_pull(struct bt_conn *conn, size_t amount,
						 size_t *length)
{
	const sys_snode_t *pdu_ready = sys_slist_peek_head(&conn->l2cap_data_ready);

	if (!pdu_ready) {
		LOG_DBG("nothing to send on this conn");
		return NULL;
	}

	struct bt_l2cap_br_chan *br_chan =
		CONTAINER_OF(pdu_ready, struct bt_l2cap_br_chan, _pdu_ready);

	if (br_chan->_pdu_buf && bt_buf_has_view(br_chan->_pdu_buf)) {
		LOG_ERR("already have view on %p", br_chan->_pdu_buf);
		return NULL;
	}

	/* Leave the PDU buffer in the queue until we have sent all its
	 * fragments.
	 */
	struct net_buf *pdu = l2cap_br_get_next_sdu(br_chan);
	struct net_buf *send_buf = NULL;

	if (br_chan->_pdu_remaining == 0) {
		struct bt_l2cap_br_window *tx_win;
		bool first = true;
		uint8_t s_bit = BT_L2CAP_CONTROL_S_RR;
		bool alloc = false;

		if (pdu && !pdu->len && L2CAP_BR_IS_S_FRAME(closure_data(pdu->user_data))) {
			/* It is a S-frame */
			s_bit = L2CAP_BR_GET_S_BIT(closure_data(pdu->user_data));
			alloc = true;
		}

		if ((atomic_test_bit(br_chan->flags, L2CAP_FLAG_RECV_FRAME_R) &&
		     atomic_test_bit(br_chan->flags, L2CAP_FLAG_RECV_FRAME_R_CHANGED)) ||
		    (atomic_test_bit(br_chan->flags, L2CAP_FLAG_SEND_FRAME_REJ) &&
		     atomic_test_bit(br_chan->flags, L2CAP_FLAG_SEND_FRAME_REJ_CHANGED)) ||
		    (atomic_test_bit(br_chan->flags, L2CAP_FLAG_SEND_FRAME_P) &&
		     atomic_test_bit(br_chan->flags, L2CAP_FLAG_SEND_FRAME_P_CHANGED)) ||
		    (atomic_test_bit(br_chan->flags, L2CAP_FLAG_LOCAL_BUSY) &&
		     atomic_test_bit(br_chan->flags, L2CAP_FLAG_LOCAL_BUSY_CHANGED)) ||
		    atomic_test_bit(br_chan->flags, L2CAP_FLAG_SEND_S_FRAME)) {
			alloc = true;
		}

		if (atomic_test_bit(br_chan->flags, L2CAP_FLAG_RECV_FRAME_P) &&
		    !l2cap_br_send_i_frame(br_chan, pdu)) {
			alloc = true;
		}

		if (alloc) {
			/* Send S-Frame with R bit set. */
			send_buf = bt_l2cap_create_pdu_timeout(&br_tx_pool, 0, K_NO_WAIT);
			if (send_buf == NULL) {
				/* No buffer can be used for transmission.
				 * Waiting for buffer available.
				 */
				LOG_DBG("no buf for sending I-frame on %p", br_chan);
				return NULL;
			}

			bt_l2cap_br_pack_s_frame_header(br_chan, send_buf, s_bit);
			atomic_clear_bit(br_chan->flags, L2CAP_FLAG_SEND_S_FRAME);

			if (BT_L2CAP_RT_FC_SDU_TAIL_SIZE(br_chan)) {
				uint16_t fcs =
					crc16_reflect(0xA001, 0, send_buf->data, send_buf->len);

				net_buf_add_le16(send_buf, fcs);
			}
			br_chan->_pdu_remaining = send_buf->len;
			br_chan->_pdu_buf = send_buf;

			make_closure(send_buf->user_data, NULL, NULL);

			if (pdu && !pdu->len && L2CAP_BR_IS_S_FRAME(closure_data(pdu->user_data))) {
				l2cap_br_sdu_is_done(br_chan, pdu, 0);
			}

			goto done;
		}

		if (atomic_test_bit(br_chan->flags, L2CAP_FLAG_RECV_FRAME_R)) {
			/* No I-frames shall be transmitted if the last
			 * RetransmissionDisableBit (R) received is set to one.
			 */
			LOG_DBG("Stop to send I-frame on %p", br_chan);
			lower_data_ready(br_chan);
			return NULL;
		}

		/* Found SREJ frame */
		if (atomic_test_bit(br_chan->flags, L2CAP_FLAG_RET_REQ_I_FRAME)) {
			tx_win = l2cap_br_find_srej(br_chan);
			if (tx_win) {
				first = false;
				goto send_i_frame;
			}
		}

		/* Retransmission i-Frame. Or, if seq_req has been updated, send next I-frame*/
		if (atomic_test_bit(br_chan->flags, L2CAP_FLAG_RET_I_FRAME) &&
		    (atomic_test_and_clear_bit(br_chan->flags, L2CAP_FLAG_PDU_RETRANS) ||
		     atomic_test_and_clear_bit(br_chan->flags, L2CAP_FLAG_REQ_SEQ_UPDATED))) {
			tx_win = (struct bt_l2cap_br_window *)sys_slist_peek_head(
				&br_chan->_pdu_outstanding);
			if (tx_win) {
				first = false;
				goto send_i_frame;
			}
		}

		if (atomic_test_bit(br_chan->flags, L2CAP_FLAG_SEND_FRAME_P)) {
			/* Cannot send any new I-frames if waiting for F bit.
			 * New I-frames cannot be sent while in the WAIT_F
			 * state to prevent the situation where retransmission of
			 * I-frames could result in the channel being disconnected.
			 */
			LOG_DBG("Waiting for F flag, cannot send I-frame on %p", br_chan);
			lower_data_ready(br_chan);
			return NULL;
		}

		if (atomic_test_bit(br_chan->flags, L2CAP_FLAG_REMOTE_BUSY)) {
			/* RemoteBusy = TRUE, Pending sending data
			 */
			LOG_DBG("Remote is busy, cannot send I-frame on %p", br_chan);
			lower_data_ready(br_chan);
			return NULL;
		}

		/* Cannot send out new I-frame */
		if (!atomic_test_bit(br_chan->flags, L2CAP_FLAG_NEW_I_FRAME)) {
			/* Sending is blocked
			 */
			LOG_DBG("Sending is blocked, cannot send I-frame on %p", br_chan);
			lower_data_ready(br_chan);
			return NULL;
		}

		/* Check whether all PDUs have been sent. But not of all are confirmed. */
		if (!pdu) {
			/* No pending I-frame needs to be sent
			 */
			LOG_DBG("No pending I-frame needs to be sent on %p", br_chan);
			lower_data_ready(br_chan);
			return NULL;
		}

		tx_win = l2cap_br_find_window(br_chan);
		if (!tx_win) {
			/* No more window for sending I-frame PDU.
			 * Remove the channel from the ready-list, it will be added
			 * back later when the S-frame PDU has been received.
			 */
			LOG_DBG("no more window for sending I-frame on %p", br_chan);
			lower_data_ready(br_chan);
			return NULL;
		}

send_i_frame:
		send_buf = bt_l2cap_create_pdu_timeout(&br_tx_pool, 0, K_NO_WAIT);
		if (send_buf == NULL) {
			/* No buffer can be used for transmission.
			 * Waiting for buffer available.
			 */
			LOG_DBG("no buf for sending I-frame on %p", br_chan);
			/* Maybe we can disconnect channel here if retransmission I-frame. */

			/* Free allocated tx_win. */
			l2cap_br_free_window(br_chan, tx_win);
			return NULL;
		}

		make_closure(send_buf->user_data, NULL, NULL);

		uint16_t pdu_len;
		uint16_t actual_pdu_len;
		uint8_t sar;
		uint16_t fcs;

		if (first) {
			bool last_seg;
			bool start_seg;

			start_seg = br_chan->_sdu_total_len ? false : true;

			if (start_seg) {
				br_chan->_sdu_total_len = pdu->len;
			} else {
				if (br_chan->_sdu_total_len <= pdu->len) {
					start_seg = true;
				}
			}

			net_buf_simple_save(&pdu->b, &tx_win->sdu_state);

			pdu_len = get_pdu_len(br_chan, pdu, start_seg);

			actual_pdu_len = pdu_len + BT_L2CAP_RT_FC_SDU_HDR_SIZE(br_chan) +
					 BT_L2CAP_RT_FC_SDU_TAIL_SIZE(br_chan);

			last_seg = pdu_len == pdu->len;

			sar = start_seg ? (last_seg ? BT_L2CAP_CONTROL_SAR_UNSEG
						    : BT_L2CAP_CONTROL_SAR_START)
					: (last_seg ? BT_L2CAP_CONTROL_SAR_END
						    : BT_L2CAP_CONTROL_SAR_CONTI);
			if (sar == BT_L2CAP_CONTROL_SAR_START) {
				net_buf_push_le16(pdu, pdu->len);
				pdu_len += BT_L2CAP_RT_FC_SDU_LEN_SIZE;
			} else {
				/* Remove the SDU length from Header */
				actual_pdu_len -= BT_L2CAP_RT_FC_SDU_LEN_SIZE;
			}

			if (net_buf_tailroom(send_buf) < actual_pdu_len) {
				LOG_ERR("Tailroom of the buffer cannot be filled %zu / %zu",
					net_buf_tailroom(send_buf), actual_pdu_len);
				__ASSERT(false, "Tailroom of the buffer cannot be filled %zu / %zu",
					 net_buf_tailroom(send_buf), actual_pdu_len);
				net_buf_unref(send_buf);
				l2cap_br_free_window(br_chan, tx_win);
				return NULL;
			}

			if (br_chan->tx.mode != BT_L2CAP_BR_LINK_MODE_STREAM) {
				br_chan->req_seq = br_chan->expected_tx_seq;
			} else {
				br_chan->req_seq = 0;
			}

			br_chan->tx_seq = br_chan->next_tx_seq;
			tx_win->tx_seq = br_chan->tx_seq;
			tx_win->data = pdu->data;
			tx_win->len = pdu_len;
			tx_win->srej = false;
			tx_win->sar = sar;
			tx_win->transmit_counter = 1;
			tx_win->sdu_total_len = br_chan->_sdu_total_len;
			tx_win->sdu = pdu;

			LOG_DBG("Sending I-frame %u: buf %p chan %p len %zu", tx_win->tx_seq,
					pdu, br_chan, pdu_len);
		} else {
			pdu_len = tx_win->len;
			actual_pdu_len = pdu_len + BT_L2CAP_RT_FC_SDU_HDR_SIZE(br_chan) +
					 BT_L2CAP_RT_FC_SDU_TAIL_SIZE(br_chan);
			sar = tx_win->sar;
			/* Remove the SDU length from Header.
			 * Do not care SDU length, it has been handled in the first time.
			 */
			actual_pdu_len -= BT_L2CAP_RT_FC_SDU_LEN_SIZE;

			tx_win->transmit_counter++;
			tx_win->srej = false;

			if (tx_win->transmit_counter > br_chan->tx.transmit) {
				/* If the TransmitCounter is equal to MaxTransmit this channel to
				 * the peer entity shall be assumed lost. The channel shall move
				 * to the CLOSED state and appropriate action shall be taken to
				 * report this to the upper layers.
				 */
				bt_l2cap_br_chan_disconnect(&br_chan->chan);
				net_buf_unref(send_buf);
				return NULL;
			}

			LOG_DBG("Retransmission I-frame %u: buf %p chan %p len %zu", tx_win->tx_seq,
				pdu, br_chan, pdu_len);
		}

		bt_l2cap_br_pack_i_frame_header(br_chan, send_buf, actual_pdu_len, sar,
						tx_win->tx_seq);

		net_buf_add_mem(send_buf, tx_win->data, pdu_len);

		if (BT_L2CAP_RT_FC_SDU_TAIL_SIZE(br_chan)) {
			fcs = crc16_reflect(0xA001, 0, send_buf->data, send_buf->len);
			net_buf_add_le16(send_buf, fcs);
		}

		br_chan->_pdu_remaining = send_buf->len;
		br_chan->_pdu_buf = send_buf;

		if (first) {
			br_chan->next_tx_seq =
				bt_l2cap_br_update_seq(br_chan, br_chan->next_tx_seq + 1);

			net_buf_pull(pdu, pdu_len);

			if (br_chan->rx.mode != BT_L2CAP_BR_LINK_MODE_STREAM) {
				sys_slist_append(&br_chan->_pdu_outstanding, &tx_win->node);
			} else {
				l2cap_br_free_window(br_chan, tx_win);
			}
		}
	}

	if (br_chan->_pdu_buf != NULL) {
		l2cap_br_start_timer(br_chan, true, false);
	}

done:
	/* This is used by `conn.c` to figure out if the PDU is done sending. */
	*length = br_chan->_pdu_remaining;

	struct net_buf *buf;

	buf = br_chan->_pdu_buf;

	if (br_chan->_pdu_remaining > amount) {
		br_chan->_pdu_remaining -= amount;
	} else {
		br_chan->_pdu_buf = NULL;
		br_chan->_pdu_remaining = 0;
		if (pdu && !pdu->len) {
			br_chan->_sdu_total_len = 0;
		}

		LOG_DBG("done sending PDU");

		/* Lowering the "request to send" and raising it again allows
		 * fair scheduling of channels on an ACL link: the channel is
		 * marked as "ready to send" by adding a reference to it on a
		 * FIFO on `conn`. Adding it again will send it to the back of
		 * the queue.
		 *
		 * TODO: add a user-controlled QoS function.
		 */
		LOG_DBG("chan %p done", br_chan);
		lower_data_ready(br_chan);

		/* Append channel to list if it still has data */
		if (chan_has_data(br_chan)) {
			LOG_DBG("chan %p ready", br_chan);
			raise_data_ready(br_chan);
		}
	}

	return buf;
}
#endif /* L2CAP_BR_RET_FC_ENABLE */

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

#if L2CAP_BR_RET_FC_ENABLE
	if (br_chan->tx.mode != BT_L2CAP_BR_LINK_MODE_BASIC) {
		return l2cap_br_ret_fc_data_pull(conn, amount, length);
	}
#endif /* L2CAP_BR_RET_FC_ENABLE */

	/* Leave the PDU buffer in the queue until we have sent all its
	 * fragments.
	 */
	const sys_snode_t *tx_pdu = sys_slist_peek_head(&br_chan->_pdu_tx_queue);

	__ASSERT(tx_pdu, "signaled ready but no PDUs in the TX queue");

	struct net_buf *pdu = CONTAINER_OF(tx_pdu, struct net_buf, node);

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
		__maybe_unused bool found;

		found = sys_slist_find_and_remove(&br_chan->_pdu_tx_queue, &pdu->node);

		__ASSERT_NO_MSG(found);

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
#if defined(CONFIG_BT_L2CAP_CLS)
		__fallthrough;
	case BT_L2CAP_INFO_CLS_MTU_MASK:
#endif /* CONFIG_BT_L2CAP_CLS */
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
#if defined(CONFIG_BT_L2CAP_CLS)
	struct bt_l2cap_chan *cls;
	struct bt_l2cap_br_chan *br_cls;
#endif /* CONFIG_BT_L2CAP_CLS */
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
		l2cap->info_feat_mask = net_buf_pull_le32(buf);
		LOG_DBG("remote info mask 0x%08x", l2cap->info_feat_mask);

		if (!(l2cap->info_feat_mask & L2CAP_FEAT_FIXED_CHAN_MASK)) {
			break;
		}
		l2cap_br_get_info(l2cap, BT_L2CAP_INFO_FIXED_CHAN);
		return 0;
	case BT_L2CAP_INFO_FIXED_CHAN:
		l2cap->info_fixed_chan = net_buf_pull_u8(buf);
		LOG_DBG("remote fixed channel mask 0x%02x", l2cap->info_fixed_chan);
		connect_optional_fixed_channels(l2cap);
#if defined(CONFIG_BT_L2CAP_CLS)
		if ((l2cap->info_feat_mask & L2CAP_FEAT_CLS_MASK) &&
			(l2cap->info_fixed_chan & BIT(BT_L2CAP_CID_CLS))) {
			l2cap_br_get_info(l2cap, BT_L2CAP_INFO_CLS_MTU_MASK);
			return 0;
		}
#endif /* CONFIG_BT_L2CAP_CLS */
		break;
#if defined(CONFIG_BT_L2CAP_CLS)
	case BT_L2CAP_INFO_CLS_MTU_MASK:
		cls = bt_l2cap_br_lookup_rx_cid(l2cap->chan.chan.conn, BT_L2CAP_CID_CLS);
		if (cls) {
			br_cls = CONTAINER_OF(cls, struct bt_l2cap_br_chan, chan);
			br_cls->tx.mtu = sys_le16_to_cpu(net_buf_pull_le16(buf));
			if (br_cls->tx.mtu < L2CAP_BR_MIN_MTU) {
				br_cls->tx.mtu = L2CAP_BR_MIN_MTU;
			}
		}
		break;
#endif /* CONFIG_BT_L2CAP_CLS */
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
#if defined(CONFIG_BT_L2CAP_CLS)
	struct bt_l2cap_chan *cls;
	struct bt_l2cap_br_chan *br_cls;
#endif /* CONFIG_BT_L2CAP_CLS */

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
		net_buf_add_le32(rsp_buf, L2CAP_EXTENDED_FEAT_MASK);
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
#if defined(CONFIG_BT_L2CAP_CLS)
	case BT_L2CAP_INFO_CLS_MTU_MASK:
		rsp->type = sys_cpu_to_le16(BT_L2CAP_INFO_CLS_MTU_MASK);
		cls = bt_l2cap_br_lookup_rx_cid(l2cap->chan.chan.conn, BT_L2CAP_CID_CLS);
		if (cls) {
			br_cls = CONTAINER_OF(cls, struct bt_l2cap_br_chan, chan);
			rsp->result = sys_cpu_to_le16(BT_L2CAP_INFO_SUCCESS);
			net_buf_add_le16(rsp_buf, br_cls->rx.mtu);
			hdr_info->len = sys_cpu_to_le16(sizeof(*rsp) + sizeof(br_cls->rx.mtu));
		} else {
			rsp->result = sys_cpu_to_le16(BT_L2CAP_INFO_NOTSUPP);
			hdr_info->len = sys_cpu_to_le16(sizeof(*rsp));
		}
		break;
#endif /* CONFIG_BT_L2CAP_CLS */
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
	if (net_buf_tailroom(buf) < (sizeof(struct bt_l2cap_conf_opt) + sizeof(mtu))) {
		LOG_WRN("No tail room for opt mtu");
		return;
	}
	net_buf_add_u8(buf, BT_L2CAP_CONF_OPT_MTU);
	net_buf_add_u8(buf, sizeof(mtu));
	net_buf_add_le16(buf, mtu);
}

#if L2CAP_BR_RET_FC_ENABLE
static void l2cap_br_conf_add_ret_fc(struct net_buf *buf,
				     const struct bt_l2cap_conf_opt_ret_fc *ret_fc)
{
	if (net_buf_tailroom(buf) < (sizeof(struct bt_l2cap_conf_opt) + sizeof(*ret_fc))) {
		LOG_WRN("No tail room for opt flash_timeout");
		return;
	}
	net_buf_add_u8(buf, BT_L2CAP_CONF_OPT_RET_FC);
	net_buf_add_u8(buf, sizeof(*ret_fc));
	net_buf_add_mem(buf, ret_fc, sizeof(*ret_fc));
}

static void l2cap_br_conf_add_fcs(struct net_buf *buf, const struct bt_l2cap_conf_opt_fcs *fcs)
{
	if (net_buf_tailroom(buf) < (sizeof(struct bt_l2cap_conf_opt) + sizeof(*fcs))) {
		LOG_WRN("No tail room for opt flash_timeout");
		return;
	}
	net_buf_add_u8(buf, BT_L2CAP_CONF_OPT_FCS);
	net_buf_add_u8(buf, sizeof(*fcs));
	net_buf_add_mem(buf, fcs, sizeof(*fcs));
}

static void l2cap_br_conf_add_ext_win_size(struct net_buf *buf,
				const struct bt_l2cap_conf_opt_ext_win_size *ext_win_size)
{
	if (net_buf_tailroom(buf) < (sizeof(struct bt_l2cap_conf_opt) + sizeof(*ext_win_size))) {
		LOG_WRN("No tail room for opt flash_timeout");
		return;
	}
	net_buf_add_u8(buf, BT_L2CAP_CONF_OPT_EXT_WIN_SIZE);
	net_buf_add_u8(buf, sizeof(*ext_win_size));
	net_buf_add_mem(buf, ext_win_size, sizeof(*ext_win_size));
}
#endif /* L2CAP_BR_RET_FC_ENABLE */

static void l2cap_br_conf_add_opt(struct net_buf *buf, const struct bt_l2cap_conf_opt *opt)
{
	if (net_buf_tailroom(buf) < (sizeof(*opt) + opt->len)) {
		LOG_WRN("No tail room for opt flash_timeout");
		return;
	}

	net_buf_add_u8(buf, opt->type & BT_L2CAP_CONF_MASK);
	net_buf_add_u8(buf, opt->len);
	net_buf_add_mem(buf, opt->data, opt->len);
}

#if L2CAP_BR_RET_FC_ENABLE
static int l2cap_br_check_chan_config(struct bt_conn *conn, struct bt_l2cap_br_chan *br_chan)
{
	struct bt_l2cap_chan *chan_sig;
	struct bt_l2cap_br *br_chan_sig;

	if (!conn) {
		return -ENOTCONN;
	}

	chan_sig = bt_l2cap_br_lookup_rx_cid(conn, BT_L2CAP_CID_BR_SIG);
	if (!chan_sig) {
		LOG_WRN("Ignoring data for unknown channel ID 0x%04x", BT_L2CAP_CID_BR_SIG);
		return -EINVAL;
	}

	br_chan_sig = CONTAINER_OF(chan_sig, struct bt_l2cap_br, chan.chan);

	/* Disable segment/reassemble of l2cap rx pdu */
	br_chan->rx.mtu = MIN(br_chan->rx.mtu, BT_L2CAP_RX_MTU);
	br_chan->rx.mps = br_chan->rx.mtu;

	br_chan->tx.mps = L2CAP_BR_DEFAULT_MTU;
	br_chan->tx.mtu = L2CAP_BR_DEFAULT_MTU;
	/* Set FCS to default value */
	br_chan->tx.fcs = BT_L2CAP_BR_FCS_16BIT;

	/* Set default value */
	br_chan->rx.window = CONFIG_BT_L2CAP_MAX_WINDOW_SIZE;
	br_chan->rx.ret_timeout = CONFIG_BT_L2CAP_BR_RET_TIMEOUT;
	br_chan->rx.monitor_timeout = CONFIG_BT_L2CAP_BR_MONITOR_TIMEOUT;

	br_chan->tx.window = CONFIG_BT_L2CAP_MAX_WINDOW_SIZE;
	br_chan->tx.ret_timeout = CONFIG_BT_L2CAP_BR_RET_TIMEOUT;
	br_chan->tx.monitor_timeout = CONFIG_BT_L2CAP_BR_MONITOR_TIMEOUT;

	/* Set all variables and sequence numbers to zero */
	br_chan->tx_seq = 0;
	br_chan->next_tx_seq = 0;
	br_chan->expected_ack_seq = 0;
	br_chan->req_seq = 0;
	br_chan->expected_tx_seq = 0;
	br_chan->buffer_seq = 0;

	memset(&br_chan->tx_win[0], 0, sizeof(br_chan->tx_win));

	sys_slist_init(&br_chan->_pdu_outstanding);
	k_fifo_init(&br_chan->_free_tx_win);

	for (int i = 0; i < ARRAY_SIZE(br_chan->tx_win); i++) {
		k_fifo_put(&br_chan->_free_tx_win, &br_chan->tx_win[i]);
	}

	switch (br_chan->rx.mode) {
	case BT_L2CAP_BR_LINK_MODE_BASIC:
		break;
	case BT_L2CAP_BR_LINK_MODE_RET:
		__fallthrough;
	case BT_L2CAP_BR_LINK_MODE_FC:
		if ((br_chan->rx.mode == BT_L2CAP_BR_LINK_MODE_FC) &&
		    (!((br_chan_sig->info_feat_mask & L2CAP_FEAT_FC_MASK) &&
		       L2CAP_EXTENDED_FEAT_MASK & L2CAP_FEAT_FC_MASK))) {
			if (!br_chan->rx.optional) {
				LOG_ERR("Invalid mode %u", br_chan->rx.mode);
				return -ENOTSUP;
			}

			LOG_WRN("Unsupp mode %u, set basic mode", br_chan->rx.mode);
			br_chan->rx.mode = BT_L2CAP_BR_LINK_MODE_BASIC;
			break;
		}

		if ((br_chan->rx.mode == BT_L2CAP_BR_LINK_MODE_RET) &&
		    (!((br_chan_sig->info_feat_mask & L2CAP_FEAT_RET_MASK) &&
		       (L2CAP_EXTENDED_FEAT_MASK & L2CAP_FEAT_RET_MASK)))) {
			if (!br_chan->rx.optional) {
				LOG_ERR("Invalid mode %u", br_chan->rx.mode);
				return -ENOTSUP;
			}

			LOG_WRN("Unsupp mode %u, set basic mode", br_chan->rx.mode);
			br_chan->rx.mode = BT_L2CAP_BR_LINK_MODE_BASIC;
			break;
		}

		if ((br_chan_sig->info_feat_mask & L2CAP_FEAT_ENH_RET_MASK) &&
		    (L2CAP_EXTENDED_FEAT_MASK & L2CAP_FEAT_ENH_RET_MASK)) {
			/* If enhance transmission mode is supported by both side,
			 * the enhance transmission mode should be used instead of
			 * transmission or flow control mode.
			 */
			br_chan->rx.mode = BT_L2CAP_BR_LINK_MODE_ERET;
			br_chan->tx.mode = BT_L2CAP_BR_LINK_MODE_ERET;
		}
		break;
	case BT_L2CAP_BR_LINK_MODE_ERET:
		if (!((br_chan_sig->info_feat_mask & L2CAP_FEAT_ENH_RET_MASK) &&
		      (L2CAP_EXTENDED_FEAT_MASK & L2CAP_FEAT_ENH_RET_MASK))) {
			if (!br_chan->rx.optional) {
				LOG_ERR("Invalid mode %u", br_chan->rx.mode);
				return -ENOTSUP;
			}

			LOG_WRN("Unsupp mode %u, set basic mode", br_chan->rx.mode);
			br_chan->rx.mode = BT_L2CAP_BR_LINK_MODE_BASIC;
			break;
		}
		break;
	case BT_L2CAP_BR_LINK_MODE_STREAM:
		if (!((br_chan_sig->info_feat_mask & L2CAP_FEAT_STREAM_MASK) &&
		      (L2CAP_EXTENDED_FEAT_MASK & L2CAP_FEAT_STREAM_MASK))) {
			if (!br_chan->rx.optional) {
				LOG_ERR("Invalid mode %u", br_chan->rx.mode);
				return -ENOTSUP;
			}

			LOG_WRN("Unsupp mode %u, set basic mode", br_chan->rx.mode);
			br_chan->rx.mode = BT_L2CAP_BR_LINK_MODE_BASIC;
			break;
		}
		break;
	default:
		br_chan->rx.mode = BT_L2CAP_BR_LINK_MODE_BASIC;
		br_chan->tx.mode = BT_L2CAP_BR_LINK_MODE_BASIC;
		break;
	}

	atomic_set_bit(br_chan->flags, L2CAP_FLAG_NEW_I_FRAME);
	atomic_set_bit(br_chan->flags, L2CAP_FLAG_RET_I_FRAME);

	switch (br_chan->rx.mode) {
	case BT_L2CAP_BR_LINK_MODE_BASIC:
		br_chan->tx.fcs = 0;
		br_chan->rx.fcs = 0;
		break;
	case BT_L2CAP_BR_LINK_MODE_RET:
		__fallthrough;
	case BT_L2CAP_BR_LINK_MODE_FC:
		if (br_chan->rx.window < 1) {
			br_chan->rx.window = 1;
		} else if (br_chan->rx.window > 32) {
			br_chan->rx.window = 32;
		}

		if (br_chan->rx.transmit < 1) {
			br_chan->rx.transmit = 1;
		}

		br_chan->tx.fcs = BT_L2CAP_BR_FCS_16BIT;
		br_chan->rx.fcs = BT_L2CAP_BR_FCS_16BIT;
		break;
	case BT_L2CAP_BR_LINK_MODE_ERET:
		if (!((br_chan_sig->info_feat_mask & L2CAP_FEAT_EXT_WIN_SIZE_MASK) &&
		      (L2CAP_EXTENDED_FEAT_MASK & L2CAP_FEAT_EXT_WIN_SIZE_MASK))) {
			LOG_WRN("Unsupported extended window size");
			br_chan->rx.extended_control = 0;
			br_chan->tx.extended_control = 0;
		}

		if (br_chan->rx.window < 1) {
			br_chan->rx.window = 1;
		} else if (br_chan->rx.window > 63) {
			if (br_chan->rx.extended_control) {
				if (br_chan->rx.window > 0x3FFF) {
					br_chan->rx.window = 0x3FFF;
				}
			} else {
				br_chan->rx.window = 63;
			}
		}

		if (br_chan->rx.transmit < 1) {
			br_chan->rx.transmit = 1;
		}

		if (!((br_chan_sig->info_feat_mask & L2CAP_FEAT_FCS_MASK) &&
		      (L2CAP_EXTENDED_FEAT_MASK & L2CAP_FEAT_FCS_MASK))) {
			LOG_WRN("Unsupported FCS option, force to enable FCS");
			br_chan->tx.fcs = BT_L2CAP_BR_FCS_16BIT;
			br_chan->rx.fcs = BT_L2CAP_BR_FCS_16BIT;
		}
		break;
	case BT_L2CAP_BR_LINK_MODE_STREAM:
		if (!((br_chan_sig->info_feat_mask & L2CAP_FEAT_EXT_WIN_SIZE_MASK) &&
		      (L2CAP_EXTENDED_FEAT_MASK & L2CAP_FEAT_EXT_WIN_SIZE_MASK))) {
			LOG_WRN("Unsupported extended window size");
			br_chan->rx.extended_control = 0;
			br_chan->tx.extended_control = 0;
		}

		br_chan->rx.window = 0;
		if (!((br_chan_sig->info_feat_mask & L2CAP_FEAT_FCS_MASK) &&
		      (L2CAP_EXTENDED_FEAT_MASK & L2CAP_FEAT_FCS_MASK))) {
			LOG_WRN("Unsupported FCS option, force to enable FCS");
			br_chan->tx.fcs = BT_L2CAP_BR_FCS_16BIT;
			br_chan->rx.fcs = BT_L2CAP_BR_FCS_16BIT;
		}
		break;
	default:
		/* Should not have gotten here */
		break;
	}

	return 0;
}
#endif /* L2CAP_BR_RET_FC_ENABLE */

static void l2cap_br_conf(struct bt_l2cap_chan *chan)
{
	struct bt_l2cap_sig_hdr *hdr;
	struct bt_l2cap_conf_req *conf;
	struct net_buf *buf;

#if L2CAP_BR_RET_FC_ENABLE
	int err;

	err = l2cap_br_check_chan_config(chan->conn, BR_CHAN(chan));
	if (err) {
		bt_l2cap_chan_disconnect(chan);
		return;
	}
#endif /* L2CAP_BR_RET_FC_ENABLE */

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

#if L2CAP_BR_RET_FC_ENABLE
	if (BR_CHAN(chan)->rx.mode != BT_L2CAP_BR_LINK_MODE_BASIC) {
		struct bt_l2cap_conf_opt_ret_fc ret_fc;
		struct bt_l2cap_conf_opt_fcs fcs;

		ret_fc.mode = BR_CHAN(chan)->rx.mode;
		ret_fc.tx_windows_size = BR_CHAN(chan)->rx.window;
		ret_fc.max_transmit = BR_CHAN(chan)->rx.transmit;
		ret_fc.retransmission_timeout = BR_CHAN(chan)->rx.ret_timeout;
		ret_fc.monitor_timeout = BR_CHAN(chan)->rx.monitor_timeout;
		ret_fc.mps = BR_CHAN(chan)->rx.mps;
		l2cap_br_conf_add_ret_fc(buf, &ret_fc);

		fcs.type = BR_CHAN(chan)->rx.fcs;
		if (fcs.type == BT_L2CAP_FCS_TYPE_NO) {
			l2cap_br_conf_add_fcs(buf, &fcs);
		}

		if ((BR_CHAN(chan)->rx.mode == BT_L2CAP_BR_LINK_MODE_ERET) ||
		    (BR_CHAN(chan)->rx.mode == BT_L2CAP_BR_LINK_MODE_STREAM)) {
			if (BR_CHAN(chan)->rx.extended_control) {
				struct bt_l2cap_conf_opt_ext_win_size ext_win_size;

				ext_win_size.max_windows_size = BR_CHAN(chan)->rx.window;
				l2cap_br_conf_add_ext_win_size(buf, &ext_win_size);
			}
		}
	}
#endif /* L2CAP_BR_RET_FC_ENABLE */

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
		const sys_snode_t *tx_buf = sys_slist_get(&br_chan->_pdu_tx_queue);

		struct net_buf *buf = CONTAINER_OF(tx_buf, struct net_buf, node);

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

#if !L2CAP_BR_RET_FC_ENABLE
	/* Disable fragmentation of l2cap rx pdu */
	BR_CHAN(chan)->rx.mtu = MIN(BR_CHAN(chan)->rx.mtu, BT_L2CAP_RX_MTU);
#endif /* L2CAP_BR_RET_FC_ENABLE */

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
			/* Recovery the ident when conn is pending */
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

#if L2CAP_BR_RET_FC_ENABLE
static uint16_t l2cap_br_conf_rsp_opt_mtu(struct bt_l2cap_chan *chan,
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
		opt_mtu->mtu = sys_cpu_to_le16(BR_CHAN(chan)->rx.mtu);
		LOG_DBG("tx MTU %u invalid", mtu);
		goto done;
	}

	if (mtu < BR_CHAN(chan)->rx.mtu) {
		BR_CHAN(chan)->rx.mtu = mtu;
	}

	LOG_DBG("rx MTU %u", BR_CHAN(chan)->rx.mtu);
done:
	return result;
}

static uint16_t l2cap_br_conf_rsp_opt_flush_timeout(struct bt_l2cap_chan *chan,
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

	LOG_DBG("Flash timeout %u", opt_to->timeout);

	opt_to->timeout = sys_cpu_to_le32(0xFFFF);
	result = BT_L2CAP_CONF_UNACCEPT;
done:
	return result;
}

static uint16_t l2cap_br_conf_rsp_opt_qos(struct bt_l2cap_chan *chan,
				   struct net_buf *buf, size_t len)
{
	uint16_t result = BT_L2CAP_CONF_SUCCESS;
	struct bt_l2cap_conf_opt_qos *opt_qos;
	struct bt_l2cap_chan *chan_sig;
	struct bt_l2cap_br *br_chan_sig;

	chan_sig = bt_l2cap_br_lookup_rx_cid(chan->conn, BT_L2CAP_CID_BR_SIG);
	if (!chan_sig) {
		LOG_WRN("Ignoring data for unknown channel ID 0x%04x", BT_L2CAP_CID_BR_SIG);
		result = BT_L2CAP_CONF_REJECT;
		goto done;
	}

	br_chan_sig = CONTAINER_OF(chan_sig, struct bt_l2cap_br, chan.chan);
	if (!((br_chan_sig->info_feat_mask & L2CAP_FEAT_QOS_MASK) &&
	      (L2CAP_EXTENDED_FEAT_MASK & L2CAP_FEAT_QOS_MASK))) {
		LOG_WRN("Unsupported extended flow spec");
		result = BT_L2CAP_CONF_REJECT;
		goto done;
	}

	if (len != sizeof(*opt_qos)) {
		LOG_ERR("qos frame length %zu invalid", len);
		result = BT_L2CAP_CONF_REJECT;
		goto done;
	}

	opt_qos = (struct bt_l2cap_conf_opt_qos *)buf->data;

	LOG_DBG("QOS Type %u", opt_qos->service_type);

	if (opt_qos->service_type == BT_L2CAP_QOS_TYPE_GUARANTEED) {
		result = BT_L2CAP_CONF_UNACCEPT;
		/* Set to default value */
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

static uint16_t l2cap_br_conf_rsp_opt_ret_fc(struct bt_l2cap_chan *chan,
				   struct net_buf *buf, size_t len)
{
	uint16_t result = BT_L2CAP_CONF_UNACCEPT;
	struct bt_l2cap_conf_opt_ret_fc *opt_ret_fc;
	struct bt_l2cap_br_chan *br_chan;
	struct bt_l2cap_chan *chan_sig;
	struct bt_l2cap_br *br_chan_sig;
	bool accept = true;

	if (len != sizeof(*opt_ret_fc)) {
		LOG_ERR("ret_fc frame length %zu invalid", len);
		result = BT_L2CAP_CONF_REJECT;
		goto done;
	}

	chan_sig = bt_l2cap_br_lookup_rx_cid(chan->conn, BT_L2CAP_CID_BR_SIG);
	if (!chan_sig) {
		LOG_WRN("Ignoring data for unknown channel ID 0x%04x", BT_L2CAP_CID_BR_SIG);
		result = BT_L2CAP_CONF_REJECT;
		goto done;
	}

	br_chan_sig = CONTAINER_OF(chan_sig, struct bt_l2cap_br, chan.chan);

	opt_ret_fc = (struct bt_l2cap_conf_opt_ret_fc *)buf->data;

	LOG_DBG("ret_fc mode %u", opt_ret_fc->mode);

	br_chan = BR_CHAN(chan);

	if (opt_ret_fc->mode != br_chan->rx.mode) {
		if (br_chan->rx.optional) {
			br_chan->rx.mode = opt_ret_fc->mode;
		} else {
			opt_ret_fc->mode = br_chan->rx.mode;
			accept = false;
		}
	}

	if (opt_ret_fc->mode != BT_L2CAP_BR_LINK_MODE_BASIC) {
		if (sys_le16_to_cpu(opt_ret_fc->mps) < br_chan->rx.mps) {
			br_chan->rx.mps = sys_le16_to_cpu(opt_ret_fc->mps);
		} else {
			opt_ret_fc->mps = sys_cpu_to_le16(br_chan->rx.mps);
		}

		if (opt_ret_fc->max_transmit < 1) {
			opt_ret_fc->max_transmit = 1;
			accept = false;
		}

		if ((opt_ret_fc->mode == BT_L2CAP_BR_LINK_MODE_RET) ||
		    (opt_ret_fc->mode == BT_L2CAP_BR_LINK_MODE_FC)) {
			if ((opt_ret_fc->tx_windows_size < 1) ||
			    (opt_ret_fc->tx_windows_size > 32)) {
				opt_ret_fc->tx_windows_size = CONFIG_BT_L2CAP_MAX_WINDOW_SIZE;
				accept = false;
			}
		}

		if (opt_ret_fc->mode == BT_L2CAP_BR_LINK_MODE_ERET) {
			if ((opt_ret_fc->tx_windows_size < 1) ||
			    (opt_ret_fc->tx_windows_size > 63)) {
				opt_ret_fc->tx_windows_size = CONFIG_BT_L2CAP_MAX_WINDOW_SIZE;
				accept = false;
			}
		}

		opt_ret_fc->retransmission_timeout =
			sys_cpu_to_le16(CONFIG_BT_L2CAP_BR_RET_TIMEOUT);
		opt_ret_fc->monitor_timeout = sys_cpu_to_le16(CONFIG_BT_L2CAP_BR_MONITOR_TIMEOUT);

		if (opt_ret_fc->tx_windows_size > CONFIG_BT_L2CAP_MAX_WINDOW_SIZE) {
			opt_ret_fc->tx_windows_size =
				sys_cpu_to_le16(CONFIG_BT_L2CAP_MAX_WINDOW_SIZE);
		}

		if (!accept) {
			goto done;
		}

		br_chan->rx.ret_timeout = CONFIG_BT_L2CAP_BR_RET_TIMEOUT;
		br_chan->rx.monitor_timeout = CONFIG_BT_L2CAP_BR_MONITOR_TIMEOUT;
		br_chan->rx.transmit = opt_ret_fc->max_transmit;
		br_chan->rx.window = sys_le16_to_cpu(opt_ret_fc->tx_windows_size);
		br_chan->rx.mps = sys_le16_to_cpu(opt_ret_fc->mps);
	}

	if (!accept) {
		goto done;
	}

	result = BT_L2CAP_CONF_SUCCESS;

done:
	return result;
}

static uint16_t l2cap_br_conf_rsp_opt_fcs(struct bt_l2cap_chan *chan,
				   struct net_buf *buf, size_t len)
{
	uint16_t result = BT_L2CAP_CONF_SUCCESS;
	struct bt_l2cap_conf_opt_fcs *opt_fcs;
	struct bt_l2cap_br_chan *br_chan;

	if (len != sizeof(*opt_fcs)) {
		LOG_ERR("fcs frame length %zu invalid", len);
		result = BT_L2CAP_CONF_REJECT;
		goto done;
	}

	opt_fcs = (struct bt_l2cap_conf_opt_fcs *)buf->data;

	LOG_DBG("FCS type %u", opt_fcs->type);

	if ((opt_fcs->type != BT_L2CAP_FCS_TYPE_NO)
	 && (opt_fcs->type != BT_L2CAP_FCS_TYPE_16BIT)) {
		LOG_ERR("fcs type %u invalid", opt_fcs->type);
		result = BT_L2CAP_CONF_REJECT;
		goto done;
	}

	br_chan = BR_CHAN(chan);

	switch (br_chan->rx.mode) {
	case BT_L2CAP_BR_LINK_MODE_BASIC:
		result = BT_L2CAP_CONF_REJECT;
		goto done;
	case BT_L2CAP_BR_LINK_MODE_RET:
		__fallthrough;
	case BT_L2CAP_BR_LINK_MODE_FC:
		LOG_ERR("fcs opt is mandatory in mode %u", br_chan->rx.mode);
		if (opt_fcs->type != BT_L2CAP_BR_FCS_16BIT) {
			result = BT_L2CAP_CONF_UNACCEPT;
			opt_fcs->type = BT_L2CAP_BR_FCS_16BIT;
			br_chan->rx.fcs = opt_fcs->type;
			goto done;
		}
		break;
	case BT_L2CAP_BR_LINK_MODE_ERET:
		__fallthrough;
	case BT_L2CAP_BR_LINK_MODE_STREAM:
		br_chan->rx.fcs = opt_fcs->type;
		break;
	default:
		/* Should not have been here */
		LOG_ERR("Unknown mode %u", br_chan->rx.mode);
		break;
	}

done:
	return result;
}

static uint16_t l2cap_br_conf_rsp_opt_ext_flow_spec(struct bt_l2cap_chan *chan,
				   struct net_buf *buf, size_t len)
{
	uint16_t result = BT_L2CAP_CONF_SUCCESS;
	struct bt_l2cap_conf_opt_ext_flow_spec *ext_flow_spec;
	struct bt_l2cap_br_chan *br_chan;

	if (len != sizeof(*ext_flow_spec)) {
		LOG_ERR("fcs frame length %zu invalid", len);
		result = BT_L2CAP_CONF_REJECT;
		goto done;
	}

	ext_flow_spec = (struct bt_l2cap_conf_opt_ext_flow_spec *)buf->data;

	LOG_DBG("Ext Flow Spec %u %u %u %u %u %u", ext_flow_spec->identifier,
		ext_flow_spec->service_type, ext_flow_spec->sdu,
		ext_flow_spec->sdu_inter_arrival_time, ext_flow_spec->access_latency,
		ext_flow_spec->flush_timeout);

	br_chan = BR_CHAN(chan);

	/* Set to default value */
	result = BT_L2CAP_CONF_UNACCEPT;
	ext_flow_spec->identifier = 0x01;
	ext_flow_spec->service_type = 0x01;
	ext_flow_spec->sdu = sys_cpu_to_le16(0xFFFF);
	ext_flow_spec->sdu_inter_arrival_time = sys_cpu_to_le32(0xFFFFFFFF);
	ext_flow_spec->access_latency = sys_cpu_to_le32(0xFFFFFFFF);
	ext_flow_spec->flush_timeout = sys_cpu_to_le32(0xFFFFFFFF);

done:
	return result;
}

static uint16_t l2cap_br_conf_rsp_opt_ext_win_size(struct bt_l2cap_chan *chan,
				   struct net_buf *buf, size_t len)
{
	uint16_t result = BT_L2CAP_CONF_SUCCESS;
	struct bt_l2cap_conf_opt_ext_win_size *opt_ext_win_size;
	struct bt_l2cap_br_chan *br_chan;
	uint16_t win_size;

	if (len != sizeof(*opt_ext_win_size)) {
		LOG_ERR("fcs frame length %zu invalid", len);
		result = BT_L2CAP_CONF_REJECT;
		goto done;
	}

	opt_ext_win_size = (struct bt_l2cap_conf_opt_ext_win_size *)buf->data;
	win_size = sys_le16_to_cpu(opt_ext_win_size->max_windows_size);

	LOG_DBG("EXT Win Size value %u", win_size);

	br_chan = BR_CHAN(chan);

	switch (br_chan->rx.mode) {
	case BT_L2CAP_BR_LINK_MODE_BASIC:
		__fallthrough;
	case BT_L2CAP_BR_LINK_MODE_RET:
		__fallthrough;
	case BT_L2CAP_BR_LINK_MODE_FC:
		/* ignore */
		LOG_WRN("Ignored by mode %u", br_chan->rx.mode);
		break;
	case BT_L2CAP_BR_LINK_MODE_ERET:
		if ((win_size < 1) || (win_size > CONFIG_BT_L2CAP_MAX_WINDOW_SIZE)) {
			/* Set to default value */
			opt_ext_win_size->max_windows_size =
				sys_cpu_to_be16(CONFIG_BT_L2CAP_MAX_WINDOW_SIZE);
			result = BT_L2CAP_CONF_UNACCEPT;
			goto done;
		}

		br_chan->tx.extended_control = true;
		break;
	case BT_L2CAP_BR_LINK_MODE_STREAM:
		if (win_size) {
			result = BT_L2CAP_CONF_UNACCEPT;
			opt_ext_win_size->max_windows_size = 0;
			goto done;
		}
		br_chan->tx.extended_control = true;
		break;
	default:
		/* Should not have been here */
		LOG_ERR("Unknown mode %u", br_chan->rx.mode);
		break;
	}

done:
	return result;
}

static int l2cap_br_conf_rsp_opt_check(struct bt_l2cap_chan *chan,
				uint16_t opt_len, struct net_buf *buf)
{
	struct bt_l2cap_conf_opt *opt = NULL;
	uint16_t result = BT_L2CAP_CONF_SUCCESS;
	uint8_t hint;

	if (opt_len > buf->len) {
		return -EINVAL;
	}

	while (opt_len >= sizeof(*opt)) {
		opt = net_buf_pull_mem(buf, sizeof(*opt));

		opt_len -= sizeof(*opt);

		if (opt_len < opt->len) {
			LOG_ERR("Received too short option data");
			return -EINVAL;
		}

		opt_len -= opt->len;

		hint = opt->type & BT_L2CAP_CONF_HINT;
		if (hint) {
			return -EINVAL;
		}

		switch (opt->type & BT_L2CAP_CONF_MASK) {
		case BT_L2CAP_CONF_OPT_MTU:
			/* getting MTU modifies buf internals */
			result = l2cap_br_conf_rsp_opt_mtu(chan, buf, opt->len);
			if (result != BT_L2CAP_CONF_SUCCESS) {
				goto invalid_opt;
			}
			break;
		case BT_L2CAP_CONF_OPT_FLUSH_TIMEOUT:
			result = l2cap_br_conf_rsp_opt_flush_timeout(chan, buf, opt->len);
			if (result != BT_L2CAP_CONF_SUCCESS) {
				goto invalid_opt;
			}
			break;
		case BT_L2CAP_CONF_OPT_QOS:
			result = l2cap_br_conf_rsp_opt_qos(chan, buf, opt->len);
			if (result != BT_L2CAP_CONF_SUCCESS) {
				goto invalid_opt;
			}
			break;
		case BT_L2CAP_CONF_OPT_RET_FC:
			result = l2cap_br_conf_rsp_opt_ret_fc(chan, buf, opt->len);
			if (result != BT_L2CAP_CONF_SUCCESS) {
				goto invalid_opt;
			}
			break;
		case BT_L2CAP_CONF_OPT_FCS:
			result = l2cap_br_conf_rsp_opt_fcs(chan, buf, opt->len);
			if (result != BT_L2CAP_CONF_SUCCESS) {
				goto invalid_opt;
			}
			break;
		case BT_L2CAP_CONF_OPT_EXT_FLOW_SPEC:
			result = l2cap_br_conf_rsp_opt_ext_flow_spec(chan, buf, opt->len);
			if (result != BT_L2CAP_CONF_SUCCESS) {
				goto invalid_opt;
			}
			break;
		case BT_L2CAP_CONF_OPT_EXT_WIN_SIZE:
			result = l2cap_br_conf_rsp_opt_ext_win_size(chan, buf, opt->len);
			if (result != BT_L2CAP_CONF_SUCCESS) {
				goto invalid_opt;
			}
			break;
		default:
			result = BT_L2CAP_CONF_UNKNOWN_OPT;
			goto invalid_opt;
		}
		net_buf_pull(buf, opt->len);
	}

	if (BR_CHAN(chan)->rx.fcs == BT_L2CAP_BR_FCS_16BIT) {
		/* If local enable FCS, peer also needs to enable it. */
		BR_CHAN(chan)->tx.fcs = BT_L2CAP_BR_FCS_16BIT;
	}

	if (BR_CHAN(chan)->rx.extended_control) {
		/* If peer enables extended control field,
		 * local also needs to enable it.
		 */
		BR_CHAN(chan)->tx.extended_control = true;
	}

	return 0;

invalid_opt:
	return -EINVAL;
}

static uint16_t l2cap_br_conf_rsp_unaccept_opt_mtu(struct bt_l2cap_chan *chan,
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
		LOG_DBG("tx MTU %u invalid", mtu);
		goto done;
	}

	if (mtu < BR_CHAN(chan)->rx.mtu) {
		BR_CHAN(chan)->rx.mtu = mtu;
	}

done:
	LOG_DBG("rx MTU %u", BR_CHAN(chan)->rx.mtu);
	return result;
}

static uint16_t l2cap_br_conf_rsp_unaccept_opt_flush_timeout(struct bt_l2cap_chan *chan,
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

	LOG_DBG("Flash timeout %u", opt_to->timeout);

done:
	return result;
}

static uint16_t l2cap_br_conf_rsp_unaccept_opt_qos(struct bt_l2cap_chan *chan,
				   struct net_buf *buf, size_t len)
{
	uint16_t result = BT_L2CAP_CONF_SUCCESS;
	struct bt_l2cap_conf_opt_qos *opt_qos;
	struct bt_l2cap_chan *chan_sig;
	struct bt_l2cap_br *br_chan_sig;

	chan_sig = bt_l2cap_br_lookup_rx_cid(chan->conn, BT_L2CAP_CID_BR_SIG);
	if (!chan_sig) {
		LOG_WRN("Ignoring data for unknown channel ID 0x%04x", BT_L2CAP_CID_BR_SIG);
		result = BT_L2CAP_CONF_REJECT;
		goto done;
	}

	br_chan_sig = CONTAINER_OF(chan_sig, struct bt_l2cap_br, chan.chan);
	if (!((br_chan_sig->info_feat_mask & L2CAP_FEAT_QOS_MASK) &&
	      (L2CAP_EXTENDED_FEAT_MASK & L2CAP_FEAT_QOS_MASK))) {
		LOG_WRN("Unsupported extended flow spec");
		result = BT_L2CAP_CONF_REJECT;
		goto done;
	}

	if (len != sizeof(*opt_qos)) {
		LOG_ERR("qos frame length %zu invalid", len);
		result = BT_L2CAP_CONF_REJECT;
		goto done;
	}

	opt_qos = (struct bt_l2cap_conf_opt_qos *)buf->data;

	LOG_DBG("QOS Type %u", opt_qos->service_type);

done:
	return result;
}

static uint16_t l2cap_br_conf_rsp_unaccept_opt_ret_fc(struct bt_l2cap_chan *chan,
				   struct net_buf *buf, size_t len)
{
	uint16_t result = BT_L2CAP_CONF_UNACCEPT;
	struct bt_l2cap_conf_opt_ret_fc *opt_ret_fc;
	struct bt_l2cap_br_chan *br_chan;

	if (len != sizeof(*opt_ret_fc)) {
		LOG_ERR("ret_fc frame length %zu invalid", len);
		result = BT_L2CAP_CONF_REJECT;
		goto done;
	}

	opt_ret_fc = (struct bt_l2cap_conf_opt_ret_fc *)buf->data;

	LOG_DBG("ret_fc mode %u", opt_ret_fc->mode);

	br_chan = BR_CHAN(chan);

	if (opt_ret_fc->mode != br_chan->rx.mode) {
		if (br_chan->rx.optional) {
			br_chan->rx.mode = opt_ret_fc->mode;
		} else {
			goto done;
		}
	}

	if (opt_ret_fc->mode != BT_L2CAP_BR_LINK_MODE_BASIC) {
		if (sys_le16_to_cpu(opt_ret_fc->mps) > br_chan->rx.mtu) {
			opt_ret_fc->mps = sys_cpu_to_le16(br_chan->rx.mtu);
		}

		if (opt_ret_fc->max_transmit < 1) {
			opt_ret_fc->max_transmit = 1;
		}

		if ((opt_ret_fc->mode == BT_L2CAP_BR_LINK_MODE_RET) ||
		    (opt_ret_fc->mode == BT_L2CAP_BR_LINK_MODE_FC)) {
			if ((opt_ret_fc->tx_windows_size < 1) ||
			    (opt_ret_fc->tx_windows_size > 32)) {
				opt_ret_fc->tx_windows_size = CONFIG_BT_L2CAP_MAX_WINDOW_SIZE;
			}
		}

		if (opt_ret_fc->mode == BT_L2CAP_BR_LINK_MODE_ERET) {
			if ((opt_ret_fc->tx_windows_size < 1) ||
			    (opt_ret_fc->tx_windows_size > 63)) {
				opt_ret_fc->tx_windows_size = CONFIG_BT_L2CAP_MAX_WINDOW_SIZE;
			}
		}

		opt_ret_fc->retransmission_timeout =
			sys_cpu_to_le16(CONFIG_BT_L2CAP_BR_RET_TIMEOUT);
		opt_ret_fc->monitor_timeout = sys_cpu_to_le16(CONFIG_BT_L2CAP_BR_MONITOR_TIMEOUT);

		if (opt_ret_fc->tx_windows_size > CONFIG_BT_L2CAP_MAX_WINDOW_SIZE) {
			opt_ret_fc->tx_windows_size =
				sys_cpu_to_le16(CONFIG_BT_L2CAP_MAX_WINDOW_SIZE);
		}

		br_chan->rx.ret_timeout = CONFIG_BT_L2CAP_BR_RET_TIMEOUT;
		br_chan->rx.monitor_timeout = CONFIG_BT_L2CAP_BR_MONITOR_TIMEOUT;
		br_chan->rx.transmit = opt_ret_fc->max_transmit;
		br_chan->rx.window = sys_le16_to_cpu(opt_ret_fc->tx_windows_size);
		br_chan->rx.mps = sys_le16_to_cpu(opt_ret_fc->mps);
	}

	result = BT_L2CAP_CONF_SUCCESS;

done:
	return result;
}

static uint16_t l2cap_br_conf_rsp_unaccept_opt_fcs(struct bt_l2cap_chan *chan,
				   struct net_buf *buf, size_t len)
{
	uint16_t result = BT_L2CAP_CONF_SUCCESS;
	struct bt_l2cap_conf_opt_fcs *opt_fcs;
	struct bt_l2cap_br_chan *br_chan;

	if (len != sizeof(*opt_fcs)) {
		LOG_ERR("fcs frame length %zu invalid", len);
		result = BT_L2CAP_CONF_REJECT;
		goto done;
	}

	opt_fcs = (struct bt_l2cap_conf_opt_fcs *)buf->data;

	LOG_DBG("FCS type %u", opt_fcs->type);

	if ((opt_fcs->type != BT_L2CAP_FCS_TYPE_NO) &&
	    (opt_fcs->type != BT_L2CAP_FCS_TYPE_16BIT)) {
		LOG_ERR("fcs type %u invalid", opt_fcs->type);
		result = BT_L2CAP_CONF_REJECT;
		goto done;
	}

	br_chan = BR_CHAN(chan);

	switch (br_chan->rx.mode) {
	case BT_L2CAP_BR_LINK_MODE_BASIC:
		goto done;
	case BT_L2CAP_BR_LINK_MODE_RET:
		__fallthrough;
	case BT_L2CAP_BR_LINK_MODE_FC:
		LOG_ERR("fcs opt is mandatory in mode %u", br_chan->rx.mode);
		br_chan->rx.fcs = BT_L2CAP_BR_FCS_16BIT;
		break;
	case BT_L2CAP_BR_LINK_MODE_ERET:
		__fallthrough;
	case BT_L2CAP_BR_LINK_MODE_STREAM:
		br_chan->rx.fcs = opt_fcs->type;
		break;
	default:
		/* Should not have been here */
		LOG_ERR("Unknown mode %u", br_chan->rx.mode);
		break;
	}

done:
	return result;
}

static uint16_t l2cap_br_conf_rsp_unaccept_opt_ext_flow_spec(struct bt_l2cap_chan *chan,
				   struct net_buf *buf, size_t len)
{
	uint16_t result = BT_L2CAP_CONF_SUCCESS;
	struct bt_l2cap_conf_opt_ext_flow_spec *ext_flow_spec;

	if (len != sizeof(*ext_flow_spec)) {
		LOG_ERR("fcs frame length %zu invalid", len);
		result = BT_L2CAP_CONF_REJECT;
		goto done;
	}

	ext_flow_spec = (struct bt_l2cap_conf_opt_ext_flow_spec *)buf->data;

	LOG_DBG("Ext Flow Spec %u %u %u %u %u %u", ext_flow_spec->identifier,
		ext_flow_spec->service_type, ext_flow_spec->sdu,
		ext_flow_spec->sdu_inter_arrival_time, ext_flow_spec->access_latency,
		ext_flow_spec->flush_timeout);
done:
	return result;
}

static uint16_t l2cap_br_conf_rsp_unaccept_opt_ext_win_size(struct bt_l2cap_chan *chan,
				   struct net_buf *buf, size_t len)
{
	uint16_t result = BT_L2CAP_CONF_SUCCESS;
	struct bt_l2cap_conf_opt_ext_win_size *opt_ext_win_size;
	struct bt_l2cap_br_chan *br_chan;
	uint16_t win_size;

	if (len != sizeof(*opt_ext_win_size)) {
		LOG_ERR("fcs frame length %zu invalid", len);
		result = BT_L2CAP_CONF_REJECT;
		goto done;
	}

	opt_ext_win_size = (struct bt_l2cap_conf_opt_ext_win_size *)buf->data;
	win_size = sys_le16_to_cpu(opt_ext_win_size->max_windows_size);

	LOG_DBG("EXT Win Size value %u", win_size);

	br_chan = BR_CHAN(chan);

	switch (br_chan->rx.mode) {
	case BT_L2CAP_BR_LINK_MODE_BASIC:
		__fallthrough;
	case BT_L2CAP_BR_LINK_MODE_RET:
		__fallthrough;
	case BT_L2CAP_BR_LINK_MODE_FC:
		/* ignore */
		LOG_WRN("Ignored by mode %u", br_chan->rx.mode);
		break;
	case BT_L2CAP_BR_LINK_MODE_ERET:
		if ((win_size < 1) || (win_size > CONFIG_BT_L2CAP_MAX_WINDOW_SIZE)) {
			/* Set to default value */
			br_chan->rx.window = CONFIG_BT_L2CAP_MAX_WINDOW_SIZE;
		} else {
			br_chan->rx.window = win_size;
		}
		break;
	case BT_L2CAP_BR_LINK_MODE_STREAM:
		br_chan->rx.window = 0;
		break;
	default:
		/* Should not have been here */
		LOG_ERR("Unknown mode %u", br_chan->rx.mode);
		break;
	}

done:
	return result;
}

static int l2cap_br_conf_rsp_unaccept_opt(struct bt_l2cap_chan *chan,
				uint16_t opt_len, struct net_buf *buf)
{
	struct bt_l2cap_conf_opt *opt = NULL;
	uint16_t result = BT_L2CAP_CONF_SUCCESS;
	uint8_t hint;

	if (opt_len > buf->len) {
		return -EINVAL;
	}

	while (opt_len >= sizeof(*opt)) {
		opt = net_buf_pull_mem(buf, sizeof(*opt));

		opt_len -= sizeof(*opt);

		if (opt_len < opt->len) {
			LOG_ERR("Received too short option data");
			return -EINVAL;
		}

		opt_len -= opt->len;

		hint = opt->type & BT_L2CAP_CONF_HINT;
		if (hint) {
			return -EINVAL;
		}

		switch (opt->type & BT_L2CAP_CONF_MASK) {
		case BT_L2CAP_CONF_OPT_MTU:
			/* getting MTU modifies buf internals */
			result = l2cap_br_conf_rsp_unaccept_opt_mtu(chan, buf, opt->len);
			if (result != BT_L2CAP_CONF_SUCCESS) {
				goto invalid_opt;
			}
			break;
		case BT_L2CAP_CONF_OPT_FLUSH_TIMEOUT:
			result = l2cap_br_conf_rsp_unaccept_opt_flush_timeout(chan, buf, opt->len);
			if (result != BT_L2CAP_CONF_SUCCESS) {
				goto invalid_opt;
			}
			break;
		case BT_L2CAP_CONF_OPT_QOS:
			result = l2cap_br_conf_rsp_unaccept_opt_qos(chan, buf, opt->len);
			if (result != BT_L2CAP_CONF_SUCCESS) {
				goto invalid_opt;
			}
			break;
		case BT_L2CAP_CONF_OPT_RET_FC:
			result = l2cap_br_conf_rsp_unaccept_opt_ret_fc(chan, buf, opt->len);
			if (result != BT_L2CAP_CONF_SUCCESS) {
				goto invalid_opt;
			}
			break;
		case BT_L2CAP_CONF_OPT_FCS:
			result = l2cap_br_conf_rsp_unaccept_opt_fcs(chan, buf, opt->len);
			if (result != BT_L2CAP_CONF_SUCCESS) {
				goto invalid_opt;
			}
			break;
		case BT_L2CAP_CONF_OPT_EXT_FLOW_SPEC:
			result = l2cap_br_conf_rsp_unaccept_opt_ext_flow_spec(chan, buf, opt->len);
			if (result != BT_L2CAP_CONF_SUCCESS) {
				goto invalid_opt;
			}
			break;
		case BT_L2CAP_CONF_OPT_EXT_WIN_SIZE:
			result = l2cap_br_conf_rsp_unaccept_opt_ext_win_size(chan, buf, opt->len);
			if (result != BT_L2CAP_CONF_SUCCESS) {
				goto invalid_opt;
			}
			break;
		default:
			result = BT_L2CAP_CONF_UNKNOWN_OPT;
			goto invalid_opt;
		}
		net_buf_pull(buf, opt->len);
	}

	if (BR_CHAN(chan)->rx.fcs == BT_L2CAP_BR_FCS_16BIT) {
		/* If local enable FCS, peer also needs to enable it. */
		BR_CHAN(chan)->tx.fcs = BT_L2CAP_BR_FCS_16BIT;
	}

	return 0;

invalid_opt:
	return -EINVAL;
}
#endif /* L2CAP_BR_RET_FC_ENABLE */

static void l2cap_br_conf_rsp(struct bt_l2cap_br *l2cap, uint8_t ident,
			      uint16_t len, struct net_buf *buf)
{
	struct bt_conn *conn = l2cap->chan.chan.conn;
	struct bt_l2cap_chan *chan;
	struct bt_l2cap_conf_rsp *rsp = (void *)buf->data;
	uint16_t flags, scid, result, opt_len;
	struct bt_l2cap_br_chan *br_chan;
#if L2CAP_BR_RET_FC_ENABLE
	int err;
#endif /* L2CAP_BR_RET_FC_ENABLE */

	if (buf->len < sizeof(*rsp)) {
		LOG_ERR("Too small L2CAP conf rsp packet size");
		return;
	}

	net_buf_pull(buf, sizeof(*rsp));

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
#if L2CAP_BR_RET_FC_ENABLE
		err = l2cap_br_conf_rsp_opt_check(chan, opt_len, buf);
		if (err) {
			/* currently disconnect channel if opt is invalid */
			bt_l2cap_chan_disconnect(chan);
			break;
		}
#endif /* L2CAP_BR_RET_FC_ENABLE */
		if (br_chan->state == BT_L2CAP_CONFIG &&
		    atomic_test_bit(br_chan->flags, L2CAP_FLAG_CONN_RCONF_DONE)) {
			LOG_DBG("scid 0x%04x rx MTU %u dcid 0x%04x tx MTU %u", br_chan->rx.cid,
				br_chan->rx.mtu, br_chan->tx.cid, br_chan->tx.mtu);

			bt_l2cap_br_chan_set_state(chan, BT_L2CAP_CONNECTED);
			if (chan->ops && chan->ops->connected) {
				chan->ops->connected(chan);
			}
		}
		break;
#if L2CAP_BR_RET_FC_ENABLE
	case BT_L2CAP_CONF_UNACCEPT:
		err = l2cap_br_conf_rsp_unaccept_opt(chan, opt_len, buf);
		if (!err) {
			l2cap_br_conf(chan);
			break;
		}
		__fallthrough;
#endif /* L2CAP_BR_RET_FC_ENABLE */
	default:
		/* currently disconnect channel on non success result */
		bt_l2cap_chan_disconnect(chan);
		break;
	}
}

int bt_l2cap_br_server_register(struct bt_l2cap_server *server)
{
	if (server->psm < L2CAP_BR_PSM_START || !server->accept) {
		return -EINVAL;
	}

	/* PSM must be odd and lsb of upper byte must be 0 */
	if ((server->psm & 0x0101) != 0x0001) {
		return -EINVAL;
	}

	if (server->sec_level > BT_SECURITY_L4) {
		return -EINVAL;
	} else if (server->sec_level == BT_SECURITY_L0 &&
		   server->psm != L2CAP_BR_PSM_SDP) {
		server->sec_level = BT_SECURITY_L1;
	}

	/* Check if given PSM is already in use */
	if (l2cap_br_server_lookup_psm(server->psm)) {
		LOG_DBG("PSM already registered");
		return -EADDRINUSE;
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

	LOG_DBG("Flash timeout %u", opt_to->timeout);

	opt_to->timeout = sys_cpu_to_le32(0xFFFF);
	result = BT_L2CAP_CONF_UNACCEPT;
done:
	return result;
}

static uint16_t l2cap_br_conf_opt_qos(struct bt_l2cap_chan *chan,
				   struct net_buf *buf, size_t len)
{
	uint16_t result = BT_L2CAP_CONF_SUCCESS;
	struct bt_l2cap_conf_opt_qos *opt_qos;
	struct bt_l2cap_chan *chan_sig;
	struct bt_l2cap_br *br_chan_sig;

	chan_sig = bt_l2cap_br_lookup_rx_cid(chan->conn, BT_L2CAP_CID_BR_SIG);
	if (!chan_sig) {
		LOG_WRN("Ignoring data for unknown channel ID 0x%04x", BT_L2CAP_CID_BR_SIG);
		result = BT_L2CAP_CONF_REJECT;
		goto done;
	}

	br_chan_sig = CONTAINER_OF(chan_sig, struct bt_l2cap_br, chan.chan);
	if (!((br_chan_sig->info_feat_mask & L2CAP_FEAT_QOS_MASK) &&
	      (L2CAP_EXTENDED_FEAT_MASK & L2CAP_FEAT_QOS_MASK))) {
		LOG_WRN("Unsupported extended flow spec");
		result = BT_L2CAP_CONF_REJECT;
		goto done;
	}

	if (len != sizeof(*opt_qos)) {
		LOG_ERR("qos frame length %zu invalid", len);
		result = BT_L2CAP_CONF_REJECT;
		goto done;
	}

	opt_qos = (struct bt_l2cap_conf_opt_qos *)buf->data;

	LOG_DBG("QOS Type %u", opt_qos->service_type);

	if (opt_qos->service_type == BT_L2CAP_QOS_TYPE_GUARANTEED) {
		result = BT_L2CAP_CONF_UNACCEPT;
		/* Set to default value */
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

#if L2CAP_BR_RET_FC_ENABLE
static uint16_t l2cap_br_conf_opt_ret_fc(struct bt_l2cap_chan *chan,
				   struct net_buf *buf, size_t len)
{
	uint16_t result = BT_L2CAP_CONF_UNACCEPT;
	struct bt_l2cap_conf_opt_ret_fc *opt_ret_fc;
	struct bt_l2cap_br_chan *br_chan;
	struct bt_l2cap_chan *chan_sig;
	struct bt_l2cap_br *br_chan_sig;
	uint16_t retransmission_timeout;
	uint16_t monitor_timeout;
	bool accept = true;
	uint8_t mode;

	if (len != sizeof(*opt_ret_fc)) {
		LOG_ERR("ret_fc frame length %zu invalid", len);
		result = BT_L2CAP_CONF_REJECT;
		goto done;
	}

	chan_sig = bt_l2cap_br_lookup_rx_cid(chan->conn, BT_L2CAP_CID_BR_SIG);
	if (!chan_sig) {
		LOG_WRN("Ignoring data for unknown channel ID 0x%04x", BT_L2CAP_CID_BR_SIG);
		result = BT_L2CAP_CONF_REJECT;
		goto done;
	}

	br_chan_sig = CONTAINER_OF(chan_sig, struct bt_l2cap_br, chan.chan);

	opt_ret_fc = (struct bt_l2cap_conf_opt_ret_fc *)buf->data;

	LOG_DBG("ret_fc mode %u", opt_ret_fc->mode);

	br_chan = BR_CHAN(chan);

	mode = opt_ret_fc->mode;

	switch (opt_ret_fc->mode) {
	case BT_L2CAP_BR_LINK_MODE_BASIC:
		break;
	case BT_L2CAP_BR_LINK_MODE_RET:
		__fallthrough;
	case BT_L2CAP_BR_LINK_MODE_FC:
		if ((br_chan_sig->info_feat_mask & L2CAP_FEAT_ENH_RET_MASK) &&
		    (L2CAP_EXTENDED_FEAT_MASK & L2CAP_FEAT_ENH_RET_MASK)) {
			/* If enhance transmission mode is supported by both side,
			 * the enhance transmission mode should be used instead of
			 * transmission or flow control mode.
			 */
			mode = BT_L2CAP_BR_LINK_MODE_ERET;
		} else {
			if ((opt_ret_fc->mode == BT_L2CAP_BR_LINK_MODE_FC) &&
			    (!((br_chan_sig->info_feat_mask & L2CAP_FEAT_FC_MASK) &&
			       (L2CAP_EXTENDED_FEAT_MASK & L2CAP_FEAT_FC_MASK)))) {
				mode = BT_L2CAP_BR_LINK_MODE_BASIC;
			}

			if ((opt_ret_fc->mode == BT_L2CAP_BR_LINK_MODE_RET) &&
			    (!((br_chan_sig->info_feat_mask & L2CAP_FEAT_RET_MASK) &&
			       (L2CAP_EXTENDED_FEAT_MASK & L2CAP_FEAT_RET_MASK)))) {
				mode = BT_L2CAP_BR_LINK_MODE_BASIC;
			}
		}
		break;
	case BT_L2CAP_BR_LINK_MODE_ERET:
		if (!((br_chan_sig->info_feat_mask & L2CAP_FEAT_ENH_RET_MASK) &&
		      (L2CAP_EXTENDED_FEAT_MASK & L2CAP_FEAT_ENH_RET_MASK))) {
			/* If enhance transmission mode is unsupported by any side,
			 * the transmission mode should be used instead of
			 * enhance transmission mode.
			 */
			if ((br_chan_sig->info_feat_mask & L2CAP_FEAT_RET_MASK) &&
			    (L2CAP_EXTENDED_FEAT_MASK & L2CAP_FEAT_RET_MASK)) {
				mode = BT_L2CAP_BR_LINK_MODE_RET;
			} else if ((br_chan_sig->info_feat_mask & L2CAP_FEAT_FC_MASK) &&
				   (L2CAP_EXTENDED_FEAT_MASK & L2CAP_FEAT_FC_MASK)) {
				mode = BT_L2CAP_BR_LINK_MODE_FC;
			} else {
				mode = BT_L2CAP_BR_LINK_MODE_BASIC;
			}
		}
		break;
	case BT_L2CAP_BR_LINK_MODE_STREAM:
		if (!((br_chan_sig->info_feat_mask & L2CAP_FEAT_STREAM_MASK) &&
		      (L2CAP_EXTENDED_FEAT_MASK & L2CAP_FEAT_STREAM_MASK))) {
			/* Unsupported mode */
			mode = BT_L2CAP_BR_LINK_MODE_BASIC;
		}
		break;
	default:
		mode = BT_L2CAP_BR_LINK_MODE_BASIC;
		break;
	}

	if (!br_chan->rx.optional) {
		switch (mode) {
		case BT_L2CAP_BR_LINK_MODE_BASIC:
			__fallthrough;
		case BT_L2CAP_BR_LINK_MODE_ERET:
			__fallthrough;
		case BT_L2CAP_BR_LINK_MODE_STREAM:
			/* The tx mode should be same as rx mode */
			if (mode != br_chan->rx.mode) {
				mode = br_chan->rx.mode;
			}
			break;
		case BT_L2CAP_BR_LINK_MODE_RET:
			__fallthrough;
		case BT_L2CAP_BR_LINK_MODE_FC:
			/* The tx mode should be aligned with rx mode */
			if ((br_chan->rx.mode != BT_L2CAP_BR_LINK_MODE_RET) &&
			    (br_chan->rx.mode != BT_L2CAP_BR_LINK_MODE_FC)) {
				mode = br_chan->rx.mode;
			}
			break;
		default:
			break;
		}
	}

	if (opt_ret_fc->mode != mode) {
		accept = false;
		opt_ret_fc->mode = mode;
	}

	br_chan->tx.mode = mode;

	if (opt_ret_fc->mode != BT_L2CAP_BR_LINK_MODE_BASIC) {
		uint16_t mps = CONFIG_BT_L2CAP_MPS;

		if (sys_le16_to_cpu(opt_ret_fc->mps) > br_chan->tx.mtu) {
			opt_ret_fc->mps = sys_cpu_to_le16(br_chan->tx.mtu);
			accept = false;
		}

		if (sys_le16_to_cpu(opt_ret_fc->mps) > mps) {
			opt_ret_fc->mps = sys_cpu_to_le16(mps);
		}

		if (opt_ret_fc->max_transmit < 1) {
			opt_ret_fc->max_transmit = 1;
			accept = false;
		}

		if ((opt_ret_fc->mode == BT_L2CAP_BR_LINK_MODE_RET) ||
		    (opt_ret_fc->mode == BT_L2CAP_BR_LINK_MODE_FC)) {
			if ((opt_ret_fc->tx_windows_size < 1) ||
			    (opt_ret_fc->tx_windows_size > 32)) {
				opt_ret_fc->tx_windows_size = CONFIG_BT_L2CAP_MAX_WINDOW_SIZE;
				accept = false;
			}
		}

		if (opt_ret_fc->mode == BT_L2CAP_BR_LINK_MODE_ERET) {
			if ((opt_ret_fc->tx_windows_size < 1) ||
			    (opt_ret_fc->tx_windows_size > 63)) {
				opt_ret_fc->tx_windows_size = CONFIG_BT_L2CAP_MAX_WINDOW_SIZE;
				accept = false;
			}
		}

		retransmission_timeout = sys_le16_to_cpu(opt_ret_fc->retransmission_timeout);

		monitor_timeout = sys_le16_to_cpu(opt_ret_fc->monitor_timeout);
		if (monitor_timeout < retransmission_timeout) {
			monitor_timeout = retransmission_timeout;
			accept = false;
		}

		if (opt_ret_fc->tx_windows_size > CONFIG_BT_L2CAP_MAX_WINDOW_SIZE) {
			opt_ret_fc->tx_windows_size =
				sys_cpu_to_le16(CONFIG_BT_L2CAP_MAX_WINDOW_SIZE);
		}

		opt_ret_fc->retransmission_timeout = sys_cpu_to_le16(retransmission_timeout);
		opt_ret_fc->monitor_timeout = sys_cpu_to_le16(monitor_timeout);

		if (!accept) {
			goto done;
		}

		br_chan->tx.ret_timeout = retransmission_timeout;
		br_chan->tx.monitor_timeout = monitor_timeout;
		br_chan->tx.transmit = opt_ret_fc->max_transmit;
		br_chan->tx.window = sys_le16_to_cpu(opt_ret_fc->tx_windows_size);
		br_chan->tx.mps = sys_le16_to_cpu(opt_ret_fc->mps);
	}

	if (!accept) {
		goto done;
	}

	result = BT_L2CAP_CONF_SUCCESS;

done:
	return result;
}

static uint16_t l2cap_br_conf_opt_fcs(struct bt_l2cap_chan *chan,
				   struct net_buf *buf, size_t len)
{
	uint16_t result = BT_L2CAP_CONF_SUCCESS;
	struct bt_l2cap_conf_opt_fcs *opt_fcs;
	struct bt_l2cap_br_chan *br_chan;
	struct bt_l2cap_chan *chan_sig;
	struct bt_l2cap_br *br_chan_sig;

	chan_sig = bt_l2cap_br_lookup_rx_cid(chan->conn, BT_L2CAP_CID_BR_SIG);
	if (!chan_sig) {
		LOG_WRN("Ignoring data for unknown channel ID 0x%04x", BT_L2CAP_CID_BR_SIG);
		result = BT_L2CAP_CONF_REJECT;
		goto done;
	}

	br_chan_sig = CONTAINER_OF(chan_sig, struct bt_l2cap_br, chan.chan);
	if (!((br_chan_sig->info_feat_mask & L2CAP_FEAT_FCS_MASK) &&
	      (L2CAP_EXTENDED_FEAT_MASK & L2CAP_FEAT_FCS_MASK))) {
		LOG_WRN("Unsupported extended flow spec");
		result = BT_L2CAP_CONF_REJECT;
		goto done;
	}

	if (len != sizeof(*opt_fcs)) {
		LOG_ERR("fcs frame length %zu invalid", len);
		result = BT_L2CAP_CONF_REJECT;
		goto done;
	}

	opt_fcs = (struct bt_l2cap_conf_opt_fcs *)buf->data;

	LOG_DBG("FCS type %u", opt_fcs->type);

	if ((opt_fcs->type != BT_L2CAP_FCS_TYPE_NO) &&
		(opt_fcs->type != BT_L2CAP_FCS_TYPE_16BIT)) {
		LOG_ERR("fcs type %u invalid", opt_fcs->type);
		result = BT_L2CAP_CONF_REJECT;
		goto done;
	}

	br_chan = BR_CHAN(chan);

	switch (br_chan->tx.mode) {
	case BT_L2CAP_BR_LINK_MODE_BASIC:
		result = BT_L2CAP_CONF_REJECT;
		goto done;
	case BT_L2CAP_BR_LINK_MODE_RET:
		__fallthrough;
	case BT_L2CAP_BR_LINK_MODE_FC:
		LOG_ERR("fcs opt is mandatory in mode %u", br_chan->tx.mode);
		if (opt_fcs->type != BT_L2CAP_BR_FCS_16BIT) {
			result = BT_L2CAP_CONF_UNACCEPT;
			opt_fcs->type = BT_L2CAP_BR_FCS_16BIT;
			br_chan->tx.fcs = opt_fcs->type;
			goto done;
		}
		break;
	case BT_L2CAP_BR_LINK_MODE_ERET:
		__fallthrough;
	case BT_L2CAP_BR_LINK_MODE_STREAM:
		br_chan->tx.fcs = opt_fcs->type;
		break;
	default:
		/* Should not have been here */
		LOG_ERR("Unknown mode %u", br_chan->tx.mode);
		break;
	}

done:
	return result;
}

static uint16_t l2cap_br_conf_opt_ext_flow_spec(struct bt_l2cap_chan *chan,
				   struct net_buf *buf, size_t len)
{
	uint16_t result = BT_L2CAP_CONF_SUCCESS;
	struct bt_l2cap_conf_opt_ext_flow_spec *ext_flow_spec;
	struct bt_l2cap_br_chan *br_chan;
	struct bt_l2cap_chan *chan_sig;
	struct bt_l2cap_br *br_chan_sig;

	chan_sig = bt_l2cap_br_lookup_rx_cid(chan->conn, BT_L2CAP_CID_BR_SIG);
	if (!chan_sig) {
		LOG_WRN("Ignoring data for unknown channel ID 0x%04x", BT_L2CAP_CID_BR_SIG);
		result = BT_L2CAP_CONF_REJECT;
		goto done;
	}

	br_chan_sig = CONTAINER_OF(chan_sig, struct bt_l2cap_br, chan.chan);
	if (!((br_chan_sig->info_feat_mask & L2CAP_FEAT_EXT_FS_MASK) &&
	      (L2CAP_EXTENDED_FEAT_MASK & L2CAP_FEAT_EXT_FS_MASK))) {
		LOG_WRN("Unsupported extended flow spec");
		result = BT_L2CAP_CONF_REJECT;
		goto done;
	}

	if (len != sizeof(*ext_flow_spec)) {
		LOG_ERR("fcs frame length %zu invalid", len);
		result = BT_L2CAP_CONF_REJECT;
		goto done;
	}

	ext_flow_spec = (struct bt_l2cap_conf_opt_ext_flow_spec *)buf->data;

	LOG_DBG("Ext Flow Spec %u %u %u %u %u %u", ext_flow_spec->identifier,
		ext_flow_spec->service_type, ext_flow_spec->sdu,
		ext_flow_spec->sdu_inter_arrival_time, ext_flow_spec->access_latency,
		ext_flow_spec->flush_timeout);

	br_chan = BR_CHAN(chan);

	/* Set to default value */
	result = BT_L2CAP_CONF_REJECT;
	ext_flow_spec->identifier = 0x01;
	ext_flow_spec->service_type = 0x01;
	ext_flow_spec->sdu = sys_cpu_to_le16(0xFFFF);
	ext_flow_spec->sdu_inter_arrival_time = sys_cpu_to_le32(0xFFFFFFFF);
	ext_flow_spec->access_latency = sys_cpu_to_le32(0xFFFFFFFF);
	ext_flow_spec->flush_timeout = sys_cpu_to_le32(0xFFFFFFFF);

done:
	return result;
}

static uint16_t l2cap_br_conf_opt_ext_win_size(struct bt_l2cap_chan *chan,
				   struct net_buf *buf, size_t len)
{
	uint16_t result = BT_L2CAP_CONF_SUCCESS;
	struct bt_l2cap_conf_opt_ext_win_size *opt_ext_win_size;
	struct bt_l2cap_br_chan *br_chan;
	uint16_t win_size;
	struct bt_l2cap_chan *chan_sig;
	struct bt_l2cap_br *br_chan_sig;

	chan_sig = bt_l2cap_br_lookup_rx_cid(chan->conn, BT_L2CAP_CID_BR_SIG);
	if (!chan_sig) {
		LOG_WRN("Ignoring data for unknown channel ID 0x%04x", BT_L2CAP_CID_BR_SIG);
		result = BT_L2CAP_CONF_REJECT;
		goto done;
	}

	br_chan_sig = CONTAINER_OF(chan_sig, struct bt_l2cap_br, chan.chan);
	if (!((br_chan_sig->info_feat_mask & L2CAP_FEAT_EXT_WIN_SIZE_MASK) &&
	      (L2CAP_EXTENDED_FEAT_MASK & L2CAP_FEAT_EXT_WIN_SIZE_MASK))) {
		LOG_WRN("Unsupported extended window size");
		result = BT_L2CAP_CONF_REJECT;
		goto done;
	}

	if (len != sizeof(*opt_ext_win_size)) {
		LOG_ERR("fcs frame length %zu invalid", len);
		result = BT_L2CAP_CONF_REJECT;
		goto done;
	}

	opt_ext_win_size = (struct bt_l2cap_conf_opt_ext_win_size *)buf->data;
	win_size = sys_le16_to_cpu(opt_ext_win_size->max_windows_size);

	LOG_DBG("EXT Win Size value %u", win_size);

	br_chan = BR_CHAN(chan);

	switch (br_chan->tx.mode) {
	case BT_L2CAP_BR_LINK_MODE_BASIC:
		__fallthrough;
	case BT_L2CAP_BR_LINK_MODE_RET:
		__fallthrough;
	case BT_L2CAP_BR_LINK_MODE_FC:
		/* ignore */
		LOG_WRN("Ignored by mode %u", br_chan->tx.mode);
		break;
	case BT_L2CAP_BR_LINK_MODE_ERET:
		if ((win_size < 1) || (win_size > 0x3FFF)) {
			/* Set to default value */
			opt_ext_win_size->max_windows_size =
				sys_cpu_to_le16(CONFIG_BT_L2CAP_MAX_WINDOW_SIZE);
			result = BT_L2CAP_CONF_UNACCEPT;
			goto done;
		}
		if (win_size > CONFIG_BT_L2CAP_MAX_WINDOW_SIZE) {
			opt_ext_win_size->max_windows_size =
				sys_cpu_to_le16(CONFIG_BT_L2CAP_MAX_WINDOW_SIZE);
		}
		br_chan->tx.extended_control = true;
		break;
	case BT_L2CAP_BR_LINK_MODE_STREAM:
		if (win_size) {
			result = BT_L2CAP_CONF_UNACCEPT;
			opt_ext_win_size->max_windows_size = 0;
			goto done;
		}
		br_chan->tx.extended_control = true;
		break;
	default:
		/* Should not have been here */
		LOG_ERR("Unknown mode %u", br_chan->tx.mode);
		break;
	}

done:
	return result;
}
#endif /* L2CAP_BR_RET_FC_ENABLE */

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
	struct net_buf *rsp_buf;

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

	rsp_buf = bt_l2cap_create_pdu(&br_sig_pool, 0);

	hdr = net_buf_add(rsp_buf, sizeof(*hdr));
	hdr->code = BT_L2CAP_CONF_RSP;
	hdr->ident = ident;
	rsp = net_buf_add(rsp_buf, sizeof(*rsp));
	(void)memset(rsp, 0, sizeof(*rsp));

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

	if (!opt_len) {
		LOG_DBG("tx default MTU %u", L2CAP_BR_DEFAULT_MTU);
		BR_CHAN(chan)->tx.mtu = L2CAP_BR_DEFAULT_MTU;
		goto send_rsp;
	}

	while (opt_len >= sizeof(*opt)) {
		opt = net_buf_pull_mem(buf, sizeof(*opt));

		opt_len -= sizeof(*opt);

		/* make sure opt object can get safe dereference in iteration */
		if (opt_len < opt->len) {
			LOG_ERR("Received too short option data");
			result = BT_L2CAP_CONF_REJECT;
			break;
		}

		opt_len -= opt->len;

		hint = opt->type & BT_L2CAP_CONF_HINT;

		switch (opt->type & BT_L2CAP_CONF_MASK) {
		case BT_L2CAP_CONF_OPT_MTU:
			/* getting MTU modifies buf internals */
			result = l2cap_br_conf_opt_mtu(chan, buf, opt->len);
			if (result != BT_L2CAP_CONF_SUCCESS) {
				goto send_rsp;
			}
			break;
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
#if L2CAP_BR_RET_FC_ENABLE
		case BT_L2CAP_CONF_OPT_RET_FC:
			result = l2cap_br_conf_opt_ret_fc(chan, buf, opt->len);
			if (result != BT_L2CAP_CONF_SUCCESS) {
				goto send_rsp;
			} else {
				l2cap_br_conf_add_opt(rsp_buf, opt);
			}
			break;
		case BT_L2CAP_CONF_OPT_FCS:
			result = l2cap_br_conf_opt_fcs(chan, buf, opt->len);
			if (result != BT_L2CAP_CONF_SUCCESS) {
				goto send_rsp;
			}
			break;
		case BT_L2CAP_CONF_OPT_EXT_FLOW_SPEC:
			result = l2cap_br_conf_opt_ext_flow_spec(chan, buf, opt->len);
			if (result != BT_L2CAP_CONF_SUCCESS) {
				goto send_rsp;
			}
			break;
		case BT_L2CAP_CONF_OPT_EXT_WIN_SIZE:
			result = l2cap_br_conf_opt_ext_win_size(chan, buf, opt->len);
			if (result != BT_L2CAP_CONF_SUCCESS) {
				goto send_rsp;
			} else {
				l2cap_br_conf_add_opt(rsp_buf, opt);
			}
			break;
#endif /* L2CAP_BR_RET_FC_ENABLE */
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
	rsp->result = sys_cpu_to_le16(result);

	/*
	 * TODO: If options other than MTU became meaningful then processing
	 * the options chain need to be modified and taken into account when
	 * sending back to peer.
	 */
	if ((result == BT_L2CAP_CONF_UNKNOWN_OPT) || (result == BT_L2CAP_CONF_UNACCEPT)) {
		if (opt) {
			l2cap_br_conf_add_opt(rsp_buf, opt);
		}
	}

	hdr->len = sys_cpu_to_le16(rsp_buf->len - sizeof(*hdr));

	l2cap_send(conn, BT_L2CAP_CID_BR_SIG, rsp_buf);

	if (result != BT_L2CAP_CONF_SUCCESS) {
		return;
	}

#if L2CAP_BR_RET_FC_ENABLE
	if (BR_CHAN(chan)->tx.fcs == BT_L2CAP_BR_FCS_16BIT) {
		/* If peer enables FCS, local also needs to enable it. */
		BR_CHAN(chan)->rx.fcs = BT_L2CAP_BR_FCS_16BIT;
	}

	if (BR_CHAN(chan)->tx.extended_control) {
		/* If peer enables extended control field,
		 * local also needs to enable it.
		 */
		BR_CHAN(chan)->rx.extended_control = true;
	}
#endif /* L2CAP_BR_RET_FC_ENABLE */

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
#if L2CAP_BR_RET_FC_ENABLE
	int err;
#endif /* L2CAP_BR_RET_FC_ENABLE */

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

#if L2CAP_BR_RET_FC_ENABLE
	err = l2cap_br_check_chan_config(conn, br_chan);
	if (err) {
		return err;
	}
#endif /* L2CAP_BR_RET_FC_ENABLE */

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
		LOG_ERR("attempt to send %u bytes on %u MTU chan",
			buf->len, br_chan->tx.mtu);
		return -EMSGSIZE;
	}

	return bt_l2cap_br_send_cb(br_chan->chan.conn, br_chan->tx.cid, buf, cb, user_data);
}

int bt_l2cap_br_chan_send(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	return bt_l2cap_br_chan_send_cb(chan, buf, NULL, NULL);
}

int bt_l2cap_br_echo(struct bt_conn *conn, struct net_buf *buf)
{
	struct bt_l2cap_sig_hdr *hdr;
	uint16_t len;

	if (!conn) {
		return -EINVAL;
	}

	if (conn->type != BT_CONN_TYPE_BR) {
		return -EINVAL;
	}

	len = (uint16_t)buf->len;

	hdr = net_buf_push(buf, sizeof(*hdr));
	hdr->code = BT_L2CAP_ECHO_REQ;
	hdr->ident = l2cap_br_get_ident();
	hdr->len = sys_cpu_to_le16(len);

	l2cap_send(conn, BT_L2CAP_CID_BR_SIG, buf);

	return 0;
}

static void l2cap_br_echo_req(struct bt_l2cap_br *l2cap, uint8_t ident,
			      struct net_buf *buf)
{
	struct bt_conn *conn = l2cap->chan.chan.conn;
	struct bt_l2cap_echo_req *req = (void *)buf->data;
	struct bt_l2cap_sig_hdr *hdr;
	struct bt_l2cap_echo_rsp *rsp;

	if (buf->len < sizeof(*req)) {
		LOG_ERR("Too small L2CAP conn rsp packet size");
		return;
	}

	buf = bt_l2cap_create_pdu(&br_sig_pool, 0);

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->code = BT_L2CAP_ECHO_RSP;
	hdr->ident = ident;
	rsp = net_buf_add(buf, sizeof(*rsp));
	(void)memset(rsp, 0, sizeof(*rsp));

	hdr->len = sys_cpu_to_le16(buf->len - sizeof(*hdr));

	l2cap_send(conn, BT_L2CAP_CID_BR_SIG, buf);
}

static void l2cap_br_echo_rsp(struct bt_l2cap_br *l2cap, uint8_t ident,
			      struct net_buf *buf)
{
	struct bt_l2cap_echo_rsp *rsp = (void *)buf->data;

	if (buf->len < sizeof(*rsp)) {
		LOG_ERR("Too small L2CAP conn rsp packet size");
		return;
	}

	LOG_DBG("Echo RSP (ident 0x%02x) with len %u", ident, buf->len);
}

static int l2cap_br_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_l2cap_br *l2cap = CONTAINER_OF(chan, struct bt_l2cap_br, chan.chan);
	struct bt_l2cap_sig_hdr *hdr;
	uint16_t len;
	struct net_buf_simple_state state;

	while (buf->len > 0) {
		if (buf->len < sizeof(*hdr)) {
			LOG_ERR("Too small L2CAP signaling PDU");
			hdr = (struct bt_l2cap_sig_hdr *)buf->data;
			l2cap_br_send_reject(chan->conn, hdr->ident,
						BT_L2CAP_REJ_NOT_UNDERSTOOD, NULL, 0);
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
		case BT_L2CAP_ECHO_REQ:
			l2cap_br_echo_req(l2cap, hdr->ident, buf);
			break;
		case BT_L2CAP_ECHO_RSP:
			l2cap_br_echo_rsp(l2cap, hdr->ident, buf);
			break;
		default:
			LOG_WRN("Unknown/Unsupported L2CAP PDU code 0x%02x", hdr->code);
			l2cap_br_send_reject(chan->conn, hdr->ident,
						BT_L2CAP_REJ_NOT_UNDERSTOOD, NULL, 0);
			break;
		}

		net_buf_simple_restore(&buf->b, &state);
		(void)net_buf_pull_mem(buf, len);
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

#if L2CAP_BR_RET_FC_ENABLE
static bool bt_l2cap_br_check_tx_seq_out_of_sequence(struct bt_l2cap_br_chan *br_chan,
						     uint16_t tx_seq)
{
	uint16_t outstanding_frames;
	uint16_t tx_frames;

	outstanding_frames =
		(uint16_t)((br_chan->buffer_seq + br_chan->rx.window) - br_chan->expected_tx_seq);
	tx_frames = (uint16_t)(tx_seq - br_chan->expected_tx_seq);

	if ((br_chan->tx.mode == BT_L2CAP_BR_LINK_MODE_ERET) ||
	    (br_chan->tx.mode == BT_L2CAP_BR_LINK_MODE_STREAM)) {
		if (br_chan->tx.extended_control) {
			outstanding_frames = outstanding_frames % BT_L2CAP_EXT_CONTROL_SEQ_MAX;
			tx_frames = tx_frames % BT_L2CAP_EXT_CONTROL_SEQ_MAX;
		} else {
			outstanding_frames = outstanding_frames % BT_L2CAP_CONTROL_SEQ_MAX;
			tx_frames = tx_frames % BT_L2CAP_CONTROL_SEQ_MAX;
		}
	} else if ((br_chan->tx.mode == BT_L2CAP_BR_LINK_MODE_RET) ||
		   (br_chan->tx.mode == BT_L2CAP_BR_LINK_MODE_FC)) {
		outstanding_frames = outstanding_frames % BT_L2CAP_CONTROL_SEQ_MAX;
		tx_frames = tx_frames % BT_L2CAP_CONTROL_SEQ_MAX;
	}

	if (tx_frames && (tx_frames < outstanding_frames)) {
		return true;
	}

	return false;
}

static bool bt_l2cap_br_check_tx_seq_duplicated(struct bt_l2cap_br_chan *br_chan, uint16_t tx_seq)
{
	uint16_t outstanding_frames;
	uint16_t tx_frames;

	outstanding_frames = (uint16_t)(br_chan->expected_tx_seq - br_chan->buffer_seq - 1);
	tx_frames = (uint16_t)(tx_seq - br_chan->buffer_seq);

	if ((br_chan->tx.mode == BT_L2CAP_BR_LINK_MODE_ERET) ||
	    (br_chan->tx.mode == BT_L2CAP_BR_LINK_MODE_STREAM)) {
		if (br_chan->tx.extended_control) {
			outstanding_frames = outstanding_frames % BT_L2CAP_EXT_CONTROL_SEQ_MAX;
			tx_frames = tx_frames % BT_L2CAP_EXT_CONTROL_SEQ_MAX;
		} else {
			outstanding_frames = outstanding_frames % BT_L2CAP_CONTROL_SEQ_MAX;
			tx_frames = tx_frames % BT_L2CAP_CONTROL_SEQ_MAX;
		}
	} else if ((br_chan->tx.mode == BT_L2CAP_BR_LINK_MODE_RET) ||
		   (br_chan->tx.mode == BT_L2CAP_BR_LINK_MODE_FC)) {
		outstanding_frames = outstanding_frames % BT_L2CAP_CONTROL_SEQ_MAX;
		tx_frames = tx_frames % BT_L2CAP_CONTROL_SEQ_MAX;
	}

	if (tx_frames <= outstanding_frames) {
		return true;
	}

	return false;
}

static void bt_l2cap_br_update_srej(struct bt_l2cap_br_chan *br_chan, uint16_t req_seq)
{
	if (bt_l2cap_br_check_req_seq_valid(br_chan, req_seq)) {
		struct bt_l2cap_br_window *tx_win, *next;

		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&br_chan->_pdu_outstanding, tx_win, next, node) {
			if (tx_win->tx_seq == req_seq) {
				tx_win->srej = true;
				break;
			}
		}
	} else {
		LOG_WRN("Invalid req seq %d received on %p", req_seq, br_chan);
		/* The L2CAP entity shall close the channel as a consequence
		 * of an ReqSeq Sequence error.
		 */
		bt_l2cap_br_chan_disconnect(&br_chan->chan);
	}
	/* Append channel to list if it still has data */
	if (chan_has_data(br_chan)) {
		LOG_DBG("chan %p ready", br_chan);
		raise_data_ready(br_chan);
	}
}

static void bt_l2cap_br_update_r(struct bt_l2cap_br_chan *br_chan, uint8_t r)
{
	int err;

	if (br_chan->rx.mode != BT_L2CAP_BR_LINK_MODE_RET) {
		LOG_DBG("Only Support retransmission mode");
		return;
	}

	if (!r) {
		if (!atomic_test_and_clear_bit(br_chan->flags, L2CAP_FLAG_RECV_FRAME_R)) {
			/* R bit without any change */
			return;
		}

		/* R bit is changed from 1 -> 0 */
		atomic_set_bit(br_chan->flags, L2CAP_FLAG_RECV_FRAME_R_CHANGED);
		if (bt_l2cap_br_get_outstanding_count(br_chan)) {
			/* If any unacknowledged I-frames have been sent then RetransmissionTimer
			 * shall be restarted.
			 */
			l2cap_br_start_timer(br_chan, true, true);
		} else {
			/* If the RetransmissionTimer is not running and the MonitorTimer is
			 * not running, then start the MonitorTimer.
			 */
			l2cap_br_start_timer(br_chan, false, false);
		}
	} else {
		if (atomic_test_and_set_bit(br_chan->flags, L2CAP_FLAG_RECV_FRAME_R)) {
			/* R bit without any change */
			return;
		}

		/* R bit is changed from 0 -> 1 */
		atomic_set_bit(br_chan->flags, L2CAP_FLAG_RECV_FRAME_R_CHANGED);
		l2cap_br_start_timer(br_chan, false, false);
	}

	err = l2cap_br_send_s_frame(br_chan, BT_L2CAP_CONTROL_S_RR, K_NO_WAIT);
	if (err) {
		LOG_ERR("Fail to send frame %d on %p", err, br_chan);
		bt_l2cap_chan_disconnect(&br_chan->chan);
	}
}

static int bt_l2cap_br_update_f(struct bt_l2cap_br_chan *br_chan, uint8_t f)
{
	if (br_chan->rx.mode == BT_L2CAP_BR_LINK_MODE_STREAM) {
		/* Ignore f bit if the mode is streaming. */
		return 0;
	}

	if (f) {
		if (!atomic_test_bit(br_chan->flags, L2CAP_FLAG_SEND_FRAME_P_CHANGED)) {
			if (!atomic_test_and_clear_bit(br_chan->flags, L2CAP_FLAG_SEND_FRAME_P)) {
				LOG_WRN("Unexpected F flag set on %p", br_chan);
				return -EINVAL;
			}

			if (bt_l2cap_br_get_outstanding_count(br_chan)) {
				/* If any unacknowledged I-frames have been sent then
				 * RetransmissionTimer shall be restarted.
				 */
				l2cap_br_start_timer(br_chan, true, true);
			} else {
				/* If the RetransmissionTimer is not running and the MonitorTimer is
				 * not running, then start the MonitorTimer.
				 */
				l2cap_br_start_timer(br_chan, false, false);
			}
		}
		/* Append channel to list if it still has data */
		if (chan_has_data(br_chan)) {
			LOG_DBG("chan %p ready", br_chan);
			raise_data_ready(br_chan);
		}
	}

	return 0;
}

static void bt_l2cap_br_ret_fc_s_recv(struct bt_l2cap_br_chan *br_chan, struct net_buf *buf)
{
	uint16_t control;
	uint32_t ext_control;
	uint16_t req_seq;
	uint8_t r = 0;
	uint8_t s;
	uint8_t f = 0;
	uint8_t p = 0;
	int err;

	if (br_chan->rx.mode == BT_L2CAP_BR_LINK_MODE_STREAM) {
		/* Ignore S-frame if the mode is streaming. */
		return;
	}

	if ((br_chan->rx.mode == BT_L2CAP_BR_LINK_MODE_ERET) ||
	    (br_chan->rx.mode == BT_L2CAP_BR_LINK_MODE_STREAM)) {
		if (br_chan->rx.extended_control) {
			ext_control = net_buf_pull_le32(buf);
			req_seq = BT_L2CAP_S_FRAME_EXT_CONTROL_GET_REQ_SEQ(ext_control);
			f = BT_L2CAP_S_FRAME_EXT_CONTROL_GET_F(ext_control);
			s = BT_L2CAP_S_FRAME_EXT_CONTROL_GET_S(ext_control);
			p = BT_L2CAP_S_FRAME_EXT_CONTROL_GET_P(ext_control);
		} else {
			control = net_buf_pull_le16(buf);
			req_seq = BT_L2CAP_S_FRAME_ENH_CONTROL_GET_REQ_SEQ(control);
			f = BT_L2CAP_S_FRAME_ENH_CONTROL_GET_F(control);
			s = BT_L2CAP_S_FRAME_ENH_CONTROL_GET_S(control);
			p = BT_L2CAP_S_FRAME_ENH_CONTROL_GET_P(control);
		}

		err = bt_l2cap_br_update_f(br_chan, f);
		if (err) {
			/* Discard the S-frame with invalid f bit */
			return;
		}
	} else {
		control = net_buf_pull_le16(buf);
		req_seq = BT_L2CAP_S_FRAME_STD_CONTROL_GET_REQ_SEQ(control);
		s = BT_L2CAP_S_FRAME_STD_CONTROL_GET_S(control);
		r = BT_L2CAP_S_FRAME_STD_CONTROL_GET_R(control);

		bt_l2cap_br_update_r(br_chan, r);
	}

	switch (s) {
	case BT_L2CAP_CONTROL_S_RR:
		if (p) {
			atomic_set_bit(br_chan->flags, L2CAP_FLAG_RECV_FRAME_P);
		} else {
			if (!atomic_test_bit(br_chan->flags, L2CAP_FLAG_SEND_FRAME_REJ)) {
				atomic_set_bit(br_chan->flags, L2CAP_FLAG_NEW_I_FRAME);
			} else if (f) {
				atomic_set_bit(br_chan->flags, L2CAP_FLAG_NEW_I_FRAME);
			}
		}

		bt_l2cap_br_update_req_seq(br_chan, req_seq, false);
		atomic_clear_bit(br_chan->flags, L2CAP_FLAG_REMOTE_BUSY);

		if (f && !atomic_test_and_clear_bit(br_chan->flags, L2CAP_FLAG_REJ_ACTIONED)) {
			/* Retransmit I-frames */
			atomic_set_bit(br_chan->flags, L2CAP_FLAG_RET_I_FRAME);
		}

		if (p) {
			/* Send IorRRorRNR(F=1) */
			err = l2cap_br_send_s_frame(br_chan, BT_L2CAP_CONTROL_S_RR, K_NO_WAIT);
			if (err) {
				bt_l2cap_br_chan_disconnect(&br_chan->chan);
			}
		}
		break;
	case BT_L2CAP_CONTROL_S_REJ:
		if (br_chan->rx.mode == BT_L2CAP_BR_LINK_MODE_FC) {
			/* Unsupported */
			LOG_DBG("Ignore REJ frame");
			break;
		}

		atomic_clear_bit(br_chan->flags, L2CAP_FLAG_REMOTE_BUSY);
		atomic_set_bit(br_chan->flags, L2CAP_FLAG_NEW_I_FRAME);

		if (!f && atomic_test_bit(br_chan->flags, L2CAP_FLAG_SEND_FRAME_P)) {
			atomic_set_bit(br_chan->flags, L2CAP_FLAG_REJ_ACTIONED);
		}

		if (!f && atomic_test_bit(br_chan->flags, L2CAP_FLAG_SEND_FRAME_REJ)) {
			atomic_set_bit(br_chan->flags, L2CAP_FLAG_RET_I_FRAME);
		}

		if (f && !atomic_test_and_clear_bit(br_chan->flags, L2CAP_FLAG_REJ_ACTIONED)) {
			/* Retransmit I-frames */
			atomic_set_bit(br_chan->flags, L2CAP_FLAG_RET_I_FRAME);
		}

		bt_l2cap_br_update_req_seq(br_chan, req_seq, true);
		/* Do not change br_chan->next_tx_seq to req_seq. Because the
		 * TxSeq are saved in tx_win.
		 */
		break;
	case BT_L2CAP_CONTROL_S_RNR:
		if (br_chan->rx.mode != BT_L2CAP_BR_LINK_MODE_ERET) {
			break;
		}

		if (p) {
			atomic_set_bit(br_chan->flags, L2CAP_FLAG_RECV_FRAME_P);
		}

		atomic_set_bit(br_chan->flags, L2CAP_FLAG_REMOTE_BUSY);
		bt_l2cap_br_update_req_seq(br_chan, req_seq, false);

		if (!atomic_test_bit(br_chan->flags, L2CAP_FLAG_SEND_FRAME_REJ)) {
			l2cap_br_start_timer(br_chan, false, true);
			atomic_clear_bit(br_chan->flags, L2CAP_FLAG_RET_I_FRAME);
		}

		if (p || atomic_test_bit(br_chan->flags, L2CAP_FLAG_SEND_FRAME_REJ)) {
			/* Send RRorRNR (F=1) or Send RR (F=0) */
			err = l2cap_br_send_s_frame(br_chan, BT_L2CAP_CONTROL_S_RR, K_NO_WAIT);
			if (err) {
				bt_l2cap_br_chan_disconnect(&br_chan->chan);
			}
		}
		break;
	case BT_L2CAP_CONTROL_S_SREJ:
		if (br_chan->rx.mode != BT_L2CAP_BR_LINK_MODE_ERET) {
			break;
		}

		bt_l2cap_br_update_srej(br_chan, req_seq);

		if (p) {
			atomic_set_bit(br_chan->flags, L2CAP_FLAG_RECV_FRAME_P);
			atomic_set_bit(br_chan->flags, L2CAP_FLAG_NEW_I_FRAME);
		}

		atomic_clear_bit(br_chan->flags, L2CAP_FLAG_REMOTE_BUSY);

		if (f) {
			if (atomic_test_and_clear_bit(br_chan->flags, L2CAP_FLAG_SREJ_ACTIONED) &&
			    (br_chan->srej_save_req_seq == req_seq)) {
				/* Clear flag L2CAP_FLAG_SREJ_ACTIONED */
			} else {
				/* Retransmit-Requested-I-frame */
				atomic_set_bit(br_chan->flags, L2CAP_FLAG_RET_REQ_I_FRAME);
			}
		} else {
			atomic_set_bit(br_chan->flags, L2CAP_FLAG_RET_REQ_I_FRAME);
			if (atomic_test_bit(br_chan->flags, L2CAP_FLAG_SEND_FRAME_P)) {
				atomic_set_bit(br_chan->flags, L2CAP_FLAG_SREJ_ACTIONED);
				br_chan->srej_save_req_seq = req_seq;
			}
		}

		if (p) {
			bt_l2cap_br_update_req_seq(br_chan, req_seq, false);
		}
		break;
	}
}

#if defined(CONFIG_BT_L2CAP_SEG_RECV)
static void bt_l2cap_br_recv_seg_direct(struct bt_l2cap_br_chan *br_chan,
				struct net_buf *seg, uint8_t sar)
{
	uint16_t seg_offset = 0;
	uint16_t sdu_remaining = 0;

	switch (sar) {
	case BT_L2CAP_CONTROL_SAR_UNSEG:
		__fallthrough;
	case BT_L2CAP_CONTROL_SAR_START:
		if (sar == BT_L2CAP_CONTROL_SAR_START) {
			br_chan->_sdu_len = net_buf_pull_le16(seg);
		} else {
			br_chan->_sdu_len = seg->len;
		}
		br_chan->_sdu_len_done = 0;

		if (br_chan->_sdu_len > br_chan->rx.mtu) {
			LOG_WRN("SDU exceeds MTU");
			bt_l2cap_chan_disconnect(&br_chan->chan);
			return;
		}
		break;
	case BT_L2CAP_CONTROL_SAR_END:
		__fallthrough;
	case BT_L2CAP_CONTROL_SAR_CONTI:
		seg_offset = br_chan->_sdu_len_done;
		sdu_remaining = br_chan->_sdu_len - br_chan->_sdu_len_done;

		br_chan->_sdu_len_done += seg->len;

		if (sar == BT_L2CAP_CONTROL_SAR_END) {
			if (br_chan->_sdu_len_done < br_chan->_sdu_len) {
				br_chan->_sdu_len = 0;
				br_chan->_sdu_len_done = 0;
				LOG_WRN("Short data packet %u < %u", br_chan->_sdu_len_done,
					br_chan->_sdu_len);
				bt_l2cap_chan_disconnect(&br_chan->chan);
				return;
			}
		}
		break;
	}

	if (sdu_remaining < seg->len) {
		br_chan->_sdu_len = 0;
		br_chan->_sdu_len_done = 0;
		LOG_WRN("L2CAP RX PDU total exceeds SDU");
		bt_l2cap_chan_disconnect(&br_chan->chan);
		return;
	}

	/* Tail call. */
	br_chan->chan.ops->seg_recv(&br_chan->chan, br_chan->_sdu_len, seg_offset, &seg->b);
}
#endif /* CONFIG_BT_L2CAP_SEG_RECV */

static struct net_buf *l2cap_br_alloc_frag(k_timeout_t timeout, void *user_data)
{
	struct bt_l2cap_br_chan *chan = user_data;
	struct net_buf *frag = NULL;

	frag = chan->chan.ops->alloc_buf(&chan->chan);
	if (!frag) {
		return NULL;
	}

	LOG_DBG("frag %p tailroom %zu", frag, net_buf_tailroom(frag));

	return frag;
}

static int bt_l2cap_br_recv_seg(struct bt_l2cap_br_chan *br_chan, struct net_buf *seg, uint8_t sar)
{
	uint16_t len;
	struct net_buf *buf;

	LOG_DBG("seg on chan %p with len %zu", br_chan, seg->len);

	if ((sar == BT_L2CAP_CONTROL_SAR_UNSEG) || (sar == BT_L2CAP_CONTROL_SAR_START)) {
		if (br_chan->_sdu) {
			LOG_ERR("Last SDU is not done");
			net_buf_unref(br_chan->_sdu);
			br_chan->_sdu = NULL;
			bt_l2cap_chan_disconnect(&br_chan->chan);
			return -ESHUTDOWN;
		}

		br_chan->_sdu = br_chan->chan.ops->alloc_buf(&br_chan->chan);
		if (!br_chan->_sdu) {
			LOG_WRN("Unable to allocate buffer for SDU");
			return -ENOBUFS;
		}

		if (sar == BT_L2CAP_CONTROL_SAR_UNSEG) {
			br_chan->_sdu_len = seg->len;
		} else {
			br_chan->_sdu_len = net_buf_pull_le16(seg);
		}
	}

	if (!br_chan->_sdu) {
		LOG_ERR("Not valid buffer");
		bt_l2cap_chan_disconnect(&br_chan->chan);
		return -ESHUTDOWN;
	}

	if ((br_chan->_sdu->len + seg->len) > br_chan->_sdu_len) {
		LOG_ERR("SDU length mismatch");
		net_buf_unref(br_chan->_sdu);
		br_chan->_sdu = NULL;
		bt_l2cap_chan_disconnect(&br_chan->chan);
		return -ESHUTDOWN;
	}

	/* Append received segment to SDU */
	len = net_buf_append_bytes(br_chan->_sdu, seg->len, seg->data, K_NO_WAIT,
				   l2cap_br_alloc_frag, br_chan);
	if (len != seg->len) {
		LOG_ERR("Unable to store SDU");
		net_buf_unref(br_chan->_sdu);
		br_chan->_sdu = NULL;
		bt_l2cap_chan_disconnect(&br_chan->chan);
		return -ESHUTDOWN;
	}

	LOG_DBG("chan %p len %zu / %zu", br_chan, br_chan->_sdu->len, br_chan->_sdu_len);

	if ((sar == BT_L2CAP_CONTROL_SAR_UNSEG) || (sar == BT_L2CAP_CONTROL_SAR_END)) {
		if (br_chan->_sdu->len < br_chan->_sdu_len) {
			LOG_ERR("SDU length mismatch");
			net_buf_unref(br_chan->_sdu);
			br_chan->_sdu = NULL;
			bt_l2cap_chan_disconnect(&br_chan->chan);
			return -ESHUTDOWN;
		}

		buf = br_chan->_sdu;
		br_chan->_sdu = NULL;
		br_chan->_sdu_len = 0;

		/* Receiving complete SDU, notify channel and reset SDU buf */
		int err;

		err = br_chan->chan.ops->recv(&br_chan->chan, buf);
		if (err < 0) {
			if (err != -EINPROGRESS) {
				LOG_ERR("err %d", err);
				bt_l2cap_chan_disconnect(&br_chan->chan);
				net_buf_unref(buf);
				return -ESHUTDOWN;
			}
			/* If the buf in progress, it will be freed by upper layer. */
			return err;
		}

		net_buf_unref(buf);
	}

	return 0;
}

static void bt_l2cap_br_update_expected_tx_seq(struct bt_l2cap_br_chan *br_chan, uint16_t seq)
{
	if (br_chan->rx.mode == BT_L2CAP_BR_LINK_MODE_STREAM) {
		/* Ignore expected_tx_seq if the mode is streaming. */
		return;
	}

	if ((br_chan->rx.mode == BT_L2CAP_BR_LINK_MODE_ERET) ||
	    (br_chan->rx.mode == BT_L2CAP_BR_LINK_MODE_STREAM)) {
		if (br_chan->rx.extended_control) {
			seq = seq % BT_L2CAP_EXT_CONTROL_SEQ_MAX;
		} else {
			seq = seq % BT_L2CAP_CONTROL_SEQ_MAX;
		}
	} else if ((br_chan->rx.mode == BT_L2CAP_BR_LINK_MODE_RET) ||
		   (br_chan->rx.mode == BT_L2CAP_BR_LINK_MODE_FC)) {
		seq = seq % BT_L2CAP_CONTROL_SEQ_MAX;
	}

	/* Restart monitor timer if it is active */
	l2cap_br_start_timer(br_chan, atomic_test_bit(br_chan->flags, L2CAP_FLAG_RET_TIMER), true);

	/* Currently, buffer seq is unsupported. */
	br_chan->buffer_seq = br_chan->expected_tx_seq;

	br_chan->expected_tx_seq = seq;

	if (chan_has_data(br_chan)) {
		LOG_DBG("chan %p ready", br_chan);
		raise_data_ready(br_chan);
	} else {
		/* Send S-Frame if there is not any pending I-frame */
		int err;

		err = l2cap_br_send_s_frame(br_chan, BT_L2CAP_CONTROL_S_RR, K_NO_WAIT);
		if (err) {
			LOG_ERR("Fail to send frame %d on %p", err, br_chan);
			bt_l2cap_chan_disconnect(&br_chan->chan);
		}
	}
}

static void bt_l2cap_br_ret_fc_i_recv(struct bt_l2cap_br_chan *br_chan, struct net_buf *buf)
{
	uint16_t control;
	uint32_t ext_control;
	uint16_t req_seq;
	uint16_t tx_seq;
	uint8_t r = 0;
	uint8_t sar = 0;
	uint8_t f = 0;
	bool discard = false;
	bool expected_tx_seq = false;
	int err;

	if ((br_chan->rx.mode == BT_L2CAP_BR_LINK_MODE_ERET) ||
	    (br_chan->rx.mode == BT_L2CAP_BR_LINK_MODE_STREAM)) {
		if (br_chan->rx.extended_control) {
			ext_control = net_buf_pull_le32(buf);
			req_seq = BT_L2CAP_I_FRAME_EXT_CONTROL_GET_REQ_SEQ(ext_control);
			f = BT_L2CAP_I_FRAME_EXT_CONTROL_GET_F(ext_control);
			sar = BT_L2CAP_I_FRAME_EXT_CONTROL_GET_SAR(ext_control);
			tx_seq = BT_L2CAP_I_FRAME_EXT_CONTROL_GET_TX_SEQ(ext_control);
		} else {
			control = net_buf_pull_le16(buf);
			req_seq = BT_L2CAP_I_FRAME_ENH_CONTROL_GET_REQ_SEQ(control);
			f = BT_L2CAP_I_FRAME_ENH_CONTROL_GET_F(control);
			sar = BT_L2CAP_I_FRAME_ENH_CONTROL_GET_SAR(control);
			tx_seq = BT_L2CAP_I_FRAME_ENH_CONTROL_GET_TX_SEQ(control);
		}

		err = bt_l2cap_br_update_f(br_chan, f);
		if (err) {
			/* Invalid F bit. Ignore the I-frame. */
			return;
		}
	} else {
		control = net_buf_pull_le16(buf);
		req_seq = BT_L2CAP_I_FRAME_STD_CONTROL_GET_REQ_SEQ(control);
		sar = BT_L2CAP_I_FRAME_STD_CONTROL_GET_SAR(control);
		r = BT_L2CAP_I_FRAME_STD_CONTROL_GET_R(control);
		tx_seq = BT_L2CAP_I_FRAME_STD_CONTROL_GET_TX_SEQ(control);
		bt_l2cap_br_update_r(br_chan, r);
	}

	if (br_chan->rx.mode == BT_L2CAP_BR_LINK_MODE_STREAM) {
		bt_l2cap_br_update_expected_tx_seq(br_chan, tx_seq + 1);
		/* Missing report */
		goto valid_frame;
	}

	/* The first valid I-frame received after an REJ was sent, with a TxSeq of the
	 * received I-frame equal to ReqSeq of the REJ, shall clear the REJ Exception
	 * condition.
	 */
	if (br_chan->req_seq == tx_seq) {
		LOG_DBG("Seq %zu equals to req_seq, clears the REJ exception", tx_seq);
		atomic_clear_bit(br_chan->flags, L2CAP_FLAG_SEND_FRAME_REJ);
	}

	if (br_chan->expected_tx_seq == tx_seq) {
		/* Valid TX seq received */
		LOG_DBG("Valid information received seq %zu", tx_seq);
		expected_tx_seq = true;
	} else {
		if (bt_l2cap_br_check_tx_seq_duplicated(br_chan, tx_seq)) {
			/*
			 * The Information field shall be discarded since it has already been
			 * received.
			 */
			discard = true;
			LOG_DBG("Duplicated Information received %zu", tx_seq);
		} else if (bt_l2cap_br_check_tx_seq_out_of_sequence(br_chan, tx_seq)) {
			/* The missing I-frame(s) are considered lost and ExpectedTXSeq is
			 * set equal to TxSeq+1.
			 */
			LOG_DBG("Missing I-frame detected %zu", tx_seq);
			if (br_chan->rx.mode == BT_L2CAP_BR_LINK_MODE_FC) {
				expected_tx_seq = true;
				/* TODO: Missing report */
			} else {
				/* A REJ exception is triggered, and an REJ frame with
				 * ReqSeq equal to ExpectedTxSeq shall be sent to initiate
				 * recovery. The received I-frame shall be discarded.
				 */
				discard = true;
				LOG_DBG("Set REJ Exception");
				if (!atomic_test_and_set_bit(br_chan->flags,
							     L2CAP_FLAG_SEND_FRAME_REJ)) {
					LOG_DBG("Set REJ Exception received flag");
					atomic_set_bit(br_chan->flags,
						       L2CAP_FLAG_SEND_FRAME_REJ_CHANGED);
					err = l2cap_br_send_s_frame(br_chan, BT_L2CAP_CONTROL_S_RR,
								    K_NO_WAIT);
					if (err) {
						LOG_ERR("Fail to send frame %d on %p", err,
							br_chan);
						bt_l2cap_chan_disconnect(&br_chan->chan);
					}
				}
			}
		} else {
			/* An invalid TxSeq value is a value that does not meet either of
			 * the above conditions and TxSeq is not equal to ExpectedTxSeq.
			 * An I-frame with an invalid TxSeq is likely to have errors in
			 * the control field and shall be silently discarded.
			 */
			return;
		}
	}

	err = bt_l2cap_br_update_req_seq(br_chan, req_seq, false);
	if (err) {
		return;
	}

	if (discard) {
		return;
	}

	if (br_chan->rx.mode == BT_L2CAP_BR_LINK_MODE_ERET) {
		if (f && !atomic_test_and_clear_bit(br_chan->flags, L2CAP_FLAG_REJ_ACTIONED)) {
			/* Retransmit I-frames and Send-Pending-I-frames */
			atomic_set_bit(br_chan->flags, L2CAP_FLAG_NEW_I_FRAME);
			atomic_set_bit(br_chan->flags, L2CAP_FLAG_RET_I_FRAME);
		}
	}

valid_frame:
	switch (sar) {
	case BT_L2CAP_CONTROL_SAR_UNSEG:
		__fallthrough;
	case BT_L2CAP_CONTROL_SAR_START:
		if (buf->len < 2) {
			LOG_WRN("Invalid SDU length");
			bt_l2cap_chan_disconnect(&br_chan->chan);
			return;
		}
		break;
	case BT_L2CAP_CONTROL_SAR_END:
		__fallthrough;
	case BT_L2CAP_CONTROL_SAR_CONTI:
		break;
	}

	/* Redirect to experimental API. */
	IF_ENABLED(CONFIG_BT_L2CAP_SEG_RECV, ({
		if (br_chan->chan.ops->seg_recv) {
			bt_l2cap_br_recv_seg_direct(br_chan, buf, sar);
			goto done;
		}
	}))

	if (br_chan->chan.ops->alloc_buf) {
		err = bt_l2cap_br_recv_seg(br_chan, buf, sar);
		if (err == -ENOBUFS) {
			expected_tx_seq = false;

			if ((br_chan->rx.mode == BT_L2CAP_BR_LINK_MODE_ERET) &&
			    (!atomic_test_and_set_bit(br_chan->flags, L2CAP_FLAG_LOCAL_BUSY))) {
				atomic_set_bit(br_chan->flags, L2CAP_FLAG_LOCAL_BUSY_CHANGED);
				err = l2cap_br_send_s_frame(br_chan, BT_L2CAP_CONTROL_S_RR,
							    K_NO_WAIT);
				if (err) {
					LOG_ERR("Fail to send frame %d on %p", err, br_chan);
					bt_l2cap_chan_disconnect(&br_chan->chan);
				}
			}
		}
		goto done;
	}

	err = br_chan->chan.ops->recv(&br_chan->chan, buf);
	if (err < 0) {
		if (err != -EINPROGRESS) {
			LOG_ERR("err %d", err);
			bt_l2cap_chan_disconnect(&br_chan->chan);
		}
		goto done;
	}

done:
	if (expected_tx_seq) {
		bt_l2cap_br_update_expected_tx_seq(br_chan, tx_seq + 1);
	}
}

static bool bt_l2cap_br_check_valid_fcs(struct bt_l2cap_br_chan *br_chan, struct net_buf *buf)
{
	if (br_chan->rx.mode != BT_L2CAP_BR_LINK_MODE_BASIC) {
		if (br_chan->rx.fcs == BT_L2CAP_BR_FCS_16BIT) {
			uint16_t lfcs;
			uint16_t cfcs;

			lfcs = net_buf_remove_le16(buf);
			cfcs = crc16_reflect(0xA001, 0, &buf->data[0], buf->len);

			if (lfcs != cfcs) {
				return false;
			}
		}
	}
	return true;
}

static uint16_t bt_l2cap_br_get_required_len(struct bt_l2cap_br_chan *br_chan)
{
	uint16_t len = 0;

	if (br_chan->rx.mode != BT_L2CAP_BR_LINK_MODE_BASIC) {
		if (br_chan->rx.fcs == BT_L2CAP_BR_FCS_16BIT) {
			len += BT_L2CAP_FCS_SIZE;
		}
	}

	if (br_chan->rx.mode != BT_L2CAP_BR_LINK_MODE_BASIC) {
		if (br_chan->rx.extended_control) {
			len += BT_L2CAP_EXT_CONTROL_SIZE;
		} else {
			len += BT_L2CAP_STD_CONTROL_SIZE;
		}
	}

	return len;
}

static void bt_l2cap_br_ret_fc_recv(struct bt_l2cap_br_chan *br_chan, struct net_buf *buf)
{
	struct bt_l2cap_hdr *hdr;
	uint16_t control;
	uint32_t ext_control;
	uint8_t type;
	struct net_buf_simple_state state;

	hdr = (struct bt_l2cap_hdr *)buf->data;

	if (bt_l2cap_br_get_required_len(br_chan) > hdr->len) {
		LOG_WRN("Invalid Pdu len %u > %u", bt_l2cap_br_get_required_len(br_chan), hdr->len);
		bt_l2cap_chan_disconnect(&br_chan->chan);
		return;
	}

	if (buf->len != (hdr->len + sizeof(*hdr))) {
		LOG_WRN("Invalid frame %zd != %zd with short packet on%p", buf->len,
			(hdr->len + sizeof(*hdr)), br_chan);
		/* Discard the frame */
		return;
	}

	if (!bt_l2cap_br_check_valid_fcs(br_chan, buf)) {
		LOG_WRN("Invalid frame with incorrect FCS on %p", br_chan);
		/* Discard the frame */
		return;
	}

	if (buf->len > br_chan->rx.mps) {
		LOG_WRN("PDU size > MPS (%u > %u)", buf->len, br_chan->rx.mps);
		bt_l2cap_chan_disconnect(&br_chan->chan);
		return;
	}

	net_buf_pull_mem(buf, sizeof(*hdr));

	net_buf_simple_save(&buf->b, &state);
	if ((br_chan->rx.mode == BT_L2CAP_BR_LINK_MODE_ERET) ||
	    (br_chan->rx.mode == BT_L2CAP_BR_LINK_MODE_STREAM)) {
		if (br_chan->rx.extended_control) {
			ext_control = net_buf_pull_le32(buf);
			type = (uint8_t)BT_L2CAP_S_FRAME_EXT_CONTROL_GET_TYPE(ext_control);
		} else {
			control = net_buf_pull_le16(buf);
			type = (uint8_t)BT_L2CAP_S_FRAME_ENH_CONTROL_GET_TYPE(control);
		}
	} else {
		control = net_buf_pull_le16(buf);
		type = (uint8_t)BT_L2CAP_S_FRAME_STD_CONTROL_GET_TYPE(control);
	}
	net_buf_simple_restore(&buf->b, &state);

	if (type == BT_L2CAP_CONTROL_TYPE_S) {
		bt_l2cap_br_ret_fc_s_recv(br_chan, buf);
	} else {
		bt_l2cap_br_ret_fc_i_recv(br_chan, buf);
	}
}
#endif /* L2CAP_BR_RET_FC_ENABLE */

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

#if L2CAP_BR_RET_FC_ENABLE
	if (BR_CHAN(chan)->rx.mode != BT_L2CAP_BR_LINK_MODE_BASIC) {
		/* Add back HDR for FCS check */
		net_buf_push(buf, sizeof(*hdr));
		bt_l2cap_br_ret_fc_recv(BR_CHAN(chan), buf);
	} else {
#endif /* L2CAP_BR_RET_FC_ENABLE */
		chan->ops->recv(chan, buf);
#if L2CAP_BR_RET_FC_ENABLE
	}
#endif /* L2CAP_BR_RET_FC_ENABLE */
	net_buf_unref(buf);
}

int bt_l2cap_br_chan_recv_complete(struct bt_l2cap_chan *chan)
{
#if L2CAP_BR_RET_FC_ENABLE
	struct bt_l2cap_br_chan *br_chan;
	int err = 0;

	br_chan = BR_CHAN(chan);

	LOG_ERR("Receiving completed on %p", br_chan);

	if ((br_chan->rx.mode == BT_L2CAP_BR_LINK_MODE_ERET) &&
	    atomic_test_and_clear_bit(br_chan->flags, L2CAP_FLAG_LOCAL_BUSY)) {

		if (!atomic_test_bit(br_chan->flags, L2CAP_FLAG_LOCAL_BUSY_CHANGED) &&
		    !atomic_test_and_set_bit(br_chan->flags, L2CAP_FLAG_SEND_FRAME_P)) {
			/* State: XMIT
			 * Local Busy Clears
			 * Action: send RR(P=1)
			 */
			br_chan->retry_count = 1;

			l2cap_br_start_timer(br_chan, false, true);

			atomic_set_bit(br_chan->flags, L2CAP_FLAG_SEND_FRAME_P_CHANGED);
			err = l2cap_br_send_s_frame(br_chan, BT_L2CAP_CONTROL_S_RR, K_NO_WAIT);
			if (err) {
				LOG_ERR("Fail to send frame %d on %p", err, br_chan);
				bt_l2cap_chan_disconnect(&br_chan->chan);
			}
		} else {
			atomic_clear_bit(br_chan->flags, L2CAP_FLAG_LOCAL_BUSY_CHANGED);
		}
	}

	return err;
#else
	return -ENOTSUP;
#endif
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

#if defined(CONFIG_BT_L2CAP_CLS)
static struct bt_l2cap_cls *l2cap_br_cls_lookup_psm(uint16_t psm)
{
	struct bt_l2cap_cls *cls, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&br_clses, cls, next, node) {
		if (cls->psm == psm) {
			return cls;
		}
	}

	return NULL;
}

int bt_l2cap_cls_register(struct bt_l2cap_cls *cls)
{
	if ((!cls) || (!cls->psm) || (!cls->recv)) {
		return -EINVAL;
	}

	if (cls->sec_level > BT_SECURITY_L4) {
		return -EINVAL;
	} else if (cls->sec_level == BT_SECURITY_L0) {
		cls->sec_level = BT_SECURITY_L1;
	}

	/* Check if given PSM is already in use */
	if (l2cap_br_cls_lookup_psm(cls->psm)) {
		LOG_DBG("PSM already registered");
		return -EADDRINUSE;
	}

	LOG_DBG("PSM 0x%04x", cls->psm);

	sys_slist_append(&br_clses, &cls->node);

	return 0;
}

int bt_l2cap_cls_unregister(uint16_t psm)
{
	struct bt_l2cap_cls *cls;

	LOG_DBG("PSM 0x%04x", psm);

	cls = l2cap_br_cls_lookup_psm(psm);
	if (!cls) {
		LOG_DBG("PSM not found");
		return -EINVAL;
	}

	sys_slist_find_and_remove(&br_clses, &cls->node);

	return 0;
}

int bt_l2cap_cls_send(struct bt_conn *conn, uint16_t psm, struct net_buf *buf)
{
	struct bt_l2cap_chan *chan;
	struct bt_l2cap_chan *chan_sig;
	struct bt_l2cap_br *l2cap;
	struct bt_l2cap_cls_hdr *hdr;

	if ((!conn) || (!psm) || (!buf)) {
		return -EINVAL;
	}

	chan = bt_l2cap_br_lookup_rx_cid(conn, BT_L2CAP_CID_CLS);
	if (!chan) {
		LOG_WRN("Ignoring data for unknown channel ID 0x%04x", BT_L2CAP_CID_CLS);
		return -EINVAL;
	}

	chan_sig = bt_l2cap_br_lookup_rx_cid(conn, BT_L2CAP_CID_BR_SIG);
	if (!chan_sig) {
		LOG_WRN("Ignoring data for unknown channel ID 0x%04x", BT_L2CAP_CID_BR_SIG);
		return -EINVAL;
	}

	l2cap = CONTAINER_OF(chan_sig, struct bt_l2cap_br, chan.chan);
	if (!(l2cap->info_feat_mask & L2CAP_FEAT_CLS_MASK)) {
		return -ENOTSUP;
	}

	hdr = net_buf_push(buf, sizeof(*hdr));
	hdr->psm = sys_cpu_to_le16(psm);

	return bt_l2cap_br_chan_send_cb(chan, buf, NULL, NULL);
}

static void l2cap_cls_connected(struct bt_l2cap_chan *chan)
{
	/* Unsupported. Ignore the callback */
}

static void l2cap_cls_disconnected(struct bt_l2cap_chan *chan)
{
	/* Unsupported. Ignore the callback */
}

static int l2cap_cls_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_l2cap_cls_hdr *hdr;
	struct bt_l2cap_cls *cls;
	int err;

	if (!chan->conn) {
		/* Not valid connection */
		LOG_WRN("Not valid connection of chan %p", chan);
		return 0;
	}

	if (buf->len < sizeof(*hdr)) {
		LOG_ERR("Too small L2CAP connectionless PDU");
		return 0;
	}

	hdr = net_buf_pull(buf, sizeof(*hdr));

	cls = l2cap_br_cls_lookup_psm(hdr->psm);
	if (!cls) {
		/* Not valid connection */
		LOG_WRN("Not valid connectionless channel found %p", chan);
		return 0;
	}

	if (cls->sec_level > chan->conn->sec_level) {
		err = bt_conn_set_security(chan->conn, cls->sec_level);
		if (err == -EBUSY) {
			LOG_WRN("Security (level %u) is in progress", cls->sec_level);
		} else if (err) {
			LOG_WRN("Fail to set security level %u", cls->sec_level);
		} else {
			LOG_DBG("Set security level %u", cls->sec_level);
		}
		return 0;
	}

	if (cls->recv) {
		cls->recv(chan->conn, buf);
	}

	return 0;
}

static int l2cap_cls_accept(struct bt_conn *conn, struct bt_l2cap_chan **chan)
{
	int i;
	static const struct bt_l2cap_chan_ops ops = {
		.connected = l2cap_cls_connected,
		.disconnected = l2cap_cls_disconnected,
		.recv = l2cap_cls_recv,
	};

	LOG_DBG("conn %p handle %u", conn, conn->handle);

	for (i = 0; i < ARRAY_SIZE(bt_l2cap_cls_pool); i++) {
		struct bt_l2cap_cls_chan *l2cap = &bt_l2cap_cls_pool[i];

		if (l2cap->chan.chan.conn) {
			continue;
		}

		l2cap->chan.chan.ops = &ops;
		l2cap->chan.rx.mtu = BT_L2CAP_CLS_RX_MTU;
		l2cap->chan.tx.mtu = L2CAP_BR_MIN_MTU;
		*chan = &l2cap->chan.chan;

		return 0;
	}

	LOG_ERR("No available L2CAP context for conn %p", conn);

	return -ENOMEM;
}

BT_L2CAP_BR_CHANNEL_DEFINE(cls_fixed_chan, BT_L2CAP_CID_CLS, l2cap_cls_accept);

#endif /* CONFIG_BT_L2CAP_CLS */

void bt_l2cap_br_init(void)
{
	sys_slist_init(&br_servers);

	if (IS_ENABLED(CONFIG_BT_RFCOMM)) {
		bt_rfcomm_init();
	}

	if (IS_ENABLED(CONFIG_BT_AVDTP)) {
		bt_avdtp_init();
	}

	bt_sdp_init();

	if (IS_ENABLED(CONFIG_BT_A2DP)) {
		bt_a2dp_init();
	}
}
