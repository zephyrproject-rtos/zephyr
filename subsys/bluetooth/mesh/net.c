/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/net_buf.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/mesh.h>

#include "common/bt_str.h"

#include "crypto.h"
#include "mesh.h"
#include "net.h"
#include "rpl.h"
#include "lpn.h"
#include "friend.h"
#include "proxy.h"
#include "proxy_cli.h"
#include "transport.h"
#include "access.h"
#include "foundation.h"
#include "beacon.h"
#include "settings.h"
#include "prov.h"
#include "cfg.h"
#include "statistic.h"
#include "sar_cfg_internal.h"
#include "brg_cfg.h"

#define LOG_LEVEL CONFIG_BT_MESH_NET_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_net);

#define LOOPBACK_MAX_PDU_LEN (BT_MESH_NET_HDR_LEN + 16)

/* Seq limit after IV Update is triggered */
#define IV_UPDATE_SEQ_LIMIT CONFIG_BT_MESH_IV_UPDATE_SEQ_LIMIT

#define IVI(pdu)           ((pdu)[0] >> 7)
#define NID(pdu)           ((pdu)[0] & 0x7f)
#define CTL(pdu)           ((pdu)[1] >> 7)
#define TTL(pdu)           ((pdu)[1] & 0x7f)
#define SEQ(pdu)           (sys_get_be24(&pdu[2]))
#define SRC(pdu)           (sys_get_be16(&(pdu)[5]))
#define DST(pdu)           (sys_get_be16(&(pdu)[7]))

/* Information needed for bridging the network PDUs */
struct pdu_ctx {
	struct net_buf_simple *sbuf;
	struct net_buf_simple_state *state;
	struct bt_mesh_net_rx *rx;
};

/* Mesh network information for persistent storage. */
struct net_val {
	uint16_t primary_addr;
	struct bt_mesh_key dev_key;
} __packed;

/* Sequence number information for persistent storage. */
struct seq_val {
	uint8_t val[3];
} __packed;

/* IV Index & IV Update information for persistent storage. */
struct iv_val {
	uint32_t iv_index;
	uint8_t  iv_update:1,
	      iv_duration:7;
} __packed;

static struct {
	uint32_t src : 15, /* MSb of source is always 0 */
	      seq : 17;
} msg_cache[CONFIG_BT_MESH_MSG_CACHE_SIZE];
static uint16_t msg_cache_next;

/* Singleton network context (the implementation only supports one) */
struct bt_mesh_net bt_mesh = {
	.local_queue = SYS_SLIST_STATIC_INIT(&bt_mesh.local_queue),
	.sar_tx = BT_MESH_SAR_TX_INIT,
	.sar_rx = BT_MESH_SAR_RX_INIT,

#if defined(CONFIG_BT_MESH_PRIV_BEACONS)
	.priv_beacon_int = 0x3c,
#endif
};

/* MshPRTv1.1: 3.11.5:
 * "A node shall not start an IV Update procedure more often than once every 192 hours."
 *
 * Mark that the IV Index Recovery has been done to prevent two recoveries to be
 * done before a normal IV Index update has been completed within 96h+96h.
 */
static bool ivi_was_recovered;

struct loopback_buf {
	sys_snode_t node;
	struct bt_mesh_subnet *sub;
	uint8_t len;
	uint8_t data[LOOPBACK_MAX_PDU_LEN];
};

K_MEM_SLAB_DEFINE(loopback_buf_pool,
		  sizeof(struct loopback_buf),
		  CONFIG_BT_MESH_LOOPBACK_BUFS, __alignof__(struct loopback_buf));

static uint32_t dup_cache[CONFIG_BT_MESH_MSG_CACHE_SIZE];
static int   dup_cache_next;

static bool check_dup(struct net_buf_simple *data)
{
	const uint8_t *tail = net_buf_simple_tail(data);
	uint32_t val;
	int i;

	val = sys_get_be32(tail - 4) ^ sys_get_be32(tail - 8);

	for (i = dup_cache_next; i > 0;) {
		if (dup_cache[--i] == val) {
			return true;
		}
	}

	for (i = ARRAY_SIZE(dup_cache); i > dup_cache_next;) {
		if (dup_cache[--i] == val) {
			return true;
		}
	}

	dup_cache_next %= ARRAY_SIZE(dup_cache);
	dup_cache[dup_cache_next++] = val;

	return false;
}

static bool msg_cache_match(struct net_buf_simple *pdu)
{
	uint16_t i;

	for (i = msg_cache_next; i > 0U;) {
		if (msg_cache[--i].src == SRC(pdu->data) &&
		    msg_cache[i].seq == (SEQ(pdu->data) & BIT_MASK(17))) {
			return true;
		}
	}

	for (i = ARRAY_SIZE(msg_cache); i > msg_cache_next;) {
		if (msg_cache[--i].src == SRC(pdu->data) &&
		    msg_cache[i].seq == (SEQ(pdu->data) & BIT_MASK(17))) {
			return true;
		}
	}

	return false;
}

static void msg_cache_add(struct bt_mesh_net_rx *rx)
{
	msg_cache_next %= ARRAY_SIZE(msg_cache);
	msg_cache[msg_cache_next].src = rx->ctx.addr;
	msg_cache[msg_cache_next].seq = rx->seq;
	msg_cache_next++;
}

static void store_iv(bool only_duration)
{
	bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_IV_PENDING);

	if (!only_duration) {
		/* Always update Seq whenever IV changes */
		bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_SEQ_PENDING);
	}
}

