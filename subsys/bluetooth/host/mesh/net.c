/*  Bluetooth Mesh */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <atomic.h>
#include <misc/util.h>
#include <misc/byteorder.h>

#include <net/buf.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/mesh.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BLUETOOTH_MESH_DEBUG_NET)
#include "common/log.h"

#include "crypto.h"
#include "adv.h"
#include "mesh.h"
#include "net.h"
#include "lpn.h"
#include "friend.h"
#include "proxy.h"
#include "transport.h"
#include "access.h"
#include "foundation.h"
#include "beacon.h"

/* Seq limit after IV Update is triggered */
#define IV_UPDATE_SEQ_LIMIT 8000000

#if defined(CONFIG_BLUETOOTH_MESH_IV_UPDATE_TEST)
/* Small test timeout for IV Update Procedure testing */
#define IV_UPDATE_TIMEOUT  K_SECONDS(120)
#else
/* Maximum time to stay in IV Update mode (96 < time < 144) */
#define IV_UPDATE_TIMEOUT  K_HOURS(120)
#endif /* CONFIG_BLUETOOTH_MESH_IV_UPDATE_TEST */

#define IVI(pdu)           ((pdu)[0] >> 7)
#define NID(pdu)           ((pdu)[0] & 0x7f)
#define CTL(pdu)           ((pdu)[1] >> 7)
#define TTL(pdu)           ((pdu)[1] & 0x7f)

/* Determine how many friendship credentials we need */
#if defined(CONFIG_BLUETOOTH_MESH_FRIEND)
#define FRIEND_CRED_COUNT CONFIG_BLUETOOTH_MESH_FRIEND_LPN_COUNT
#elif defined(CONFIG_BLUETOOTH_MESH_LOW_POWER)
#define FRIEND_CRED_COUNT CONFIG_BLUETOOTH_MESH_SUBNET_COUNT
#else
#define FRIEND_CRED_COUNT 0
#endif

#if FRIEND_CRED_COUNT > 0
static struct bt_mesh_friend_cred friend_cred[FRIEND_CRED_COUNT];
#endif

static u64_t msg_cache[CONFIG_BLUETOOTH_MESH_MSG_CACHE_SIZE];
static u16_t msg_cache_next;

/* Singleton network context (the implementation only supports one) */
struct bt_mesh_net bt_mesh = {
	.sub = {
		[ 0 ... (CONFIG_BLUETOOTH_MESH_SUBNET_COUNT - 1) ] =
			{ .net_idx = BT_MESH_KEY_UNUSED, }
	},
	.app_keys = {
		[ 0 ... (CONFIG_BLUETOOTH_MESH_APP_KEY_COUNT - 1) ] =
			{ .net_idx = BT_MESH_KEY_UNUSED, }
	},
};

static u32_t dup_cache[4];
static int   dup_cache_next;

static bool check_dup(struct net_buf_simple *data)
{
	const u8_t *tail = net_buf_simple_tail(data);
	u32_t val;
	int i;

	val = sys_get_be32(tail - 4) ^ sys_get_be32(tail - 8);

	for (i = 0; i < ARRAY_SIZE(dup_cache); i++) {
		if (dup_cache[i] == val) {
			return true;
		}
	}

	dup_cache[dup_cache_next++] = val;
	dup_cache_next %= ARRAY_SIZE(dup_cache);

	return false;
}

static u64_t msg_hash(struct net_buf_simple *pdu)
{
	u8_t *tpdu_last;
	u64_t hash;

	/* Last byte of TransportPDU */
	tpdu_last = net_buf_simple_tail(pdu) - (CTL(pdu->data) ? 8 : 4) - 1;

	((u8_t *)(&hash))[0] = pdu->data[0];
	((u8_t *)(&hash))[1] = (pdu->data[1] & 0xc0);
	((u8_t *)(&hash))[2] = *tpdu_last;
	memcpy(&((u8_t *)&hash)[3], &pdu->data[2], 5);

	return hash;
}

static void msg_cache_add(u64_t new_hash)
{
	msg_cache[msg_cache_next++] = new_hash;
	msg_cache_next %= ARRAY_SIZE(msg_cache);
}

static bool msg_is_known(u64_t hash)
{
	u16_t i;

	for (i = 0; i < ARRAY_SIZE(msg_cache); i++) {
		if (msg_cache[i] == hash) {
			return true;
		}
	}

	return false;
}

static inline u32_t net_seq(struct net_buf_simple *buf)
{
	return ((net_buf_simple_pull_u8(buf) << 16) & 0xff0000) |
		((net_buf_simple_pull_u8(buf) << 8) & 0xff00) |
		net_buf_simple_pull_u8(buf);
}

struct bt_mesh_subnet *bt_mesh_subnet_get(u16_t net_idx)
{
	int i;

	if (net_idx == BT_MESH_KEY_ANY) {
		return &bt_mesh.sub[0];
	}

	for (i = 0; i < ARRAY_SIZE(bt_mesh.sub); i++) {
		if (bt_mesh.sub[i].net_idx == net_idx) {
			return &bt_mesh.sub[i];
		}
	}

	return NULL;
}

