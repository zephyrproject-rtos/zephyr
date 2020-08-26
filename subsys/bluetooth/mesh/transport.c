/*  Bluetooth Mesh */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/util.h>
#include <sys/byteorder.h>

#include <net/buf.h>

#include <bluetooth/hci.h>
#include <bluetooth/mesh.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_MESH_DEBUG_TRANS)
#define LOG_MODULE_NAME bt_mesh_transport
#include "common/log.h"

#include "host/testing.h"

#include "crypto.h"
#include "adv.h"
#include "mesh.h"
#include "net.h"
#include "lpn.h"
#include "friend.h"
#include "access.h"
#include "foundation.h"
#include "settings.h"
#include "transport.h"

#define AID_MASK                    ((uint8_t)(BIT_MASK(6)))

#define SEG(data)                   ((data)[0] >> 7)
#define AKF(data)                   (((data)[0] >> 6) & 0x01)
#define AID(data)                   ((data)[0] & AID_MASK)
#define ASZMIC(data)                (((data)[1] >> 7) & 1)

#define APP_MIC_LEN(aszmic) ((aszmic) ? BT_MESH_MIC_LONG : BT_MESH_MIC_SHORT)

#define UNSEG_HDR(akf, aid)         ((akf << 6) | (aid & AID_MASK))
#define SEG_HDR(akf, aid)           (UNSEG_HDR(akf, aid) | 0x80)

#define BLOCK_COMPLETE(seg_n)       (uint32_t)(((uint64_t)1 << (seg_n + 1)) - 1)

#define SEQ_AUTH(iv_index, seq)     (((uint64_t)iv_index) << 24 | (uint64_t)seq)

/* Number of retransmit attempts (after the initial transmit) per segment */
#define SEG_RETRANSMIT_ATTEMPTS     4

/* "This timer shall be set to a minimum of 200 + 50 * TTL milliseconds.".
 * We use 400 since 300 is a common send duration for standard HCI, and we
 * need to have a timeout that's bigger than that.
 */
#define SEG_RETRANSMIT_TIMEOUT_UNICAST(tx) (400 + 50 * (tx)->ttl)
/* When sending to a group, the messages are not acknowledged, and there's no
 * reason to delay the repetitions significantly. Delaying by more than 0 ms
 * to avoid flooding the network.
 */
#define SEG_RETRANSMIT_TIMEOUT_GROUP 50

#define SEG_RETRANSMIT_TIMEOUT(tx)                                             \
	(BT_MESH_ADDR_IS_UNICAST(tx->dst) ?                                    \
		 SEG_RETRANSMIT_TIMEOUT_UNICAST(tx) :                          \
		 SEG_RETRANSMIT_TIMEOUT_GROUP)
/* How long to wait for available buffers before giving up */
#define BUF_TIMEOUT                 K_NO_WAIT

static struct seg_tx {
	struct bt_mesh_subnet *sub;
	void                  *seg[CONFIG_BT_MESH_TX_SEG_MAX];
	uint64_t              seq_auth;
	uint16_t              src;
	uint16_t              dst;
	uint16_t              len;
	uint8_t               hdr;
	uint8_t               xmit;
	uint8_t               seg_n;         /* Last segment index */
	uint8_t               seg_o;         /* Segment being sent */
	uint8_t               nack_count;    /* Number of unacked segs */
	uint8_t               attempts;      /* Remaining tx attempts */
	uint8_t               ttl;           /* Transmitted TTL value */
	uint8_t               seg_pending;   /* Number of segments pending */
	uint8_t               blocked:1,     /* Blocked by ongoing tx */
			      ctl:1,         /* Control packet */
			      aszmic:1,      /* MIC size */
			      started:1,     /* Start cb called */
			      sending:1,     /* Sending is in progress */
			      friend_cred:1; /* Using Friend credentials */
	const struct bt_mesh_send_cb *cb;
	void                  *cb_data;
	struct k_delayed_work retransmit;    /* Retransmit timer */
} seg_tx[CONFIG_BT_MESH_TX_SEG_MSG_COUNT];

static struct seg_rx {
	struct bt_mesh_subnet   *sub;
	void                    *seg[CONFIG_BT_MESH_RX_SEG_MAX];
	uint64_t                    seq_auth;
	uint16_t                    src;
	uint16_t                    dst;
	uint16_t                    len;
	uint8_t                     hdr;
	uint8_t                     seg_n:5,
				 ctl:1,
				 in_use:1,
				 obo:1;
	uint8_t                     ttl;
	uint32_t                    block;
	uint32_t                    last;
	struct k_delayed_work    ack;
} seg_rx[CONFIG_BT_MESH_RX_SEG_MSG_COUNT];

K_MEM_SLAB_DEFINE(segs, BT_MESH_APP_SEG_SDU_MAX, CONFIG_BT_MESH_SEG_BUFS, 4);

static uint16_t hb_sub_dst = BT_MESH_ADDR_UNASSIGNED;

void bt_mesh_set_hb_sub_dst(uint16_t addr)
{
	hb_sub_dst = addr;
}

static int send_unseg(struct bt_mesh_net_tx *tx, struct net_buf_simple *sdu,
		      const struct bt_mesh_send_cb *cb, void *cb_data,
		      const uint8_t *ctl_op)
{
	struct net_buf *buf;

	buf = bt_mesh_adv_create(BT_MESH_ADV_DATA, tx->xmit, BUF_TIMEOUT);
	if (!buf) {
		BT_ERR("Out of network buffers");
		return -ENOBUFS;
	}

	net_buf_reserve(buf, BT_MESH_NET_HDR_LEN);

	if (ctl_op) {
		net_buf_add_u8(buf, TRANS_CTL_HDR(*ctl_op, 0));
	} else if (BT_MESH_IS_DEV_KEY(tx->ctx->app_idx)) {
		net_buf_add_u8(buf, UNSEG_HDR(0, 0));
	} else {
		net_buf_add_u8(buf, UNSEG_HDR(1, tx->aid));
	}

	net_buf_add_mem(buf, sdu->data, sdu->len);

	if (IS_ENABLED(CONFIG_BT_MESH_FRIEND)) {
		if (!bt_mesh_friend_queue_has_space(tx->sub->net_idx,
						    tx->src, tx->ctx->addr,
						    NULL, 1)) {
			if (BT_MESH_ADDR_IS_UNICAST(tx->ctx->addr)) {
				BT_ERR("Not enough space in Friend Queue");
				net_buf_unref(buf);
				return -ENOBUFS;
			} else {
				BT_WARN("No space in Friend Queue");
				goto send;
			}
		}

		if (bt_mesh_friend_enqueue_tx(tx, BT_MESH_FRIEND_PDU_SINGLE,
					      NULL, 1, &buf->b) &&
		    BT_MESH_ADDR_IS_UNICAST(tx->ctx->addr)) {
			/* PDUs for a specific Friend should only go
			 * out through the Friend Queue.
			 */
			net_buf_unref(buf);
			send_cb_finalize(cb, cb_data);
			return 0;
		}
	}

send:
	return bt_mesh_net_send(tx, buf, cb, cb_data);
}

static inline uint8_t seg_len(bool ctl)
{
	if (ctl) {
		return BT_MESH_CTL_SEG_SDU_MAX;
	} else {
		return BT_MESH_APP_SEG_SDU_MAX;
	}
}

bool bt_mesh_tx_in_progress(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(seg_tx); i++) {
		if (seg_tx[i].nack_count) {
			return true;
		}
	}

	return false;
}

static void seg_tx_done(struct seg_tx *tx, uint8_t seg_idx)
{
	k_mem_slab_free(&segs, (void **)&tx->seg[seg_idx]);
	tx->seg[seg_idx] = NULL;
	tx->nack_count--;
}

static bool seg_tx_blocks(struct seg_tx *tx, uint16_t src, uint16_t dst)
{
	return (tx->src == src) && (tx->dst == dst);
}

static void seg_tx_unblock_check(struct seg_tx *tx)
{
	struct seg_tx *blocked = NULL;
	int i;

	/* Unblock the first blocked tx with the same params. */
	for (i = 0; i < ARRAY_SIZE(seg_tx); ++i) {
		if (&seg_tx[i] != tx &&
		    seg_tx[i].blocked &&
		    seg_tx_blocks(tx, seg_tx[i].src, seg_tx[i].dst) &&
		    (!blocked || seg_tx[i].seq_auth < blocked->seq_auth)) {
			blocked = &seg_tx[i];
		}
	}

	if (blocked) {
		BT_DBG("Unblocked 0x%04x",
		       (uint16_t)(blocked->seq_auth & TRANS_SEQ_ZERO_MASK));
		blocked->blocked = false;
		k_delayed_work_submit(&blocked->retransmit, K_NO_WAIT);
	}
}