void bt_mesh_net_seq_store(bool force)
{
	if (!force &&
	    CONFIG_BT_MESH_SEQ_STORE_RATE > 1 &&
	    (bt_mesh.seq % CONFIG_BT_MESH_SEQ_STORE_RATE)) {
		return;
	}

	bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_SEQ_PENDING);
}

int bt_mesh_net_create(uint16_t idx, uint8_t flags, const struct bt_mesh_key *key,
		       uint32_t iv_index)
{
	int err;

	LOG_DBG("idx %u flags 0x%02x iv_index %u", idx, flags, iv_index);

	LOG_DBG("NetKey %s", bt_hex(key, sizeof(struct bt_mesh_key)));

	if (BT_MESH_KEY_REFRESH(flags)) {
		err = bt_mesh_subnet_set(idx, BT_MESH_KR_PHASE_2, NULL, key);
	} else {
		err = bt_mesh_subnet_set(idx, BT_MESH_KR_NORMAL, key, NULL);
	}

	if (err) {
		LOG_ERR("Failed creating subnet");
		return err;
	}

	(void)memset(msg_cache, 0, sizeof(msg_cache));
	msg_cache_next = 0U;

	bt_mesh.iv_index = iv_index;
	atomic_set_bit_to(bt_mesh.flags, BT_MESH_IVU_IN_PROGRESS,
			  BT_MESH_IV_UPDATE(flags));

	/* If the node is added to a network when the network is in Normal
	 * operation, then it shall operate in Normal operation for at least
	 * 96 hours. If a node is added to a network while the network is
	 * in the IV Update in Progress state, then the node shall be given
	 * the new IV Index value and operate in IV Update in Progress
	 * operation without the restriction of being in this state for at
	 * least 96 hours.
	 */
	if (BT_MESH_IV_UPDATE(flags)) {
		bt_mesh.ivu_duration = BT_MESH_IVU_MIN_HOURS;
	} else {
		bt_mesh.ivu_duration = 0U;
	}

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		LOG_DBG("Storing network information persistently");
		bt_mesh_subnet_store(idx);
		store_iv(false);
	}

	return 0;
}

#if defined(CONFIG_BT_MESH_IV_UPDATE_TEST)
void bt_mesh_iv_update_test(bool enable)
{
	atomic_set_bit_to(bt_mesh.flags, BT_MESH_IVU_TEST, enable);
	/* Reset the duration variable - needed for some PTS tests */
	bt_mesh.ivu_duration = 0U;
}

bool bt_mesh_iv_update(void)
{
	if (!bt_mesh_is_provisioned()) {
		LOG_ERR("Not yet provisioned");
		return false;
	}

	if (atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_IN_PROGRESS)) {
		bt_mesh_net_iv_update(bt_mesh.iv_index, false);
	} else {
		bt_mesh_net_iv_update(bt_mesh.iv_index + 1, true);
	}

	return atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_IN_PROGRESS);
}
#endif /* CONFIG_BT_MESH_IV_UPDATE_TEST */

bool bt_mesh_net_iv_update(uint32_t iv_index, bool iv_update)
{
	bool iv_update_is_same;

	/* Check if IV index should to be recovered. */
	if (iv_index < bt_mesh.iv_index ||
	    iv_index > bt_mesh.iv_index + 42) {
		LOG_ERR("IV Index out of sync: 0x%08x != 0x%08x", iv_index, bt_mesh.iv_index);
		return false;
	}

	/* Discard [iv, false] --> [iv, true] */
	if (iv_index == bt_mesh.iv_index && iv_update) {
		LOG_DBG("Ignore previous IV update procedure");
		return false;
	}

	iv_update_is_same = (atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_IN_PROGRESS) == iv_update);

	/* MshPRTv1.1 allows to initiate an
	 * IV Index Recovery procedure if previous IV update has
	 * been missed. This allows the node to remain
	 * functional.
	 *
	 * Upon receiving and successfully authenticating a
	 * Secure Network beacon for a primary subnet whose
	 * IV Index is 1 or more higher than the current known IV
	 * Index, the node shall set its current IV Index and its
	 * current IV Update procedure state from the values in
	 * this Secure Network beacon.
	 */
	if (((iv_index - bt_mesh.iv_index) + iv_update_is_same) > 1) {
		if (ivi_was_recovered &&
		    (bt_mesh.ivu_duration < (2 * BT_MESH_IVU_MIN_HOURS))) {
			LOG_ERR("IV Index Recovery before minimum delay");
			return false;
		}

		LOG_WRN("Performing IV Index Recovery");
		ivi_was_recovered = true;
		bt_mesh_rpl_clear();
		bt_mesh.iv_index = iv_index;
		bt_mesh.seq = 0U;

		goto do_update;
	}

	if (iv_update_is_same) {
		LOG_DBG("No change for IV Update procedure");
		return false;
	}

	if (!(IS_ENABLED(CONFIG_BT_MESH_IV_UPDATE_TEST) &&
	      atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_TEST))) {
		if (bt_mesh.ivu_duration < BT_MESH_IVU_MIN_HOURS) {
			LOG_WRN("IV Update before minimum duration");
			return false;
		}
	}

	/* Defer change to Normal Operation if there are pending acks */
	if (!iv_update && bt_mesh_tx_in_progress()) {
		LOG_WRN("IV Update deferred because of pending transfer");
		atomic_set_bit(bt_mesh.flags, BT_MESH_IVU_PENDING);
		return false;
	}

	if (iv_update) {
		bt_mesh.iv_index = iv_index;
		LOG_DBG("IV Update state entered. New index 0x%08x", bt_mesh.iv_index);

		bt_mesh_rpl_reset();
		ivi_was_recovered = false;
	} else {
		LOG_DBG("Normal mode entered");
		bt_mesh.seq = 0U;
	}

