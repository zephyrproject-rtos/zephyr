/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr.h>
#include <sys/byteorder.h>

#include <net/buf.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/mesh.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_MESH_DEBUG_FRIEND)
#define LOG_MODULE_NAME bt_mesh_friend
#include "common/log.h"

#include "crypto.h"
#include "adv.h"
#include "mesh.h"
#include "net.h"
#include "app_keys.h"
#include "transport.h"
#include "access.h"
#include "foundation.h"
#include "friend.h"

/* We reserve one extra buffer for each friendship, since we need to be able
 * to resend the last sent PDU, which sits separately outside of the queue.
 */
#define FRIEND_BUF_COUNT    ((CONFIG_BT_MESH_FRIEND_QUEUE_SIZE + 1) * \
			     CONFIG_BT_MESH_FRIEND_LPN_COUNT)

#define FRIEND_ADV(buf) CONTAINER_OF(BT_MESH_ADV(buf), struct friend_adv, adv)

/* PDUs from Friend to the LPN should only be transmitted once with the
 * smallest possible interval (20ms).
 */
#define FRIEND_XMIT         BT_MESH_TRANSMIT(0, 20)

struct friend_pdu_info {
	uint16_t  src;
	uint16_t  dst;

	uint8_t   seq[3];

	uint8_t   ttl:7,
	       ctl:1;

	uint32_t  iv_index;
};

NET_BUF_POOL_FIXED_DEFINE(friend_buf_pool, FRIEND_BUF_COUNT,
			  BT_MESH_ADV_DATA_SIZE, NULL);

static struct friend_adv {
	struct bt_mesh_adv adv;
	uint16_t app_idx;
} adv_pool[FRIEND_BUF_COUNT];

static struct bt_mesh_adv *adv_alloc(int id)
{
	adv_pool[id].app_idx = BT_MESH_KEY_UNUSED;
	return &adv_pool[id].adv;
}

static bool friend_is_allocated(const struct bt_mesh_friend *frnd)
{
	return frnd->subnet != NULL;
}

static bool is_lpn_unicast(struct bt_mesh_friend *frnd, uint16_t addr)
{
	if (frnd->lpn == BT_MESH_ADDR_UNASSIGNED) {
		return false;
	}

	return (addr >= frnd->lpn && addr < (frnd->lpn + frnd->num_elem));
}

struct bt_mesh_friend *bt_mesh_friend_find(uint16_t net_idx, uint16_t lpn_addr,
					   bool valid, bool established)
{
	int i;

	BT_DBG("net_idx 0x%04x lpn_addr 0x%04x", net_idx, lpn_addr);

	for (i = 0; i < ARRAY_SIZE(bt_mesh.frnd); i++) {
		struct bt_mesh_friend *frnd = &bt_mesh.frnd[i];

		if (valid && !friend_is_allocated(frnd)) {
			continue;
		}

		if (established && !frnd->established) {
			continue;
		}

		if (net_idx != BT_MESH_KEY_ANY &&
		    (!frnd->subnet || frnd->subnet->net_idx != net_idx)) {
			continue;
		}

		if (is_lpn_unicast(frnd, lpn_addr)) {
			return frnd;
		}
	}

	return NULL;
}

static int friend_cred_create(struct bt_mesh_friend *frnd, uint8_t idx)
{
	return bt_mesh_friend_cred_create(&frnd->cred[idx], frnd->lpn,
					  bt_mesh_primary_addr(),
					  frnd->lpn_counter, frnd->counter,
					  frnd->subnet->keys[idx].net);
}

static void purge_buffers(sys_slist_t *list)
{
	while (!sys_slist_is_empty(list)) {
		struct net_buf *buf;

		buf = (void *)sys_slist_get_not_empty(list);

		buf->frags = NULL;
		buf->flags &= ~NET_BUF_FRAGS;

		net_buf_unref(buf);
	}
}

/* Intentionally start a little bit late into the ReceiveWindow when
 * it's large enough. This may improve reliability with some platforms,
 * like the PTS, where the receiver might not have sufficiently compensated
 * for internal latencies required to start scanning.
 */
static int32_t recv_delay(struct bt_mesh_friend *frnd)
{
#if CONFIG_BT_MESH_FRIEND_RECV_WIN > 50
	return (int32_t)frnd->recv_delay + (CONFIG_BT_MESH_FRIEND_RECV_WIN / 5);
#else
	return frnd->recv_delay;
#endif
}

static void friend_clear(struct bt_mesh_friend *frnd)
{
	int i;

	BT_DBG("LPN 0x%04x", frnd->lpn);

	/* If cancelling the timer fails, we'll exit early in the work handler. */
	(void)k_work_cancel_delayable(&frnd->timer);

	memset(frnd->cred, 0, sizeof(frnd->cred));

	if (frnd->last) {
		/* Cancel the sending if necessary */
		if (frnd->pending_buf) {
			BT_MESH_ADV(frnd->last)->busy = 0U;
		}

		net_buf_unref(frnd->last);
		frnd->last = NULL;
	}

	purge_buffers(&frnd->queue);

	for (i = 0; i < ARRAY_SIZE(frnd->seg); i++) {
		struct bt_mesh_friend_seg *seg = &frnd->seg[i];

		purge_buffers(&seg->queue);
		seg->seg_count = 0U;
	}

	Z_STRUCT_SECTION_FOREACH(bt_mesh_friend_cb, cb) {
		if (frnd->established && cb->terminated) {
			cb->terminated(frnd->subnet->net_idx, frnd->lpn);
		}
	}

	frnd->counter++;
	frnd->subnet = NULL;
	frnd->established = 0U;
	frnd->pending_buf = 0U;
	frnd->fsn = 0U;
	frnd->queue_size = 0U;
	frnd->pending_req = 0U;
	(void)memset(frnd->sub_list, 0, sizeof(frnd->sub_list));
}

void bt_mesh_friends_clear(void)
{
	int i;

	BT_DBG("");

	for (i = 0; i < ARRAY_SIZE(bt_mesh.frnd); i++) {
		struct bt_mesh_friend *frnd = &bt_mesh.frnd[i];

		if (!friend_is_allocated(frnd)) {
			continue;
		}

		friend_clear(frnd);
	}
}

static void enqueue_update(struct bt_mesh_friend *frnd, uint8_t md);

void bt_mesh_friend_sec_update(uint16_t net_idx)
{
	int i;

	BT_DBG("net_idx 0x%04x", net_idx);

	for (i = 0; i < ARRAY_SIZE(bt_mesh.frnd); i++) {
		struct bt_mesh_friend *frnd = &bt_mesh.frnd[i];

		if (!friend_is_allocated(frnd)) {
			continue;
		}

		if (net_idx == BT_MESH_KEY_ANY ||
		    frnd->subnet->net_idx == net_idx) {
			enqueue_update(frnd, 0x00);
		}
	}
}