int bt_mesh_net_keys_create(struct bt_mesh_subnet_keys *keys,
			    const u8_t key[16])
{
	u8_t p[] = { 0 };
	u8_t nid;
	int err;

	err = bt_mesh_k2(key, p, sizeof(p), &nid, keys->enc, keys->privacy);
	if (err) {
		BT_ERR("Unable to generate NID, EncKey & PrivacyKey");
		return err;
	}

	memcpy(keys->net, key, 16);

	keys->nid = nid;

	BT_DBG("NID 0x%02x EncKey %s", keys->nid, bt_hex(keys->enc, 16));
	BT_DBG("PrivacyKey %s", bt_hex(keys->privacy, 16));

	err = bt_mesh_k3(key, keys->net_id);
	if (err) {
		BT_ERR("Unable to generate Net ID");
		return err;
	}

	BT_DBG("NetID %s", bt_hex(keys->net_id, 8));

#if defined(CONFIG_BLUETOOTH_MESH_GATT_PROXY)
	err = bt_mesh_identity_key(key, keys->identity);
	if (err) {
		BT_ERR("Unable to generate IdentityKey");
		return err;
	}

	BT_DBG("IdentityKey %s", bt_hex(keys->identity, 16));
#endif /* GATT_PROXY */

	err = bt_mesh_beacon_key(key, keys->beacon);
	if (err) {
		BT_ERR("Unable to generate beacon key");
		return err;
	}

	BT_DBG("BeaconKey %s", bt_hex(keys->beacon, 16));

	return 0;
}

#if (defined(CONFIG_BLUETOOTH_MESH_LOW_POWER) || \
     defined(CONFIG_BLUETOOTH_MESH_FRIEND))
int bt_mesh_friend_cred_set(struct bt_mesh_friend_cred *cred, u8_t idx,
			    const u8_t net_key[16])
{
	u16_t lpn_addr, frnd_addr;
	int err;
	u8_t p[9];

#if defined(CONFIG_BLUETOOTH_MESH_LOW_POWER)
	if (cred->addr == bt_mesh.lpn.frnd) {
		lpn_addr = bt_mesh_primary_addr();
		frnd_addr = cred->addr;
	} else {
		lpn_addr = cred->addr;
		frnd_addr = bt_mesh_primary_addr();
	}
#else
	lpn_addr = cred->addr;
	frnd_addr = bt_mesh_primary_addr();
#endif

	BT_DBG("LPNAddress 0x%04x FriendAddress 0x%04x", lpn_addr, frnd_addr);
	BT_DBG("LPNCounter 0x%04x FriendCounter 0x%04x", cred->lpn_counter,
	       cred->frnd_counter);

	p[0] = 0x01;
	sys_put_be16(lpn_addr, p + 1);
	sys_put_be16(frnd_addr, p + 3);
	sys_put_be16(cred->lpn_counter, p + 5);
	sys_put_be16(cred->frnd_counter, p + 7);

	err = bt_mesh_k2(net_key, p, sizeof(p), &cred->cred[idx].nid,
			 cred->cred[idx].enc, cred->cred[idx].privacy);
	if (err) {
		BT_ERR("Unable to generate NID, EncKey & PrivacyKey");
		return err;
	}

	BT_DBG("Friend NID 0x%02x EncKey %s", cred->cred[idx].nid,
	       bt_hex(cred->cred[idx].enc, 16));
	BT_DBG("Friend PrivacyKey %s", bt_hex(cred->cred[idx].privacy, 16));

	return 0;
}

void bt_mesh_friend_cred_refresh(u16_t net_idx)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(friend_cred); i++) {
		struct bt_mesh_friend_cred *cred = &friend_cred[i];

		if (cred->addr != BT_MESH_ADDR_UNASSIGNED &&
		    cred->net_idx == net_idx) {
			memcpy(&cred->cred[0], &cred->cred[1],
			       sizeof(cred->cred[0]));
		}
	}
}

int bt_mesh_friend_cred_update(u16_t net_idx, u8_t idx, const u8_t net_key[16])
{
	int err, i;

	for (i = 0; i < ARRAY_SIZE(friend_cred); i++) {
		struct bt_mesh_friend_cred *cred = &friend_cred[i];

		if (cred->addr == BT_MESH_ADDR_UNASSIGNED ||
		    cred->net_idx != net_idx) {
			continue;
		}

		err = bt_mesh_friend_cred_set(cred, idx, net_key);
		if (err) {
			return err;
		}
	}

	return 0;
}

struct bt_mesh_friend_cred *bt_mesh_friend_cred_add(u16_t net_idx,
						    const u8_t net_key[16],
						    u8_t idx, u16_t addr,
						    u16_t lpn_counter,
						    u16_t frnd_counter)
{
	struct bt_mesh_friend_cred *cred;
	int i, err;

	BT_DBG("net_idx 0x%04x addr 0x%04x idx %u", net_idx, addr, idx);

	for (cred = NULL, i = 0; i < ARRAY_SIZE(friend_cred); i++) {
		if ((friend_cred[i].addr == BT_MESH_ADDR_UNASSIGNED) ||
		    (friend_cred[i].addr == addr &&
		     friend_cred[i].net_idx == net_idx)) {
			cred = &friend_cred[i];
			break;
		}
	}

	if (!cred) {
		return NULL;
	}

	cred->net_idx = net_idx;
	cred->addr = addr;
	cred->lpn_counter = lpn_counter;
	cred->frnd_counter = frnd_counter;

	err = bt_mesh_friend_cred_set(cred, idx, net_key);
	if (err) {
		bt_mesh_friend_cred_clear(cred);
		return NULL;
	}

	return cred;
}