do_update:
	atomic_set_bit_to(bt_mesh.flags, BT_MESH_IVU_IN_PROGRESS, iv_update);
	bt_mesh.ivu_duration = 0U;

	k_work_reschedule(&bt_mesh.ivu_timer, BT_MESH_IVU_TIMEOUT);

	/* Notify other modules */
	if (IS_ENABLED(CONFIG_BT_MESH_FRIEND)) {
		bt_mesh_friend_sec_update(BT_MESH_KEY_ANY);
	}

	bt_mesh_subnet_foreach(bt_mesh_beacon_update);

	if (IS_ENABLED(CONFIG_BT_MESH_GATT_PROXY) &&
		(bt_mesh_gatt_proxy_get() == BT_MESH_GATT_PROXY_ENABLED ||
		 bt_mesh_priv_gatt_proxy_get() == BT_MESH_PRIV_GATT_PROXY_ENABLED)) {
		bt_mesh_proxy_beacon_send(NULL);
	}

	if (IS_ENABLED(CONFIG_BT_MESH_CDB)) {
		bt_mesh_cdb_iv_update(iv_index, iv_update);
	}

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		store_iv(false);
	}

	return true;
}

uint32_t bt_mesh_next_seq(void)
{
	uint32_t seq = bt_mesh.seq++;

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		bt_mesh_net_seq_store(false);
	}

	if (!atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_IN_PROGRESS) &&
	    bt_mesh.seq > IV_UPDATE_SEQ_LIMIT &&
	    bt_mesh_subnet_get(BT_MESH_KEY_PRIMARY)) {
		bt_mesh_beacon_ivu_initiator(true);
		bt_mesh_net_iv_update(bt_mesh.iv_index + 1, true);
	}

	return seq;
}

static void bt_mesh_net_local(struct k_work *work)
{
	struct net_buf_simple sbuf;
	sys_snode_t *node;

	while ((node = sys_slist_get(&bt_mesh.local_queue))) {
		struct loopback_buf *buf = CONTAINER_OF(node, struct loopback_buf, node);
		struct bt_mesh_net_rx rx = {
			.ctx = {
				.net_idx = buf->sub->net_idx,
				/* Initialize AppIdx to a sane value */
				.app_idx = BT_MESH_KEY_UNUSED,
				.recv_ttl = TTL(buf->data),
				/* TTL=1 only goes to local IF */
				.send_ttl = 1U,
				.addr = SRC(buf->data),
				.recv_dst = DST(buf->data),
				.recv_rssi = 0,
			},
			.net_if = BT_MESH_NET_IF_LOCAL,
			.sub = buf->sub,
			.old_iv = (IVI(buf->data) != (bt_mesh.iv_index & 0x01)),
			.ctl = CTL(buf->data),
			.seq = SEQ(buf->data),
			.new_key = SUBNET_KEY_TX_IDX(buf->sub),
			.local_match = 1U,
			.friend_match = 0U,
		};

		LOG_DBG("src: 0x%04x dst: 0x%04x seq 0x%06x sub %p", rx.ctx.addr, rx.ctx.addr,
			rx.seq, buf->sub);

		net_buf_simple_init_with_data(&sbuf, buf->data, buf->len);
		(void)bt_mesh_trans_recv(&sbuf, &rx);
		k_mem_slab_free(&loopback_buf_pool, (void *)buf);
	}
}

static const struct bt_mesh_net_cred *net_tx_cred_get(struct bt_mesh_net_tx *tx)
{
#if defined(CONFIG_BT_MESH_LOW_POWER)
	if (tx->friend_cred && bt_mesh.lpn.frnd) {
		return &bt_mesh.lpn.cred[SUBNET_KEY_TX_IDX(tx->sub)];
	}
#endif

	tx->friend_cred = 0U;
	return &tx->sub->keys[SUBNET_KEY_TX_IDX(tx->sub)].msg;
}

static int net_header_encode(struct bt_mesh_net_tx *tx, uint8_t nid,
			     struct net_buf_simple *buf)
{
	const bool ctl = (tx->ctx->app_idx == BT_MESH_KEY_UNUSED);

	if (ctl && net_buf_simple_tailroom(buf) < 8) {
		LOG_ERR("Insufficient MIC space for CTL PDU");
		return -EINVAL;
	} else if (net_buf_simple_tailroom(buf) < 4) {
		LOG_ERR("Insufficient MIC space for PDU");
		return -EINVAL;
	}

	LOG_DBG("src 0x%04x dst 0x%04x ctl %u seq 0x%06x", tx->src, tx->ctx->addr, ctl,
		bt_mesh.seq);

	net_buf_simple_push_be16(buf, tx->ctx->addr);
	net_buf_simple_push_be16(buf, tx->src);
	net_buf_simple_push_be24(buf, bt_mesh_next_seq());

	if (ctl) {
		net_buf_simple_push_u8(buf, tx->ctx->send_ttl | 0x80);
	} else {
		net_buf_simple_push_u8(buf, tx->ctx->send_ttl);
	}

	net_buf_simple_push_u8(buf, (nid | (BT_MESH_NET_IVI_TX & 1) << 7));

	return 0;
}

static int net_encrypt(struct net_buf_simple *buf,
		       const struct bt_mesh_net_cred *cred, uint32_t iv_index,
		       enum bt_mesh_nonce_type proxy)
{
	int err;

	err = bt_mesh_net_encrypt(&cred->enc, buf, iv_index, proxy);
	if (err) {
		return err;
	}

