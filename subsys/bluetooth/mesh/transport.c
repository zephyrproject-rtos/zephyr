/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/net/buf.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/mesh.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_MESH_DEBUG_TRANS)
#define LOG_MODULE_NAME bt_mesh_transport
#include "common/log.h"

#include "host/testing.h"

#include "crypto.h"
#include "adv.h"
#include "mesh.h"
#include "net.h"
#include "app_keys.h"
#include "lpn.h"
#include "rpl.h"
#include "friend.h"
#include "access.h"
#include "foundation.h"
#include "settings.h"
#include "heartbeat.h"
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
#define SEG_RETRANSMIT_ATTEMPTS     CONFIG_BT_MESH_TX_SEG_RETRANS_COUNT

/* "This timer shall be set to a minimum of 200 + 50 * TTL milliseconds.".
 * We use 400 since 300 is a common send duration for standard HCI, and we
 * need to have a timeout that's bigger than that.
 */
#define SEG_RETRANSMIT_TIMEOUT_UNICAST(tx) \
	(CONFIG_BT_MESH_TX_SEG_RETRANS_TIMEOUT_UNICAST + 50 * (tx)->ttl)

/* When sending to a group, the messages are not acknowledged, and there's no
 * reason to delay the repetitions significantly. Delaying by more than 0 ms
 * to avoid flooding the network.
 */
#define SEG_RETRANSMIT_TIMEOUT_GROUP CONFIG_BT_MESH_TX_SEG_RETRANS_TIMEOUT_GROUP

#define SEG_RETRANSMIT_TIMEOUT(tx)                                             \
	(BT_MESH_ADDR_IS_UNICAST(tx->dst) ?                                    \
		 SEG_RETRANSMIT_TIMEOUT_UNICAST(tx) :                          \
		 SEG_RETRANSMIT_TIMEOUT_GROUP)
/* How long to wait for available buffers before giving up */
#define BUF_TIMEOUT                 K_NO_WAIT

struct virtual_addr {
	uint16_t ref:15,
		 changed:1;
	uint16_t addr;
	uint8_t  uuid[16];
};

/* Virtual Address information for persistent storage. */
struct va_val {
	uint16_t ref;
	uint16_t addr;
	uint8_t uuid[16];
} __packed;

static struct seg_tx {
	struct bt_mesh_subnet *sub;
	void                  *seg[BT_MESH_TX_SEG_MAX];
	uint64_t              seq_auth;
	uint16_t              src;
	uint16_t              dst;
	uint16_t              ack_src;
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
	struct k_work_delayable retransmit;    /* Retransmit timer */
} seg_tx[CONFIG_BT_MESH_TX_SEG_MSG_COUNT];

static struct seg_rx {
	struct bt_mesh_subnet   *sub;
	void                    *seg[BT_MESH_RX_SEG_MAX];
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
	struct k_work_delayable  ack;
} seg_rx[CONFIG_BT_MESH_RX_SEG_MSG_COUNT];

K_MEM_SLAB_DEFINE(segs, BT_MESH_APP_SEG_SDU_MAX, CONFIG_BT_MESH_SEG_BUFS, 4);

static struct virtual_addr virtual_addrs[CONFIG_BT_MESH_LABEL_COUNT];

static int send_unseg(struct bt_mesh_net_tx *tx, struct net_buf_simple *sdu,
		      const struct bt_mesh_send_cb *cb, void *cb_data,
		      const uint8_t *ctl_op)
{
	struct net_buf *buf;

	buf = bt_mesh_adv_create(BT_MESH_ADV_DATA, BT_MESH_LOCAL_ADV,
				 tx->xmit, BUF_TIMEOUT);
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
		k_work_reschedule(&blocked->retransmit, K_NO_WAIT);
	}
}