int bt_mesh_friend_clear(struct bt_mesh_net_rx *rx, struct net_buf_simple *buf)
{
	struct bt_mesh_ctl_friend_clear *msg = (void *)buf->data;
	struct bt_mesh_friend *frnd;
	uint16_t lpn_addr, lpn_counter;
	struct bt_mesh_net_tx tx = {
		.sub  = rx->sub,
		.ctx  = &rx->ctx,
		.src  = bt_mesh_primary_addr(),
		.xmit = bt_mesh_net_transmit_get(),
	};
	struct bt_mesh_ctl_friend_clear_confirm cfm;

	if (buf->len < sizeof(*msg)) {
		BT_WARN("Too short Friend Clear");
		return -EINVAL;
	}

	lpn_addr = sys_be16_to_cpu(msg->lpn_addr);
	lpn_counter = sys_be16_to_cpu(msg->lpn_counter);

	BT_DBG("LPN addr 0x%04x counter 0x%04x", lpn_addr, lpn_counter);

	frnd = bt_mesh_friend_find(rx->sub->net_idx, lpn_addr, false, false);
	if (!frnd) {
		BT_WARN("No matching LPN addr 0x%04x", lpn_addr);
		return 0;
	}

	/* A Friend Clear message is considered valid if the result of the
	 * subtraction of the value of the LPNCounter field of the Friend
	 * Request message (the one that initiated the friendship) from the
	 * value of the LPNCounter field of the Friend Clear message, modulo
	 * 65536, is in the range 0 to 255 inclusive.
	 */
	if (lpn_counter - frnd->lpn_counter > 255) {
		BT_WARN("LPN Counter out of range (old %u new %u)",
			frnd->lpn_counter, lpn_counter);
		return 0;
	}

	tx.ctx->send_ttl = BT_MESH_TTL_MAX;

	cfm.lpn_addr    = msg->lpn_addr;
	cfm.lpn_counter = msg->lpn_counter;

	bt_mesh_ctl_send(&tx, TRANS_CTL_OP_FRIEND_CLEAR_CFM, &cfm,
			 sizeof(cfm), NULL, NULL);

	friend_clear(frnd);

	return 0;
}

static void friend_sub_add(struct bt_mesh_friend *frnd, uint16_t addr)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(frnd->sub_list); i++) {
		if (frnd->sub_list[i] == BT_MESH_ADDR_UNASSIGNED) {
			frnd->sub_list[i] = addr;
			return;
		}
	}

	BT_WARN("No space in friend subscription list");
}

static void friend_sub_rem(struct bt_mesh_friend *frnd, uint16_t addr)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(frnd->sub_list); i++) {
		if (frnd->sub_list[i] == addr) {
			frnd->sub_list[i] = BT_MESH_ADDR_UNASSIGNED;
			return;
		}
	}
}

static struct net_buf *create_friend_pdu(struct bt_mesh_friend *frnd,
					 struct friend_pdu_info *info,
					 struct net_buf_simple *sdu)
{
	struct net_buf *buf;

	buf = bt_mesh_adv_create_from_pool(&friend_buf_pool, adv_alloc,
					   BT_MESH_ADV_DATA,
					   FRIEND_XMIT, K_NO_WAIT);
	if (!buf) {
		return NULL;
	}

	net_buf_add_u8(buf, (info->iv_index & 1) << 7); /* Will be reset in encryption */

	if (info->ctl) {
		net_buf_add_u8(buf, info->ttl | 0x80);
	} else {
		net_buf_add_u8(buf, info->ttl);
	}

	net_buf_add_mem(buf, info->seq, sizeof(info->seq));

	net_buf_add_be16(buf, info->src);
	net_buf_add_be16(buf, info->dst);

	net_buf_add_mem(buf, sdu->data, sdu->len);

	return buf;
}

struct unseg_app_sdu_meta {
	struct bt_mesh_app_crypto_ctx crypto;
	const uint8_t *key;
	struct bt_mesh_subnet *subnet;
	uint8_t aid;
};

static int unseg_app_sdu_unpack(struct bt_mesh_friend *frnd,
				struct net_buf *buf,
				struct unseg_app_sdu_meta *meta)
{
	uint16_t app_idx = FRIEND_ADV(buf)->app_idx;
	struct bt_mesh_net_rx net = {
		.ctx = {
			.app_idx = app_idx,
			.net_idx = frnd->subnet->net_idx,
		},
	};
	int err;

	meta->subnet = frnd->subnet;
	bt_mesh_net_header_parse(&buf->b, &net);
	err = bt_mesh_keys_resolve(&net.ctx, &net.sub, &meta->key, &meta->aid);
	if (err) {
		return err;
	}

	meta->crypto.src = net.ctx.addr;
	meta->crypto.dst = net.ctx.recv_dst;
	meta->crypto.iv_index = BT_MESH_NET_IVI_TX;
	meta->crypto.dev_key = BT_MESH_IS_DEV_KEY(app_idx);
	meta->crypto.seq_num = net.seq;
	meta->crypto.aszmic = 0;

	if (BT_MESH_ADDR_IS_VIRTUAL(meta->crypto.dst)) {
		meta->crypto.ad = bt_mesh_va_label_get(meta->crypto.dst);
		if (!meta->crypto.ad) {
			return -ENOENT;
		}
	} else {
		meta->crypto.ad = NULL;
	}

	return 0;
}

static int unseg_app_sdu_decrypt(struct bt_mesh_friend *frnd,
				 struct net_buf *buf,
				 const struct unseg_app_sdu_meta *meta)
{
	struct net_buf_simple sdu;

	net_buf_simple_clone(&buf->b, &sdu);
	net_buf_simple_pull(&sdu, 10);
	sdu.len -= 4;

	return bt_mesh_app_decrypt(meta->key, &meta->crypto, &sdu, &sdu);
}

static int unseg_app_sdu_encrypt(struct bt_mesh_friend *frnd,
				 struct net_buf *buf,
				 const struct unseg_app_sdu_meta *meta)
{
	struct net_buf_simple sdu;

	net_buf_simple_clone(&buf->b, &sdu);
	net_buf_simple_pull(&sdu, 10);
	sdu.len -= 4;

	return bt_mesh_app_encrypt(meta->key, &meta->crypto, &sdu);
}

static int unseg_app_sdu_prepare(struct bt_mesh_friend *frnd,
				 struct net_buf *buf)
{
	struct unseg_app_sdu_meta meta;
	int err;

	if (FRIEND_ADV(buf)->app_idx == BT_MESH_KEY_UNUSED) {
		return 0;
	}

	err = unseg_app_sdu_unpack(frnd, buf, &meta);
	if (err) {
		return err;
	}

	/* No need to reencrypt the message if the sequence number is
	 * unchanged.
	 */
	if (meta.crypto.seq_num == bt_mesh.seq) {
		return 0;
	}

	BT_DBG("Re-encrypting friend pdu (SeqNum %06x -> %06x)",
	       meta.crypto.seq_num, bt_mesh.seq);

	err = unseg_app_sdu_decrypt(frnd, buf, &meta);
	if (err) {
		BT_WARN("Decryption failed! %d", err);
		return err;
	}

	meta.crypto.seq_num = bt_mesh.seq;

	err = unseg_app_sdu_encrypt(frnd, buf, &meta);
	if (err) {
		BT_WARN("Re-encryption failed! %d", err);
	}