	return bt_mesh_net_obfuscate(buf->data, iv_index, &cred->privacy);
}

int bt_mesh_net_encode(struct bt_mesh_net_tx *tx, struct net_buf_simple *buf,
		       enum bt_mesh_nonce_type type)
{
	const struct bt_mesh_net_cred *cred;
	int err;

	cred = net_tx_cred_get(tx);
	err = net_header_encode(tx, cred->nid, buf);
	if (err) {
		return err;
	}

	return net_encrypt(buf, cred, BT_MESH_NET_IVI_TX, type);
}

static int net_loopback(const struct bt_mesh_net_tx *tx, const uint8_t *data,
		    size_t len)
{
	int err;
	struct loopback_buf *buf;

	err = k_mem_slab_alloc(&loopback_buf_pool, (void **)&buf, K_NO_WAIT);
	if (err) {
		LOG_WRN("Unable to allocate loopback");
		return -ENOMEM;
	}

	buf->sub = tx->sub;

	(void)memcpy(buf->data, data, len);
	buf->len = len;

	sys_slist_append(&bt_mesh.local_queue, &buf->node);

	k_work_submit(&bt_mesh.local_work);

	return 0;
}

int bt_mesh_net_send(struct bt_mesh_net_tx *tx, struct bt_mesh_adv *adv,
		     const struct bt_mesh_send_cb *cb, void *cb_data)
{
	const struct bt_mesh_net_cred *cred;
	int err;

	LOG_DBG("src 0x%04x dst 0x%04x len %u headroom %zu tailroom %zu", tx->src, tx->ctx->addr,
		adv->b.len, net_buf_simple_headroom(&adv->b), net_buf_simple_tailroom(&adv->b));
	LOG_DBG("Payload len %u: %s", adv->b.len, bt_hex(adv->b.data, adv->b.len));
	LOG_DBG("Seq 0x%06x", bt_mesh.seq);

	cred = net_tx_cred_get(tx);
	err = net_header_encode(tx, cred->nid, &adv->b);
	if (err) {
		goto done;
	}

	/* Deliver to local network interface if necessary */
	if (bt_mesh_fixed_group_match(tx->ctx->addr) ||
	    bt_mesh_has_addr(tx->ctx->addr)) {
		err = net_loopback(tx, adv->b.data, adv->b.len);

		/* Local unicast messages should not go out to network */
		if (BT_MESH_ADDR_IS_UNICAST(tx->ctx->addr) ||
		    tx->ctx->send_ttl == 1U) {
			if (!err) {
				send_cb_finalize(cb, cb_data);
			}

			goto done;
		}
	}

	/* MshPRTv1.1: 3.4.5.2: "The output filter of the interface connected to
	 * advertising or GATT bearers shall drop all messages with TTL value
	 * set to 1." If a TTL=1 packet wasn't for a local interface, it is
	 * invalid.
	 */
	if (tx->ctx->send_ttl == 1U) {
		err = -EINVAL;
		goto done;
	}

	err = net_encrypt(&adv->b, cred, BT_MESH_NET_IVI_TX, BT_MESH_NONCE_NETWORK);
	if (err) {
		goto done;
	}

	adv->ctx.cb = cb;
	adv->ctx.cb_data = cb_data;

	/* Deliver to GATT Proxy Clients if necessary. */
	if (IS_ENABLED(CONFIG_BT_MESH_GATT_PROXY)) {
		(void)bt_mesh_proxy_relay(adv, tx->ctx->addr);
	}

	/* Deliver to GATT Proxy Servers if necessary. */
	if (IS_ENABLED(CONFIG_BT_MESH_PROXY_CLIENT)) {
		(void)bt_mesh_proxy_cli_relay(adv);
	}

	bt_mesh_adv_send(adv, cb, cb_data);

done:
	bt_mesh_adv_unref(adv);
	return err;
}

void bt_mesh_net_loopback_clear(uint16_t net_idx)
{
	sys_slist_t new_list;
	sys_snode_t *node;

	LOG_DBG("0x%04x", net_idx);

	sys_slist_init(&new_list);

	while ((node = sys_slist_get(&bt_mesh.local_queue))) {
		struct loopback_buf *buf = CONTAINER_OF(node, struct loopback_buf, node);

		if (net_idx == BT_MESH_KEY_ANY || net_idx == buf->sub->net_idx) {
			LOG_DBG("Dropped 0x%06x", SEQ(buf->data));
			k_mem_slab_free(&loopback_buf_pool, (void *)buf);
		} else {
			sys_slist_append(&new_list, &buf->node);
		}
	}

	bt_mesh.local_queue = new_list;
}

static bool net_decrypt(struct bt_mesh_net_rx *rx, struct net_buf_simple *in,
			struct net_buf_simple *out,
			const struct bt_mesh_net_cred *cred)
{
	bool proxy = (rx->net_if == BT_MESH_NET_IF_PROXY_CFG);

	if (NID(in->data) != cred->nid) {
		return false;
	}

	LOG_DBG("NID 0x%02x", NID(in->data));
	LOG_DBG("IVI %u net->iv_index 0x%08x", IVI(in->data), bt_mesh.iv_index);

	rx->old_iv = (IVI(in->data) != (bt_mesh.iv_index & 0x01));

	net_buf_simple_reset(out);
	net_buf_simple_add_mem(out, in->data, in->len);

	if (bt_mesh_net_obfuscate(out->data, BT_MESH_NET_IVI_RX(rx),
				  &cred->privacy)) {
		return false;
	}

	rx->ctx.addr = SRC(out->data);
	if (!BT_MESH_ADDR_IS_UNICAST(rx->ctx.addr)) {
		LOG_DBG("Ignoring non-unicast src addr 0x%04x", rx->ctx.addr);
		return false;
	}

	if (bt_mesh_has_addr(rx->ctx.addr)) {
		LOG_DBG("Dropping locally originated packet");
		return false;
	}

	if (rx->net_if == BT_MESH_NET_IF_ADV && msg_cache_match(out)) {
		LOG_DBG("Duplicate found in Network Message Cache");
		return false;
	}

	LOG_DBG("src 0x%04x", rx->ctx.addr);

	return bt_mesh_net_decrypt(&cred->enc, out, BT_MESH_NET_IVI_RX(rx),
				   proxy) == 0;
}