void bt_mesh_friend_cred_clear(struct bt_mesh_friend_cred *cred)
{
	cred->net_idx = BT_MESH_KEY_UNUSED;
	cred->addr = BT_MESH_ADDR_UNASSIGNED;
	cred->lpn_counter = 0;
	cred->frnd_counter = 0;
	memset(cred->cred, 0, sizeof(cred->cred));
}

int bt_mesh_friend_cred_del(u16_t net_idx, u16_t addr)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(friend_cred); i++) {
		struct bt_mesh_friend_cred *cred = &friend_cred[i];

		if (cred->addr == addr && cred->net_idx == net_idx) {
			bt_mesh_friend_cred_clear(cred);
			return 0;
		}
	}

	return -ENOENT;
}

static int friend_cred_get(u16_t net_idx, u16_t addr, u8_t idx,
			   u8_t *nid, const u8_t **enc, const u8_t **priv)
{
	int i;

	BT_DBG("net_idx 0x%04x addr 0x%04x idx %u", net_idx, addr, idx);

	for (i = 0; i < ARRAY_SIZE(friend_cred); i++) {
		struct bt_mesh_friend_cred *cred = &friend_cred[i];

		if (cred->net_idx != net_idx) {
			continue;
		}

		if (addr != BT_MESH_ADDR_UNASSIGNED && cred->addr != addr) {
			continue;
		}

		if (nid) {
			*nid = cred->cred[idx].nid;
		}

		if (enc) {
			*enc = cred->cred[idx].enc;
		}

		if (priv) {
			*priv = cred->cred[idx].privacy;
		}

		return 0;
	}

	return -ENOENT;
}
#else
static inline int friend_cred_get(u16_t net_idx, u16_t addr, u8_t idx,
				  u8_t *nid, const u8_t **enc,
				  const u8_t **priv)
{
	return -ENOENT;
}
#endif /* FRIEND || LOW_POWER */

int bt_mesh_net_beacon_update(struct bt_mesh_subnet *sub)
{
	struct bt_mesh_subnet_keys *keys;
	u8_t flags;

	if (sub->kr_flag) {
		BT_DBG("NetIndex %u Using new key", sub->net_idx);
		flags = BT_MESH_NET_FLAG_KR;
		keys = &sub->keys[1];
	} else {
		BT_DBG("NetIndex %u Using current key", sub->net_idx);
		flags = 0x00;
		keys = &sub->keys[0];
	}

	if (bt_mesh.iv_update) {
		flags |= BT_MESH_NET_FLAG_IVU;
	}

	BT_DBG("flags 0x%02x, IVI 0x%08x", flags, bt_mesh.iv_index);

	return bt_mesh_beacon_auth(keys->beacon, flags, keys->net_id,
				   bt_mesh.iv_index, sub->auth);
}

int bt_mesh_net_create(u16_t idx, u8_t flags, const u8_t key[16],
		       u32_t iv_index)
{
	struct bt_mesh_subnet *sub;
	int err;

	BT_DBG("idx %u iv_index %u", idx, iv_index);

	BT_DBG("NetKey %s", bt_hex(key, 16));

	if (bt_mesh.valid) {
		return -EALREADY;
	}

	sub = &bt_mesh.sub[0];

	err = bt_mesh_net_keys_create(&sub->keys[0], key);
	if (err) {
		return -EIO;
	}

	bt_mesh.valid = 1;
	sub->net_idx = idx;

	if (IS_ENABLED(CONFIG_BLUETOOTH_MESH_GATT_PROXY)) {
		sub->node_id = BT_MESH_NODE_IDENTITY_RUNNING;
	} else {
		sub->node_id = BT_MESH_NODE_IDENTITY_NOT_SUPPORTED;
	}

	sub->kr_flag = BT_MESH_KEY_REFRESH(flags);
	if (sub->kr_flag) {
		memcpy(&sub->keys[1], &sub->keys[0], sizeof(sub->keys[0]));
		sub->kr_phase = BT_MESH_KR_PHASE_2;
	}

	bt_mesh.iv_index = iv_index;
	bt_mesh.iv_update = BT_MESH_IV_UPDATE(flags);

	/* Set initial IV Update procedure state time-stamp */
	bt_mesh.last_update = k_uptime_get();

	return 0;
}