static void seg_tx_reset(struct seg_tx *tx)
{
	int i;

	k_delayed_work_cancel(&tx->retransmit);

	tx->cb = NULL;
	tx->cb_data = NULL;
	tx->seq_auth = 0U;
	tx->sub = NULL;
	tx->src = BT_MESH_ADDR_UNASSIGNED;
	tx->dst = BT_MESH_ADDR_UNASSIGNED;
	tx->blocked = false;

	for (i = 0; i <= tx->seg_n && tx->nack_count; i++) {
		if (!tx->seg[i]) {
			continue;
		}

		seg_tx_done(tx, i);
	}

	tx->nack_count = 0;

	if (atomic_test_and_clear_bit(bt_mesh.flags, BT_MESH_IVU_PENDING)) {
		BT_DBG("Proceeding with pending IV Update");
		/* bt_mesh_net_iv_update() will re-enable the flag if this
		 * wasn't the only transfer.
		 */
		if (bt_mesh_net_iv_update(bt_mesh.iv_index, false)) {
			bt_mesh_net_sec_update(NULL);
		}
	}
}

static inline void seg_tx_complete(struct seg_tx *tx, int err)
{
	const struct bt_mesh_send_cb *cb = tx->cb;
	void *cb_data = tx->cb_data;

	seg_tx_unblock_check(tx);

	seg_tx_reset(tx);

	if (cb && cb->end) {
		cb->end(err, cb_data);
	}
}

static void schedule_retransmit(struct seg_tx *tx)
{
	if (!tx->nack_count) {
		return;
	}

	if (--tx->seg_pending || tx->sending) {
		return;
	}

	BT_DBG("");

	/* If we haven't gone through all the segments for this attempt yet,
	 * (likely because of a buffer allocation failure or because we
	 * called this from inside bt_mesh_net_send), we should continue the
	 * retransmit immediately, as we just freed up a tx buffer.
	 */
	k_delayed_work_submit(&tx->retransmit,
			      tx->seg_o ? K_NO_WAIT :
					  K_MSEC(SEG_RETRANSMIT_TIMEOUT(tx)));
}

static void seg_send_start(uint16_t duration, int err, void *user_data)
{
	struct seg_tx *tx = user_data;

	if (!tx->started && tx->cb && tx->cb->start) {
		tx->cb->start(duration, err, tx->cb_data);
		tx->started = 1U;
	}

	/* If there's an error in transmitting the 'sent' callback will never
	 * be called. Make sure that we kick the retransmit timer also in this
	 * case since otherwise we risk the transmission of becoming stale.
	 */
	if (err) {
		schedule_retransmit(tx);
	}
}

static void seg_sent(int err, void *user_data)
{
	struct seg_tx *tx = user_data;

	schedule_retransmit(tx);
}

static const struct bt_mesh_send_cb seg_sent_cb = {
	.start = seg_send_start,
	.end = seg_sent,
};

static void seg_tx_buf_build(struct seg_tx *tx, uint8_t seg_o,
			     struct net_buf_simple *buf)
{
	uint16_t seq_zero = tx->seq_auth & TRANS_SEQ_ZERO_MASK;
	uint8_t len = MIN(seg_len(tx->ctl), tx->len - (seg_len(tx->ctl) * seg_o));

	net_buf_simple_add_u8(buf, tx->hdr);
	net_buf_simple_add_u8(buf, (tx->aszmic << 7) | seq_zero >> 6);
	net_buf_simple_add_u8(buf, (((seq_zero & 0x3f) << 2) | (seg_o >> 3)));
	net_buf_simple_add_u8(buf, ((seg_o & 0x07) << 5) | tx->seg_n);
	net_buf_simple_add_mem(buf, tx->seg[seg_o], len);
}

static void seg_tx_send_unacked(struct seg_tx *tx)
{
	if (!tx->nack_count) {
		return;
	}

	struct bt_mesh_msg_ctx ctx = {
		.net_idx = tx->sub->net_idx,
		/* App idx only used by network to detect control messages: */
		.app_idx = (tx->ctl ? BT_MESH_KEY_UNUSED : 0),
		.addr = tx->dst,
		.send_rel = true,
		.send_ttl = tx->ttl,
	};
	struct bt_mesh_net_tx net_tx = {
		.sub = tx->sub,
		.ctx = &ctx,
		.src = tx->src,
		.xmit = tx->xmit,
		.friend_cred = tx->friend_cred,
		.aid = tx->hdr & AID_MASK,
	};

	if (!tx->attempts) {
		if (BT_MESH_ADDR_IS_UNICAST(tx->dst)) {
			BT_ERR("Ran out of retransmit attempts");
			seg_tx_complete(tx, -ETIMEDOUT);
		} else {
			/* Segmented sending to groups doesn't have acks, so
			 * running out of attempts is the expected behavior.
			 */
			seg_tx_complete(tx, 0);
		}

		return;
	}

	BT_DBG("SeqZero: 0x%04x Attempts: %u",
	       (uint16_t)(tx->seq_auth & TRANS_SEQ_ZERO_MASK), tx->attempts);

	tx->sending = 1U;

	for (; tx->seg_o <= tx->seg_n; tx->seg_o++) {
		struct net_buf *seg;
		int err;

		if (!tx->seg[tx->seg_o]) {
			continue;
		}

		seg = bt_mesh_adv_create(BT_MESH_ADV_DATA, tx->xmit,
					 BUF_TIMEOUT);
		if (!seg) {
			BT_DBG("Allocating segment failed");
			goto end;
		}

		net_buf_reserve(seg, BT_MESH_NET_HDR_LEN);
		seg_tx_buf_build(tx, tx->seg_o, &seg->b);

		tx->seg_pending++;

		BT_DBG("Sending %u/%u", tx->seg_o, tx->seg_n);

		err = bt_mesh_net_send(&net_tx, seg, &seg_sent_cb, tx);
		if (err) {
			BT_DBG("Sending segment failed");
			tx->seg_pending--;
			goto end;
		}
	}

	tx->seg_o = 0U;
	tx->attempts--;

end:
	if (!tx->seg_pending) {
		k_delayed_work_submit(&tx->retransmit,
				      K_MSEC(SEG_RETRANSMIT_TIMEOUT(tx)));
	}

	tx->sending = 0U;
}

static void seg_retransmit(struct k_work *work)
{
	struct seg_tx *tx = CONTAINER_OF(work, struct seg_tx, retransmit);

	seg_tx_send_unacked(tx);
}

