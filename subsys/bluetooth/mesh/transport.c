/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/net/buf.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/mesh.h>

#include "common/bt_str.h"

#include "host/testing.h"

#include "crypto.h"
#include "mesh.h"
#include "net.h"
#include "app_keys.h"
#include "lpn.h"
#include "rpl.h"
#include "friend.h"
#include "access.h"
#include "foundation.h"
#include "sar_cfg_internal.h"
#include "settings.h"
#include "heartbeat.h"
#include "transport.h"
#include "va.h"

#define LOG_LEVEL CONFIG_BT_MESH_TRANS_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_transport);

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

/* How long to wait for available buffers before giving up */
#define BUF_TIMEOUT                 K_NO_WAIT

#define ACK_DELAY(seg_n)                                                       \
	(MIN(2 * seg_n + 1, BT_MESH_SAR_RX_ACK_DELAY_INC_X2) *                 \
	 BT_MESH_SAR_RX_SEG_INT_MS / 2)

#define SEQAUTH_ALREADY_PROCESSED_TIMEOUT                                      \
	(BT_MESH_SAR_RX_ACK_DELAY_INC_X2 * BT_MESH_SAR_RX_SEG_INT_MS / 2)

static struct seg_tx {
	struct bt_mesh_subnet *sub;
	void                  *seg[BT_MESH_TX_SEG_MAX];
	uint64_t              seq_auth;
	int64_t               adv_start_timestamp; /* Calculate adv duration and adjust intervals*/
	uint16_t              src;
	uint16_t              dst;
	uint16_t              ack_src;
	uint16_t              len;
	uint8_t               hdr;
	uint8_t               xmit;
	uint8_t               seg_n;         /* Last segment index */
	uint8_t               seg_o;         /* Segment being sent */
	uint8_t               nack_count;    /* Number of unacked segs */
	uint8_t               attempts_left;
	uint8_t               attempts_left_without_progress;
	uint8_t               ttl;           /* Transmitted TTL value */
	uint8_t               blocked:1,     /* Blocked by ongoing tx */
			      ctl:1,         /* Control packet */
			      aszmic:1,      /* MIC size */
			      started:1,     /* Start cb called */
			      friend_cred:1, /* Using Friend credentials */
			      seg_send_started:1, /* Used to check if seg_send_start cb is called */
			      ack_received:1; /* Ack received during seg message transmission. */
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
	uint8_t                     attempts_left;
	uint32_t                    block;
	uint32_t                    last_ack;
	struct k_work_delayable    ack;
	struct k_work_delayable    discard;
} seg_rx[CONFIG_BT_MESH_RX_SEG_MSG_COUNT];

K_MEM_SLAB_DEFINE(segs, BT_MESH_APP_SEG_SDU_MAX, CONFIG_BT_MESH_SEG_BUFS, 4);