static void seg_tx_reset(struct seg_tx *tx)
{
	int i;

	/* If this call fails, the handler will exit early, as nack_count is 0. */
	(void)k_work_cancel_delayable(&tx->retransmit);

	tx->cb = NULL;
	tx->cb_data = NULL;
	tx->seq_auth = 0U;
	tx->sub = NULL;
	tx->src = BT_MESH_ADDR_UNASSIGNED;
	tx->dst = BT_MESH_ADDR_UNASSIGNED;
	tx->ack_src = BT_MESH_ADDR_UNASSIGNED;
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
		bt_mesh_net_iv_update(bt_mesh.iv_index, false);
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
	k_work_reschedule(&tx->retransmit,
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

		seg = bt_mesh_adv_create(BT_MESH_ADV_DATA, BT_MESH_LOCAL_ADV,
					 tx->xmit, BUF_TIMEOUT);
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
	if (IS_ENABLED(CONFIG_BT_MESH_LOW_POWER) &&
	    bt_mesh_lpn_established()) {
		bt_mesh_lpn_poll();
	}

	if (!tx->seg_pending) {
		k_work_reschedule(&tx->retransmit,
				  K_MSEC(SEG_RETRANSMIT_TIMEOUT(tx)));
	}

	tx->sending = 0U;
}

static void seg_retransmit(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct seg_tx *tx = CONTAINER_OF(dwork, struct seg_tx, retransmit);

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
	tx->ttl = net_tx->ctx->send_ttl;

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

	return 0;
}

static int trans_encrypt(const struct bt_mesh_net_tx *tx, const uint8_t *key,
			 struct net_buf_simple *msg)
{
	struct bt_mesh_app_crypto_ctx crypto = {
		.dev_key = BT_MESH_IS_DEV_KEY(tx->ctx->app_idx),
		.aszmic = tx->aszmic,
		.src = tx->src,
		.dst = tx->ctx->addr,
		.seq_num = bt_mesh.seq,
		.iv_index = BT_MESH_NET_IVI_TX,
	};

	if (BT_MESH_ADDR_IS_VIRTUAL(tx->ctx->addr)) {
		crypto.ad = bt_mesh_va_label_get(tx->ctx->addr);
	}

	return bt_mesh_app_encrypt(key, &crypto, msg);
}

int bt_mesh_trans_send(struct bt_mesh_net_tx *tx, struct net_buf_simple *msg,
		       const struct bt_mesh_send_cb *cb, void *cb_data)
{
	const uint8_t *key;
	uint8_t aid;
	int err;

	if (msg->len < 1) {
		BT_ERR("Zero-length SDU not allowed");
		return -EINVAL;
	}

	if (msg->len > BT_MESH_TX_SDU_MAX - BT_MESH_MIC_SHORT) {
		BT_ERR("Message too big: %u", msg->len);
		return -EMSGSIZE;
	}

	if (net_buf_simple_tailroom(msg) < BT_MESH_MIC_SHORT) {
		BT_ERR("Insufficient tailroom for Transport MIC");
		return -EINVAL;
	}

	if (tx->ctx->send_ttl == BT_MESH_TTL_DEFAULT) {
		tx->ctx->send_ttl = bt_mesh_default_ttl_get();
	} else if (tx->ctx->send_ttl > BT_MESH_TTL_MAX) {
		BT_ERR("TTL too large (max 127)");
		return -EINVAL;
	}

	if (msg->len > BT_MESH_SDU_UNSEG_MAX) {
		tx->ctx->send_rel = true;
	}

	if (tx->ctx->addr == BT_MESH_ADDR_UNASSIGNED ||
	    (!BT_MESH_ADDR_IS_UNICAST(tx->ctx->addr) &&
	     BT_MESH_IS_DEV_KEY(tx->ctx->app_idx))) {
		BT_ERR("Invalid destination address");
		return -EINVAL;
	}

	err = bt_mesh_keys_resolve(tx->ctx, &tx->sub, &key, &aid);
	if (err) {
		return err;
	}

	BT_DBG("net_idx 0x%04x app_idx 0x%04x dst 0x%04x", tx->sub->net_idx,
	       tx->ctx->app_idx, tx->ctx->addr);
	BT_DBG("len %u: %s", msg->len, bt_hex(msg->data, msg->len));

	tx->xmit = bt_mesh_net_transmit_get();
	tx->aid = aid;

	if (!tx->ctx->send_rel || net_buf_simple_tailroom(msg) < 8) {
		tx->aszmic = 0U;
	} else {
		tx->aszmic = 1U;
	}

	err = trans_encrypt(tx, key, msg);
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

struct decrypt_ctx {
	struct bt_mesh_app_crypto_ctx crypto;
	struct net_buf_simple *buf;
	struct net_buf_simple *sdu;
	struct seg_rx *seg;
};

static int sdu_try_decrypt(struct bt_mesh_net_rx *rx, const uint8_t key[16],
			   void *cb_data)
{
	const struct decrypt_ctx *ctx = cb_data;

	if (ctx->seg) {
		seg_rx_assemble(ctx->seg, ctx->buf, ctx->crypto.aszmic);
	}

	net_buf_simple_reset(ctx->sdu);

	return bt_mesh_app_decrypt(key, &ctx->crypto, ctx->buf, ctx->sdu);
}

static int sdu_recv(struct bt_mesh_net_rx *rx, uint8_t hdr, uint8_t aszmic,
		    struct net_buf_simple *buf, struct net_buf_simple *sdu,
		    struct seg_rx *seg)
{
	struct decrypt_ctx ctx = {
		.crypto = {
			.dev_key = !AKF(&hdr),
			.aszmic = aszmic,
			.src = rx->ctx.addr,
			.dst = rx->ctx.recv_dst,
			.seq_num = seg ? (seg->seq_auth & 0xffffff) : rx->seq,
			.iv_index = BT_MESH_NET_IVI_RX(rx),
		},
		.buf = buf,
		.sdu = sdu,
		.seg = seg,
	};

	BT_DBG("AKF %u AID 0x%02x", !ctx.crypto.dev_key, AID(&hdr));

	if (IS_ENABLED(CONFIG_BT_MESH_FRIEND) && !rx->local_match) {
		BT_DBG("Ignoring PDU for LPN 0x%04x of this Friend",
		       rx->ctx.recv_dst);
		return 0;
	}

	if (BT_MESH_ADDR_IS_VIRTUAL(rx->ctx.recv_dst)) {
		ctx.crypto.ad = bt_mesh_va_label_get(rx->ctx.recv_dst);
	}

	rx->ctx.app_idx = bt_mesh_app_key_find(ctx.crypto.dev_key, AID(&hdr),
					       rx, sdu_try_decrypt, &ctx);
	if (rx->ctx.app_idx == BT_MESH_KEY_UNUSED) {
		BT_DBG("No matching AppKey");
		return 0;
	}

	BT_DBG("Decrypted (AppIdx: 0x%03x)", rx->ctx.app_idx);

	bt_mesh_model_recv(rx, sdu);

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
		if (obo && (tx->nack_count == tx->seg_n + 1 || tx->ack_src == addr)) {
			tx->ack_src = addr;
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

	while ((bit = find_lsb_set(ack))) {
		if (tx->seg[bit - 1]) {
			BT_DBG("seg %u/%u acked", bit - 1, tx->seg_n);
			seg_tx_done(tx, bit - 1);
		}

		ack &= ~BIT(bit - 1);
	}

	if (tx->nack_count) {
		/* According to the Bluetooth Mesh Profile specification,
		 * section 3.5.3.3, we should reset the retransmit timer and
		 * retransmit immediately when receiving a valid ack message:
		 */
		k_work_reschedule(&tx->retransmit, K_NO_WAIT);
	} else {
		BT_DBG("SDU TX complete");
		seg_tx_complete(tx, 0);
	}

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
		return bt_mesh_hb_recv(rx, buf);
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
	NET_BUF_SIMPLE_DEFINE_STATIC(sdu, BT_MESH_SDU_UNSEG_MAX);
	uint8_t hdr;

	BT_DBG("AFK %u AID 0x%02x", AKF(buf->data), AID(buf->data));

	if (buf->len < 1) {
		BT_ERR("Too small unsegmented PDU");
		return -EINVAL;
	}

	if (bt_mesh_rpl_check(rx, NULL)) {
		BT_WARN("Replay: src 0x%04x dst 0x%04x seq 0x%06x",
			rx->ctx.addr, rx->ctx.recv_dst, rx->seq);
		return -EINVAL;
	}

	hdr = net_buf_simple_pull_u8(buf);

	if (rx->ctl) {
		return ctl_recv(rx, hdr, buf, seq_auth);
	}

	if (buf->len < 1 + APP_MIC_LEN(0)) {
		BT_ERR("Too short SDU + MIC");
		return -EINVAL;
	}

	/* Adjust the length to not contain the MIC at the end */
	buf->len -= APP_MIC_LEN(0);

	return sdu_recv(rx, hdr, 0, buf, &sdu, NULL);
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

	if (tx->ctx->send_ttl == BT_MESH_TTL_DEFAULT) {
		tx->ctx->send_ttl = bt_mesh_default_ttl_get();
	} else if (tx->ctx->send_ttl > BT_MESH_TTL_MAX) {
		BT_ERR("TTL too large (max 127)");
		return -EINVAL;
	}

	net_buf_simple_init_with_data(&buf, data, data_len);

	if (data_len > BT_MESH_SDU_UNSEG_MAX) {
		tx->ctx->send_rel = true;
	}

	tx->ctx->app_idx = BT_MESH_KEY_UNUSED;

	if (tx->ctx->addr == BT_MESH_ADDR_UNASSIGNED ||
	    BT_MESH_ADDR_IS_VIRTUAL(tx->ctx->addr)) {
		BT_ERR("Invalid destination address");
		return -EINVAL;
	}

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

	/* If this fails, the handler will exit early on the next execution, as
	 * it checks rx->in_use.
	 */
	(void)k_work_cancel_delayable(&rx->ack);

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
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct seg_rx *rx = CONTAINER_OF(dwork, struct seg_rx, ack);
	int32_t timeout;

	if (!rx->in_use || rx->block == BLOCK_COMPLETE(rx->seg_n)) {
		/* Cancellation of this timer may have failed. If it fails as
		 * part of seg_reset, in_use will be false.
		 * If it fails as part of the processing of a fully received
		 * SDU, the ack is already being sent from the receive handler,
		 * and the timer based ack sending can be ignored.
		 */
		return;
	}

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

	timeout = ack_timeout(rx);
	k_work_schedule(&rx->ack, K_MSEC(timeout));
}

static inline bool sdu_len_is_ok(bool ctl, uint8_t seg_n)
{
	return (seg_n < BT_MESH_RX_SEG_MAX);
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
	uint32_t auth_seqnum;
	uint8_t seg_n;
	uint8_t seg_o;
	int err;

	if (buf->len < 5) {
		BT_ERR("Too short segmented message (len %u)", buf->len);
		return -EINVAL;
	}

	if (bt_mesh_rpl_check(net_rx, &rpl)) {
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
	auth_seqnum = *seq_auth & BIT_MASK(24);
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
				bt_mesh_rpl_update(rpl, net_rx);
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

	/* Keep track of the received SeqAuth values received from this address
	 * and discard segmented messages that are not newer, as described in
	 * the Bluetooth Mesh specification section 3.5.3.4.
	 *
	 * The logic on the first segmented receive is a bit special, since the
	 * initial value of rpl->seg is 0, which would normally fail the
	 * comparison check with auth_seqnum:
	 * - If this is the first time we receive from this source, rpl->src
	 *   will be 0, and we can skip this check.
	 * - If this is the first time we receive from this source on the new IV
	 *   index, rpl->old_iv will be set, and the check is also skipped.
	 * - If this is the first segmented message on the new IV index, but we
	 *   have received an unsegmented message already, the unsegmented
	 *   message will have reset rpl->seg to 0, and this message's SeqAuth
	 *   cannot be zero.
	 */
	if (rpl && rpl->src && auth_seqnum <= rpl->seg &&
	    (!rpl->old_iv || net_rx->old_iv)) {
		BT_WARN("Ignoring old SeqAuth 0x%06x", auth_seqnum);
		return -EALREADY;
	}

	/* Look for free slot for a new RX session */
	rx = seg_rx_alloc(net_rx, hdr, seq_auth, seg_n);
	if (!rx) {
		/* Warn but don't cancel since the existing slots will
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

	if (!bt_mesh_lpn_established()) {
		int32_t timeout = ack_timeout(rx);
		/* Should only start ack timer if it isn't running already: */
		k_work_schedule(&rx->ack, K_MSEC(timeout));
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
		bt_mesh_rpl_update(rpl, net_rx);
		/* Update the seg, unless it has already been surpassed:
		 * This needs to happen after rpl_update to ensure that the IV
		 * update reset logic inside rpl_update doesn't overwrite the
		 * change.
		 */
		rpl->seg = MAX(rpl->seg, auth_seqnum);
	}

	*pdu_type = BT_MESH_FRIEND_PDU_COMPLETE;

	/* If this fails, the work handler will either exit early because the
	 * block is fully received, or rx->in_use is false.
	 */
	(void)k_work_cancel_delayable(&rx->ack);
	send_ack(net_rx->sub, net_rx->ctx.recv_dst, net_rx->ctx.addr,
		 net_rx->ctx.send_ttl, seq_auth, rx->block, rx->obo);

	if (net_rx->ctl) {
		NET_BUF_SIMPLE_DEFINE(sdu, BT_MESH_RX_CTL_MAX);
		seg_rx_assemble(rx, &sdu, 0U);
		err = ctl_recv(net_rx, *hdr, &sdu, seq_auth);
	} else if (rx->len < 1 + APP_MIC_LEN(ASZMIC(hdr))) {
		BT_ERR("Too short SDU + MIC");
		err = -EINVAL;
	} else {
		NET_BUF_SIMPLE_DEFINE_STATIC(seg_buf, BT_MESH_RX_SDU_MAX);
		struct net_buf_simple sdu;

		/* Decrypting in place to avoid creating two assembly buffers.
		 * We'll reassemble the buffer from the segments before each
		 * decryption attempt.
		 */
		net_buf_simple_init(&seg_buf, 0);
		net_buf_simple_init_with_data(
			&sdu, seg_buf.data, rx->len - APP_MIC_LEN(ASZMIC(hdr)));

		err = sdu_recv(net_rx, *hdr, ASZMIC(hdr), &seg_buf, &sdu, rx);
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

	/* Notify LPN state machine so a Friend Poll will be sent. */
	if (IS_ENABLED(CONFIG_BT_MESH_LOW_POWER)) {
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
}

static void store_va_label(void)
{
	bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_VA_PENDING);
}

void bt_mesh_trans_reset(void)
{
	int i;

	bt_mesh_rx_reset();

	BT_DBG("");

	for (i = 0; i < ARRAY_SIZE(seg_tx); i++) {
		seg_tx_reset(&seg_tx[i]);
	}

	for (i = 0; i < ARRAY_SIZE(virtual_addrs); i++) {
		if (virtual_addrs[i].ref) {
			virtual_addrs[i].ref = 0U;
			virtual_addrs[i].changed = 1U;
		}
	}

	bt_mesh_rpl_clear();

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		store_va_label();
	}
}

void bt_mesh_trans_init(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(seg_tx); i++) {
		k_work_init_delayable(&seg_tx[i].retransmit, seg_retransmit);
	}

	for (i = 0; i < ARRAY_SIZE(seg_rx); i++) {
		k_work_init_delayable(&seg_rx[i].ack, seg_ack);
	}
}

static inline void va_store(struct virtual_addr *store)
{
	store->changed = 1U;
	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		store_va_label();
	}
}

uint8_t bt_mesh_va_add(const uint8_t uuid[16], uint16_t *addr)
{
	struct virtual_addr *va = NULL;
	int err;

	for (int i = 0; i < ARRAY_SIZE(virtual_addrs); i++) {
		if (!virtual_addrs[i].ref) {
			if (!va) {
				va = &virtual_addrs[i];
			}

			continue;
		}

		if (!memcmp(uuid, virtual_addrs[i].uuid,
			    ARRAY_SIZE(virtual_addrs[i].uuid))) {
			*addr = virtual_addrs[i].addr;
			virtual_addrs[i].ref++;
			va_store(&virtual_addrs[i]);
			return STATUS_SUCCESS;
		}
	}

	if (!va) {
		return STATUS_INSUFF_RESOURCES;
	}

	memcpy(va->uuid, uuid, ARRAY_SIZE(va->uuid));
	err = bt_mesh_virtual_addr(uuid, &va->addr);
	if (err) {
		va->addr = BT_MESH_ADDR_UNASSIGNED;
		return STATUS_UNSPECIFIED;
	}

	va->ref = 1;
	va_store(va);

	*addr = va->addr;

	return STATUS_SUCCESS;
}

uint8_t bt_mesh_va_del(const uint8_t uuid[16], uint16_t *addr)
{
	struct virtual_addr *va = NULL;

	for (int i = 0; i < ARRAY_SIZE(virtual_addrs); i++) {
		if (virtual_addrs[i].ref &&
		    !memcmp(uuid, virtual_addrs[i].uuid,
			    ARRAY_SIZE(virtual_addrs[i].uuid))) {
			va = &virtual_addrs[i];
			break;
		}
	}

	if (!va) {
		return STATUS_CANNOT_REMOVE;
	}

	va->ref--;
	if (addr) {
		*addr = va->addr;
	}

	va_store(va);
	return STATUS_SUCCESS;
}

uint8_t *bt_mesh_va_label_get(uint16_t addr)
{
	int i;

	BT_DBG("addr 0x%04x", addr);

	for (i = 0; i < ARRAY_SIZE(virtual_addrs); i++) {
		if (virtual_addrs[i].ref && virtual_addrs[i].addr == addr) {
			BT_DBG("Found Label UUID for 0x%04x: %s", addr,
			       bt_hex(virtual_addrs[i].uuid, 16));
			return virtual_addrs[i].uuid;
		}
	}

	BT_WARN("No matching Label UUID for 0x%04x", addr);

	return NULL;
}

#if CONFIG_BT_MESH_LABEL_COUNT > 0
static struct virtual_addr *bt_mesh_va_get(uint16_t index)
{
	if (index >= ARRAY_SIZE(virtual_addrs)) {
		return NULL;
	}

	return &virtual_addrs[index];
}

static int va_set(const char *name, size_t len_rd,
		  settings_read_cb read_cb, void *cb_arg)
{
	struct va_val va;
	struct virtual_addr *lab;
	uint16_t index;
	int err;

	if (!name) {
		BT_ERR("Insufficient number of arguments");
		return -ENOENT;
	}

	index = strtol(name, NULL, 16);

	if (len_rd == 0) {
		BT_WARN("Mesh Virtual Address length = 0");
		return 0;
	}

	err = bt_mesh_settings_set(read_cb, cb_arg, &va, sizeof(va));
	if (err) {
		BT_ERR("Failed to set \'virtual address\'");
		return err;
	}

	if (va.ref == 0) {
		BT_WARN("Ignore Mesh Virtual Address ref = 0");
		return 0;
	}

	lab = bt_mesh_va_get(index);
	if (lab == NULL) {
		BT_WARN("Out of labels buffers");
		return -ENOBUFS;
	}

	memcpy(lab->uuid, va.uuid, 16);
	lab->addr = va.addr;
	lab->ref = va.ref;

	BT_DBG("Restored Virtual Address, addr 0x%04x ref 0x%04x",
	       lab->addr, lab->ref);

	return 0;
}

BT_MESH_SETTINGS_DEFINE(va, "Va", va_set);

#define IS_VA_DEL(_label)	((_label)->ref == 0)
void bt_mesh_va_pending_store(void)
{
	struct virtual_addr *lab;
	struct va_val va;
	char path[18];
	uint16_t i;
	int err;

	for (i = 0; (lab = bt_mesh_va_get(i)) != NULL; i++) {
		if (!lab->changed) {
			continue;
		}

		lab->changed = 0U;

		snprintk(path, sizeof(path), "bt/mesh/Va/%x", i);

		if (IS_VA_DEL(lab)) {
			err = settings_delete(path);
		} else {
			va.ref = lab->ref;
			va.addr = lab->addr;
			memcpy(va.uuid, lab->uuid, 16);

			err = settings_save_one(path, &va, sizeof(va));
		}

		if (err) {
			BT_ERR("Failed to %s %s value (err %d)",
			       IS_VA_DEL(lab) ? "delete" : "store",
			       log_strdup(path), err);
		} else {
			BT_DBG("%s %s value",
			       IS_VA_DEL(lab) ? "Deleted" : "Stored",
			       log_strdup(path));
		}
	}
}
#else
void bt_mesh_va_pending_store(void)
{
	/* Do nothing. */
}
#endif /* CONFIG_BT_MESH_LABEL_COUNT > 0 */