static int send_seg(struct bt_mesh_net_tx *net_tx, struct net_buf_simple *sdu,
		    const struct bt_mesh_send_cb *cb, void *cb_data,
		    uint8_t *ctl_op)
{
	bool blocked = false;
	struct seg_tx *tx;
	uint8_t seg_o;
	int i;

	BT_DBG("src 0x%04x dst 0x%04x app_idx 0x%04x aszmic %u sdu_len %u",
	       net_tx->src, net_tx->ctx->addr, net_tx->ctx->app_idx,
	       net_tx->aszmic, sdu->len);

	for (tx = NULL, i = 0; i < ARRAY_SIZE(seg_tx); i++) {
		if (seg_tx[i].nack_count) {
			blocked |= seg_tx_blocks(&seg_tx[i], net_tx->src,
						 net_tx->ctx->addr);
		} else if (!tx) {
			tx = &seg_tx[i];
		}
	}

	if (!tx) {
		BT_ERR("No multi-segment message contexts available");
		return -EBUSY;
	}

	if (ctl_op) {
		tx->hdr = TRANS_CTL_HDR(*ctl_op, 1);
	} else if (BT_MESH_IS_DEV_KEY(net_tx->ctx->app_idx)) {
		tx->hdr = SEG_HDR(0, 0);
	} else {
		tx->hdr = SEG_HDR(1, net_tx->aid);
	}

	tx->src = net_tx->src;
	tx->dst = net_tx->ctx->addr;
	tx->seg_n = (sdu->len - 1) / seg_len(!!ctl_op);
	tx->seg_o = 0;
	tx->len = sdu->len;
	tx->nack_count = tx->seg_n + 1;
	tx->seq_auth = SEQ_AUTH(BT_MESH_NET_IVI_TX, bt_mesh.seq);
	tx->sub = net_tx->sub;
	tx->cb = cb;
	tx->cb_data = cb_data;
	tx->attempts = SEG_RETRANSMIT_ATTEMPTS;
	tx->seg_pending = 0;
	tx->xmit = net_tx->xmit;
	tx->aszmic = net_tx->aszmic;
	tx->friend_cred = net_tx->friend_cred;
	tx->blocked = blocked;
	tx->started = 0;
	tx->ctl = !!ctl_op;

	if (net_tx->ctx->send_ttl == BT_MESH_TTL_DEFAULT) {
		tx->ttl = bt_mesh_default_ttl_get();
	} else {
		tx->ttl = net_tx->ctx->send_ttl;
	}

	BT_DBG("SeqZero 0x%04x (segs: %u)",
	       (uint16_t)(tx->seq_auth & TRANS_SEQ_ZERO_MASK), tx->nack_count);

	if (IS_ENABLED(CONFIG_BT_MESH_FRIEND) &&
	    !bt_mesh_friend_queue_has_space(tx->sub->net_idx, net_tx->src,
					    tx->dst, &tx->seq_auth,
					    tx->seg_n + 1) &&
	    BT_MESH_ADDR_IS_UNICAST(tx->dst)) {
		BT_ERR("Not enough space in Friend Queue for %u segments",
		       tx->seg_n + 1);
		seg_tx_reset(tx);
		return -ENOBUFS;
	}

	for (seg_o = 0U; sdu->len; seg_o++) {
		void *buf;
		uint16_t len;
		int err;

		err = k_mem_slab_alloc(&segs, &buf, BUF_TIMEOUT);
		if (err) {
			BT_ERR("Out of segment buffers");
			seg_tx_reset(tx);
			return -ENOBUFS;
		}

		len = MIN(sdu->len, seg_len(!!ctl_op));
		memcpy(buf, net_buf_simple_pull_mem(sdu, len), len);

		BT_DBG("seg %u: %s", seg_o, bt_hex(buf, len));

		tx->seg[seg_o] = buf;

		if (IS_ENABLED(CONFIG_BT_MESH_FRIEND)) {
			enum bt_mesh_friend_pdu_type type;

			NET_BUF_SIMPLE_DEFINE(seg, 16);
			seg_tx_buf_build(tx, seg_o, &seg);

			if (seg_o == tx->seg_n) {
				type = BT_MESH_FRIEND_PDU_COMPLETE;
			} else {
				type = BT_MESH_FRIEND_PDU_PARTIAL;
			}

			if (bt_mesh_friend_enqueue_tx(
				    net_tx, type, ctl_op ? NULL : &tx->seq_auth,
				    tx->seg_n + 1, &seg) &&
			    BT_MESH_ADDR_IS_UNICAST(net_tx->ctx->addr)) {
				/* PDUs for a specific Friend should only go
				 * out through the Friend Queue.
				 */
				k_mem_slab_free(&segs, &buf);
				tx->seg[seg_o] = NULL;
			}

		}

	}

	/* This can happen if segments only went into the Friend Queue */
	if (IS_ENABLED(CONFIG_BT_MESH_FRIEND) && !tx->seg[0]) {
		seg_tx_reset(tx);

		/* If there was a callback notify sending immediately since
		 * there's no other way to track this (at least currently)
		 * with the Friend Queue.
		 */
		send_cb_finalize(cb, cb_data);
		return 0;
	}

	if (blocked) {
		/* Move the sequence number, so we don't end up creating
		 * another segmented transmission with the same SeqZero while
		 * this one is blocked.
		 */
		bt_mesh_next_seq();
		BT_DBG("Blocked.");
		return 0;
	}

	seg_tx_send_unacked(tx);

	if (IS_ENABLED(CONFIG_BT_MESH_LOW_POWER) &&
	    bt_mesh_lpn_established()) {
		bt_mesh_lpn_poll();
	}

	return 0;
}

struct bt_mesh_app_key *bt_mesh_app_key_find(uint16_t app_idx)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(bt_mesh.app_keys); i++) {
		struct bt_mesh_app_key *key = &bt_mesh.app_keys[i];

		if (key->net_idx != BT_MESH_KEY_UNUSED &&
		    key->app_idx == app_idx) {
			return key;
		}
	}

	return NULL;
}

int bt_mesh_trans_send(struct bt_mesh_net_tx *tx, struct net_buf_simple *msg,
		       const struct bt_mesh_send_cb *cb, void *cb_data)
{
	const uint8_t *key;
	uint8_t *ad;
	uint8_t aid;
	int err;

	if (msg->len < 1) {
		BT_ERR("Zero-length SDU not allowed");
		return -EINVAL;
	}

	if (msg->len > BT_MESH_TX_SDU_MAX) {
		BT_ERR("Not enough segment buffers for length %u", msg->len);
		return -EMSGSIZE;
	}

	if (net_buf_simple_tailroom(msg) < 4) {
		BT_ERR("Insufficient tailroom for Transport MIC");
		return -EINVAL;
	}

	if (msg->len > BT_MESH_SDU_UNSEG_MAX) {
		tx->ctx->send_rel = true;
	}

	BT_DBG("net_idx 0x%04x app_idx 0x%04x dst 0x%04x", tx->sub->net_idx,
	       tx->ctx->app_idx, tx->ctx->addr);
	BT_DBG("len %u: %s", msg->len, bt_hex(msg->data, msg->len));

	err = bt_mesh_app_key_get(tx->sub, tx->ctx->app_idx,
				  tx->ctx->addr, &key, &aid);
	if (err) {
		return err;
	}

	tx->aid = aid;

	if (!tx->ctx->send_rel || net_buf_simple_tailroom(msg) < 8) {
		tx->aszmic = 0U;
	} else {
		tx->aszmic = 1U;
	}

	if (BT_MESH_ADDR_IS_VIRTUAL(tx->ctx->addr)) {
		ad = bt_mesh_label_uuid_get(tx->ctx->addr);
	} else {
		ad = NULL;
	}

	err = bt_mesh_app_encrypt(key, BT_MESH_IS_DEV_KEY(tx->ctx->app_idx),
				  tx->aszmic, msg, ad, tx->src, tx->ctx->addr,
				  bt_mesh.seq, BT_MESH_NET_IVI_TX);
	if (err) {
		return err;
	}

	if (tx->ctx->send_rel) {
		err = send_seg(tx, msg, cb, cb_data, NULL);
	} else {
		err = send_unseg(tx, msg, cb, cb_data, NULL);
	}

	return err;
}

static void update_rpl(struct bt_mesh_rpl *rpl, struct bt_mesh_net_rx *rx)
{
	rpl->src = rx->ctx.addr;
	rpl->seq = rx->seq;
	rpl->old_iv = rx->old_iv;

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		bt_mesh_store_rpl(rpl);
	}
}

/* Check the Replay Protection List for a replay attempt. If non-NULL match
 * parameter is given the RPL slot is returned but it is not immediately
 * updated (needed for segmented messages), whereas if a NULL match is given
 * the RPL is immediately updated (used for unsegmented messages).
 */
static bool is_replay(struct bt_mesh_net_rx *rx, struct bt_mesh_rpl **match)
{
	int i;

	/* Don't bother checking messages from ourselves */
	if (rx->net_if == BT_MESH_NET_IF_LOCAL) {
		return false;
	}

	/* The RPL is used only for the local node */
	if (!rx->local_match) {
		return false;
	}

	for (i = 0; i < ARRAY_SIZE(bt_mesh.rpl); i++) {
		struct bt_mesh_rpl *rpl = &bt_mesh.rpl[i];

		/* Empty slot */
		if (!rpl->src) {
			if (match) {
				*match = rpl;
			} else {
				update_rpl(rpl, rx);
			}

			return false;
		}

		/* Existing slot for given address */
		if (rpl->src == rx->ctx.addr) {
			if (rx->old_iv && !rpl->old_iv) {
				return true;
			}

			if ((!rx->old_iv && rpl->old_iv) ||
			    rpl->seq < rx->seq) {
				if (match) {
					*match = rpl;
				} else {
					update_rpl(rpl, rx);
				}

				return false;
			} else {
				return true;
			}
		}
	}

	BT_ERR("RPL is full!");
	return true;
}