static int send_unseg(struct bt_mesh_net_tx *tx, struct net_buf_simple *sdu,
		      const struct bt_mesh_send_cb *cb, void *cb_data,
		      const uint8_t *ctl_op)
{
	struct bt_mesh_adv *adv;

	adv = bt_mesh_adv_create(BT_MESH_ADV_DATA, BT_MESH_ADV_TAG_LOCAL,
				 tx->xmit, BUF_TIMEOUT);
	if (!adv) {
		LOG_ERR("Out of network advs");
		return -ENOBUFS;
	}

	net_buf_simple_reserve(&adv->b, BT_MESH_NET_HDR_LEN);

	if (ctl_op) {
		net_buf_simple_add_u8(&adv->b, TRANS_CTL_HDR(*ctl_op, 0));
	} else if (BT_MESH_IS_DEV_KEY(tx->ctx->app_idx)) {
		net_buf_simple_add_u8(&adv->b, UNSEG_HDR(0, 0));
	} else {
		net_buf_simple_add_u8(&adv->b, UNSEG_HDR(1, tx->aid));
	}

	net_buf_simple_add_mem(&adv->b, sdu->data, sdu->len);

	if (IS_ENABLED(CONFIG_BT_MESH_FRIEND)) {
		if (!bt_mesh_friend_queue_has_space(tx->sub->net_idx,
						    tx->src, tx->ctx->addr,
						    NULL, 1)) {
			if (BT_MESH_ADDR_IS_UNICAST(tx->ctx->addr)) {
				LOG_ERR("Not enough space in Friend Queue");
				bt_mesh_adv_unref(adv);
				return -ENOBUFS;
			} else {
				LOG_WRN("No space in Friend Queue");
				goto send;
			}
		}

		if (bt_mesh_friend_enqueue_tx(tx, BT_MESH_FRIEND_PDU_SINGLE,
					      NULL, 1, &adv->b) &&
		    BT_MESH_ADDR_IS_UNICAST(tx->ctx->addr)) {
			/* PDUs for a specific Friend should only go
			 * out through the Friend Queue.
			 */
			bt_mesh_adv_unref(adv);
			send_cb_finalize(cb, cb_data);
			return 0;
		}
	}

send:
	return bt_mesh_net_send(tx, adv, cb, cb_data);
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
	k_mem_slab_free(&segs, (void *)tx->seg[seg_idx]);
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
		LOG_DBG("Unblocked 0x%04x", (uint16_t)(blocked->seq_auth & TRANS_SEQ_ZERO_MASK));
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
	tx->seg_send_started = 0;
	tx->ack_received = 0;

	if (atomic_test_and_clear_bit(bt_mesh.flags, BT_MESH_IVU_PENDING)) {
		LOG_DBG("Proceeding with pending IV Update");
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

static void schedule_transmit_continue(struct seg_tx *tx, uint32_t delta)
{
	uint32_t timeout = 0;

	if (!tx->nack_count) {
		return;
	}

	LOG_DBG("");

	if (delta < BT_MESH_SAR_TX_SEG_INT_MS) {
		timeout = BT_MESH_SAR_TX_SEG_INT_MS - delta;
	}

	/* If it is not the last segment then continue transmission after Segment Interval,
	 * otherwise continue immediately as the callback will finish this transmission and
	 * progress into retransmission.
	 */
	k_work_reschedule(&tx->retransmit,
			  (tx->seg_o <= tx->seg_n) ?
					K_MSEC(timeout) :
					K_NO_WAIT);
}

static void seg_send_start(uint16_t duration, int err, void *user_data)
{
	struct seg_tx *tx = user_data;

	if (!tx->started && tx->cb && tx->cb->start) {
		tx->cb->start(duration, err, tx->cb_data);
		tx->started = 1U;
	}

	tx->seg_send_started = 1U;
	tx->adv_start_timestamp = k_uptime_get();

	/* If there's an error in transmitting the 'sent' callback will never
	 * be called. Make sure that we kick the retransmit timer also in this
	 * case since otherwise we risk the transmission of becoming stale.
	 */
	if (err) {
		schedule_transmit_continue(tx, 0);
	}
}

static void seg_sent(int err, void *user_data)
{
	struct seg_tx *tx = user_data;
	uint32_t delta_ms = (uint32_t)(k_uptime_get() - tx->adv_start_timestamp);

	if (!tx->seg_send_started) {
		return;
	}

	schedule_transmit_continue(tx, delta_ms);
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

	uint32_t delta_ms;
	uint32_t timeout;
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

	if (BT_MESH_ADDR_IS_UNICAST(tx->dst) &&
	    !tx->attempts_left_without_progress) {
		LOG_ERR("Ran out of retransmit without progress attempts");
		seg_tx_complete(tx, -ETIMEDOUT);
		return;
	}

	if (!tx->attempts_left) {
		if (BT_MESH_ADDR_IS_UNICAST(tx->dst)) {
			LOG_ERR("Ran out of retransmit attempts");
			seg_tx_complete(tx, -ETIMEDOUT);
		} else {
			/* Segmented sending to groups doesn't have acks, so
			 * running out of attempts is the expected behavior.
			 */
			seg_tx_complete(tx, 0);
		}

		return;
	}

	LOG_DBG("SeqZero: 0x%04x Attempts: %u",
		(uint16_t)(tx->seq_auth & TRANS_SEQ_ZERO_MASK), tx->attempts_left);

	while (tx->seg_o <= tx->seg_n) {
		struct bt_mesh_adv *seg;
		int err;

		if (!tx->seg[tx->seg_o]) {
			/* Move on to the next segment */
			tx->seg_o++;
			continue;
		}

		seg = bt_mesh_adv_create(BT_MESH_ADV_DATA, BT_MESH_ADV_TAG_LOCAL,
					 tx->xmit, BUF_TIMEOUT);
		if (!seg) {
			LOG_DBG("Allocating segment failed");
			goto end;
		}

		net_buf_simple_reserve(&seg->b, BT_MESH_NET_HDR_LEN);
		seg_tx_buf_build(tx, tx->seg_o, &seg->b);

		LOG_DBG("Sending %u/%u", tx->seg_o, tx->seg_n);

		err = bt_mesh_net_send(&net_tx, seg, &seg_sent_cb, tx);
		if (err) {
			LOG_DBG("Sending segment failed");
			goto end;
		}

		/* Move on to the next segment */
		tx->seg_o++;

		tx->ack_received = 0U;

		/* Return here to let the advertising layer process the message.
		 * This function will be called again after Segment Interval.
		 */
		return;
	}


	/* All segments have been sent */
	tx->seg_o = 0U;
	tx->attempts_left--;
	if (BT_MESH_ADDR_IS_UNICAST(tx->dst) && !tx->ack_received) {
		tx->attempts_left_without_progress--;
	}

end:
	if (IS_ENABLED(CONFIG_BT_MESH_LOW_POWER) &&
	    bt_mesh_lpn_established() && !bt_mesh_has_addr(ctx.addr)) {
		bt_mesh_lpn_poll();
	}

	delta_ms = (uint32_t)(k_uptime_get() - tx->adv_start_timestamp);
	if (tx->ack_received) {
		/* Schedule retransmission immediately but keep SAR segment interval time if
		 * SegAck was received while sending last segment.
		 */
		timeout = BT_MESH_SAR_TX_SEG_INT_MS;
		tx->ack_received = 0U;
	} else {
		timeout = BT_MESH_SAR_TX_RETRANS_TIMEOUT_MS(tx->dst, tx->ttl);
	}

	if (delta_ms < timeout) {
		timeout -= delta_ms;
	}

	/* Schedule a retransmission */
	k_work_reschedule(&tx->retransmit, K_MSEC(timeout));
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

	LOG_DBG("src 0x%04x dst 0x%04x app_idx 0x%04x aszmic %u sdu_len %u", net_tx->src,
		net_tx->ctx->addr, net_tx->ctx->app_idx, net_tx->aszmic, sdu->len);

	for (tx = NULL, i = 0; i < ARRAY_SIZE(seg_tx); i++) {
		if (seg_tx[i].nack_count) {
			blocked |= seg_tx_blocks(&seg_tx[i], net_tx->src,
						 net_tx->ctx->addr);
		} else if (!tx) {
			tx = &seg_tx[i];
		}
	}

	if (!tx) {
		LOG_ERR("No multi-segment message contexts available");
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
	tx->attempts_left = BT_MESH_SAR_TX_RETRANS_COUNT(tx->dst);
	tx->attempts_left_without_progress = BT_MESH_SAR_TX_RETRANS_NO_PROGRESS;
	tx->xmit = net_tx->xmit;
	tx->aszmic = net_tx->aszmic;
	tx->friend_cred = net_tx->friend_cred;
	tx->blocked = blocked;
	tx->started = 0;
	tx->seg_send_started = 0;
	tx->ctl = !!ctl_op;
	tx->ttl = net_tx->ctx->send_ttl;

	LOG_DBG("SeqZero 0x%04x (segs: %u)", (uint16_t)(tx->seq_auth & TRANS_SEQ_ZERO_MASK),
		tx->nack_count);

	if (IS_ENABLED(CONFIG_BT_MESH_FRIEND) &&
	    !bt_mesh_friend_queue_has_space(tx->sub->net_idx, net_tx->src,
					    tx->dst, &tx->seq_auth,
					    tx->seg_n + 1) &&
	    BT_MESH_ADDR_IS_UNICAST(tx->dst)) {
		LOG_ERR("Not enough space in Friend Queue for %u segments", tx->seg_n + 1);
		seg_tx_reset(tx);
		return -ENOBUFS;
	}

	for (seg_o = 0U; sdu->len; seg_o++) {
		void *buf;
		uint16_t len;
		int err;

		err = k_mem_slab_alloc(&segs, &buf, BUF_TIMEOUT);
		if (err) {
			LOG_ERR("Out of segment buffers");
			seg_tx_reset(tx);
			return -ENOBUFS;
		}

		len = MIN(sdu->len, seg_len(!!ctl_op));
		memcpy(buf, net_buf_simple_pull_mem(sdu, len), len);

		LOG_DBG("seg %u: %s", seg_o, bt_hex(buf, len));

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
				k_mem_slab_free(&segs, buf);
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
		LOG_DBG("Blocked.");
		return 0;
	}

	seg_tx_send_unacked(tx);

	return 0;
}

static int trans_encrypt(const struct bt_mesh_net_tx *tx, const struct bt_mesh_key *key,
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
		crypto.ad = tx->ctx->uuid;
	}

	return bt_mesh_app_encrypt(key, &crypto, msg);
}

int bt_mesh_trans_send(struct bt_mesh_net_tx *tx, struct net_buf_simple *msg,
		       const struct bt_mesh_send_cb *cb, void *cb_data)
{
	const struct bt_mesh_key *key;
	uint8_t aid;
	int err;

	if (msg->len < 1) {
		LOG_ERR("Zero-length SDU not allowed");
		return -EINVAL;
	}

	if (msg->len > BT_MESH_TX_SDU_MAX - BT_MESH_MIC_SHORT) {
		LOG_ERR("Message too big: %u", msg->len);
		return -EMSGSIZE;
	}

	if (net_buf_simple_tailroom(msg) < BT_MESH_MIC_SHORT) {
		LOG_ERR("Insufficient tailroom for Transport MIC");
		return -EINVAL;
	}

	if (tx->ctx->send_ttl == BT_MESH_TTL_DEFAULT) {
		tx->ctx->send_ttl = bt_mesh_default_ttl_get();
	} else if (tx->ctx->send_ttl > BT_MESH_TTL_MAX) {
		LOG_ERR("TTL too large (max 127)");
		return -EINVAL;
	}

	if (msg->len > BT_MESH_SDU_UNSEG_MAX) {
		tx->ctx->send_rel = true;
	}

	if (tx->ctx->addr == BT_MESH_ADDR_UNASSIGNED ||
	    (!BT_MESH_ADDR_IS_UNICAST(tx->ctx->addr) &&
	     BT_MESH_IS_DEV_KEY(tx->ctx->app_idx))) {
		LOG_ERR("Invalid destination address");
		return -EINVAL;
	}

	err = bt_mesh_keys_resolve(tx->ctx, &tx->sub, &key, &aid);
	if (err) {
		return err;
	}

	LOG_DBG("net_idx 0x%04x app_idx 0x%04x dst 0x%04x", tx->sub->net_idx, tx->ctx->app_idx,
		tx->ctx->addr);
	LOG_DBG("len %u: %s", msg->len, bt_hex(msg->data, msg->len));

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

static int sdu_try_decrypt(struct bt_mesh_net_rx *rx, const struct bt_mesh_key *key,
			   void *cb_data)
{
	struct decrypt_ctx *ctx = cb_data;
	int err;

	ctx->crypto.ad = NULL;

	do {
		if (ctx->seg) {
			seg_rx_assemble(ctx->seg, ctx->buf, ctx->crypto.aszmic);
		}

		if (BT_MESH_ADDR_IS_VIRTUAL(rx->ctx.recv_dst)) {
			ctx->crypto.ad = bt_mesh_va_uuid_get(rx->ctx.recv_dst, ctx->crypto.ad,
							     NULL);

			if (!ctx->crypto.ad) {
				return -ENOENT;
			}
		}

		net_buf_simple_reset(ctx->sdu);

		err = bt_mesh_app_decrypt(key, &ctx->crypto, ctx->buf, ctx->sdu);
	} while (err && ctx->crypto.ad != NULL);

	if (!err && BT_MESH_ADDR_IS_VIRTUAL(rx->ctx.recv_dst)) {
		rx->ctx.uuid = ctx->crypto.ad;
	}

	return err;
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

	LOG_DBG("AKF %u AID 0x%02x", !ctx.crypto.dev_key, AID(&hdr));

	if (!rx->local_match) {
		/* if friend_match was set the frame is for LPN which we are friends. */
		return rx->friend_match ? 0 : -ENXIO;
	}

	rx->ctx.app_idx = bt_mesh_app_key_find(ctx.crypto.dev_key, AID(&hdr),
					       rx, sdu_try_decrypt, &ctx);
	if (rx->ctx.app_idx == BT_MESH_KEY_UNUSED) {
		LOG_DBG("No matching AppKey");
		return -EACCES;
	}

	rx->ctx.uuid = ctx.crypto.ad;

	LOG_DBG("Decrypted (AppIdx: 0x%03x)", rx->ctx.app_idx);

	return bt_mesh_access_recv(&rx->ctx, sdu);
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
	bool new_seg_ack = false;
	struct seg_tx *tx;
	unsigned int bit;
	uint32_t ack;
	uint16_t seq_zero;
	uint8_t obo;

	if (buf->len < 6) {
		LOG_ERR("Too short ack message");
		return -EBADMSG;
	}

	seq_zero = net_buf_simple_pull_be16(buf);
	obo = seq_zero >> 15;
	seq_zero = (seq_zero >> 2) & TRANS_SEQ_ZERO_MASK;

	if (IS_ENABLED(CONFIG_BT_MESH_FRIEND) && rx->friend_match) {
		LOG_DBG("Ack for LPN 0x%04x of this Friend", rx->ctx.recv_dst);
		/* Best effort - we don't have enough info for true SeqAuth */
		*seq_auth = SEQ_AUTH(BT_MESH_NET_IVI_RX(rx), seq_zero);
		return 0;
	} else if (!rx->local_match) {
		return 0;
	}

	ack = net_buf_simple_pull_be32(buf);

	LOG_DBG("OBO %u seq_zero 0x%04x ack 0x%08x", obo, seq_zero, ack);

	tx = seg_tx_lookup(seq_zero, obo, rx->ctx.addr);
	if (!tx) {
		LOG_DBG("No matching TX context for ack");
		return -ENOENT;
	}

	if (!BT_MESH_ADDR_IS_UNICAST(tx->dst)) {
		LOG_ERR("Received ack for group seg");
		return -EINVAL;
	}

	*seq_auth = tx->seq_auth;

	if (!ack) {
		LOG_WRN("SDU canceled");
		seg_tx_complete(tx, -ECANCELED);
		return 0;
	}

	if (find_msb_set(ack) - 1 > tx->seg_n) {
		LOG_ERR("Too large segment number in ack");
		return -EINVAL;
	}

	while ((bit = find_lsb_set(ack))) {
		if (tx->seg[bit - 1]) {
			LOG_DBG("seg %u/%u acked", bit - 1, tx->seg_n);
			seg_tx_done(tx, bit - 1);
			new_seg_ack = true;
		}

		ack &= ~BIT(bit - 1);
	}

	if (new_seg_ack) {
		tx->attempts_left_without_progress =
			BT_MESH_SAR_TX_RETRANS_NO_PROGRESS;
	}

	if (tx->nack_count) {
		/* If transmission is not in progress it means
		 * that Retransmission Timer is running
		 */
		if (tx->seg_o == 0) {
			k_timeout_t timeout = K_NO_WAIT;

			/* If there are no retransmission attempts left we
			 * immediately trigger the retransmit call that will
			 * end the transmission.
			 */
			if ((BT_MESH_ADDR_IS_UNICAST(tx->dst) &&
			     !tx->attempts_left_without_progress) ||
			    !tx->attempts_left) {
				goto reschedule;
			}

			uint32_t delta_ms = (uint32_t)(k_uptime_get() - tx->adv_start_timestamp);

			/* According to MshPRTv1.1: 3.5.3.3.2, we should reset the retransmit timer
			 * and retransmit immediately when receiving a valid ack message while
			 * Retransmisison timer is running. However, transport should still keep
			 * segment transmission interval time between transmission of each segment.
			 */
			if (delta_ms < BT_MESH_SAR_TX_SEG_INT_MS) {
				timeout = K_MSEC(BT_MESH_SAR_TX_SEG_INT_MS - delta_ms);
			}

reschedule:
			k_work_reschedule(&tx->retransmit, timeout);
		} else {
			tx->ack_received = 1U;
		}
	} else {
		LOG_DBG("SDU TX complete");
		seg_tx_complete(tx, 0);
	}

	return 0;
}

static int ctl_recv(struct bt_mesh_net_rx *rx, uint8_t hdr,
		    struct net_buf_simple *buf, uint64_t *seq_auth)
{
	uint8_t ctl_op = TRANS_CTL_OP(&hdr);

	LOG_DBG("OpCode 0x%02x len %u", ctl_op, buf->len);

	switch (ctl_op) {
	case TRANS_CTL_OP_ACK:
		return trans_ack(rx, hdr, buf, seq_auth);
	case TRANS_CTL_OP_HEARTBEAT:
		return bt_mesh_hb_recv(rx, buf);
	}

	/* Only acks for friendship and heartbeats may need processing without local_match */
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
			LOG_WRN("Message from friend with wrong credentials");
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

	LOG_WRN("Unhandled TransOpCode 0x%02x", ctl_op);

	return -EBADMSG;
}

static int trans_unseg(struct net_buf_simple *buf, struct bt_mesh_net_rx *rx,
		       uint64_t *seq_auth)
{
	NET_BUF_SIMPLE_DEFINE_STATIC(sdu, BT_MESH_SDU_UNSEG_MAX);
	uint8_t hdr;
	struct bt_mesh_rpl *rpl = NULL;
	int err;

	LOG_DBG("AFK %u AID 0x%02x", AKF(buf->data), AID(buf->data));

	if (buf->len < 1) {
		LOG_ERR("Too small unsegmented PDU");
		return -EBADMSG;
	}

	if (bt_mesh_rpl_check(rx, &rpl)) {
		LOG_WRN("Replay: src 0x%04x dst 0x%04x seq 0x%06x", rx->ctx.addr, rx->ctx.recv_dst,
			rx->seq);
		return -EINVAL;
	}

	hdr = net_buf_simple_pull_u8(buf);

	if (rx->ctl) {
		err = ctl_recv(rx, hdr, buf, seq_auth);
	} else if (buf->len < 1 + APP_MIC_LEN(0)) {
		LOG_ERR("Too short SDU + MIC");
		err = -EINVAL;
	} else {
		/* Adjust the length to not contain the MIC at the end */
		buf->len -= APP_MIC_LEN(0);
		err = sdu_recv(rx, hdr, 0, buf, &sdu, NULL);
	}

	/* Update rpl only if there is place and upper logic accepted incoming data. */
	if (err == 0 && rpl != NULL) {
		bt_mesh_rpl_update(rpl, rx);
	}

	return err;
}

int bt_mesh_ctl_send(struct bt_mesh_net_tx *tx, uint8_t ctl_op, void *data,
		     size_t data_len,
		     const struct bt_mesh_send_cb *cb, void *cb_data)
{
	struct net_buf_simple buf;

	if (tx->ctx->send_ttl == BT_MESH_TTL_DEFAULT) {
		tx->ctx->send_ttl = bt_mesh_default_ttl_get();
	} else if (tx->ctx->send_ttl > BT_MESH_TTL_MAX) {
		LOG_ERR("TTL too large (max 127)");
		return -EINVAL;
	}

	net_buf_simple_init_with_data(&buf, data, data_len);

	if (data_len > BT_MESH_SDU_UNSEG_MAX) {
		tx->ctx->send_rel = true;
	}

	tx->ctx->app_idx = BT_MESH_KEY_UNUSED;

	if (tx->ctx->addr == BT_MESH_ADDR_UNASSIGNED ||
	    BT_MESH_ADDR_IS_VIRTUAL(tx->ctx->addr)) {
		LOG_ERR("Invalid destination address");
		return -EINVAL;
	}

	LOG_DBG("src 0x%04x dst 0x%04x ttl 0x%02x ctl 0x%02x", tx->src, tx->ctx->addr,
		tx->ctx->send_ttl, ctl_op);
	LOG_DBG("len %zu: %s", data_len, bt_hex(data, data_len));

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

	LOG_DBG("SeqZero 0x%04x Block 0x%08x OBO %u", seq_zero, block, obo);

	if (bt_mesh_lpn_established() && !bt_mesh_has_addr(ctx.addr)) {
		LOG_WRN("Not sending ack when LPN is enabled");
		return 0;
	}

	/* This can happen if the segmented message was destined for a group
	 * or virtual address.
	 */
	if (!BT_MESH_ADDR_IS_UNICAST(src)) {
		LOG_DBG("Not sending ack for non-unicast address");
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

	LOG_DBG("rx %p", rx);

	/* If this fails, the handler will exit early on the next execution, as
	 * it checks rx->in_use.
	 */
	(void)k_work_cancel_delayable(&rx->ack);
	(void)k_work_cancel_delayable(&rx->discard);

	if (IS_ENABLED(CONFIG_BT_MESH_FRIEND) && rx->obo &&
	    rx->block != BLOCK_COMPLETE(rx->seg_n)) {
		LOG_WRN("Clearing incomplete buffers from Friend queue");
		bt_mesh_friend_clear_incomplete(rx->sub, rx->src, rx->dst,
						&rx->seq_auth);
	}

	for (i = 0; i <= rx->seg_n; i++) {
		if (!rx->seg[i]) {
			continue;
		}

		k_mem_slab_free(&segs, rx->seg[i]);
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

static void seg_discard(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct seg_rx *rx = CONTAINER_OF(dwork, struct seg_rx, discard);

	LOG_WRN("SAR Discard timeout expired");
	seg_rx_reset(rx, false);
	rx->block = 0U;

	if (IS_ENABLED(CONFIG_BT_TESTING)) {
		bt_test_mesh_trans_incomp_timer_exp();
	}
}

static void seg_ack(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct seg_rx *rx = CONTAINER_OF(dwork, struct seg_rx, ack);

	if (!rx->in_use || rx->block == BLOCK_COMPLETE(rx->seg_n)) {
		/* Cancellation of this timer may have failed. If it fails as
		 * part of seg_reset, in_use will be false.
		 * If it fails as part of the processing of a fully received
		 * SDU, the ack is already being sent from the receive handler,
		 * and the timer based ack sending can be ignored.
		 */
		return;
	}

	LOG_DBG("rx %p", rx);

	send_ack(rx->sub, rx->dst, rx->src, rx->ttl, &rx->seq_auth,
		 rx->block, rx->obo);

	rx->last_ack = k_uptime_get_32();

	if (rx->attempts_left == 0) {
		LOG_DBG("Ran out of retransmit attempts");
		return;
	}

	if (rx->seg_n > BT_MESH_SAR_RX_SEG_THRESHOLD) {
		--rx->attempts_left;
		k_work_schedule(&rx->ack, K_MSEC(BT_MESH_SAR_RX_SEG_INT_MS));
	}
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
			LOG_WRN("Duplicate SDU from src 0x%04x", net_rx->ctx.addr);

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
		LOG_ERR("Invalid segment for ongoing session");
		return false;
	}

	if (rx->src != net_rx->ctx.addr || rx->dst != net_rx->ctx.recv_dst) {
		LOG_ERR("Invalid source or destination for segment");
		return false;
	}

	if (rx->ctl != net_rx->ctl) {
		LOG_ERR("Inconsistent CTL in segment");
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
		LOG_WRN("Not enough segments for incoming message");
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

		LOG_DBG("New RX context. Block Complete 0x%08x", BLOCK_COMPLETE(seg_n));

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
		LOG_ERR("Too short segmented message (len %u)", buf->len);
		return -EBADMSG;
	}

	if (bt_mesh_rpl_check(net_rx, &rpl)) {
		LOG_WRN("Replay: src 0x%04x dst 0x%04x seq 0x%06x", net_rx->ctx.addr,
			net_rx->ctx.recv_dst, net_rx->seq);
		return -EINVAL;
	}

	LOG_DBG("ASZMIC %u AKF %u AID 0x%02x", ASZMIC(hdr), AKF(hdr), AID(hdr));

	net_buf_simple_pull(buf, 1);

	seq_zero = net_buf_simple_pull_be16(buf);
	seg_o = (seq_zero & 0x03) << 3;
	seq_zero = (seq_zero >> 2) & TRANS_SEQ_ZERO_MASK;
	seg_n = net_buf_simple_pull_u8(buf);
	seg_o |= seg_n >> 5;
	seg_n &= 0x1f;

	LOG_DBG("SeqZero 0x%04x SegO %u SegN %u", seq_zero, seg_o, seg_n);

	if (seg_o > seg_n) {
		LOG_ERR("SegO greater than SegN (%u > %u)", seg_o, seg_n);
		return -EBADMSG;
	}

	/* According to MshPRTv1.1:
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
			LOG_WRN("Ignoring old SeqAuth");
			return -EINVAL;
		}

		if (!seg_rx_is_valid(rx, net_rx, hdr, seg_n)) {
			return -EINVAL;
		}

		if (rx->in_use) {
			LOG_DBG("Existing RX context. Block 0x%08x", rx->block);
			goto found_rx;
		}

		if (rx->block == BLOCK_COMPLETE(rx->seg_n)) {
			LOG_DBG("Got segment for already complete SDU");

			/* We should not send more than one Segment Acknowledgment message
			 * for the same SeqAuth in a period of:
			 * [acknowledgment delay increment * segment transmission interval]
			 *  milliseconds
			 */
			if (k_uptime_get_32() - rx->last_ack >
			    SEQAUTH_ALREADY_PROCESSED_TIMEOUT) {
				send_ack(net_rx->sub, net_rx->ctx.recv_dst,
					 net_rx->ctx.addr, net_rx->ctx.send_ttl,
					 seq_auth, rx->block, rx->obo);
				rx->last_ack = k_uptime_get_32();
			}

			if (rpl) {
				bt_mesh_rpl_update(rpl, net_rx);
			}

			return -EALREADY;
		}

		/* We ignore instead of sending block ack 0 since the
		 * ack timer is always smaller than the incomplete
		 * timer, i.e. the sender is misbehaving.
		 */
		LOG_WRN("Got segment for canceled SDU");
		return -EINVAL;
	}

	/* Bail out early if we're not ready to receive such a large SDU */
	if (!sdu_len_is_ok(net_rx->ctl, seg_n)) {
		LOG_ERR("Too big incoming SDU length");
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
		LOG_ERR("No space in Friend Queue for %u segments", *seg_count);
		send_ack(net_rx->sub, net_rx->ctx.recv_dst, net_rx->ctx.addr,
			 net_rx->ctx.send_ttl, seq_auth, 0,
			 net_rx->friend_match);
		return -ENOBUFS;
	}

	/* Keep track of the received SeqAuth values received from this address
	 * and discard segmented messages that are not newer, as described in
	 * MshPRTv1.1: 3.5.3.4.
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
		LOG_WRN("Ignoring old SeqAuth 0x%06x", auth_seqnum);
		return -EALREADY;
	}

	/* Look for free slot for a new RX session */
	rx = seg_rx_alloc(net_rx, hdr, seq_auth, seg_n);
	if (!rx) {
		/* Warn but don't cancel since the existing slots will
		 * eventually be freed up and we'll be able to process
		 * this one.
		 */
		LOG_WRN("No free slots for new incoming segmented messages");
		return -ENOMEM;
	}

	rx->obo = net_rx->friend_match;

found_rx:
	if (BIT(seg_o) & rx->block) {
		LOG_DBG("Received already received fragment");
		return -EALREADY;
	}

	/* All segments, except the last one, must either have 8 bytes of
	 * payload (for 64bit Net MIC) or 12 bytes of payload (for 32bit
	 * Net MIC).
	 */
	if (seg_o == seg_n) {
		/* Set the expected final buffer length */
		rx->len = seg_n * seg_len(rx->ctl) + buf->len;
		LOG_DBG("Target len %u * %u + %u = %u", seg_n, seg_len(rx->ctl), buf->len, rx->len);

		if (rx->len > BT_MESH_RX_SDU_MAX) {
			LOG_ERR("Too large SDU len");
			send_ack(net_rx->sub, net_rx->ctx.recv_dst,
				 net_rx->ctx.addr, net_rx->ctx.send_ttl,
				 seq_auth, 0, rx->obo);
			seg_rx_reset(rx, true);
			return -EMSGSIZE;
		}
	} else {
		if (buf->len != seg_len(rx->ctl)) {
			LOG_ERR("Incorrect segment size for message type");
			return -EINVAL;
		}
	}

	LOG_DBG("discard timeout %u", BT_MESH_SAR_RX_DISCARD_TIMEOUT_MS);
	k_work_schedule(&rx->discard,
			K_MSEC(BT_MESH_SAR_RX_DISCARD_TIMEOUT_MS));
	rx->attempts_left = BT_MESH_SAR_RX_ACK_RETRANS_COUNT;

	if (!bt_mesh_lpn_established() && BT_MESH_ADDR_IS_UNICAST(rx->dst)) {
		LOG_DBG("ack delay %u", ACK_DELAY(rx->seg_n));
		k_work_reschedule(&rx->ack, K_MSEC(ACK_DELAY(rx->seg_n)));
	}

	/* Allocated segment here */
	err = k_mem_slab_alloc(&segs, &rx->seg[seg_o], K_NO_WAIT);
	if (err) {
		LOG_WRN("Unable allocate buffer for Seg %u", seg_o);
		return -ENOBUFS;
	}

	memcpy(rx->seg[seg_o], buf->data, buf->len);

	LOG_DBG("Received %u/%u", seg_o, seg_n);

	/* Mark segment as received */
	rx->block |= BIT(seg_o);

	if (rx->block != BLOCK_COMPLETE(seg_n)) {
		*pdu_type = BT_MESH_FRIEND_PDU_PARTIAL;
		return 0;
	}

	LOG_DBG("Complete SDU");
	*pdu_type = BT_MESH_FRIEND_PDU_COMPLETE;

	/* If this fails, the work handler will either exit early because the
	 * block is fully received, or rx->in_use is false.
	 */
	(void)k_work_cancel_delayable(&rx->ack);

	send_ack(net_rx->sub, net_rx->ctx.recv_dst, net_rx->ctx.addr,
		 net_rx->ctx.send_ttl, seq_auth, rx->block, rx->obo);
	rx->last_ack = k_uptime_get_32();

	if (net_rx->ctl) {
		NET_BUF_SIMPLE_DEFINE(sdu, BT_MESH_RX_CTL_MAX);
		seg_rx_assemble(rx, &sdu, 0U);
		err = ctl_recv(net_rx, *hdr, &sdu, seq_auth);
	} else if (rx->len < 1 + APP_MIC_LEN(ASZMIC(hdr))) {
		LOG_ERR("Too short SDU + MIC");
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

	/* Update rpl only if there is place and upper logic accepted incoming data. */
	if (err == 0 && rpl != NULL) {
		bt_mesh_rpl_update(rpl, net_rx);
		/* Update the seg, unless it has already been surpassed:
		 * This needs to happen after rpl_update to ensure that the IV
		 * update reset logic inside rpl_update doesn't overwrite the
		 * change.
		 */
		rpl->seg = MAX(rpl->seg, auth_seqnum);
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

	LOG_DBG("src 0x%04x dst 0x%04x seq 0x%08x friend_match %u", rx->ctx.addr, rx->ctx.recv_dst,
		rx->seq, rx->friend_match);

	/* Remove network headers */
	net_buf_simple_pull(buf, BT_MESH_NET_HDR_LEN);

	LOG_DBG("Payload %s", bt_hex(buf->data, buf->len));

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
		LOG_WRN("Ignoring unexpected message in Low Power mode");
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

	LOG_DBG("");

	for (i = 0; i < ARRAY_SIZE(seg_rx); i++) {
		seg_rx_reset(&seg_rx[i], true);
	}
}

void bt_mesh_trans_reset(void)
{
	int i;

	bt_mesh_rx_reset();

	LOG_DBG("");

	for (i = 0; i < ARRAY_SIZE(seg_tx); i++) {
		seg_tx_reset(&seg_tx[i]);
	}

	bt_mesh_rpl_clear();
	bt_mesh_va_clear();
}

void bt_mesh_trans_init(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(seg_tx); i++) {
		k_work_init_delayable(&seg_tx[i].retransmit, seg_retransmit);
	}

	for (i = 0; i < ARRAY_SIZE(seg_rx); i++) {
		k_work_init_delayable(&seg_rx[i].ack, seg_ack);
		k_work_init_delayable(&seg_rx[i].discard, seg_discard);
	}
}