bool bt_mesh_kr_update(struct bt_mesh_subnet *sub, u8_t new_kr, bool new_key)
{
	if (new_kr != sub->kr_flag && sub->kr_phase == BT_MESH_KR_NORMAL) {
		BT_WARN("KR change in normal operation. Are we blacklisted?");
		return false;
	}

	sub->kr_flag = new_kr;

	if (sub->kr_flag) {
		if (sub->kr_phase == BT_MESH_KR_PHASE_1) {
			BT_DBG("Phase 1 -> Phase 2");
			sub->kr_phase = BT_MESH_KR_PHASE_2;
			return true;
		}
	} else {
		switch (sub->kr_phase) {
		case BT_MESH_KR_PHASE_1:
			if (!new_key) {
				/* Ignore */
				break;
			}
		/* Upon receiving a Secure Network beacon with the KR flag set
		 * to 0 using the new NetKey in Phase 1, the node shall
		 * immediately transition to Phase 3, which effectively skips
		 * Phase 2.
		 *
		 * Intentional fall-through.
		 */
		case BT_MESH_KR_PHASE_2:
			BT_DBG("KR Phase 0x%02x -> Normal", sub->kr_phase);
			memcpy(&sub->keys[0], &sub->keys[1],
			       sizeof(sub->keys[0]));
			if (IS_ENABLED(CONFIG_BLUETOOTH_MESH_LOW_POWER) ||
			    IS_ENABLED(CONFIG_BLUETOOTH_MESH_FRIEND)) {
				bt_mesh_friend_cred_refresh(sub->net_idx);
			}
			sub->kr_phase = BT_MESH_KR_NORMAL;
			return true;
		}
	}

	return false;
}

void bt_mesh_rpl_reset(void)
{
	int i;

	/* Discard "old old" IV Index entries from RPL and flag
	 * any other ones (which are valid) as old.
	 */
	for (i = 0; i < ARRAY_SIZE(bt_mesh.rpl); i++) {
		struct bt_mesh_rpl *rpl = &bt_mesh.rpl[i];

		if (rpl->src) {
			if (rpl->old_iv) {
				memset(rpl, 0, sizeof(*rpl));
			} else {
				rpl->old_iv = true;
			}
		}
	}
}

void bt_mesh_iv_update(u32_t iv_index, bool iv_update)
{
	int i;

	if (iv_index < bt_mesh.iv_index || iv_index > bt_mesh.iv_index + 42) {
		BT_ERR("IV Index completely out of sync: 0x%08x != 0x%08x",
			iv_index, bt_mesh.iv_index);
		return;
	}

	if (iv_index > bt_mesh.iv_index + 1) {
		BT_WARN("Performing IV Index Recovery");
		memset(bt_mesh.rpl, 0, sizeof(bt_mesh.rpl));
		bt_mesh.iv_index = iv_index;
		bt_mesh.seq = 0;
		goto do_update;
	}

	if (iv_update == bt_mesh.iv_update) {
		if (iv_index != bt_mesh.iv_index) {
			BT_WARN("No update, but IV Index 0x%08x != 0x%08x",
				iv_index, bt_mesh.iv_index);
		}
		return;
	}

	if (bt_mesh.iv_update) {
		if (iv_index != bt_mesh.iv_index) {
			BT_WARN("IV Index mismatch: 0x%08x != 0x%08x",
				iv_index, bt_mesh.iv_index);
			return;
		}
	} else {
		if (iv_index != bt_mesh.iv_index + 1) {
			BT_WARN("Wrong new IV Index: 0x%08x != 0x%08x + 1",
				iv_index, bt_mesh.iv_index);
			return;
		}
	}

	if (!IS_ENABLED(CONFIG_BLUETOOTH_MESH_IV_UPDATE_TEST)) {
		s64_t delta = k_uptime_get() - bt_mesh.last_update;

		if (delta < K_HOURS(96)) {
			BT_WARN("IV Update before minimum duration");
			return;
		}
	}

	/* Defer change to Normal Operation if there are pending acks */
	if (!iv_update && bt_mesh_tx_in_progress()) {
		BT_WARN("IV Update deferred because of pending transfer");
		bt_mesh.pending_update = 1;
		return;
	}

do_update:
	bt_mesh.iv_update = iv_update;

	if (bt_mesh.iv_update) {
		bt_mesh.iv_index = iv_index;
		BT_DBG("IV Update state entered. New index 0x%08x",
		       bt_mesh.iv_index);

		bt_mesh_rpl_reset();

		k_delayed_work_submit(&bt_mesh.ivu_complete,
				      IV_UPDATE_TIMEOUT);
	} else {
		BT_DBG("Normal mode entered");
		bt_mesh.seq = 0;
		k_delayed_work_cancel(&bt_mesh.ivu_complete);
	}

	/* Store time-stamp of the IV procedure state change */
	bt_mesh.last_update = k_uptime_get();

	for (i = 0; i < ARRAY_SIZE(bt_mesh.sub); i++) {
		if (bt_mesh.sub[i].net_idx != BT_MESH_KEY_UNUSED) {
			bt_mesh_net_beacon_update(&bt_mesh.sub[i]);
		}
	}
}