	return err;
}

static int encrypt_friend_pdu(struct bt_mesh_friend *frnd, struct net_buf *buf,
			      bool master_cred)
{
	const struct bt_mesh_net_cred *cred;
	uint32_t iv_index;
	uint16_t src;
	int err;

	if (master_cred) {
		cred = &frnd->subnet->keys[SUBNET_KEY_TX_IDX(frnd->subnet)]
				.msg;
	} else {
		cred = &frnd->cred[SUBNET_KEY_TX_IDX(frnd->subnet)];
	}

	src = sys_get_be16(&buf->data[5]);

	if (bt_mesh_has_addr(src)) {
		uint32_t seq;

		if (FRIEND_ADV(buf)->app_idx != BT_MESH_KEY_UNUSED) {
			err = unseg_app_sdu_prepare(frnd, buf);
			if (err) {
				return err;
			}
		}

		seq = bt_mesh_next_seq();
		sys_put_be24(seq, &buf->data[2]);

		iv_index = BT_MESH_NET_IVI_TX;
		FRIEND_ADV(buf)->app_idx = BT_MESH_KEY_UNUSED;
	} else {
		uint8_t ivi = (buf->data[0] >> 7);
		iv_index = (bt_mesh.iv_index - ((bt_mesh.iv_index & 1) != ivi));
	}

	buf->data[0] = (cred->nid | (iv_index & 1) << 7);

	if (bt_mesh_net_encrypt(cred->enc, &buf->b, iv_index, false)) {
		BT_ERR("Encrypting failed");
		return -EINVAL;
	}

	if (bt_mesh_net_obfuscate(buf->data, iv_index, cred->privacy)) {
		BT_ERR("Obfuscating failed");
		return -EINVAL;
	}

	return 0;
}

static struct net_buf *encode_friend_ctl(struct bt_mesh_friend *frnd,
					 uint8_t ctl_op,
					 struct net_buf_simple *sdu)
{
	struct friend_pdu_info info;

	BT_DBG("LPN 0x%04x", frnd->lpn);

	net_buf_simple_push_u8(sdu, TRANS_CTL_HDR(ctl_op, 0));

	info.src = bt_mesh_primary_addr();
	info.dst = frnd->lpn;

	info.ctl = 1U;
	info.ttl = 0U;

	memset(info.seq, 0, sizeof(info.seq));

	info.iv_index = BT_MESH_NET_IVI_TX;

	return create_friend_pdu(frnd, &info, sdu);
}

static struct net_buf *encode_update(struct bt_mesh_friend *frnd, uint8_t md)
{
	struct bt_mesh_ctl_friend_update *upd;
	NET_BUF_SIMPLE_DEFINE(sdu, 1 + sizeof(*upd));

	__ASSERT_NO_MSG(friend_is_allocated(frnd));

	BT_DBG("lpn 0x%04x md 0x%02x", frnd->lpn, md);

	net_buf_simple_reserve(&sdu, 1);

	upd = net_buf_simple_add(&sdu, sizeof(*upd));
	upd->flags = bt_mesh_net_flags(frnd->subnet);
	upd->iv_index = sys_cpu_to_be32(bt_mesh.iv_index);
	upd->md = md;

	return encode_friend_ctl(frnd, TRANS_CTL_OP_FRIEND_UPDATE, &sdu);
}

static void enqueue_sub_cfm(struct bt_mesh_friend *frnd, uint8_t xact)
{
	struct bt_mesh_ctl_friend_sub_confirm *cfm;
	NET_BUF_SIMPLE_DEFINE(sdu, 1 + sizeof(*cfm));
	struct net_buf *buf;

	BT_DBG("lpn 0x%04x xact 0x%02x", frnd->lpn, xact);

	net_buf_simple_reserve(&sdu, 1);

	cfm = net_buf_simple_add(&sdu, sizeof(*cfm));
	cfm->xact = xact;

	buf = encode_friend_ctl(frnd, TRANS_CTL_OP_FRIEND_SUB_CFM, &sdu);
	if (!buf) {
		BT_ERR("Unable to encode Subscription List Confirmation");
		return;
	}

	if (encrypt_friend_pdu(frnd, buf, false)) {
		return;
	}

	if (frnd->last) {
		BT_DBG("Discarding last PDU");
		net_buf_unref(frnd->last);
	}

	frnd->last = buf;
	frnd->send_last = 1U;
}

static void friend_recv_delay(struct bt_mesh_friend *frnd)
{
	int32_t delay = recv_delay(frnd);

	frnd->pending_req = 1U;
	k_work_reschedule(&frnd->timer, K_MSEC(delay));
	BT_DBG("Waiting RecvDelay of %d ms", delay);
}

int bt_mesh_friend_sub_add(struct bt_mesh_net_rx *rx,
			   struct net_buf_simple *buf)
{
	struct bt_mesh_friend *frnd;
	uint8_t xact;

	if (buf->len < BT_MESH_FRIEND_SUB_MIN_LEN) {
		BT_WARN("Too short Friend Subscription Add");
		return -EINVAL;
	}

	frnd = bt_mesh_friend_find(rx->sub->net_idx, rx->ctx.addr, true, true);
	if (!frnd) {
		BT_WARN("No matching LPN addr 0x%04x", rx->ctx.addr);
		return 0;
	}

	if (frnd->pending_buf) {
		BT_WARN("Previous buffer not yet sent!");
		return 0;
	}

	friend_recv_delay(frnd);

	xact = net_buf_simple_pull_u8(buf);

	while (buf->len >= 2U) {
		friend_sub_add(frnd, net_buf_simple_pull_be16(buf));
	}

	enqueue_sub_cfm(frnd, xact);

	return 0;
}

int bt_mesh_friend_sub_rem(struct bt_mesh_net_rx *rx,
			   struct net_buf_simple *buf)
{
	struct bt_mesh_friend *frnd;
	uint8_t xact;

	if (buf->len < BT_MESH_FRIEND_SUB_MIN_LEN) {
		BT_WARN("Too short Friend Subscription Remove");
		return -EINVAL;
	}

	frnd = bt_mesh_friend_find(rx->sub->net_idx, rx->ctx.addr, true, true);
	if (!frnd) {
		BT_WARN("No matching LPN addr 0x%04x", rx->ctx.addr);
		return 0;
	}

	if (frnd->pending_buf) {
		BT_WARN("Previous buffer not yet sent!");
		return 0;
	}

	friend_recv_delay(frnd);

	xact = net_buf_simple_pull_u8(buf);

	while (buf->len >= 2U) {
		friend_sub_rem(frnd, net_buf_simple_pull_be16(buf));
	}

	enqueue_sub_cfm(frnd, xact);

	return 0;
}

static void enqueue_buf(struct bt_mesh_friend *frnd, struct net_buf *buf)
{
	net_buf_slist_put(&frnd->queue, buf);
	frnd->queue_size++;
}

static void enqueue_update(struct bt_mesh_friend *frnd, uint8_t md)
{
	struct net_buf *buf;

	buf = encode_update(frnd, md);
	if (!buf) {
		BT_ERR("Unable to encode Friend Update");
		return;
	}

	enqueue_buf(frnd, buf);
}