static void seg_rx_assemble(struct seg_rx *rx, struct net_buf_simple *buf,
			    uint8_t aszmic)
{
	int i;

	net_buf_simple_reset(buf);

	for (i = 0; i <= rx->seg_n; i++) {
		net_buf_simple_add_mem(buf, rx->seg[i],
				       MIN(seg_len(rx->ctl),
					   rx->len - (i * seg_len(rx->ctl))));
	}

	/* Adjust the length to not contain the MIC at the end */
	if (!rx->ctl) {
		buf->len -= APP_MIC_LEN(aszmic);
	}
}

static int remote_devkey_decrypt(struct bt_mesh_net_rx *rx, uint32_t seq, uint8_t *ad,
				 uint8_t aszmic, struct net_buf_simple *buf,
				 struct net_buf_simple *sdu)
{
	struct bt_mesh_cdb_node *node;
	int err;

	if (!IS_ENABLED(CONFIG_BT_MESH_PROVISIONER)) {
		return -ENOENT;
	}

	/* We will try our local devkey separately. */
	if (bt_mesh_elem_find(rx->ctx.addr)) {
		return -ENOENT;
	}

	/*
	 * There is no way of knowing if we should use our
	 * local DevKey or the remote DevKey to decrypt the
	 * message so we must try both.
	 */
	node = bt_mesh_cdb_node_get(rx->ctx.addr);
	if (node == NULL) {
		BT_ERR("No node found for addr 0x%04x", rx->ctx.addr);
		return -EINVAL;
	}

	err = bt_mesh_app_decrypt(node->dev_key, true, aszmic, buf, sdu, ad,
				  rx->ctx.addr, rx->ctx.recv_dst, seq,
				  BT_MESH_NET_IVI_RX(rx));
	if (err) {
		BT_DBG("Unable to decrypt with node DevKey");
		return -EINVAL;
	}

	return 0;
}

static int app_key_decrypt(struct bt_mesh_net_rx *rx,
			   struct bt_mesh_app_key *key, uint32_t seq, uint8_t *ad,
			   uint8_t hdr, uint8_t aszmic, struct net_buf_simple *buf,
			   struct net_buf_simple *sdu)
{
	struct bt_mesh_app_keys *keys;
	int err;

	/* Check that this AppKey matches received net_idx */
	if (key->net_idx != rx->sub->net_idx) {
		return -EINVAL;
	}

	if (rx->new_key && key->updated) {
		keys = &key->keys[1];
	} else {
		keys = &key->keys[0];
	}

	/* Check that the AppKey ID matches */
	if (AID(&hdr) != keys->id) {
		return -EINVAL;
	}

	err = bt_mesh_app_decrypt(keys->val, false, aszmic, buf, sdu, ad,
				  rx->ctx.addr, rx->ctx.recv_dst, seq,
				  BT_MESH_NET_IVI_RX(rx));
	if (err) {
		BT_WARN("Unable to decrypt with AppKey 0x%03x", key->app_idx);
	}

	return err;
}

static int sdu_recv_unseg(struct bt_mesh_net_rx *rx, uint8_t hdr,
			  struct net_buf_simple *buf)
{
	NET_BUF_SIMPLE_DEFINE(sdu, BT_MESH_SDU_UNSEG_MAX);
	uint8_t *ad;
	uint16_t i;
	int err;

	BT_DBG("AKF %u AID 0x%02x", AKF(&hdr), AID(&hdr));
	BT_DBG("len %u: %s", buf->len, bt_hex(buf->data, buf->len));

	if (buf->len < 1 + APP_MIC_LEN(0)) {
		BT_ERR("Too short SDU + MIC");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_MESH_FRIEND) && !rx->local_match) {
		BT_DBG("Ignoring PDU for LPN 0x%04x of this Friend",
		       rx->ctx.recv_dst);
		return 0;
	}

	if (BT_MESH_ADDR_IS_VIRTUAL(rx->ctx.recv_dst)) {
		ad = bt_mesh_label_uuid_get(rx->ctx.recv_dst);
	} else {
		ad = NULL;
	}

	/* Adjust the length to not contain the MIC at the end */
	buf->len -= APP_MIC_LEN(0);

	if (!AKF(&hdr)) {
		/* Attempt remote dev key first, as that is only available for
		 * provisioner devices, which normally don't interact with nodes
		 * that know their local dev key.
		 */
		net_buf_simple_reset(&sdu);
		err = remote_devkey_decrypt(rx, rx->seq, ad, 0, buf, &sdu);
		if (!err) {
			rx->ctx.app_idx = BT_MESH_KEY_DEV_REMOTE;
			bt_mesh_model_recv(rx, &sdu);
			return 0;
		}

		net_buf_simple_reset(&sdu);
		err = bt_mesh_app_decrypt(bt_mesh.dev_key, true, 0, buf, &sdu,
					  ad, rx->ctx.addr, rx->ctx.recv_dst,
					  rx->seq, BT_MESH_NET_IVI_RX(rx));
		if (err) {
			BT_ERR("Unable to decrypt with local DevKey");
			return err;
		}

		rx->ctx.app_idx = BT_MESH_KEY_DEV_LOCAL;
		bt_mesh_model_recv(rx, &sdu);
		return 0;
	}

	for (i = 0U; i < ARRAY_SIZE(bt_mesh.app_keys); i++) {
		struct bt_mesh_app_key *key = &bt_mesh.app_keys[i];

		net_buf_simple_reset(&sdu);
		err = app_key_decrypt(rx, &bt_mesh.app_keys[i], rx->seq, ad,
				      hdr, 0, buf, &sdu);

		if (err) {
			continue;
		}

		rx->ctx.app_idx = key->app_idx;

		bt_mesh_model_recv(rx, &sdu);
		return 0;
	}

	if (rx->local_match) {
		BT_WARN("No matching AppKey");
	}

	return 0;
}

static int sdu_recv_seg(struct seg_rx *seg, uint8_t hdr, uint8_t aszmic,
			struct bt_mesh_net_rx *rx)
{
	NET_BUF_SIMPLE_DEFINE(buf, BT_MESH_RX_SDU_MAX);
	struct net_buf_simple sdu;
	uint32_t seq = (seg->seq_auth & 0xffffff);
	uint8_t *ad;
	uint16_t i;
	int err;

	BT_DBG("ASZMIC %u AKF %u AID 0x%02x", aszmic, AKF(&hdr), AID(&hdr));

	if (seg->len < 1 + APP_MIC_LEN(aszmic)) {
		BT_ERR("Too short SDU + MIC");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_MESH_FRIEND) && !rx->local_match) {
		BT_DBG("Ignoring PDU for LPN 0x%04x of this Friend",
		       rx->ctx.recv_dst);
		return 0;
	}

	if (BT_MESH_ADDR_IS_VIRTUAL(rx->ctx.recv_dst)) {
		ad = bt_mesh_label_uuid_get(rx->ctx.recv_dst);
	} else {
		ad = NULL;
	}

	/* Decrypting in place to avoid creating two assembly buffers.
	 * We'll reassemble the buffer from the segments before each decryption
	 * attempt.
	 */
	if (!AKF(&hdr)) {
		/* Attempt remote dev key first, as that is only available for
		 * provisioner devices, which normally don't interact with nodes
		 * that know their local dev key.
		 */
		seg_rx_assemble(seg, &buf, aszmic);
		net_buf_simple_init_with_data(&sdu, buf.data,
					      seg->len - APP_MIC_LEN(aszmic));
		sdu.len = 0;

		err = remote_devkey_decrypt(rx, seq, ad, aszmic, &buf, &sdu);
		if (!err) {
			rx->ctx.app_idx = BT_MESH_KEY_DEV_REMOTE;
			bt_mesh_model_recv(rx, &sdu);
			return 0;
		}

		seg_rx_assemble(seg, &buf, aszmic);
		net_buf_simple_init_with_data(&sdu, buf.data,
					      seg->len - APP_MIC_LEN(aszmic));
		sdu.len = 0;

		err = bt_mesh_app_decrypt(bt_mesh.dev_key, true, aszmic, &buf,
					  &sdu, ad, rx->ctx.addr,
					  rx->ctx.recv_dst, seq,
					  BT_MESH_NET_IVI_RX(rx));
		if (err) {
			BT_ERR("Unable to decrypt with local DevKey");
			return err;
		}

		rx->ctx.app_idx = BT_MESH_KEY_DEV_LOCAL;
		bt_mesh_model_recv(rx, &sdu);
		return 0;
	}

	for (i = 0U; i < ARRAY_SIZE(bt_mesh.app_keys); i++) {
		struct bt_mesh_app_key *key = &bt_mesh.app_keys[i];

		seg_rx_assemble(seg, &buf, aszmic);
		net_buf_simple_init_with_data(&sdu, buf.data,
					      seg->len - APP_MIC_LEN(aszmic));
		sdu.len = 0;

		err = app_key_decrypt(rx, &bt_mesh.app_keys[i], seq, ad, hdr,
				      aszmic, &buf, &sdu);
		if (err) {
			continue;
		}

		rx->ctx.app_idx = key->app_idx;

		bt_mesh_model_recv(rx, &sdu);
		return 0;
	}

	if (rx->local_match) {
		BT_WARN("No matching AppKey");
	}

	return 0;
}