int bt_mesh_net_resend(struct bt_mesh_subnet *sub, struct net_buf *buf,
		       bool new_key, bool friend_cred, bt_mesh_adv_func_t cb)
{
	const u8_t *enc, *priv;
	int err;

	BT_DBG("net_idx 0x%04x, len %u", sub->net_idx, buf->len);

	if (friend_cred) {
		err = friend_cred_get(sub->net_idx, BT_MESH_ADDR_UNASSIGNED,
				      new_key, NULL, &enc, &priv);
		if (err) {
			return err;
		}
	} else {
		enc = sub->keys[new_key].enc;
		priv = sub->keys[new_key].privacy;
	}

	err = bt_mesh_net_obfuscate(buf->data, BT_MESH_NET_IVI_TX, priv);
	if (err) {
		BT_ERR("deobfuscate failed (err %d)", err);
		return err;
	}

	err = bt_mesh_net_decrypt(enc, &buf->b, BT_MESH_NET_IVI_TX, false);
	if (err) {
		BT_ERR("decrypt failed (err %d)", err);
		return err;
	}

	/* Update with a new sequence number */
	buf->data[2] = (bt_mesh.seq >> 16);
	buf->data[3] = (bt_mesh.seq >> 8);
	buf->data[4] = bt_mesh.seq++;

	err = bt_mesh_net_encrypt(enc, &buf->b, BT_MESH_NET_IVI_TX, false);
	if (err) {
		BT_ERR("encrypt failed (err %d)", err);
		return err;
	}

	err = bt_mesh_net_obfuscate(buf->data, BT_MESH_NET_IVI_TX, priv);
	if (err) {
		BT_ERR("obfuscate failed (err %d)", err);
		return err;
	}

	bt_mesh_adv_send(buf, cb);

	if (!bt_mesh.iv_update && bt_mesh.seq > IV_UPDATE_SEQ_LIMIT) {
		bt_mesh_beacon_ivu_initiator(true);
		bt_mesh_iv_update(bt_mesh.iv_index + 1, true);
	}

	return 0;
}

#if defined(CONFIG_BLUETOOTH_MESH_LOCAL_INTERFACE)
static void bt_mesh_net_local(struct k_work *work)
{
	struct net_buf *buf;

	while ((buf = net_buf_get(&bt_mesh.local_queue, K_NO_WAIT))) {
		BT_DBG("len %u: %s", buf->len, bt_hex(buf->data, buf->len));
		bt_mesh_net_recv(&buf->b, 0, BT_MESH_NET_IF_LOCAL);
		net_buf_unref(buf);
	}
}
#endif

int bt_mesh_net_encode(struct bt_mesh_net_tx *tx, struct net_buf_simple *buf,
		       bool proxy)
{
	const bool ctl = (tx->ctx->app_idx == BT_MESH_KEY_UNUSED);
	u8_t nid;
	const u8_t *enc, *priv;
	u8_t *seq;
	int err;

	if (ctl && net_buf_simple_tailroom(buf) < 8) {
		BT_ERR("Insufficient MIC space for CTL PDU");
		return -EINVAL;
	} else if (net_buf_simple_tailroom(buf) < 4) {
		BT_ERR("Insufficient MIC space for PDU");
		return -EINVAL;
	}

	BT_DBG("src 0x%04x dst 0x%04x ctl %u seq 0x%06x",
	       tx->src, tx->ctx->addr, ctl, bt_mesh.seq);

	net_buf_simple_push_be16(buf, tx->ctx->addr);
	net_buf_simple_push_be16(buf, tx->src);

	seq = net_buf_simple_push(buf, 3);
	seq[0] = (bt_mesh.seq >> 16);
	seq[1] = (bt_mesh.seq >> 8);
	seq[2] = bt_mesh.seq++;

	if (ctl) {
		net_buf_simple_push_u8(buf, tx->ctx->send_ttl | 0x80);
	} else {
		net_buf_simple_push_u8(buf, tx->ctx->send_ttl);
	}

	if (tx->sub->kr_phase == BT_MESH_KR_PHASE_2) {
		if (tx->ctx->friend_cred) {
			err = friend_cred_get(tx->sub->net_idx,
					      BT_MESH_ADDR_UNASSIGNED,
					      1, &nid, &enc, &priv);
			if (err) {
				return err;
			}
		} else {
			nid = tx->sub->keys[1].nid;
			enc = tx->sub->keys[1].enc;
			priv = tx->sub->keys[1].privacy;
		}
	} else {
		if (tx->ctx->friend_cred) {
			err = friend_cred_get(tx->sub->net_idx,
					      BT_MESH_ADDR_UNASSIGNED,
					      0, &nid, &enc, &priv);
			if (err) {
				return err;
			}
		} else {
			nid = tx->sub->keys[0].nid;
			enc = tx->sub->keys[0].enc;
			priv = tx->sub->keys[0].privacy;
		}
	}

	net_buf_simple_push_u8(buf, (nid | (BT_MESH_NET_IVI_TX & 1) << 7));

	err = bt_mesh_net_encrypt(enc, buf, BT_MESH_NET_IVI_TX, proxy);
	if (err) {
		return err;
	}

	return bt_mesh_net_obfuscate(buf->data, BT_MESH_NET_IVI_TX, priv);
}