int bt_mesh_friend_poll(struct bt_mesh_net_rx *rx, struct net_buf_simple *buf)
{
	struct bt_mesh_ctl_friend_poll *msg = (void *)buf->data;
	struct bt_mesh_friend *frnd;

	if (buf->len < sizeof(*msg)) {
		BT_WARN("Too short Friend Poll");
		return -EINVAL;
	}

	frnd = bt_mesh_friend_find(rx->sub->net_idx, rx->ctx.addr, true, false);
	if (!frnd) {
		BT_WARN("No matching LPN addr 0x%04x", rx->ctx.addr);
		return 0;
	}

	if (msg->fsn & ~1) {
		BT_WARN("Prohibited (non-zero) padding bits");
		return -EINVAL;
	}

	if (frnd->pending_buf) {
		BT_WARN("Previous buffer not yet sent");
		return 0;
	}

	BT_DBG("msg->fsn %u frnd->fsn %u", (msg->fsn & 1), frnd->fsn);

	friend_recv_delay(frnd);

	Z_STRUCT_SECTION_FOREACH(bt_mesh_friend_cb, cb) {
		if (cb->polled) {
			cb->polled(frnd->subnet->net_idx, frnd->lpn);
		}
	}

	if (!frnd->established) {
		BT_DBG("Friendship established with 0x%04x", frnd->lpn);
		frnd->established = 1U;

		Z_STRUCT_SECTION_FOREACH(bt_mesh_friend_cb, cb) {
			if (cb->established) {
				cb->established(frnd->subnet->net_idx, frnd->lpn, frnd->recv_delay,
						frnd->poll_to);
			}
		}
	}

	if (msg->fsn == frnd->fsn && frnd->last) {
		BT_DBG("Re-sending last PDU");
		frnd->send_last = 1U;
	} else {
		if (frnd->last) {
			net_buf_unref(frnd->last);
			frnd->last = NULL;
		}

		frnd->fsn = msg->fsn;

		if (sys_slist_is_empty(&frnd->queue)) {
			enqueue_update(frnd, 0);
			BT_DBG("Enqueued Friend Update to empty queue");
		}
	}

	return 0;
}

static struct bt_mesh_friend *find_clear(uint16_t prev_friend)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(bt_mesh.frnd); i++) {
		struct bt_mesh_friend *frnd = &bt_mesh.frnd[i];

		if (frnd->clear.frnd == prev_friend) {
			return frnd;
		}
	}

	return NULL;
}

static void friend_clear_sent(int err, void *user_data)
{
	struct bt_mesh_friend *frnd = user_data;

	k_work_reschedule(&frnd->clear.timer,
			  K_SECONDS(frnd->clear.repeat_sec));
	frnd->clear.repeat_sec *= 2U;
}

static const struct bt_mesh_send_cb clear_sent_cb = {
	.end = friend_clear_sent,
};

static void send_friend_clear(struct bt_mesh_friend *frnd)
{
	struct bt_mesh_msg_ctx ctx = {
		.net_idx  = frnd->subnet->net_idx,
		.app_idx  = BT_MESH_KEY_UNUSED,
		.addr     = frnd->clear.frnd,
		.send_ttl = BT_MESH_TTL_MAX,
	};
	struct bt_mesh_net_tx tx = {
		.sub  = frnd->subnet,
		.ctx  = &ctx,
		.src  = bt_mesh_primary_addr(),
		.xmit = bt_mesh_net_transmit_get(),
	};
	struct bt_mesh_ctl_friend_clear req = {
		.lpn_addr    = sys_cpu_to_be16(frnd->lpn),
		.lpn_counter = sys_cpu_to_be16(frnd->lpn_counter),
	};

	BT_DBG("");

	bt_mesh_ctl_send(&tx, TRANS_CTL_OP_FRIEND_CLEAR, &req,
			 sizeof(req), &clear_sent_cb, frnd);
}

static void clear_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct bt_mesh_friend *frnd = CONTAINER_OF(dwork, struct bt_mesh_friend,
						   clear.timer);
	uint32_t duration;

	if (frnd->clear.frnd == BT_MESH_ADDR_UNASSIGNED) {
		/* Failed cancelling timer, return early. */
		return;
	}

	BT_DBG("LPN 0x%04x (old) Friend 0x%04x", frnd->lpn, frnd->clear.frnd);

	duration = k_uptime_get_32() - frnd->clear.start;
	if (duration > 2 * frnd->poll_to) {
		BT_DBG("Clear Procedure timer expired");
		frnd->clear.frnd = BT_MESH_ADDR_UNASSIGNED;
		return;
	}

	send_friend_clear(frnd);
}

static void clear_procedure_start(struct bt_mesh_friend *frnd)
{
	BT_DBG("LPN 0x%04x (old) Friend 0x%04x", frnd->lpn, frnd->clear.frnd);

	frnd->clear.start = k_uptime_get_32();
	frnd->clear.repeat_sec = 1U;

	send_friend_clear(frnd);
}

int bt_mesh_friend_clear_cfm(struct bt_mesh_net_rx *rx,
			     struct net_buf_simple *buf)
{
	struct bt_mesh_ctl_friend_clear_confirm *msg = (void *)buf->data;
	struct bt_mesh_friend *frnd;
	uint16_t lpn_addr, lpn_counter;

	BT_DBG("");

	if (buf->len < sizeof(*msg)) {
		BT_WARN("Too short Friend Clear Confirm");
		return -EINVAL;
	}

	frnd = find_clear(rx->ctx.addr);
	if (!frnd) {
		BT_WARN("No pending clear procedure for 0x%02x", rx->ctx.addr);
		return 0;
	}

	lpn_addr = sys_be16_to_cpu(msg->lpn_addr);
	if (lpn_addr != frnd->lpn) {
		BT_WARN("LPN address mismatch (0x%04x != 0x%04x)",
			lpn_addr, frnd->lpn);
		return 0;
	}

	lpn_counter = sys_be16_to_cpu(msg->lpn_counter);
	if (lpn_counter != frnd->lpn_counter) {
		BT_WARN("LPN counter mismatch (0x%04x != 0x%04x)",
			lpn_counter, frnd->lpn_counter);
		return 0;
	}

	/* If this fails, the unassigned check will make the handler return early. */
	(void)k_work_cancel_delayable(&frnd->clear.timer);
	frnd->clear.frnd = BT_MESH_ADDR_UNASSIGNED;

	return 0;
}