/* Relaying from advertising to the advertising bearer should only happen
 * if the Relay state is set to enabled. Locally originated packets always
 * get sent to the advertising bearer. If the packet came in through GATT,
 * then we should only relay it if the GATT Proxy state is enabled.
 */
static bool relay_to_adv(enum bt_mesh_net_if net_if)
{
	switch (net_if) {
	case BT_MESH_NET_IF_ADV:
		return (bt_mesh_relay_get() == BT_MESH_RELAY_ENABLED);
	case BT_MESH_NET_IF_PROXY:
		return (bt_mesh_gatt_proxy_get() == BT_MESH_GATT_PROXY_ENABLED) ||
			(bt_mesh_priv_gatt_proxy_get() == BT_MESH_PRIV_GATT_PROXY_ENABLED);
	default:
		return false;
	}
}

static void bt_mesh_net_relay(struct net_buf_simple *sbuf, struct bt_mesh_net_rx *rx, bool bridge)
{
	const struct bt_mesh_net_cred *cred;
	struct bt_mesh_adv *adv;
	uint8_t transmit;

	if (rx->ctx.recv_ttl <= 1U) {
		return;
	}

	if (rx->net_if == BT_MESH_NET_IF_ADV && !rx->friend_cred && !bridge &&
	    bt_mesh_relay_get() != BT_MESH_RELAY_ENABLED &&
	    bt_mesh_gatt_proxy_get() != BT_MESH_GATT_PROXY_ENABLED &&
	    bt_mesh_priv_gatt_proxy_get() != BT_MESH_PRIV_GATT_PROXY_ENABLED) {
		return;
	}

	LOG_DBG("TTL %u CTL %u dst 0x%04x", rx->ctx.recv_ttl, rx->ctl, rx->ctx.recv_dst);

	/* The Relay Retransmit state is only applied to adv-adv relaying.
	 * Anything else (like GATT to adv, or locally originated packets)
	 * use the Network Transmit state.
	 */
	if (rx->net_if == BT_MESH_NET_IF_ADV && !rx->friend_cred) {
		transmit = bt_mesh_relay_retransmit_get();
	} else {
		transmit = bt_mesh_net_transmit_get();
	}

	adv = bt_mesh_adv_create(BT_MESH_ADV_DATA, BT_MESH_ADV_TAG_RELAY,
				 transmit, K_NO_WAIT);
	if (!adv) {
		LOG_DBG("Out of relay advs");
		return;
	}

	/* Leave CTL bit intact */
	sbuf->data[1] &= 0x80;
	sbuf->data[1] |= rx->ctx.recv_ttl - 1U;

	net_buf_simple_add_mem(&adv->b, sbuf->data, sbuf->len);

	cred = &rx->sub->keys[SUBNET_KEY_TX_IDX(rx->sub)].msg;

	LOG_DBG("Relaying packet. TTL is now %u", TTL(adv->b.data));

	/* Update NID if RX, RX was with friend credentials or when bridging the message */
	if (rx->friend_cred || bridge) {
		adv->b.data[0] &= 0x80; /* Clear everything except IVI */
		adv->b.data[0] |= cred->nid;
	}

	/* We re-encrypt and obfuscate using the received IVI rather than
	 * the normal TX IVI (which may be different) since the transport
	 * layer nonce includes the IVI.
	 */
	if (net_encrypt(&adv->b, cred, BT_MESH_NET_IVI_RX(rx), BT_MESH_NONCE_NETWORK)) {
		LOG_ERR("Re-encrypting failed");
		goto done;
	}

	/* When the Friend node relays message for lpn, the message will be
	 * retransmitted using the managed flooding security credentials and
	 * the Network PDU shall be retransmitted to all network interfaces.
	 */
	if (IS_ENABLED(CONFIG_BT_MESH_GATT_PROXY) &&
	    (rx->friend_cred ||
	     bt_mesh_gatt_proxy_get() == BT_MESH_GATT_PROXY_ENABLED ||
	     bt_mesh_priv_gatt_proxy_get() == BT_MESH_PRIV_GATT_PROXY_ENABLED)) {
		bt_mesh_proxy_relay(adv, rx->ctx.recv_dst);
	}

	if (relay_to_adv(rx->net_if) || rx->friend_cred || bridge) {
		bt_mesh_adv_send(adv, NULL, NULL);
	}

done:
	bt_mesh_adv_unref(adv);
}

#if IS_ENABLED(CONFIG_BT_MESH_BRG_CFG_SRV)
static bool find_subnet_cb(struct bt_mesh_subnet *sub, void *cb_data)
{
	uint16_t *net_idx = cb_data;

	return sub->net_idx == *net_idx;
}