int bt_mesh_net_send(struct bt_mesh_net_tx *tx, struct net_buf *buf,
		     bt_mesh_adv_func_t cb)
{
	int err;

	BT_DBG("src 0x%04x dst 0x%04x len %u headroom %zu tailroom %zu",
	       tx->src, tx->ctx->addr, buf->len, net_buf_headroom(buf),
	       net_buf_tailroom(buf));
	BT_DBG("Payload len %u: %s", buf->len, bt_hex(buf->data, buf->len));
	BT_DBG("Seq 0x%06x", bt_mesh.seq);

#if defined(CONFIG_BLUETOOTH_MESH_LOW_POWER)
	/* Communication between LPN & Friend should always be using
	 * the Friendship Credentials. Any other destination should
	 * use the Master Credentials.
	 */
	if (bt_mesh_lpn_established()) {
		tx->ctx->friend_cred = (tx->ctx->addr == bt_mesh.lpn.frnd);
	}
#endif

	if (tx->ctx->send_ttl == BT_MESH_TTL_DEFAULT) {
		tx->ctx->send_ttl = bt_mesh_default_ttl_get();
	}

	err = bt_mesh_net_encode(tx, &buf->b, false);
	if (err) {
		goto done;
	}

	/* Deliver to GATT Proxy Clients if necessary */
	if (IS_ENABLED(CONFIG_BLUETOOTH_MESH_GATT_PROXY)) {
		if (bt_mesh_proxy_relay(&buf->b, tx->ctx->addr) &&
		    BT_MESH_ADDR_IS_UNICAST(tx->ctx->addr)) {
			err = 0;
			goto done;
		}
	}

#if defined(CONFIG_BLUETOOTH_MESH_LOCAL_INTERFACE)
	/* Deliver to local network interface if necessary */
	if (bt_mesh_fixed_group_match(tx->ctx->addr) ||
	    bt_mesh_elem_find(tx->ctx->addr)) {
		net_buf_put(&bt_mesh.local_queue, net_buf_ref(buf));
		if (cb) {
			cb(buf, 0);
		}
		k_work_submit(&bt_mesh.local_work);
	} else {
		bt_mesh_adv_send(buf, cb);
	}
#else
	bt_mesh_adv_send(buf, cb);
#endif

done:
	net_buf_unref(buf);
	return err;
}

static bool auth_match(struct bt_mesh_subnet_keys *keys,
		       const u8_t net_id[8], u8_t flags,
		       u32_t iv_index, const u8_t auth[8])
{
	u8_t net_auth[8];

	if (memcmp(net_id, keys->net_id, 8)) {
		return false;
	}

	bt_mesh_beacon_auth(keys->beacon, flags, keys->net_id, iv_index,
			    net_auth);

	if (memcmp(auth, net_auth, 8)) {
		BT_WARN("Authentication Value %s != %s",
			bt_hex(auth, 8), bt_hex(net_auth, 8));
		return false;
	}

	return true;
}

struct bt_mesh_subnet *bt_mesh_subnet_find(const u8_t net_id[8], u8_t flags,
					   u32_t iv_index, const u8_t auth[8],
					   bool *new_key)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(bt_mesh.sub); i++) {
		struct bt_mesh_subnet *sub = &bt_mesh.sub[i];

		if (sub->net_idx == BT_MESH_KEY_UNUSED) {
			continue;
		}

		if (auth_match(&sub->keys[0], net_id, flags, iv_index, auth)) {
			*new_key = false;
			return sub;
		}

		if (sub->kr_phase == BT_MESH_KR_NORMAL) {
			continue;
		}

		if (auth_match(&sub->keys[1], net_id, flags, iv_index, auth)) {
			*new_key = true;
			return sub;
		}
	}

	return NULL;
}

static int net_decrypt(struct bt_mesh_subnet *sub, u8_t idx, const u8_t *data,
		       size_t data_len, struct bt_mesh_net_rx *rx,
		       struct net_buf_simple *buf)
{
	const uint8_t *enc, *priv;

	BT_DBG("NID 0x%02x, PDU NID 0x%02x net_idx 0x%04x idx %u",
	       sub->keys[idx].nid, NID(data), sub->net_idx, idx);

	if (NID(data) == sub->keys[idx].nid) {
		rx->ctx.friend_cred = false;
		enc = sub->keys[idx].enc;
		priv = sub->keys[idx].privacy;
		rx->ctx.friend_cred = 0;
	} else {
		u8_t nid;

		if (friend_cred_get(sub->net_idx, BT_MESH_ADDR_UNASSIGNED, idx,
				    &nid, &enc, &priv)) {
			return -ENOENT;
		}

		if (nid != NID(data)) {
			return -ENOENT;
		}

		rx->ctx.friend_cred = 1;
	}

	BT_DBG("IVI %u net->iv_index 0x%08x", IVI(data), bt_mesh.iv_index);

	rx->old_iv = (IVI(data) != (bt_mesh.iv_index & 0x01));

	net_buf_simple_init(buf, 0);
	memcpy(net_buf_simple_add(buf, data_len), data, data_len);

	if (bt_mesh_net_obfuscate(buf->data, BT_MESH_NET_IVI_RX(rx), priv)) {
		return -ENOENT;
	}

	if (msg_is_known(rx->hash)) {
		return -EALREADY;
	}

	rx->ctx.addr = sys_get_be16(&buf->data[5]);
	if (!BT_MESH_ADDR_IS_UNICAST(rx->ctx.addr)) {
		BT_WARN("Ignoring non-unicast src addr 0x%04x", rx->ctx.addr);
		return -EINVAL;
	}