static struct seg_tx *seg_tx_lookup(uint16_t seq_zero, uint8_t obo, uint16_t addr)
{
	struct seg_tx *tx;
	int i;

	for (i = 0; i < ARRAY_SIZE(seg_tx); i++) {
		tx = &seg_tx[i];

		if ((tx->seq_auth & TRANS_SEQ_ZERO_MASK) != seq_zero) {
			continue;
		}

		if (tx->dst == addr) {
			return tx;
		}

		/* If the expected remote address doesn't match,
		 * but the OBO flag is set and this is the first
		 * acknowledgement, assume it's a Friend that's
		 * responding and therefore accept the message.
		 */
		if (obo && tx->nack_count == tx->seg_n + 1) {
			tx->dst = addr;
			return tx;
		}
	}

	return NULL;
}

static int trans_ack(struct bt_mesh_net_rx *rx, uint8_t hdr,
		     struct net_buf_simple *buf, uint64_t *seq_auth)
{
	struct seg_tx *tx;
	unsigned int bit;
	uint32_t ack;
	uint16_t seq_zero;
	uint8_t obo;

	if (buf->len < 6) {
		BT_ERR("Too short ack message");
		return -EINVAL;
	}

	seq_zero = net_buf_simple_pull_be16(buf);
	obo = seq_zero >> 15;
	seq_zero = (seq_zero >> 2) & TRANS_SEQ_ZERO_MASK;

	if (IS_ENABLED(CONFIG_BT_MESH_FRIEND) && rx->friend_match) {
		BT_DBG("Ack for LPN 0x%04x of this Friend", rx->ctx.recv_dst);
		/* Best effort - we don't have enough info for true SeqAuth */
		*seq_auth = SEQ_AUTH(BT_MESH_NET_IVI_RX(rx), seq_zero);
		return 0;
	}

	ack = net_buf_simple_pull_be32(buf);

	BT_DBG("OBO %u seq_zero 0x%04x ack 0x%08x", obo, seq_zero, ack);

	tx = seg_tx_lookup(seq_zero, obo, rx->ctx.addr);
	if (!tx) {
		BT_WARN("No matching TX context for ack");
		return -EINVAL;
	}

	if (!BT_MESH_ADDR_IS_UNICAST(tx->dst)) {
		BT_ERR("Received ack for group seg");
		return -EINVAL;
	}

	*seq_auth = tx->seq_auth;

	if (!ack) {
		BT_WARN("SDU canceled");
		seg_tx_complete(tx, -ECANCELED);
		return 0;
	}

	if (find_msb_set(ack) - 1 > tx->seg_n) {
		BT_ERR("Too large segment number in ack");
		return -EINVAL;
	}

	k_delayed_work_cancel(&tx->retransmit);

	while ((bit = find_lsb_set(ack))) {
		if (tx->seg[bit - 1]) {
			BT_DBG("seg %u/%u acked", bit - 1, tx->seg_n);
			seg_tx_done(tx, bit - 1);
		}

		ack &= ~BIT(bit - 1);
	}

	if (tx->nack_count) {
		seg_tx_send_unacked(tx);
	} else {
		BT_DBG("SDU TX complete");
		seg_tx_complete(tx, 0);
	}

	return 0;
}

static int trans_heartbeat(struct bt_mesh_net_rx *rx,
			   struct net_buf_simple *buf)
{
	uint8_t init_ttl, hops;
	uint16_t feat;

	if (buf->len < 3) {
		BT_ERR("Too short heartbeat message");
		return -EINVAL;
	}

	if (rx->ctx.recv_dst != hb_sub_dst) {
		BT_WARN("Ignoring heartbeat to non-subscribed destination");
		return 0;
	}

	init_ttl = (net_buf_simple_pull_u8(buf) & 0x7f);
	feat = net_buf_simple_pull_be16(buf);

	hops = (init_ttl - rx->ctx.recv_ttl + 1);

	BT_DBG("src 0x%04x TTL %u InitTTL %u (%u hop%s) feat 0x%04x",
	       rx->ctx.addr, rx->ctx.recv_ttl, init_ttl, hops,
	       (hops == 1U) ? "" : "s", feat);

	bt_mesh_heartbeat(rx->ctx.addr, rx->ctx.recv_dst, hops, feat);

	return 0;
}

static int ctl_recv(struct bt_mesh_net_rx *rx, uint8_t hdr,
		    struct net_buf_simple *buf, uint64_t *seq_auth)
{
	uint8_t ctl_op = TRANS_CTL_OP(&hdr);

	BT_DBG("OpCode 0x%02x len %u", ctl_op, buf->len);

	switch (ctl_op) {
	case TRANS_CTL_OP_ACK:
		return trans_ack(rx, hdr, buf, seq_auth);
	case TRANS_CTL_OP_HEARTBEAT:
		return trans_heartbeat(rx, buf);
	}

	/* Only acks and heartbeats may need processing without local_match */
	if (!rx->local_match) {
		return 0;
	}

	if (IS_ENABLED(CONFIG_BT_MESH_FRIEND) && !bt_mesh_lpn_established()) {
		switch (ctl_op) {
		case TRANS_CTL_OP_FRIEND_POLL:
			return bt_mesh_friend_poll(rx, buf);
		case TRANS_CTL_OP_FRIEND_REQ:
			return bt_mesh_friend_req(rx, buf);
		case TRANS_CTL_OP_FRIEND_CLEAR:
			return bt_mesh_friend_clear(rx, buf);
		case TRANS_CTL_OP_FRIEND_CLEAR_CFM:
			return bt_mesh_friend_clear_cfm(rx, buf);
		case TRANS_CTL_OP_FRIEND_SUB_ADD:
			return bt_mesh_friend_sub_add(rx, buf);
		case TRANS_CTL_OP_FRIEND_SUB_REM:
			return bt_mesh_friend_sub_rem(rx, buf);
		}
	}

#if defined(CONFIG_BT_MESH_LOW_POWER)
	if (ctl_op == TRANS_CTL_OP_FRIEND_OFFER) {
		return bt_mesh_lpn_friend_offer(rx, buf);
	}

	if (rx->ctx.addr == bt_mesh.lpn.frnd) {
		if (ctl_op == TRANS_CTL_OP_FRIEND_CLEAR_CFM) {
			return bt_mesh_lpn_friend_clear_cfm(rx, buf);
		}

		if (!rx->friend_cred) {
			BT_WARN("Message from friend with wrong credentials");
			return -EINVAL;
		}

		switch (ctl_op) {
		case TRANS_CTL_OP_FRIEND_UPDATE:
			return bt_mesh_lpn_friend_update(rx, buf);
		case TRANS_CTL_OP_FRIEND_SUB_CFM:
			return bt_mesh_lpn_friend_sub_cfm(rx, buf);
		}
	}
#endif /* CONFIG_BT_MESH_LOW_POWER */

	BT_WARN("Unhandled TransOpCode 0x%02x", ctl_op);

	return -ENOENT;
}

static int trans_unseg(struct net_buf_simple *buf, struct bt_mesh_net_rx *rx,
		       uint64_t *seq_auth)
{
	uint8_t hdr;

	BT_DBG("AFK %u AID 0x%02x", AKF(buf->data), AID(buf->data));

	if (buf->len < 1) {
		BT_ERR("Too small unsegmented PDU");
		return -EINVAL;
	}

	if (is_replay(rx, NULL)) {
		BT_WARN("Replay: src 0x%04x dst 0x%04x seq 0x%06x",
			rx->ctx.addr, rx->ctx.recv_dst, rx->seq);
		return -EINVAL;
	}

	hdr = net_buf_simple_pull_u8(buf);

	if (rx->ctl) {
		return ctl_recv(rx, hdr, buf, seq_auth);
	} else {
		/* SDUs must match a local element or an LPN of this Friend. */
		if (!rx->local_match && !rx->friend_match) {
			return 0;
		}

		return sdu_recv_unseg(rx, hdr, buf);
	}
}