static void bt_mesh_sbr_check_cb(uint16_t new_net_idx, void *user_data)
{
	struct pdu_ctx *ctx = (struct pdu_ctx *)user_data;

	if (new_net_idx < BT_MESH_BRG_CFG_NETIDX_NOMATCH) {
		struct bt_mesh_subnet *subnet = bt_mesh_subnet_find(find_subnet_cb, &new_net_idx);

		if (!subnet) {
			LOG_ERR("Failed to find subnet 0x%04x", new_net_idx);
			return;
		}

		ctx->rx->sub = subnet;
		ctx->rx->ctx.net_idx = new_net_idx;

		net_buf_simple_restore(ctx->sbuf, ctx->state);
		bt_mesh_net_relay(ctx->sbuf, ctx->rx, true);
	}
}
#endif

void bt_mesh_net_header_parse(struct net_buf_simple *buf,
			      struct bt_mesh_net_rx *rx)
{
	rx->old_iv = (IVI(buf->data) != (bt_mesh.iv_index & 0x01));
	rx->ctl = CTL(buf->data);
	rx->ctx.recv_ttl = TTL(buf->data);
	rx->seq = SEQ(buf->data);
	rx->ctx.addr = SRC(buf->data);
	rx->ctx.recv_dst = DST(buf->data);
}

int bt_mesh_net_decode(struct net_buf_simple *in, enum bt_mesh_net_if net_if,
		       struct bt_mesh_net_rx *rx, struct net_buf_simple *out)
{
	if (in->len < BT_MESH_NET_MIN_PDU_LEN) {
		LOG_WRN("Dropping too short mesh packet (len %u)", in->len);
		LOG_WRN("%s", bt_hex(in->data, in->len));
		return -EINVAL;
	}

	if (in->len > BT_MESH_NET_MAX_PDU_LEN) {
		LOG_WRN("Dropping too long mesh packet (len %u)", in->len);
		return -EINVAL;
	}

	if (net_if == BT_MESH_NET_IF_ADV && check_dup(in)) {
		return -EINVAL;
	}

	LOG_DBG("%u bytes: %s", in->len, bt_hex(in->data, in->len));

	rx->net_if = net_if;

	if (!bt_mesh_net_cred_find(rx, in, out, net_decrypt)) {
		LOG_DBG("Unable to find matching net for packet");
		return -ENOENT;
	}

	/* Initialize AppIdx to a sane value */
	rx->ctx.app_idx = BT_MESH_KEY_UNUSED;

	rx->ctx.recv_ttl = TTL(out->data);

	/* Default to responding with TTL 0 for non-routed messages */
	if (rx->ctx.recv_ttl == 0U) {
		rx->ctx.send_ttl = 0U;
	} else {
		rx->ctx.send_ttl = BT_MESH_TTL_DEFAULT;
	}

	rx->ctl = CTL(out->data);
	rx->seq = SEQ(out->data);
	rx->ctx.recv_dst = DST(out->data);

	LOG_DBG("Decryption successful. Payload len %u", out->len);

	if (net_if != BT_MESH_NET_IF_PROXY_CFG &&
	    rx->ctx.recv_dst == BT_MESH_ADDR_UNASSIGNED) {
		LOG_ERR("Destination address is unassigned; dropping packet");
		return -EBADMSG;
	}

	LOG_DBG("src 0x%04x dst 0x%04x ttl %u", rx->ctx.addr, rx->ctx.recv_dst, rx->ctx.recv_ttl);
	LOG_DBG("PDU: %s", bt_hex(out->data, out->len));

	msg_cache_add(rx);

	return 0;
}

void bt_mesh_net_recv(struct net_buf_simple *data, int8_t rssi,
		      enum bt_mesh_net_if net_if)
{
	NET_BUF_SIMPLE_DEFINE(buf, BT_MESH_NET_MAX_PDU_LEN);
	struct bt_mesh_net_rx rx = { .ctx.recv_rssi = rssi };
	struct net_buf_simple_state state;
	int err;

	LOG_DBG("rssi %d net_if %u", rssi, net_if);

	if (!bt_mesh_is_provisioned()) {
		return;
	}

	if (bt_mesh_net_decode(data, net_if, &rx, &buf)) {
		return;
	}

	if (IS_ENABLED(CONFIG_BT_MESH_STATISTIC)) {
		bt_mesh_stat_rx(net_if);
	}

	/* Save the state so the buffer can later be relayed */
	net_buf_simple_save(&buf, &state);

	rx.local_match = (bt_mesh_fixed_group_match(rx.ctx.recv_dst) ||
			  bt_mesh_has_addr(rx.ctx.recv_dst));

	if (IS_ENABLED(CONFIG_BT_MESH_GATT_PROXY) &&
	    net_if == BT_MESH_NET_IF_PROXY) {
		bt_mesh_proxy_addr_add(data, rx.ctx.addr);

		if (bt_mesh_gatt_proxy_get() == BT_MESH_GATT_PROXY_DISABLED &&
		    bt_mesh_priv_gatt_proxy_get() == BT_MESH_PRIV_GATT_PROXY_DISABLED &&
		    !rx.local_match) {
			LOG_INF("Proxy is disabled; ignoring message");
			return;
		}
	}

	err = bt_mesh_trans_recv(&buf, &rx);
	if (err == -EAGAIN) {
		/* The transport layer has indicated that it has rejected the message,
		 * but would like to see it again if it is received in the future.
		 * This can happen if a message is received when the device is in
		 * Low Power mode, but the message was not encrypted with the friend
		 * credentials. Remove it from the message cache so that we accept
		 * it again in the future.
		 */
		LOG_WRN("Removing rejected message from Network Message Cache");
		/* Rewind the next index now that we're not using this entry */
		msg_cache[--msg_cache_next].src = BT_MESH_ADDR_UNASSIGNED;
		dup_cache[--dup_cache_next] = 0;
		return;
	} else if (err == -EBADMSG) {
		LOG_DBG("Not relaying message rejected by the Transport layer");
		return;
	}