static void enqueue_offer(struct bt_mesh_friend *frnd, int8_t rssi)
{
	struct bt_mesh_ctl_friend_offer *off;
	NET_BUF_SIMPLE_DEFINE(sdu, 1 + sizeof(*off));
	struct net_buf *buf;

	BT_DBG("");

	net_buf_simple_reserve(&sdu, 1);

	off = net_buf_simple_add(&sdu, sizeof(*off));

	off->recv_win = CONFIG_BT_MESH_FRIEND_RECV_WIN,
	off->queue_size = CONFIG_BT_MESH_FRIEND_QUEUE_SIZE,
	off->sub_list_size = ARRAY_SIZE(frnd->sub_list),
	off->rssi = rssi,

	/* The Counter may be used in the later key update procedure. Therefore
	 * we should postpone the update of counter until we terminated friendship.
	 */
	off->frnd_counter = sys_cpu_to_be16(frnd->counter);

	buf = encode_friend_ctl(frnd, TRANS_CTL_OP_FRIEND_OFFER, &sdu);
	if (!buf) {
		BT_ERR("Unable to encode Friend Offer");
		return;
	}

	if (encrypt_friend_pdu(frnd, buf, true)) {
		return;
	}

	if (frnd->last) {
		net_buf_unref(frnd->last);
	}

	frnd->last = buf;
	frnd->send_last = 1U;
}

#define RECV_WIN                  CONFIG_BT_MESH_FRIEND_RECV_WIN
#define RSSI_FACT(crit)           (((crit) >> 5) & (uint8_t)BIT_MASK(2))
#define RECV_WIN_FACT(crit)       (((crit) >> 3) & (uint8_t)BIT_MASK(2))
#define MIN_QUEUE_SIZE_LOG(crit)  ((crit) & (uint8_t)BIT_MASK(3))
#define MIN_QUEUE_SIZE(crit)      ((uint32_t)BIT(MIN_QUEUE_SIZE_LOG(crit)))

static int32_t offer_delay(struct bt_mesh_friend *frnd, int8_t rssi, uint8_t crit)
{
	/* Scaling factors. The actual values are 1, 1.5, 2 & 2.5, but we
	 * want to avoid floating-point arithmetic.
	 */
	static const uint8_t fact[] = { 10, 15, 20, 25 };
	int32_t delay;

	BT_DBG("ReceiveWindowFactor %u ReceiveWindow %u RSSIFactor %u RSSI %d",
	       fact[RECV_WIN_FACT(crit)], RECV_WIN,
	       fact[RSSI_FACT(crit)], rssi);

	/* Delay = ReceiveWindowFactor * ReceiveWindow - RSSIFactor * RSSI */
	delay = (int32_t)fact[RECV_WIN_FACT(crit)] * RECV_WIN;
	delay -= (int32_t)fact[RSSI_FACT(crit)] * rssi;
	delay /= 10;

	BT_DBG("Local Delay calculated as %d ms", delay);

	return MAX(delay, 100);
}

int bt_mesh_friend_req(struct bt_mesh_net_rx *rx, struct net_buf_simple *buf)
{
	struct bt_mesh_ctl_friend_req *msg = (void *)buf->data;
	struct bt_mesh_friend *frnd = NULL;
	uint32_t poll_to;
	int32_t delay;
	int i, err;

	if (rx->net_if == BT_MESH_NET_IF_LOCAL) {
		BT_DBG("Ignoring Friend request from local interface");
		return 0;
	}

	if (buf->len < sizeof(*msg)) {
		BT_WARN("Too short Friend Request");
		return -EINVAL;
	}

	if (msg->recv_delay <= 0x09) {
		BT_WARN("Prohibited ReceiveDelay (0x%02x)", msg->recv_delay);
		return -EINVAL;
	}

	poll_to = sys_get_be24(msg->poll_to);

	if (poll_to <= 0x000009 || poll_to >= 0x34bc00) {
		BT_WARN("Prohibited PollTimeout (0x%06x)", poll_to);
		return -EINVAL;
	}

	if (msg->num_elem == 0x00) {
		BT_WARN("Prohibited NumElements value (0x00)");
		return -EINVAL;
	}

	if (!BT_MESH_ADDR_IS_UNICAST(rx->ctx.addr + msg->num_elem - 1)) {
		BT_WARN("LPN elements stretch outside of unicast range");
		return -EINVAL;
	}

	if (!MIN_QUEUE_SIZE_LOG(msg->criteria)) {
		BT_WARN("Prohibited Minimum Queue Size in Friend Request");
		return -EINVAL;
	}

	if (CONFIG_BT_MESH_FRIEND_QUEUE_SIZE < MIN_QUEUE_SIZE(msg->criteria)) {
		BT_WARN("We have a too small Friend Queue size (%u < %u)",
			CONFIG_BT_MESH_FRIEND_QUEUE_SIZE,
			MIN_QUEUE_SIZE(msg->criteria));
		return 0;
	}

	frnd = bt_mesh_friend_find(rx->sub->net_idx, rx->ctx.addr, true, false);
	if (frnd) {
		BT_WARN("Existing LPN re-requesting Friendship");
		friend_clear(frnd);
		goto init_friend;
	}

	for (i = 0; i < ARRAY_SIZE(bt_mesh.frnd); i++) {
		if (!bt_mesh.frnd[i].subnet) {
			frnd = &bt_mesh.frnd[i];
			break;
		}
	}

	if (!frnd) {
		BT_WARN("No free Friend contexts for new LPN");
		return -ENOMEM;
	}

init_friend:
	frnd->lpn = rx->ctx.addr;
	frnd->num_elem = msg->num_elem;
	frnd->subnet = rx->sub;
	frnd->recv_delay = msg->recv_delay;
	frnd->poll_to = poll_to * 100U;
	frnd->lpn_counter = sys_be16_to_cpu(msg->lpn_counter);
	frnd->clear.frnd = sys_be16_to_cpu(msg->prev_addr);

	err = friend_cred_create(frnd, SUBNET_KEY_TX_IDX(frnd->subnet));
	if (err) {
		BT_ERR("Failed to create friend credentials");
		friend_clear(frnd);
		return -EIO;
	}

	BT_DBG("LPN 0x%04x rssi %d recv_delay %u poll_to %ums",
	       frnd->lpn, rx->ctx.recv_rssi, frnd->recv_delay, frnd->poll_to);

	if (BT_MESH_ADDR_IS_UNICAST(frnd->clear.frnd) &&
	    !bt_mesh_has_addr(frnd->clear.frnd)) {
		clear_procedure_start(frnd);
	}

	delay = offer_delay(frnd, rx->ctx.recv_rssi, msg->criteria);
	k_work_reschedule(&frnd->timer, K_MSEC(delay));

	enqueue_offer(frnd, rx->ctx.recv_rssi);

	return 0;
}

static bool is_seg(struct bt_mesh_friend_seg *seg, uint16_t src, uint16_t seq_zero)
{
	struct net_buf *buf = (void *)sys_slist_peek_head(&seg->queue);
	struct net_buf_simple_state state;
	uint16_t buf_seq_zero;
	uint16_t buf_src;

	if (!buf) {
		return false;
	}

	net_buf_simple_save(&buf->b, &state);
	net_buf_skip(buf, 5);   /* skip IVI, NID, CTL, TTL, SEQ */
	buf_src = net_buf_pull_be16(buf);
	net_buf_skip(buf, 3);   /* skip DST, OP/AID */
	buf_seq_zero = ((net_buf_pull_be16(buf) >> 2) & TRANS_SEQ_ZERO_MASK);
	net_buf_simple_restore(&buf->b, &state);

	return ((src == buf_src) && (seq_zero == buf_seq_zero));
}