static inline int32_t ack_timeout(struct seg_rx *rx)
{
	int32_t to;
	uint8_t ttl;

	if (rx->ttl == BT_MESH_TTL_DEFAULT) {
		ttl = bt_mesh_default_ttl_get();
	} else {
		ttl = rx->ttl;
	}

	/* The acknowledgment timer shall be set to a minimum of
	 * 150 + 50 * TTL milliseconds.
	 */
	to = 150 + (ttl * 50U);

	/* 100 ms for every not yet received segment */
	to += ((rx->seg_n + 1) - popcount(rx->block)) * 100U;

	/* Make sure we don't send more frequently than the duration for
	 * each packet (default is 300ms).
	 */
	return MAX(to, 400);
}

int bt_mesh_ctl_send(struct bt_mesh_net_tx *tx, uint8_t ctl_op, void *data,
		     size_t data_len,
		     const struct bt_mesh_send_cb *cb, void *cb_data)
{
	struct net_buf_simple buf;

	net_buf_simple_init_with_data(&buf, data, data_len);

	if (data_len > BT_MESH_SDU_UNSEG_MAX) {
		tx->ctx->send_rel = true;
	}

	tx->ctx->app_idx = BT_MESH_KEY_UNUSED;

	BT_DBG("src 0x%04x dst 0x%04x ttl 0x%02x ctl 0x%02x", tx->src,
	       tx->ctx->addr, tx->ctx->send_ttl, ctl_op);
	BT_DBG("len %zu: %s", data_len, bt_hex(data, data_len));

	if (tx->ctx->send_rel) {
		return send_seg(tx, &buf, cb, cb_data, &ctl_op);
	} else {
		return send_unseg(tx, &buf, cb, cb_data, &ctl_op);
	}
}

static int send_ack(struct bt_mesh_subnet *sub, uint16_t src, uint16_t dst,
		    uint8_t ttl, uint64_t *seq_auth, uint32_t block, uint8_t obo)
{
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = sub->net_idx,
		.app_idx = BT_MESH_KEY_UNUSED,
		.addr = dst,
		.send_ttl = ttl,
	};
	struct bt_mesh_net_tx tx = {
		.sub = sub,
		.ctx = &ctx,
		.src = obo ? bt_mesh_primary_addr() : src,
		.xmit = bt_mesh_net_transmit_get(),
	};
	uint16_t seq_zero = *seq_auth & TRANS_SEQ_ZERO_MASK;
	uint8_t buf[6];

	BT_DBG("SeqZero 0x%04x Block 0x%08x OBO %u", seq_zero, block, obo);

	if (bt_mesh_lpn_established()) {
		BT_WARN("Not sending ack when LPN is enabled");
		return 0;
	}

	/* This can happen if the segmented message was destined for a group
	 * or virtual address.
	 */
	if (!BT_MESH_ADDR_IS_UNICAST(src)) {
		BT_DBG("Not sending ack for non-unicast address");
		return 0;
	}

	sys_put_be16(((seq_zero << 2) & 0x7ffc) | (obo << 15), buf);
	sys_put_be32(block, &buf[2]);

	return bt_mesh_ctl_send(&tx, TRANS_CTL_OP_ACK, buf, sizeof(buf),
				NULL, NULL);
}

static void seg_rx_reset(struct seg_rx *rx, bool full_reset)
{
	int i;

	BT_DBG("rx %p", rx);

	k_delayed_work_cancel(&rx->ack);

	if (IS_ENABLED(CONFIG_BT_MESH_FRIEND) && rx->obo &&
	    rx->block != BLOCK_COMPLETE(rx->seg_n)) {
		BT_WARN("Clearing incomplete buffers from Friend queue");
		bt_mesh_friend_clear_incomplete(rx->sub, rx->src, rx->dst,
						&rx->seq_auth);
	}

	for (i = 0; i <= rx->seg_n; i++) {
		if (!rx->seg[i]) {
			continue;
		}

		k_mem_slab_free(&segs, &rx->seg[i]);
		rx->seg[i] = NULL;
	}

	rx->in_use = 0U;

	/* We don't always reset these values since we need to be able to
	 * send an ack if we receive a segment after we've already received
	 * the full SDU.
	 */
	if (full_reset) {
		rx->seq_auth = 0U;
		rx->sub = NULL;
		rx->src = BT_MESH_ADDR_UNASSIGNED;
		rx->dst = BT_MESH_ADDR_UNASSIGNED;
	}
}

static void seg_ack(struct k_work *work)
{
	struct seg_rx *rx = CONTAINER_OF(work, struct seg_rx, ack);

	BT_DBG("rx %p", rx);

	if (k_uptime_get_32() - rx->last > (60 * MSEC_PER_SEC)) {
		BT_WARN("Incomplete timer expired");
		seg_rx_reset(rx, false);

		if (IS_ENABLED(CONFIG_BT_TESTING)) {
			bt_test_mesh_trans_incomp_timer_exp();
		}

		return;
	}

	send_ack(rx->sub, rx->dst, rx->src, rx->ttl, &rx->seq_auth,
		 rx->block, rx->obo);

	k_delayed_work_submit(&rx->ack, K_MSEC(ack_timeout(rx)));
}

static inline bool sdu_len_is_ok(bool ctl, uint8_t seg_n)
{
	return (seg_n < CONFIG_BT_MESH_RX_SEG_MAX);
}

static struct seg_rx *seg_rx_find(struct bt_mesh_net_rx *net_rx,
				  const uint64_t *seq_auth)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(seg_rx); i++) {
		struct seg_rx *rx = &seg_rx[i];

		if (rx->src != net_rx->ctx.addr ||
		    rx->dst != net_rx->ctx.recv_dst) {
			continue;
		}

		/* Return newer RX context in addition to an exact match, so
		 * the calling function can properly discard an old SeqAuth.
		 */
		if (rx->seq_auth >= *seq_auth) {
			return rx;
		}

		if (rx->in_use) {
			BT_WARN("Duplicate SDU from src 0x%04x",
				net_rx->ctx.addr);

			/* Clear out the old context since the sender
			 * has apparently started sending a new SDU.
			 */
			seg_rx_reset(rx, true);

			/* Return non-match so caller can re-allocate */
			return NULL;
		}
	}

	return NULL;
}

static bool seg_rx_is_valid(struct seg_rx *rx, struct bt_mesh_net_rx *net_rx,
			    const uint8_t *hdr, uint8_t seg_n)
{
	if (rx->hdr != *hdr || rx->seg_n != seg_n) {
		BT_ERR("Invalid segment for ongoing session");
		return false;
	}

	if (rx->src != net_rx->ctx.addr || rx->dst != net_rx->ctx.recv_dst) {
		BT_ERR("Invalid source or destination for segment");
		return false;
	}

	if (rx->ctl != net_rx->ctl) {
		BT_ERR("Inconsistent CTL in segment");
		return false;
	}

	return true;
}

static struct seg_rx *seg_rx_alloc(struct bt_mesh_net_rx *net_rx,
				   const uint8_t *hdr, const uint64_t *seq_auth,
				   uint8_t seg_n)
{
	int i;

	/* No race condition on this check, as this function only executes in
	 * the collaborative Bluetooth rx thread:
	 */
	if (k_mem_slab_num_free_get(&segs) < 1) {
		BT_WARN("Not enough segments for incoming message");
		return NULL;
	}

	for (i = 0; i < ARRAY_SIZE(seg_rx); i++) {
		struct seg_rx *rx = &seg_rx[i];

		if (rx->in_use) {
			continue;
		}

		rx->in_use = 1U;
		rx->sub = net_rx->sub;
		rx->ctl = net_rx->ctl;
		rx->seq_auth = *seq_auth;
		rx->seg_n = seg_n;
		rx->hdr = *hdr;
		rx->ttl = net_rx->ctx.send_ttl;
		rx->src = net_rx->ctx.addr;
		rx->dst = net_rx->ctx.recv_dst;
		rx->block = 0U;

		BT_DBG("New RX context. Block Complete 0x%08x",
		       BLOCK_COMPLETE(seg_n));

		return rx;
	}

	return NULL;
}