	/* Relay if this was a group/virtual address, or if the destination
	 * was neither a local element nor an LPN we're Friends for.
	 */
	if (!BT_MESH_ADDR_IS_UNICAST(rx.ctx.recv_dst) ||
	    (!rx.local_match && !rx.friend_match)) {
		net_buf_simple_restore(&buf, &state);
		bt_mesh_net_relay(&buf, &rx, false);
	}

#if IS_ENABLED(CONFIG_BT_MESH_BRG_CFG_SRV)
	struct pdu_ctx tx_ctx = {
		.sbuf = &buf,
		.state = &state,
		.rx = &rx,
	};

	/* Bridge the traffic if enabled */
	if (!bt_mesh_brg_cfg_enable_get()) {
		return;
	}

	if (bt_mesh_rpl_check(&rx, NULL, true)) {
		return;
	}

	bt_mesh_brg_cfg_tbl_foreach_subnet(rx.ctx.addr, rx.ctx.recv_dst, rx.ctx.net_idx,
					   bt_mesh_sbr_check_cb, &tx_ctx);
#endif
}

static void ivu_refresh(struct k_work *work)
{
	if (!bt_mesh_is_provisioned()) {
		return;
	}

	bt_mesh.ivu_duration = MIN(UINT8_MAX,
	       bt_mesh.ivu_duration + BT_MESH_IVU_HOURS);

	LOG_DBG("%s for %u hour%s",
		atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_IN_PROGRESS) ? "IVU in Progress"
									: "IVU Normal mode",
		bt_mesh.ivu_duration, bt_mesh.ivu_duration == 1U ? "" : "s");

	if (bt_mesh.ivu_duration < BT_MESH_IVU_MIN_HOURS) {
		if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
			store_iv(true);
		}

		goto end;
	}

	/* Because the beacon may be cached, iv update or iv recovery
	 * cannot be performed after 96 hours or 192 hours.
	 * So we need clear beacon cache.
	 */
	if (!(bt_mesh.ivu_duration % BT_MESH_IVU_MIN_HOURS)) {
		bt_mesh_subnet_foreach(bt_mesh_beacon_cache_clear);
	}

	if (atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_IN_PROGRESS)) {
		bt_mesh_beacon_ivu_initiator(true);
		bt_mesh_net_iv_update(bt_mesh.iv_index, false);
	} else if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		store_iv(true);
	}

end:
	k_work_reschedule(&bt_mesh.ivu_timer, BT_MESH_IVU_TIMEOUT);
}

void bt_mesh_net_init(void)
{
	k_work_init_delayable(&bt_mesh.ivu_timer, ivu_refresh);

	k_work_init(&bt_mesh.local_work, bt_mesh_net_local);
}

static int net_set(const char *name, size_t len_rd, settings_read_cb read_cb,
		   void *cb_arg)
{
	struct net_val net;
	struct bt_mesh_key key;
	int err;

	if (len_rd == 0) {
		LOG_DBG("val (null)");

		bt_mesh_comp_unprovision();
		bt_mesh_key_destroy(&bt_mesh.dev_key);
		memset(&bt_mesh.dev_key, 0, sizeof(struct bt_mesh_key));
		return 0;
	}

	err = bt_mesh_settings_set(read_cb, cb_arg, &net, sizeof(net));
	if (err) {
		LOG_ERR("Failed to set \'net\'");
		return err;
	}

	/* One extra copying since net.dev_key is from packed structure
	 * and might be unaligned.
	 */
	memcpy(&key, &net.dev_key, sizeof(struct bt_mesh_key));

	bt_mesh_key_assign(&bt_mesh.dev_key, &key);
	bt_mesh_comp_provision(net.primary_addr);

	LOG_DBG("Provisioned with primary address 0x%04x", net.primary_addr);
	LOG_DBG("Recovered DevKey %s", bt_hex(&bt_mesh.dev_key, sizeof(struct bt_mesh_key)));

	return 0;
}

BT_MESH_SETTINGS_DEFINE(net, "Net", net_set);

static int iv_set(const char *name, size_t len_rd, settings_read_cb read_cb,
		  void *cb_arg)
{
	struct iv_val iv;
	int err;

	if (len_rd == 0) {
		LOG_DBG("IV deleted");

		bt_mesh.iv_index = 0U;
		atomic_clear_bit(bt_mesh.flags, BT_MESH_IVU_IN_PROGRESS);
		return 0;
	}

	err = bt_mesh_settings_set(read_cb, cb_arg, &iv, sizeof(iv));
	if (err) {
		LOG_ERR("Failed to set \'iv\'");
		return err;
	}

	bt_mesh.iv_index = iv.iv_index;
	atomic_set_bit_to(bt_mesh.flags, BT_MESH_IVU_IN_PROGRESS, iv.iv_update);
	bt_mesh.ivu_duration = iv.iv_duration;

	LOG_DBG("IV Index 0x%04x (IV Update Flag %u) duration %u hours", iv.iv_index, iv.iv_update,
		iv.iv_duration);

	return 0;
}

BT_MESH_SETTINGS_DEFINE(iv, "IV", iv_set);