static struct bt_mesh_friend_seg *get_seg(struct bt_mesh_friend *frnd,
					  uint16_t src, uint16_t seq_zero,
					  uint8_t seg_count)
{
	struct bt_mesh_friend_seg *unassigned = NULL;
	int i;

	for (i = 0; i < ARRAY_SIZE(frnd->seg); i++) {
		struct bt_mesh_friend_seg *seg = &frnd->seg[i];

		if (is_seg(seg, src, seq_zero)) {
			return seg;
		}

		if (!unassigned && !sys_slist_peek_head(&seg->queue)) {
			unassigned = seg;
		}
	}

	if (unassigned) {
		unassigned->seg_count = seg_count;
	}

	return unassigned;
}

static void enqueue_friend_pdu(struct bt_mesh_friend *frnd,
			       enum bt_mesh_friend_pdu_type type,
			       uint16_t src, uint8_t seg_count,
			       struct net_buf *buf)
{
	struct bt_mesh_friend_seg *seg;

	BT_DBG("type %u", type);

	if (type == BT_MESH_FRIEND_PDU_SINGLE) {
		enqueue_buf(frnd, buf);
		return;
	}

	uint16_t seq_zero = (((buf->data[10] << 8 | buf->data[11]) >> 2) & TRANS_SEQ_ZERO_MASK);

	seg = get_seg(frnd, src, seq_zero, seg_count);
	if (!seg) {
		BT_ERR("No free friend segment RX contexts for 0x%04x", src);
		net_buf_unref(buf);
		return;
	}

	net_buf_slist_put(&seg->queue, buf);

	if (type == BT_MESH_FRIEND_PDU_COMPLETE) {
		sys_slist_merge_slist(&frnd->queue, &seg->queue);

		frnd->queue_size += seg->seg_count;
		seg->seg_count = 0U;
	} else {
		/* Mark the buffer as having more to come after it */
		buf->flags |= NET_BUF_FRAGS;
	}
}

static void buf_send_start(uint16_t duration, int err, void *user_data)
{
	struct bt_mesh_friend *frnd = user_data;

	BT_DBG("err %d", err);

	frnd->pending_buf = 0U;

	/* Friend Offer doesn't follow the re-sending semantics */
	if (!frnd->established && frnd->last) {
		net_buf_unref(frnd->last);
		frnd->last = NULL;
	}
}

static void buf_send_end(int err, void *user_data)
{
	struct bt_mesh_friend *frnd = user_data;

	BT_DBG("err %d", err);

	if (frnd->pending_req) {
		BT_WARN("Another request before previous completed sending");
		return;
	}

	if (frnd->established) {
		/* Always restart poll timeout timer after sending */
		k_work_reschedule(&frnd->timer, K_MSEC(frnd->poll_to));
		BT_DBG("Waiting %u ms for next poll", frnd->poll_to);
	} else {
		/* Friend offer timeout is 1 second */
		k_work_reschedule(&frnd->timer, K_SECONDS(1));
		BT_DBG("Waiting for first poll");
	}
}

static void update_overwrite(struct net_buf *buf, uint8_t md)
{
	struct net_buf_simple_state state;
	struct bt_mesh_ctl_friend_update *upd;

	if (buf->len != 16) {
		return;
	}

	net_buf_simple_save(&buf->b, &state);

	net_buf_skip(buf, 1); /* skip IVI, NID */

	if (!(net_buf_pull_u8(buf) >> 7)) {
		goto end;
	}

	net_buf_skip(buf, 7); /* skip seqnum src dec*/

	if (TRANS_CTL_OP((uint8_t *) net_buf_pull_mem(buf, 1))
			!= TRANS_CTL_OP_FRIEND_UPDATE) {
		goto end;
	}

	upd = net_buf_pull_mem(buf, sizeof(*upd));
	BT_DBG("Update Previous Friend Update MD 0x%02x -> 0x%02x", upd->md, md);
	upd->md = md;

end:
	net_buf_simple_restore(&buf->b, &state);
}

static void friend_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct bt_mesh_friend *frnd = CONTAINER_OF(dwork, struct bt_mesh_friend,
						   timer);
	static const struct bt_mesh_send_cb buf_sent_cb = {
		.start = buf_send_start,
		.end = buf_send_end,
	};
	uint8_t md;

	if (!friend_is_allocated(frnd)) {
		return;
	}

	__ASSERT_NO_MSG(frnd->pending_buf == 0U);

	BT_DBG("lpn 0x%04x send_last %u last %p", frnd->lpn,
	       frnd->send_last, frnd->last);

	if (frnd->send_last && frnd->last) {
		BT_DBG("Sending frnd->last %p", frnd->last);
		frnd->send_last = 0U;
		goto send_last;
	}

	if (frnd->established && !frnd->pending_req) {
		BT_WARN("Friendship lost with 0x%04x", frnd->lpn);
		friend_clear(frnd);
		return;
	}

	frnd->last = (void *)sys_slist_get(&frnd->queue);
	if (!frnd->last) {
		BT_WARN("Friendship not established with 0x%04x",
			frnd->lpn);
		friend_clear(frnd);
		return;
	}

	md = (uint8_t)(sys_slist_peek_head(&frnd->queue) != NULL);

	update_overwrite(frnd->last, md);

	if (encrypt_friend_pdu(frnd, frnd->last, false)) {
		return;
	}

	/* Clear the flag we use for segment tracking */
	frnd->last->flags &= ~NET_BUF_FRAGS;
	frnd->last->frags = NULL;

	BT_DBG("Sending buf %p from Friend Queue of LPN 0x%04x",
	       frnd->last, frnd->lpn);
	frnd->queue_size--;

send_last:
	frnd->pending_req = 0U;
	frnd->pending_buf = 1U;
	bt_mesh_adv_send(frnd->last, &buf_sent_cb, frnd);
}

static void subnet_evt(struct bt_mesh_subnet *sub, enum bt_mesh_key_evt evt)
{
	int i, err;

	if (evt == BT_MESH_KEY_ADDED) {
		return;
	}

	for (i = 0; i < ARRAY_SIZE(bt_mesh.frnd); i++) {
		struct bt_mesh_friend *frnd = &bt_mesh.frnd[i];

		if (frnd->subnet != sub) {
			continue;
		}

		switch (evt) {
		case BT_MESH_KEY_DELETED:
			BT_DBG("Cleared network for 0x%04x", frnd->lpn);
			friend_clear(frnd);
			break;
		case BT_MESH_KEY_UPDATED:
			BT_DBG("Generating new keys for 0x%04x", frnd->lpn);
			err = friend_cred_create(frnd, 1);
			if (err) {
				BT_ERR("Failed updating friend cred for 0x%04x",
				       frnd->lpn);
				friend_clear(frnd);
			}
			break;
		case BT_MESH_KEY_SWAPPED:
			enqueue_update(frnd, 0);
			break;
		case BT_MESH_KEY_REVOKED:
			BT_DBG("Revoking old keys for 0x%04x", frnd->lpn);
			memcpy(&frnd->cred[0], &frnd->cred[1],
			       sizeof(frnd->cred[0]));
			memset(&frnd->cred[1], 0, sizeof(frnd->cred[1]));
			enqueue_update(frnd, 0);
			break;
		default:
			break;
		}
	}
}