static int trans_seg(struct net_buf_simple *buf, struct bt_mesh_net_rx *net_rx,
		     enum bt_mesh_friend_pdu_type *pdu_type, uint64_t *seq_auth,
		     uint8_t *seg_count)
{
	struct bt_mesh_rpl *rpl = NULL;
	struct seg_rx *rx;
	uint8_t *hdr = buf->data;
	uint16_t seq_zero;
	uint8_t seg_n;
	uint8_t seg_o;
	int err;

	if (buf->len < 5) {
		BT_ERR("Too short segmented message (len %u)", buf->len);
		return -EINVAL;
	}

	if (is_replay(net_rx, &rpl)) {
		BT_WARN("Replay: src 0x%04x dst 0x%04x seq 0x%06x",
			net_rx->ctx.addr, net_rx->ctx.recv_dst, net_rx->seq);
		return -EINVAL;
	}

	BT_DBG("ASZMIC %u AKF %u AID 0x%02x", ASZMIC(hdr), AKF(hdr), AID(hdr));

	net_buf_simple_pull(buf, 1);

	seq_zero = net_buf_simple_pull_be16(buf);
	seg_o = (seq_zero & 0x03) << 3;
	seq_zero = (seq_zero >> 2) & TRANS_SEQ_ZERO_MASK;
	seg_n = net_buf_simple_pull_u8(buf);
	seg_o |= seg_n >> 5;
	seg_n &= 0x1f;

	BT_DBG("SeqZero 0x%04x SegO %u SegN %u", seq_zero, seg_o, seg_n);

	if (seg_o > seg_n) {
		BT_ERR("SegO greater than SegN (%u > %u)", seg_o, seg_n);
		return -EINVAL;
	}

	/* According to Mesh 1.0 specification:
	 * "The SeqAuth is composed of the IV Index and the sequence number
	 *  (SEQ) of the first segment"
	 *
	 * Therefore we need to calculate very first SEQ in order to find
	 * seqAuth. We can calculate as below:
	 *
	 * SEQ(0) = SEQ(n) - (delta between seqZero and SEQ(n) by looking into
	 * 14 least significant bits of SEQ(n))
	 *
	 * Mentioned delta shall be >= 0, if it is not then seq_auth will
	 * be broken and it will be verified by the code below.
	 */
	*seq_auth = SEQ_AUTH(BT_MESH_NET_IVI_RX(net_rx),
			     (net_rx->seq -
			      ((((net_rx->seq & BIT_MASK(14)) - seq_zero)) &
			       BIT_MASK(13))));

	*seg_count = seg_n + 1;

	/* Look for old RX sessions */
	rx = seg_rx_find(net_rx, seq_auth);
	if (rx) {
		/* Discard old SeqAuth packet */
		if (rx->seq_auth > *seq_auth) {
			BT_WARN("Ignoring old SeqAuth");
			return -EINVAL;
		}

		if (!seg_rx_is_valid(rx, net_rx, hdr, seg_n)) {
			return -EINVAL;
		}

		if (rx->in_use) {
			BT_DBG("Existing RX context. Block 0x%08x", rx->block);
			goto found_rx;
		}

		if (rx->block == BLOCK_COMPLETE(rx->seg_n)) {
			BT_DBG("Got segment for already complete SDU");

			send_ack(net_rx->sub, net_rx->ctx.recv_dst,
				 net_rx->ctx.addr, net_rx->ctx.send_ttl,
				 seq_auth, rx->block, rx->obo);

			if (rpl) {
				update_rpl(rpl, net_rx);
			}

			return -EALREADY;
		}

		/* We ignore instead of sending block ack 0 since the
		 * ack timer is always smaller than the incomplete
		 * timer, i.e. the sender is misbehaving.
		 */
		BT_WARN("Got segment for canceled SDU");
		return -EINVAL;
	}

	/* Bail out early if we're not ready to receive such a large SDU */
	if (!sdu_len_is_ok(net_rx->ctl, seg_n)) {
		BT_ERR("Too big incoming SDU length");
		send_ack(net_rx->sub, net_rx->ctx.recv_dst, net_rx->ctx.addr,
			 net_rx->ctx.send_ttl, seq_auth, 0,
			 net_rx->friend_match);
		return -EMSGSIZE;
	}

	/* Verify early that there will be space in the Friend Queue(s) in
	 * case this message is destined to an LPN of ours.
	 */
	if (IS_ENABLED(CONFIG_BT_MESH_FRIEND) &&
	    net_rx->friend_match && !net_rx->local_match &&
	    !bt_mesh_friend_queue_has_space(net_rx->sub->net_idx,
					    net_rx->ctx.addr,
					    net_rx->ctx.recv_dst, seq_auth,
					    *seg_count)) {
		BT_ERR("No space in Friend Queue for %u segments", *seg_count);
		send_ack(net_rx->sub, net_rx->ctx.recv_dst, net_rx->ctx.addr,
			 net_rx->ctx.send_ttl, seq_auth, 0,
			 net_rx->friend_match);
		return -ENOBUFS;
	}

	/* Look for free slot for a new RX session */
	rx = seg_rx_alloc(net_rx, hdr, seq_auth, seg_n);
	if (!rx) {
		/* Warn but don't cancel since the existing slots willl
		 * eventually be freed up and we'll be able to process
		 * this one.
		 */
		BT_WARN("No free slots for new incoming segmented messages");
		return -ENOMEM;
	}

	rx->obo = net_rx->friend_match;

found_rx:
	if (BIT(seg_o) & rx->block) {
		BT_DBG("Received already received fragment");
		return -EALREADY;
	}

	/* All segments, except the last one, must either have 8 bytes of
	 * payload (for 64bit Net MIC) or 12 bytes of payload (for 32bit
	 * Net MIC).
	 */
	if (seg_o == seg_n) {
		/* Set the expected final buffer length */
		rx->len = seg_n * seg_len(rx->ctl) + buf->len;
		BT_DBG("Target len %u * %u + %u = %u", seg_n, seg_len(rx->ctl),
		       buf->len, rx->len);

		if (rx->len > BT_MESH_RX_SDU_MAX) {
			BT_ERR("Too large SDU len");
			send_ack(net_rx->sub, net_rx->ctx.recv_dst,
				 net_rx->ctx.addr, net_rx->ctx.send_ttl,
				 seq_auth, 0, rx->obo);
			seg_rx_reset(rx, true);
			return -EMSGSIZE;
		}
	} else {
		if (buf->len != seg_len(rx->ctl)) {
			BT_ERR("Incorrect segment size for message type");
			return -EINVAL;
		}
	}

	/* Reset the Incomplete Timer */
	rx->last = k_uptime_get_32();

	if (!k_delayed_work_remaining_get(&rx->ack) &&
	    !bt_mesh_lpn_established()) {
		k_delayed_work_submit(&rx->ack, K_MSEC(ack_timeout(rx)));
	}

	/* Allocated segment here */
	err = k_mem_slab_alloc(&segs, &rx->seg[seg_o], K_NO_WAIT);
	if (err) {
		BT_WARN("Unable allocate buffer for Seg %u", seg_o);
		return -ENOBUFS;
	}

	memcpy(rx->seg[seg_o], buf->data, buf->len);

	BT_DBG("Received %u/%u", seg_o, seg_n);

	/* Mark segment as received */
	rx->block |= BIT(seg_o);

	if (rx->block != BLOCK_COMPLETE(seg_n)) {
		*pdu_type = BT_MESH_FRIEND_PDU_PARTIAL;
		return 0;
	}

	BT_DBG("Complete SDU");

	if (rpl) {
		update_rpl(rpl, net_rx);
	}

	*pdu_type = BT_MESH_FRIEND_PDU_COMPLETE;

	k_delayed_work_cancel(&rx->ack);
	send_ack(net_rx->sub, net_rx->ctx.recv_dst, net_rx->ctx.addr,
		 net_rx->ctx.send_ttl, seq_auth, rx->block, rx->obo);

	if (net_rx->ctl) {
		NET_BUF_SIMPLE_DEFINE(sdu, BT_MESH_RX_CTL_MAX);
		seg_rx_assemble(rx, &sdu, 0U);
		err = ctl_recv(net_rx, *hdr, &sdu, seq_auth);
	} else {
		err = sdu_recv_seg(rx, *hdr, ASZMIC(hdr), net_rx);
	}

	seg_rx_reset(rx, false);

	return err;
}