static int seq_set(const char *name, size_t len_rd, settings_read_cb read_cb,
		   void *cb_arg)
{
	struct seq_val seq;
	int err;

	if (len_rd == 0) {
		LOG_DBG("val (null)");

		bt_mesh.seq = 0U;
		return 0;
	}

	err = bt_mesh_settings_set(read_cb, cb_arg, &seq, sizeof(seq));
	if (err) {
		LOG_ERR("Failed to set \'seq\'");
		return err;
	}

	bt_mesh.seq = sys_get_le24(seq.val);

	if (CONFIG_BT_MESH_SEQ_STORE_RATE > 0) {
		/* Make sure we have a large enough sequence number. We
		 * subtract 1 so that the first transmission causes a write
		 * to the settings storage.
		 */
		bt_mesh.seq += (CONFIG_BT_MESH_SEQ_STORE_RATE -
				(bt_mesh.seq % CONFIG_BT_MESH_SEQ_STORE_RATE));
		bt_mesh.seq--;
	}

	LOG_DBG("Sequence Number 0x%06x", bt_mesh.seq);

	return 0;
}

BT_MESH_SETTINGS_DEFINE(seq, "Seq", seq_set);

#if defined(CONFIG_BT_MESH_RPR_SRV)
static int dev_key_cand_set(const char *name, size_t len_rd, settings_read_cb read_cb,
		   void *cb_arg)
{	int err;

	if (len_rd < 16) {
		return -EINVAL;
	}

	err = bt_mesh_settings_set(read_cb, cb_arg, &bt_mesh.dev_key_cand,
				   sizeof(struct bt_mesh_key));
	if (!err) {
		LOG_DBG("DevKey candidate recovered from storage");
		atomic_set_bit(bt_mesh.flags, BT_MESH_DEVKEY_CAND);
	}

	return err;
}

BT_MESH_SETTINGS_DEFINE(dev_key, "DevKeyC", dev_key_cand_set);
#endif

void bt_mesh_net_pending_dev_key_cand_store(void)
{
#if defined(CONFIG_BT_MESH_RPR_SRV)
	int err;

	if (atomic_test_bit(bt_mesh.flags, BT_MESH_DEVKEY_CAND)) {
		err = settings_save_one("bt/mesh/DevKeyC", &bt_mesh.dev_key_cand,
					sizeof(struct bt_mesh_key));
	} else {
		err = settings_delete("bt/mesh/DevKeyC");
	}

	if (err) {
		LOG_ERR("Failed to update DevKey candidate value");
	} else {
		LOG_DBG("Stored DevKey candidate value");
	}
#endif
}

void bt_mesh_net_dev_key_cand_store(void)
{
	bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_DEV_KEY_CAND_PENDING);
}

static void clear_iv(void)
{
	int err;

	err = settings_delete("bt/mesh/IV");
	if (err) {
		LOG_ERR("Failed to clear IV");
	} else {
		LOG_DBG("Cleared IV");
	}
}

static void store_pending_iv(void)
{
	struct iv_val iv;
	int err;

	iv.iv_index = bt_mesh.iv_index;
	iv.iv_update = atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_IN_PROGRESS);
	iv.iv_duration = bt_mesh.ivu_duration;

	err = settings_save_one("bt/mesh/IV", &iv, sizeof(iv));
	if (err) {
		LOG_ERR("Failed to store IV value");
	} else {
		LOG_DBG("Stored IV value");
	}
}

void bt_mesh_net_pending_iv_store(void)
{
	if (atomic_test_bit(bt_mesh.flags, BT_MESH_VALID)) {
		store_pending_iv();
	} else {
		clear_iv();
	}
}

static void clear_net(void)
{
	int err;

	err = settings_delete("bt/mesh/Net");
	if (err) {
		LOG_ERR("Failed to clear Network");
	} else {
		LOG_DBG("Cleared Network");
	}
}

static void store_pending_net(void)
{
	struct net_val net;
	int err;

	LOG_DBG("addr 0x%04x DevKey %s", bt_mesh_primary_addr(),
		bt_hex(&bt_mesh.dev_key, sizeof(struct bt_mesh_key)));

	net.primary_addr = bt_mesh_primary_addr();
	memcpy(&net.dev_key, &bt_mesh.dev_key, sizeof(struct bt_mesh_key));

	err = settings_save_one("bt/mesh/Net", &net, sizeof(net));
	if (err) {
		LOG_ERR("Failed to store Network value");
	} else {
		LOG_DBG("Stored Network value");
	}
}

void bt_mesh_net_pending_net_store(void)
{
	if (atomic_test_bit(bt_mesh.flags, BT_MESH_VALID)) {
		store_pending_net();
	} else {
		clear_net();
	}
}

void bt_mesh_net_pending_seq_store(void)
{
	struct seq_val seq;
	int err;

	if (atomic_test_bit(bt_mesh.flags, BT_MESH_VALID)) {
		sys_put_le24(bt_mesh.seq, seq.val);

		err = settings_save_one("bt/mesh/Seq", &seq, sizeof(seq));
		if (err) {
			LOG_ERR("Failed to stor Seq value");
		} else {
			LOG_DBG("Stored Seq value");
		}
	} else {
		err = settings_delete("bt/mesh/Seq");
		if (err) {
			LOG_ERR("Failed to clear Seq value");
		} else {
			LOG_DBG("Cleared Seq value");
		}
	}
}

void bt_mesh_net_store(void)
{
	bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_NET_PENDING);
}

void bt_mesh_net_clear(void)
{
	bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_NET_PENDING);
	bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_IV_PENDING);
	bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_CFG_PENDING);
	bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_SEQ_PENDING);
}

void bt_mesh_net_settings_commit(void)
{
	if (bt_mesh.ivu_duration < BT_MESH_IVU_MIN_HOURS) {
		k_work_reschedule(&bt_mesh.ivu_timer, BT_MESH_IVU_TIMEOUT);
	}
}