	BT_DBG("src 0x%04x", rx->ctx.addr);

	if (IS_ENABLED(CONFIG_BLUETOOTH_MESH_PROXY) &&
	    rx->net_if == BT_MESH_NET_IF_PROXY_CFG) {
		return bt_mesh_net_decrypt(enc, buf, BT_MESH_NET_IVI_RX(rx),
					   true);
	}

	return bt_mesh_net_decrypt(enc, buf, BT_MESH_NET_IVI_RX(rx), false);
}

static int net_find_and_decrypt(const u8_t *data, size_t data_len,
				struct bt_mesh_net_rx *rx,
				struct net_buf_simple *buf)
{
	int i;

	BT_DBG("");

	for (i = 0; i < ARRAY_SIZE(bt_mesh.sub); i++) {
		struct bt_mesh_subnet *sub = &bt_mesh.sub[i];
		int err;

		if (sub->net_idx == BT_MESH_KEY_UNUSED) {
			continue;
		}

		err = net_decrypt(sub, 0, data, data_len, rx, buf);
		if (!err) {
			rx->ctx.net_idx = sub->net_idx;
			rx->sub = sub;
			return true;
		}

		if (sub->kr_phase == BT_MESH_KR_NORMAL) {
			continue;
		}

		err = net_decrypt(sub, 1, data, data_len, rx, buf);
		if (!err) {
			rx->ctx.net_idx = sub->net_idx;
			rx->sub = sub;
			rx->new_key = 1;
			return true;
		}
	}

	return false;
}

#if (defined(CONFIG_BLUETOOTH_MESH_RELAY) || \
     defined(CONFIG_BLUETOOTH_MESH_FRIEND) || \
     defined(CONFIG_BLUETOOTH_MESH_GATT_PROXY))
static void bt_mesh_net_relay(struct net_buf_simple *sbuf,
			      struct bt_mesh_net_rx *rx)
{
	const u8_t *enc, *priv;
	struct net_buf *buf;
	u8_t nid, transmit;

	BT_DBG("TTL %u CTL %u dst 0x%04x", rx->ctx.recv_ttl, CTL(sbuf->data),
	       rx->dst);

	if (rx->ctx.recv_ttl <= 1) {
		return;
	}

	transmit = bt_mesh_relay_retransmit_get();
	buf = bt_mesh_adv_create(BT_MESH_ADV_DATA, TRANSMIT_COUNT(transmit),
				 TRANSMIT_INT(transmit), K_NO_WAIT);
	if (!buf) {
		BT_ERR("Out of relay buffers");
		return;
	}

	net_buf_add_mem(buf, sbuf->data, sbuf->len);

	/* Only decrement TTL for non-locally originated packets */
	if (rx->net_if != BT_MESH_NET_IF_LOCAL) {
		/* Leave CTL bit intact */
		buf->data[1] &= 0x80;
		buf->data[1] |= rx->ctx.recv_ttl - 1;
	}

	if (rx->sub->kr_phase == BT_MESH_KR_PHASE_2) {
		if (bt_mesh_friend_dst_is_lpn(rx->dst)) {
			if (friend_cred_get(rx->sub->net_idx,
					    BT_MESH_ADDR_UNASSIGNED,
					    1, &nid, &enc, &priv)) {
				BT_ERR("friend_cred_get failed");
				goto done;
			}
		} else {
			enc = rx->sub->keys[1].enc;
			priv = rx->sub->keys[1].privacy;
			nid = rx->sub->keys[1].nid;
		}
	} else {
		if (bt_mesh_friend_dst_is_lpn(rx->dst)) {
			if (friend_cred_get(rx->sub->net_idx,
					    BT_MESH_ADDR_UNASSIGNED,
					    0, &nid, &enc, &priv)) {
				BT_ERR("friend_cred_get failed");
				goto done;
			}
		} else  {
			enc = rx->sub->keys[0].enc;
			priv = rx->sub->keys[0].privacy;
			nid = rx->sub->keys[0].nid;
		}
	}

	BT_DBG("Relaying packet. TTL is now %u", TTL(buf->data));

	/* Update NID if RX or TX is with friend credentials */
	if (rx->ctx.friend_cred || bt_mesh_friend_dst_is_lpn(rx->dst)) {
		buf->data[0] &= 0x80; /* Clear everything except IVI */
		buf->data[0] |= nid;
	}

	/* We re-encrypt and obfuscate using the received IVI rather than
	 * the normal TX IVI (which may be different) since the transport
	 * layer nonce includes the IVI.
	 */
	if (bt_mesh_net_encrypt(enc, &buf->b, BT_MESH_NET_IVI_RX(rx), false)) {
		BT_ERR("Re-encrypting failed");
		goto done;
	}

	if (bt_mesh_net_obfuscate(buf->data, BT_MESH_NET_IVI_RX(rx), priv)) {
		BT_ERR("Re-obfuscating failed");
		goto done;
	}

	if (IS_ENABLED(CONFIG_BLUETOOTH_MESH_FRIEND)) {
		if (bt_mesh_friend_enqueue(buf, rx->dst) &&
		    BT_MESH_ADDR_IS_UNICAST(rx->dst)) {
			goto done;
		}
	}