int bt_mesh_trans_recv(struct net_buf_simple *buf, struct bt_mesh_net_rx *rx)
{
	uint64_t seq_auth = TRANS_SEQ_AUTH_NVAL;
	enum bt_mesh_friend_pdu_type pdu_type = BT_MESH_FRIEND_PDU_SINGLE;
	struct net_buf_simple_state state;
	uint8_t seg_count = 0;
	int err;

	if (IS_ENABLED(CONFIG_BT_MESH_FRIEND)) {
		rx->friend_match = bt_mesh_friend_match(rx->sub->net_idx,
							rx->ctx.recv_dst);
	} else {
		rx->friend_match = false;
	}

	BT_DBG("src 0x%04x dst 0x%04x seq 0x%08x friend_match %u",
	       rx->ctx.addr, rx->ctx.recv_dst, rx->seq, rx->friend_match);

	/* Remove network headers */
	net_buf_simple_pull(buf, BT_MESH_NET_HDR_LEN);

	BT_DBG("Payload %s", bt_hex(buf->data, buf->len));

	if (IS_ENABLED(CONFIG_BT_TESTING)) {
		bt_test_mesh_net_recv(rx->ctx.recv_ttl, rx->ctl, rx->ctx.addr,
				      rx->ctx.recv_dst, buf->data, buf->len);
	}

	/* If LPN mode is enabled messages are only accepted when we've
	 * requested the Friend to send them. The messages must also
	 * be encrypted using the Friend Credentials.
	 */
	if (IS_ENABLED(CONFIG_BT_MESH_LOW_POWER) &&
	    bt_mesh_lpn_established() && rx->net_if == BT_MESH_NET_IF_ADV &&
	    (!bt_mesh_lpn_waiting_update() || !rx->friend_cred)) {
		BT_WARN("Ignoring unexpected message in Low Power mode");
		return -EAGAIN;
	}

	/* Save the app-level state so the buffer can later be placed in
	 * the Friend Queue.
	 */
	net_buf_simple_save(buf, &state);

	if (SEG(buf->data)) {
		/* Segmented messages must match a local element or an
		 * LPN of this Friend.
		 */
		if (!rx->local_match && !rx->friend_match) {
			return 0;
		}

		err = trans_seg(buf, rx, &pdu_type, &seq_auth, &seg_count);
	} else {
		seg_count = 1;
		err = trans_unseg(buf, rx, &seq_auth);
	}

	/* Notify LPN state machine so a Friend Poll will be sent. If the
	 * message was a Friend Update it's possible that a Poll was already
	 * queued for sending, however that's fine since then the
	 * bt_mesh_lpn_waiting_update() function will return false:
	 * we still need to go through the actual sending to the bearer and
	 * wait for ReceiveDelay before transitioning to WAIT_UPDATE state.
	 * Another situation where we want to notify the LPN state machine
	 * is if it's configured to use an automatic Friendship establishment
	 * timer, in which case we want to reset the timer at this point.
	 *
	 */
	if (IS_ENABLED(CONFIG_BT_MESH_LOW_POWER) &&
	    (bt_mesh_lpn_timer() ||
	     (bt_mesh_lpn_established() && bt_mesh_lpn_waiting_update()))) {
		bt_mesh_lpn_msg_received(rx);
	}

	net_buf_simple_restore(buf, &state);

	if (IS_ENABLED(CONFIG_BT_MESH_FRIEND) && rx->friend_match && !err) {
		if (seq_auth == TRANS_SEQ_AUTH_NVAL) {
			bt_mesh_friend_enqueue_rx(rx, pdu_type, NULL,
						  seg_count, buf);
		} else {
			bt_mesh_friend_enqueue_rx(rx, pdu_type, &seq_auth,
						  seg_count, buf);
		}
	}

	return err;
}

void bt_mesh_rx_reset(void)
{
	int i;

	BT_DBG("");

	for (i = 0; i < ARRAY_SIZE(seg_rx); i++) {
		seg_rx_reset(&seg_rx[i], true);
	}

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		bt_mesh_clear_rpl();
	} else {
		(void)memset(bt_mesh.rpl, 0, sizeof(bt_mesh.rpl));
	}
}

void bt_mesh_tx_reset(void)
{
	int i;

	BT_DBG("");

	for (i = 0; i < ARRAY_SIZE(seg_tx); i++) {
		seg_tx_reset(&seg_tx[i]);
	}
}

void bt_mesh_trans_init(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(seg_tx); i++) {
		k_delayed_work_init(&seg_tx[i].retransmit, seg_retransmit);
	}

	for (i = 0; i < ARRAY_SIZE(seg_rx); i++) {
		k_delayed_work_init(&seg_rx[i].ack, seg_ack);
	}
}

void bt_mesh_rpl_clear(void)
{
	BT_DBG("");
	(void)memset(bt_mesh.rpl, 0, sizeof(bt_mesh.rpl));
}

int bt_mesh_heartbeat_send(const struct bt_mesh_send_cb *cb, void *cb_data)
{
	struct bt_mesh_cfg_srv *cfg = bt_mesh_cfg_get();
	uint16_t feat = 0U;
	struct __packed {
		uint8_t  init_ttl;
		uint16_t feat;
	} hb;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = cfg->hb_pub.net_idx,
		.app_idx = BT_MESH_KEY_UNUSED,
		.addr = cfg->hb_pub.dst,
		.send_ttl = cfg->hb_pub.ttl,
	};
	struct bt_mesh_net_tx tx = {
		.sub = bt_mesh_subnet_get(cfg->hb_pub.net_idx),
		.ctx = &ctx,
		.src = bt_mesh_model_elem(cfg->model)->addr,
		.xmit = bt_mesh_net_transmit_get(),
	};

	/* Do nothing if heartbeat publication is not enabled */
	if (cfg->hb_pub.dst == BT_MESH_ADDR_UNASSIGNED) {
		return 0;
	}

	hb.init_ttl = cfg->hb_pub.ttl;

	if (bt_mesh_relay_get() == BT_MESH_RELAY_ENABLED) {
		feat |= BT_MESH_FEAT_RELAY;
	}

	if (bt_mesh_gatt_proxy_get() == BT_MESH_GATT_PROXY_ENABLED) {
		feat |= BT_MESH_FEAT_PROXY;
	}

	if (bt_mesh_friend_get() == BT_MESH_FRIEND_ENABLED) {
		feat |= BT_MESH_FEAT_FRIEND;
	}

	if (bt_mesh_lpn_established()) {
		feat |= BT_MESH_FEAT_LOW_POWER;
	}

	hb.feat = sys_cpu_to_be16(feat);

	BT_DBG("InitTTL %u feat 0x%04x", cfg->hb_pub.ttl, feat);

	return bt_mesh_ctl_send(&tx, TRANS_CTL_OP_HEARTBEAT, &hb, sizeof(hb),
				cb, cb_data);
}

int bt_mesh_app_key_get(const struct bt_mesh_subnet *subnet, uint16_t app_idx,
			uint16_t addr, const uint8_t **key, uint8_t *aid)
{
	struct bt_mesh_app_key *app_key;

	if (app_idx == BT_MESH_KEY_DEV_LOCAL ||
	    (app_idx == BT_MESH_KEY_DEV_REMOTE &&
	     bt_mesh_elem_find(addr) != NULL)) {
		*aid = 0;
		*key = bt_mesh.dev_key;
		return 0;
	} else if (app_idx == BT_MESH_KEY_DEV_REMOTE) {
		if (!IS_ENABLED(CONFIG_BT_MESH_CDB)) {
			return -EINVAL;
		}

		struct bt_mesh_cdb_node *node = bt_mesh_cdb_node_get(addr);
		if (!node) {
			return -EINVAL;
		}

		*key = node->dev_key;
		*aid = 0;
		return 0;
	}

	if (!subnet) {
		return -EINVAL;
	}

	app_key = bt_mesh_app_key_find(app_idx);
	if (!app_key) {
		return -ENOENT;
	}

	if (subnet->kr_phase == BT_MESH_KR_PHASE_2 && app_key->updated) {
		*key = app_key->keys[1].val;
		*aid = app_key->keys[1].id;
	} else {
		*key = app_key->keys[0].val;
		*aid = app_key->keys[0].id;
	}

	return 0;
}