BT_MESH_SUBNET_CB_DEFINE(subnet_evt);

int bt_mesh_friend_init(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(bt_mesh.frnd); i++) {
		struct bt_mesh_friend *frnd = &bt_mesh.frnd[i];
		int j;

		sys_slist_init(&frnd->queue);

		k_work_init_delayable(&frnd->timer, friend_timeout);
		k_work_init_delayable(&frnd->clear.timer, clear_timeout);

		for (j = 0; j < ARRAY_SIZE(frnd->seg); j++) {
			sys_slist_init(&frnd->seg[j].queue);
		}
	}

	return 0;
}

static bool is_segack(struct net_buf *buf, const uint64_t *seqauth, uint16_t src)
{
	struct net_buf_simple_state state;
	bool found = false;

	if (buf->len != 16) {
		return false;
	}

	net_buf_simple_save(&buf->b, &state);

	net_buf_skip(buf, 1); /* skip IVI, NID */

	if (!(net_buf_pull_u8(buf) >> 7)) {
		goto end;
	}

	net_buf_pull(buf, 3); /* skip SEQNUM */

	if (src != net_buf_pull_be16(buf)) {
		goto end;
	}

	net_buf_skip(buf, 2); /* skip dst */

	if (TRANS_CTL_OP((uint8_t *) net_buf_pull_mem(buf, 1)) != TRANS_CTL_OP_ACK) {
		goto end;
	}

	found = ((net_buf_pull_be16(buf) >> 2) & TRANS_SEQ_ZERO_MASK) ==
		(*seqauth & TRANS_SEQ_ZERO_MASK);
end:
	net_buf_simple_restore(&buf->b, &state);
	return found;
}

static void friend_purge_old_ack(struct bt_mesh_friend *frnd,
				 const uint64_t *seq_auth, uint16_t src)
{
	sys_snode_t *cur, *prev = NULL;

	BT_DBG("SeqAuth %llx src 0x%04x", *seq_auth, src);

	for (cur = sys_slist_peek_head(&frnd->queue);
	     cur != NULL; prev = cur, cur = sys_slist_peek_next(cur)) {
		struct net_buf *buf = (void *)cur;

		if (is_segack(buf, seq_auth, src)) {
			BT_DBG("Removing old ack from Friend Queue");

			sys_slist_remove(&frnd->queue, prev, cur);
			frnd->queue_size--;
			/* Make sure old slist entry state doesn't remain */
			buf->frags = NULL;

			net_buf_unref(buf);
			break;
		}
	}
}

static void friend_lpn_enqueue_rx(struct bt_mesh_friend *frnd,
				  struct bt_mesh_net_rx *rx,
				  enum bt_mesh_friend_pdu_type type,
				  const uint64_t *seq_auth, uint8_t seg_count,
				  struct net_buf_simple *sbuf)
{
	struct friend_pdu_info info;
	struct net_buf *buf;

	/* Because of network loopback, tx packets will also be passed into
	 * this rx function. These packets have already been added to the
	 * queue, and should be ignored.
	 */
	if (bt_mesh_has_addr(rx->ctx.addr)) {
		return;
	}

	BT_DBG("LPN 0x%04x queue_size %u", frnd->lpn, frnd->queue_size);

	if (type == BT_MESH_FRIEND_PDU_SINGLE && seq_auth) {
		friend_purge_old_ack(frnd, seq_auth, rx->ctx.addr);
	}

	info.src = rx->ctx.addr;
	info.dst = rx->ctx.recv_dst;

	if (rx->net_if == BT_MESH_NET_IF_LOCAL) {
		info.ttl = rx->ctx.recv_ttl;
	} else {
		info.ttl = rx->ctx.recv_ttl - 1U;
	}

	info.ctl = rx->ctl;

	sys_put_be24(rx->seq, info.seq);

	info.iv_index = BT_MESH_NET_IVI_RX(rx);

	buf = create_friend_pdu(frnd, &info, sbuf);
	if (!buf) {
		BT_ERR("Failed to encode Friend buffer");
		return;
	}

	enqueue_friend_pdu(frnd, type, info.src, seg_count, buf);

	BT_DBG("Queued message for LPN 0x%04x, queue_size %u",
	       frnd->lpn, frnd->queue_size);
}

static void friend_lpn_enqueue_tx(struct bt_mesh_friend *frnd,
				  struct bt_mesh_net_tx *tx,
				  enum bt_mesh_friend_pdu_type type,
				  const uint64_t *seq_auth, uint8_t seg_count,
				  struct net_buf_simple *sbuf)
{
	struct friend_pdu_info info;
	struct net_buf *buf;

	BT_DBG("LPN 0x%04x", frnd->lpn);

	if (type == BT_MESH_FRIEND_PDU_SINGLE && seq_auth) {
		friend_purge_old_ack(frnd, seq_auth, tx->src);
	}

	info.src = tx->src;
	info.dst = tx->ctx->addr;

	info.ttl = tx->ctx->send_ttl;
	info.ctl = (tx->ctx->app_idx == BT_MESH_KEY_UNUSED);

	sys_put_be24(bt_mesh.seq, info.seq);

	info.iv_index = BT_MESH_NET_IVI_TX;

	buf = create_friend_pdu(frnd, &info, sbuf);
	if (!buf) {
		BT_ERR("Failed to encode Friend buffer");
		return;
	}

	if (type == BT_MESH_FRIEND_PDU_SINGLE && !info.ctl) {
		/* Unsegmented application packets may be reencrypted later,
		 * as they depend on the the sequence number being the same
		 * when encrypting in transport and network.
		 */
		FRIEND_ADV(buf)->app_idx = tx->ctx->app_idx;
	}

	enqueue_friend_pdu(frnd, type, info.src, seg_count, buf);

	BT_DBG("Queued message for LPN 0x%04x", frnd->lpn);
}

static bool friend_lpn_matches(struct bt_mesh_friend *frnd, uint16_t net_idx,
			       uint16_t addr)
{
	int i;

	if (!frnd->established) {
		return false;
	}

	if (net_idx != frnd->subnet->net_idx) {
		return false;
	}

	if (BT_MESH_ADDR_IS_UNICAST(addr)) {
		return is_lpn_unicast(frnd, addr);
	}

	for (i = 0; i < ARRAY_SIZE(frnd->sub_list); i++) {
		if (frnd->sub_list[i] == addr) {
			return true;
		}
	}

	return false;
}

bool bt_mesh_friend_match(uint16_t net_idx, uint16_t addr)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(bt_mesh.frnd); i++) {
		struct bt_mesh_friend *frnd = &bt_mesh.frnd[i];

		if (friend_lpn_matches(frnd, net_idx, addr)) {
			BT_DBG("LPN 0x%04x matched address 0x%04x",
			       frnd->lpn, addr);
			return true;
		}
	}

	BT_DBG("No matching LPN for address 0x%04x", addr);

	return false;
}