	if (IS_ENABLED(CONFIG_BLUETOOTH_MESH_GATT_PROXY)) {
		if (bt_mesh_proxy_relay(&buf->b, rx->dst) &&
			    BT_MESH_ADDR_IS_UNICAST(rx->dst)) {
			goto done;
		}
	}

	if (rx->net_if != BT_MESH_NET_IF_ADV ||
	    bt_mesh_relay_get() == BT_MESH_RELAY_ENABLED) {
		bt_mesh_adv_send(buf, NULL);
	}

done:
	net_buf_unref(buf);
}
#endif /* RELAY || FRIEND || GATT_PROXY */

int bt_mesh_net_decode(struct net_buf_simple *data, enum bt_mesh_net_if net_if,
		       struct bt_mesh_net_rx *rx, struct net_buf_simple *buf,
		       struct net_buf_simple_state *state)
{
	if (data->len < 18) {
		BT_WARN("Dropping too short mesh packet (len %u)", data->len);
		BT_WARN("%s", bt_hex(data->data, data->len));
		return -EINVAL;
	}

	if (net_if == BT_MESH_NET_IF_ADV && check_dup(data)) {
		return -EINVAL;
	}

	BT_DBG("%u bytes: %s", data->len, bt_hex(data->data, data->len));

	rx->net_if = net_if;

	if (net_if == BT_MESH_NET_IF_ADV) {
		rx->hash = msg_hash(data);
	}

	if (!net_find_and_decrypt(data->data, data->len, rx, buf)) {
		BT_DBG("Unable to find matching net for packet");
		return -ENOENT;
	}

	/* Initialize AppIdx to a sane value */
	rx->ctx.app_idx = BT_MESH_KEY_UNUSED;

	/* Save parsing state so the buffer can later be relayed */
	if (state) {
		net_buf_simple_save(buf, state);
	}

	rx->ctx.recv_ttl = TTL(buf->data);

	/* Default to responding with TTL 0 for non-routed messages */
	if (rx->ctx.recv_ttl == 0) {
		rx->ctx.send_ttl = 0;
	} else {
		rx->ctx.send_ttl = BT_MESH_TTL_DEFAULT;
	}

	rx->ctl = CTL(buf->data);
	net_buf_simple_pull(buf, 2); /* SRC, already parsed by net_decrypt() */
	rx->seq = net_seq(buf);
	net_buf_simple_pull(buf, 2);
	rx->dst = net_buf_simple_pull_be16(buf);

	BT_DBG("Decryption successful. Payload len %u", buf->len);

	if (net_if != BT_MESH_NET_IF_LOCAL && bt_mesh_elem_find(rx->ctx.addr)) {
		BT_DBG("Dropping locally originated packet");
		return -EBADMSG;
	}

	if (net_if == BT_MESH_NET_IF_ADV) {
		msg_cache_add(rx->hash);
	}

	BT_DBG("src 0x%04x dst 0x%04x ttl %u", rx->ctx.addr, rx->dst,
	       rx->ctx.recv_ttl);
	BT_DBG("PDU: %s", bt_hex(buf->data, buf->len));

	return 0;
}

void bt_mesh_net_recv(struct net_buf_simple *data, s8_t rssi,
		      enum bt_mesh_net_if net_if)
{
	struct net_buf_simple *buf = NET_BUF_SIMPLE(29);
	struct net_buf_simple_state state;
	struct bt_mesh_net_rx rx;

	BT_DBG("rssi %d net_if %u", rssi, net_if);

	if (!bt_mesh_is_provisioned()) {
		return;
	}

	if (bt_mesh_net_decode(data, net_if, &rx, buf, &state)) {
		return;
	}

	if (IS_ENABLED(CONFIG_BLUETOOTH_MESH_GATT_PROXY) &&
	    net_if == BT_MESH_NET_IF_PROXY) {
		bt_mesh_proxy_addr_add(data, rx.ctx.addr);
	}

	if (bt_mesh_fixed_group_match(rx.dst) || bt_mesh_elem_find(rx.dst)) {
		bt_mesh_trans_recv(buf, &rx);

		if (BT_MESH_ADDR_IS_UNICAST(rx.dst)) {
			return;
		}
	}

#if (defined(CONFIG_BLUETOOTH_MESH_RELAY) || \
     defined(CONFIG_BLUETOOTH_MESH_FRIEND) || \
     defined(CONFIG_BLUETOOTH_MESH_GATT_PROXY))
	net_buf_simple_restore(buf, &state);
	bt_mesh_net_relay(buf, &rx);
#endif /* CONFIG_BLUETOOTH_MESH_RELAY  || FRIEND || GATT_PROXY */
}

static void ivu_complete(struct k_work *work)
{
	BT_DBG("");

	bt_mesh_beacon_ivu_initiator(true);
	bt_mesh_iv_update(bt_mesh.iv_index, false);
}

void bt_mesh_net_init(void)
{
	k_delayed_work_init(&bt_mesh.ivu_complete, ivu_complete);

#if defined(CONFIG_BLUETOOTH_MESH_LOCAL_INTERFACE)
	k_work_init(&bt_mesh.local_work, bt_mesh_net_local);
#endif
}