static bool friend_queue_has_space(struct bt_mesh_friend *frnd, uint16_t addr,
				   const uint64_t *seq_auth, uint8_t seg_count)
{
	uint32_t total = 0;
	int i;

	if (seg_count > CONFIG_BT_MESH_FRIEND_QUEUE_SIZE) {
		return false;
	}

	for (i = 0; i < ARRAY_SIZE(frnd->seg); i++) {
		struct bt_mesh_friend_seg *seg = &frnd->seg[i];

		if (seq_auth && is_seg(seg, addr, *seq_auth & TRANS_SEQ_ZERO_MASK)) {
			/* If there's a segment queue for this message then the
			 * space verification has already happened.
			 */
			return true;
		}

		total += seg->seg_count;
	}

	/* If currently pending segments combined with this segmented message
	 * are more than the Friend Queue Size, then there's no space. This
	 * is because we don't have a mechanism of aborting already pending
	 * segmented messages to free up buffers.
	 */
	return (CONFIG_BT_MESH_FRIEND_QUEUE_SIZE - total) > seg_count;
}

bool bt_mesh_friend_queue_has_space(uint16_t net_idx, uint16_t src, uint16_t dst,
				    uint64_t *seq_auth, uint8_t seg_count)
{
	bool someone_has_space = false, friend_match = false;
	int i;

	for (i = 0; i < ARRAY_SIZE(bt_mesh.frnd); i++) {
		struct bt_mesh_friend *frnd = &bt_mesh.frnd[i];

		if (!friend_lpn_matches(frnd, net_idx, dst)) {
			continue;
		}

		friend_match = true;

		if (friend_queue_has_space(frnd, src, seq_auth, seg_count)) {
			someone_has_space = true;
		}
	}

	/* If there were no matched LPNs treat this as success, so the
	 * transport layer can continue its work.
	 */
	if (!friend_match) {
		return true;
	}

	/* From the transport layers perspective it's good enough that at
	 * least one Friend Queue has space. If there were multiple Friend
	 * matches then the destination must be a group address, in which
	 * case e.g. segment acks are not sent.
	 */
	return someone_has_space;
}

static bool friend_queue_prepare_space(struct bt_mesh_friend *frnd, uint16_t addr,
				       const uint64_t *seq_auth, uint8_t seg_count)
{
	bool pending_segments;
	uint8_t avail_space;

	if (!friend_queue_has_space(frnd, addr, seq_auth, seg_count)) {
		return false;
	}

	avail_space = CONFIG_BT_MESH_FRIEND_QUEUE_SIZE - frnd->queue_size;
	pending_segments = false;

	while (pending_segments || avail_space < seg_count) {
		struct net_buf *buf = (void *)sys_slist_get(&frnd->queue);

		if (!buf) {
			BT_ERR("Unable to free up enough buffers");
			return false;
		}

		frnd->queue_size--;
		avail_space++;

		pending_segments = (buf->flags & NET_BUF_FRAGS);

		/* Make sure old slist entry state doesn't remain */
		buf->frags = NULL;
		buf->flags &= ~NET_BUF_FRAGS;

		net_buf_unref(buf);
	}

	return true;
}

void bt_mesh_friend_enqueue_rx(struct bt_mesh_net_rx *rx,
			       enum bt_mesh_friend_pdu_type type,
			       const uint64_t *seq_auth, uint8_t seg_count,
			       struct net_buf_simple *sbuf)
{
	int i;

	if (!rx->friend_match ||
	    (rx->ctx.recv_ttl <= 1U && rx->net_if != BT_MESH_NET_IF_LOCAL) ||
	    bt_mesh_friend_get() != BT_MESH_FRIEND_ENABLED) {
		return;
	}

	BT_DBG("recv_ttl %u net_idx 0x%04x src 0x%04x dst 0x%04x",
	       rx->ctx.recv_ttl, rx->sub->net_idx, rx->ctx.addr,
	       rx->ctx.recv_dst);

	for (i = 0; i < ARRAY_SIZE(bt_mesh.frnd); i++) {
		struct bt_mesh_friend *frnd = &bt_mesh.frnd[i];

		if (!friend_lpn_matches(frnd, rx->sub->net_idx,
					rx->ctx.recv_dst)) {
			continue;
		}

		if (friend_lpn_matches(frnd, rx->sub->net_idx,
					rx->ctx.addr)) {
			continue;
		}

		if (!friend_queue_prepare_space(frnd, rx->ctx.addr, seq_auth,
						seg_count)) {
			continue;
		}

		friend_lpn_enqueue_rx(frnd, rx, type, seq_auth, seg_count,
				      sbuf);
	}
}

bool bt_mesh_friend_enqueue_tx(struct bt_mesh_net_tx *tx,
			       enum bt_mesh_friend_pdu_type type,
			       const uint64_t *seq_auth, uint8_t seg_count,
			       struct net_buf_simple *sbuf)
{
	bool matched = false;
	int i;

	if (!bt_mesh_friend_match(tx->sub->net_idx, tx->ctx->addr) ||
	    bt_mesh_friend_get() != BT_MESH_FRIEND_ENABLED) {
		return matched;
	}

	BT_DBG("net_idx 0x%04x dst 0x%04x src 0x%04x", tx->sub->net_idx,
	       tx->ctx->addr, tx->src);

	for (i = 0; i < ARRAY_SIZE(bt_mesh.frnd); i++) {
		struct bt_mesh_friend *frnd = &bt_mesh.frnd[i];

		if (!friend_lpn_matches(frnd, tx->sub->net_idx,
					tx->ctx->addr)) {
			continue;
		}

		if (!friend_queue_prepare_space(frnd, tx->src, seq_auth,
						seg_count)) {
			continue;
		}

		friend_lpn_enqueue_tx(frnd, tx, type, seq_auth, seg_count,
				      sbuf);
		matched = true;
	}

	return matched;
}

int bt_mesh_friend_terminate(uint16_t lpn_addr)
{
	struct bt_mesh_friend *frnd;

	frnd = bt_mesh_friend_find(BT_MESH_KEY_ANY, lpn_addr, false, false);
	if (!frnd) {
		return -ENOENT;
	}

	friend_clear(frnd);

	return 0;
}

void bt_mesh_friend_clear_incomplete(struct bt_mesh_subnet *sub, uint16_t src,
				     uint16_t dst, uint64_t *seq_auth)
{
	int i;

	BT_DBG("");

	for (i = 0; i < ARRAY_SIZE(bt_mesh.frnd); i++) {
		struct bt_mesh_friend *frnd = &bt_mesh.frnd[i];
		int j;

		if (!friend_lpn_matches(frnd, sub->net_idx, dst)) {
			continue;
		}

		for (j = 0; j < ARRAY_SIZE(frnd->seg); j++) {
			struct bt_mesh_friend_seg *seg = &frnd->seg[j];

			if (!is_seg(seg, src, *seq_auth & TRANS_SEQ_ZERO_MASK)) {
				continue;
			}

			BT_WARN("Clearing incomplete segments for 0x%04x", src);

			purge_buffers(&seg->queue);
			seg->seg_count = 0U;
			break;
		}
	}
}
