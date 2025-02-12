/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <errno.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/net/buf.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/mesh.h>

#include "common/bt_str.h"

#include "mesh.h"
#include "net.h"
#include "prov.h"
#include "crypto.h"
#include "beacon.h"
#include "cfg.h"

#define LOG_LEVEL CONFIG_BT_MESH_BEACON_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_beacon);

#define PROVISIONED_INTERVAL       K_SECONDS(10)

#define BEACON_TYPE_UNPROVISIONED  0x00
#define BEACON_TYPE_SECURE         0x01
#define BEACON_TYPE_PRIVATE        0x02

/* 3 transmissions, 20ms interval */
#define UNPROV_XMIT                BT_MESH_TRANSMIT(2, 20)

/* 1 transmission, 20ms interval */
#define PROV_XMIT                  BT_MESH_TRANSMIT(0, 20)

static struct k_work_delayable beacon_timer;
static struct bt_mesh_subnet *beacon_send_sub_curr;

#if defined(CONFIG_BT_MESH_PRIV_BEACONS)
static struct {
	/**
	 * Identifier for the current Private beacon random-value.
	 * Each time we regenerate the random-value, we'll update this idx.
	 * Whenever it's time for a subnet to create a beacon, it'll compare
	 * the subnet's beacon idx to determine whether the random value has
	 * changed since the last beacon was sent. If this is the case, we'll
	 * regenerate the beacon based on the new random value.
	 */
	uint16_t idx;
	uint8_t val[13];
	uint64_t timestamp;
} priv_random;
#endif

struct beacon_params {
	bool private;
	union {
		const uint8_t *net_id;
		struct {
			const uint8_t *data;
			const uint8_t *random;
		};
	};
	const uint8_t *auth;
	uint32_t iv_index;
	uint8_t flags;

	bool new_key;
};

#if defined(CONFIG_BT_MESH_PRIV_BEACONS)
static int private_beacon_create(struct bt_mesh_subnet *sub,
				 struct net_buf_simple *buf);
static int private_beacon_update(struct bt_mesh_subnet *sub);
#endif

static struct bt_mesh_beacon *subnet_beacon_get_by_type(struct bt_mesh_subnet *sub, bool priv)
{
#if defined(CONFIG_BT_MESH_PRIV_BEACONS)
	return priv ? &sub->priv_beacon : &sub->secure_beacon;
#else
	return &sub->secure_beacon;
#endif
}

static bool beacon_cache_match(struct bt_mesh_subnet *sub, void *data)
{
	struct beacon_params *params;
	struct bt_mesh_beacon *beacon;

	params = data;
	beacon = subnet_beacon_get_by_type(sub, params->private);

	return !memcmp(beacon->cache, params->auth, sizeof(beacon->cache));
}

static void cache_add(const uint8_t auth[8], struct bt_mesh_beacon *beacon)
{
	memcpy(beacon->cache, auth, sizeof(beacon->cache));
}

void bt_mesh_beacon_cache_clear(struct bt_mesh_subnet *sub)
{
	(void)memset(sub->secure_beacon.cache, 0, sizeof(sub->secure_beacon.cache));
#if defined(CONFIG_BT_MESH_PRIV_BEACONS)
	(void)memset(sub->priv_beacon.cache, 0, sizeof(sub->priv_beacon.cache));
#endif
}

static void beacon_start(uint16_t duration, int err, void *user_data)
{
	if (err) {
		LOG_ERR("Failed to send beacon: err %d", err);
		if (beacon_send_sub_curr) {
			k_work_reschedule(&beacon_timer, K_NO_WAIT);
		}
	}
}

static void beacon_complete(int err, void *user_data)
{
	struct bt_mesh_beacon *beacon = user_data;

	LOG_DBG("err %d", err);
	beacon->sent = k_uptime_get_32();

	if (beacon_send_sub_curr) {
		k_work_reschedule(&beacon_timer, K_MSEC(20));
	}
}

static int secure_beacon_create(struct bt_mesh_subnet *sub,
				struct net_buf_simple *buf)
{
	uint8_t flags = bt_mesh_net_flags(sub);
	struct bt_mesh_subnet_keys *keys;

	net_buf_simple_add_u8(buf, BEACON_TYPE_SECURE);

	keys = &sub->keys[SUBNET_KEY_TX_IDX(sub)];

	net_buf_simple_add_u8(buf, flags);

	/* Network ID */
	net_buf_simple_add_mem(buf, keys->net_id, 8);

	/* IV Index */
	net_buf_simple_add_be32(buf, bt_mesh.iv_index);

	net_buf_simple_add_mem(buf, sub->secure_beacon.auth, 8);

	LOG_DBG("net_idx 0x%04x flags 0x%02x NetID %s", sub->net_idx, flags,
		bt_hex(keys->net_id, 8));
	LOG_DBG("IV Index 0x%08x Auth %s", bt_mesh.iv_index, bt_hex(sub->secure_beacon.auth, 8));

	return 0;
}

#if defined(CONFIG_BT_MESH_PRIV_BEACONS)
static int private_random_update(void)
{
	uint8_t interval = bt_mesh_priv_beacon_update_interval_get();
	uint64_t uptime = k_uptime_get();
	int err;

	/* The Private beacon random value should change every N seconds to maintain privacy.
	 * N = (10 * interval) seconds, or on every beacon creation, if the interval is 0.
	 */
	if (bt_mesh_priv_beacon_get() == BT_MESH_FEATURE_ENABLED &&
	    interval &&
	    uptime - priv_random.timestamp < (10 * interval * MSEC_PER_SEC) &&
	    priv_random.timestamp != 0) {
		/* Not time yet */
		return 0;
	}

	err = bt_rand(priv_random.val, sizeof(priv_random.val));
	if (err) {
		return err;
	}

	/* Update the index to indicate to all subnets that the private beacon must be regenerated.
	 * Each subnet maintains the random index their private beacon data was generated with.
	 */
	priv_random.idx++;
	priv_random.timestamp = uptime;

	return 0;
}

static int private_beacon_update(struct bt_mesh_subnet *sub)
{
	struct bt_mesh_subnet_keys *keys = &sub->keys[SUBNET_KEY_TX_IDX(sub)];
	uint8_t flags = bt_mesh_net_flags(sub);
	int err;

	err = bt_mesh_beacon_encrypt(&keys->priv_beacon, flags, bt_mesh.iv_index,
				     priv_random.val, sub->priv_beacon_ctx.data,
				     sub->priv_beacon.auth);
	if (err) {
		LOG_ERR("Can't encrypt private beacon");
		return err;
	}

	sub->priv_beacon_ctx.idx = priv_random.idx;
	return 0;
}

static int private_beacon_create(struct bt_mesh_subnet *sub,
				 struct net_buf_simple *buf)
{
	int err;

	/* Refresh beacon data */
	err = private_random_update();
	if (err) {
		return err;
	}

	if (sub->priv_beacon_ctx.idx != priv_random.idx) {
		err = private_beacon_update(sub);
		if (err) {
			return err;
		}
	}

	net_buf_simple_add_u8(buf, BEACON_TYPE_PRIVATE);
	net_buf_simple_add_mem(buf, priv_random.val, 13);
	net_buf_simple_add_mem(buf, sub->priv_beacon_ctx.data, 5);
	net_buf_simple_add_mem(buf, sub->priv_beacon.auth, 8);

	LOG_DBG("0x%03x", sub->net_idx);
	return 0;
}
#endif

int bt_mesh_beacon_create(struct bt_mesh_subnet *sub, struct net_buf_simple *buf, bool priv)
{
#if defined(CONFIG_BT_MESH_PRIV_BEACONS)
	if (priv) {
		return private_beacon_create(sub, buf);
	}
#endif

	secure_beacon_create(sub, buf);
	return 0;
}

/* If the interval has passed or is within 5 seconds from now send a beacon */
#define BEACON_THRESHOLD(beacon) \
	((10 * ((beacon)->last + 1)) * MSEC_PER_SEC - (5 * MSEC_PER_SEC))

static bool secure_beacon_is_running(void)
{
	return bt_mesh_beacon_enabled() ||
	       atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_INITIATOR);
}

static int net_beacon_send(struct bt_mesh_subnet *sub, struct bt_mesh_beacon *beacon,
			    int (*beacon_create)(struct bt_mesh_subnet *sub,
						 struct net_buf_simple *buf))
{
	static const struct bt_mesh_send_cb send_cb = {
		.start = beacon_start,
		.end = beacon_complete,
	};
	uint32_t now = k_uptime_get_32();
	struct bt_mesh_adv *adv;
	uint32_t time_diff;
	uint32_t time_since_last_recv;
	int err;

	LOG_DBG("");

	time_diff = now - beacon->sent;
	time_since_last_recv = now - beacon->recv;
	if (time_diff < (600 * MSEC_PER_SEC) &&
		(time_diff < BEACON_THRESHOLD(beacon) ||
		 time_since_last_recv < (10 * MSEC_PER_SEC))) {
		return -ENOMSG;
	}

	adv = bt_mesh_adv_create(BT_MESH_ADV_BEACON, BT_MESH_ADV_TAG_LOCAL,
				 PROV_XMIT, K_NO_WAIT);
	if (!adv) {
		LOG_ERR("Unable to allocate beacon adv");
		return -ENOMEM; /* Bail out */
	}

	err = beacon_create(sub, &adv->b);
	if (!err) {
		bt_mesh_adv_send(adv, &send_cb, beacon);
	}

	bt_mesh_adv_unref(adv);

	return err;
}

static int net_beacon_for_subnet_send(struct bt_mesh_subnet *sub)
{
	int err = -ENOMSG;

	struct {
		struct bt_mesh_beacon *beacon;
		bool enabled;
		int (*create_fn)(struct bt_mesh_subnet *sub, struct net_buf_simple *buf);
	} beacons[] = {
		[0] = {
			.beacon = &sub->secure_beacon,
			.enabled = secure_beacon_is_running(),
			.create_fn = secure_beacon_create,
		},
#if defined(CONFIG_BT_MESH_PRIV_BEACONS)
		[1] = {
			.beacon = &sub->priv_beacon,
			.enabled = bt_mesh_priv_beacon_get() == BT_MESH_FEATURE_ENABLED,
			.create_fn = private_beacon_create,
		},
#endif
	};

	for (int i = 0; i < ARRAY_SIZE(beacons); i++) {
		if (!beacons[i].enabled) {
			continue;
		}

		err = net_beacon_send(sub, beacons[i].beacon, beacons[i].create_fn);
		if (err < 0) {
			/* Bail out */
			break;
		}
	}

	return err;
}

static int unprovisioned_beacon_send(void)
{
	const struct bt_mesh_prov *prov;
	uint8_t uri_hash[16] = { 0 };
	struct bt_mesh_adv *adv;
	uint16_t oob_info;

	LOG_DBG("");

	adv = bt_mesh_adv_create(BT_MESH_ADV_BEACON, BT_MESH_ADV_TAG_LOCAL,
				 UNPROV_XMIT, K_NO_WAIT);
	if (!adv) {
		LOG_ERR("Unable to allocate beacon adv");
		return -ENOBUFS;
	}

	prov = bt_mesh_prov_get();

	net_buf_simple_add_u8(&adv->b, BEACON_TYPE_UNPROVISIONED);
	net_buf_simple_add_mem(&adv->b, prov->uuid, 16);

	if (prov->uri && bt_mesh_s1_str(prov->uri, uri_hash) == 0) {
		oob_info = prov->oob_info | BT_MESH_PROV_OOB_URI;
	} else {
		oob_info = prov->oob_info;
	}

	net_buf_simple_add_be16(&adv->b, oob_info);
	net_buf_simple_add_mem(&adv->b, uri_hash, 4);

	bt_mesh_adv_send(adv, NULL, NULL);
	bt_mesh_adv_unref(adv);

	if (prov->uri) {
		size_t len;

		adv = bt_mesh_adv_create(BT_MESH_ADV_URI, BT_MESH_ADV_TAG_LOCAL,
					 UNPROV_XMIT, K_NO_WAIT);
		if (!adv) {
			LOG_ERR("Unable to allocate URI adv");
			return -ENOBUFS;
		}

		len = strlen(prov->uri);
		if (net_buf_simple_tailroom(&adv->b) < len) {
			LOG_WRN("Too long URI to fit advertising data");
		} else {
			net_buf_simple_add_mem(&adv->b, prov->uri, len);
			bt_mesh_adv_send(adv, NULL, NULL);
		}

		bt_mesh_adv_unref(adv);
	}

	return 0;
}

static void unprovisioned_beacon_recv(struct net_buf_simple *buf)
{
	const struct bt_mesh_prov *prov;
	uint8_t *uuid;
	uint16_t oob_info;
	uint32_t uri_hash_val;
	uint32_t *uri_hash = NULL;

	prov = bt_mesh_prov_get();

	if (!prov->unprovisioned_beacon) {
		return;
	}

	if (buf->len != 18 && buf->len != 22) {
		LOG_ERR("Invalid unprovisioned beacon length (%u)", buf->len);
		return;
	}

	uuid = net_buf_simple_pull_mem(buf, 16);
	oob_info = net_buf_simple_pull_be16(buf);

	if (buf->len == 4) {
		uri_hash_val = net_buf_simple_pull_be32(buf);
		uri_hash = &uri_hash_val;
	}

	LOG_DBG("uuid %s", bt_hex(uuid, 16));

	prov->unprovisioned_beacon(uuid,
				   (bt_mesh_prov_oob_info_t)oob_info,
				   uri_hash);
}

static void sub_update_beacon_observation(struct bt_mesh_subnet *sub)
{
	sub->secure_beacon.last = sub->secure_beacon.cur;
	sub->secure_beacon.cur = 0U;

#if defined(CONFIG_BT_MESH_PRIV_BEACONS)
	sub->priv_beacon.last = sub->priv_beacon.cur;
	sub->priv_beacon.cur = 0U;
#endif
}

static void update_beacon_observation(void)
{
	static bool first_half;

	/* Observation period is 20 seconds, whereas the beacon timer
	 * runs every 10 seconds. We process what's happened during the
	 * window only after the second half.
	 */
	first_half = !first_half;
	if (first_half) {
		return;
	}

	bt_mesh_subnet_foreach(sub_update_beacon_observation);
}

static bool net_beacon_is_running(void)
{
	return secure_beacon_is_running() ||
	       (bt_mesh_priv_beacon_get() == BT_MESH_FEATURE_ENABLED);
}

static bool beacons_send_next(void)
{
	int err;
	struct bt_mesh_subnet *sub_first = bt_mesh_subnet_next(NULL);
	struct bt_mesh_subnet *sub_next;

	do {
		sub_next = bt_mesh_subnet_next(beacon_send_sub_curr);
		if (sub_next == sub_first && beacon_send_sub_curr != NULL) {
			beacon_send_sub_curr = NULL;
			return false;
		}

		beacon_send_sub_curr = sub_next;
		err = net_beacon_for_subnet_send(beacon_send_sub_curr);
		if (err < 0 && (err != -ENOMSG)) {
			LOG_ERR("Failed to advertise subnet %d: err %d",
				beacon_send_sub_curr->net_idx, err);
		}
	} while (err);

	return true;
}

static void beacon_send(struct k_work *work)
{
	LOG_DBG("");

	if (bt_mesh_is_provisioned()) {
		if (!net_beacon_is_running()) {
			return;
		}

		if (!beacon_send_sub_curr) {
			update_beacon_observation();
		}

		if (!beacons_send_next()) {
			k_work_schedule(&beacon_timer, PROVISIONED_INTERVAL);
		}

		return;
	}

	if (IS_ENABLED(CONFIG_BT_MESH_PB_ADV)) {
		/* Don't send anything if we have an active provisioning link */
		if (!bt_mesh_prov_active()) {
			unprovisioned_beacon_send();
		}

		k_work_schedule(&beacon_timer, K_SECONDS(CONFIG_BT_MESH_UNPROV_BEACON_INT));
	}
}

static bool auth_match(struct bt_mesh_subnet_keys *keys,
		       const struct beacon_params *params)
{
	uint8_t net_auth[8];

	if (memcmp(params->net_id, keys->net_id, 8)) {
		return false;
	}

	if (bt_mesh_beacon_auth(&keys->beacon, params->flags, keys->net_id, params->iv_index,
				net_auth)) {
		return false;
	}

	if (memcmp(params->auth, net_auth, 8)) {
		LOG_WRN("Invalid auth value. Received auth: %s", bt_hex(params->auth, 8));
		LOG_WRN("Calculated auth: %s", bt_hex(net_auth, 8));
		return false;
	}

	return true;
}

static bool secure_beacon_authenticate(struct bt_mesh_subnet *sub, void *cb_data)
{
	struct beacon_params *params = cb_data;

	for (int i = 0; i < ARRAY_SIZE(sub->keys); i++) {
		if (sub->keys[i].valid && auth_match(&sub->keys[i], params)) {
			params->new_key = (i > 0);
#if defined(CONFIG_BT_TESTING)
			struct bt_mesh_snb beacon_info;

			beacon_info.flags = params->flags;
			memcpy(&beacon_info.net_id, params->net_id, 8);
			beacon_info.iv_idx = params->iv_index;
			memcpy(&beacon_info.auth_val, params->auth, 8);

			STRUCT_SECTION_FOREACH(bt_mesh_beacon_cb, cb) {
				if (cb->snb_received) {
					cb->snb_received(&beacon_info);
				}
			}
#endif

			return true;
		}
	}

	return false;
}

static bool priv_beacon_decrypt(struct bt_mesh_subnet *sub, void *cb_data)
{
	struct beacon_params *params = cb_data;
	uint8_t out[5];
	int err;

	for (int i = 0; i < ARRAY_SIZE(sub->keys); i++) {
		if (!sub->keys[i].valid) {
			continue;
		}

		err = bt_mesh_beacon_decrypt(&sub->keys[i].priv_beacon, params->random,
					     params->data, params->auth, out);
		if (!err) {
			params->new_key = (i > 0);
			params->flags = out[0];
			params->iv_index = sys_get_be32(&out[1]);

#if defined(CONFIG_BT_TESTING)
			struct bt_mesh_prb beacon_info;

			memcpy(beacon_info.random, params->random, 13);
			beacon_info.flags = params->flags;
			beacon_info.iv_idx = params->iv_index;
			memcpy(&beacon_info.auth_tag, params->auth, 8);

			STRUCT_SECTION_FOREACH(bt_mesh_beacon_cb, cb) {
				if (cb->priv_received) {
					cb->priv_received(&beacon_info);
				}
			}
#endif
			return true;
		}
	}

	return false;
}

static void net_beacon_register(struct bt_mesh_beacon *beacon, bool priv)
{
	if (((priv && bt_mesh_priv_beacon_get() == BT_MESH_PRIV_GATT_PROXY_ENABLED) ||
	     bt_mesh_beacon_enabled()) && beacon->cur < 0xff) {
		beacon->cur++;
		beacon->recv = k_uptime_get_32();
	}
}

static void net_beacon_recv(struct bt_mesh_subnet *sub,
			    const struct beacon_params *params)
{
	bt_mesh_kr_update(sub, BT_MESH_KEY_REFRESH(params->flags),
			  params->new_key);

	/* If we have NetKey0 accept IV index initiation only from it */
	if (bt_mesh_subnet_get(BT_MESH_KEY_PRIMARY) &&
	    sub->net_idx != BT_MESH_KEY_PRIMARY) {
		LOG_WRN("Ignoring secure beacon on non-primary subnet");
		return;
	}

	LOG_DBG("net_idx 0x%04x flags %u iv_index 0x%08x, "
		"current iv_index 0x%08x",
		sub->net_idx, params->flags, params->iv_index, bt_mesh.iv_index);

	if (atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_INITIATOR) &&
	    (atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_IN_PROGRESS) ==
	     BT_MESH_IV_UPDATE(params->flags))) {
		bt_mesh_beacon_ivu_initiator(false);
	}

	bt_mesh_net_iv_update(params->iv_index,
			      BT_MESH_IV_UPDATE(params->flags));
}

static void net_beacon_resolve(struct beacon_params *params,
			       bool (*matcher)(struct bt_mesh_subnet *sub,
					       void *cb_data))
{
	struct bt_mesh_subnet *sub;
	struct bt_mesh_beacon *beacon;

	sub = bt_mesh_subnet_find(beacon_cache_match, (void *)params);
	if (sub) {
		beacon = subnet_beacon_get_by_type(sub, params->private);

		/* We've seen this beacon before - just update the stats */
		net_beacon_register(beacon, params->private);
		return;
	}

	sub = bt_mesh_subnet_find(matcher, params);
	if (!sub) {
		LOG_DBG("No subnet that matched beacon");
		return;
	}

	if (sub->kr_phase == BT_MESH_KR_PHASE_2 && !params->new_key) {
		LOG_WRN("Ignoring Phase 2 KR Update secured using old key");
		return;
	}

	beacon = subnet_beacon_get_by_type(sub, params->private);

	cache_add(params->auth, beacon);

	net_beacon_recv(sub, params);
	net_beacon_register(beacon, params->private);
}

static void secure_beacon_recv(struct net_buf_simple *buf)
{
	struct beacon_params params;

	if (buf->len < 21) {
		LOG_ERR("Too short secure beacon (len %u)", buf->len);
		return;
	}

	params.private = false;
	params.flags = net_buf_simple_pull_u8(buf);
	params.net_id = net_buf_simple_pull_mem(buf, 8);
	params.iv_index = net_buf_simple_pull_be32(buf);
	params.auth = buf->data;

	net_beacon_resolve(&params, secure_beacon_authenticate);
}

static void private_beacon_recv(struct net_buf_simple *buf)
{
	struct beacon_params params;

	if (buf->len < 26) {
		LOG_ERR("Too short private beacon (len %u)", buf->len);
		return;
	}

	params.private = true;
	params.random = net_buf_simple_pull_mem(buf, 13);
	params.data = net_buf_simple_pull_mem(buf, 5);
	params.auth = buf->data;

	net_beacon_resolve(&params, priv_beacon_decrypt);
}

void bt_mesh_beacon_recv(struct net_buf_simple *buf)
{
	uint8_t type;

	LOG_DBG("%u bytes: %s", buf->len, bt_hex(buf->data, buf->len));

	if (buf->len < 1) {
		LOG_ERR("Too short beacon");
		return;
	}

	type = net_buf_simple_pull_u8(buf);
	switch (type) {
	case BEACON_TYPE_UNPROVISIONED:
		if (IS_ENABLED(CONFIG_BT_MESH_PB_ADV)) {
			unprovisioned_beacon_recv(buf);
		}
		break;
	case BEACON_TYPE_SECURE:
		secure_beacon_recv(buf);
		break;
	case BEACON_TYPE_PRIVATE:
		private_beacon_recv(buf);
		break;
	default:
		LOG_WRN("Unknown beacon type 0x%02x", type);
		break;
	}
}

void bt_mesh_beacon_update(struct bt_mesh_subnet *sub)
{
	uint8_t flags = bt_mesh_net_flags(sub);
	struct bt_mesh_subnet_keys *keys;

	keys = &sub->keys[SUBNET_KEY_TX_IDX(sub)];

	LOG_DBG("NetIndex 0x%03x Using %s key", sub->net_idx,
		SUBNET_KEY_TX_IDX(sub) ? "new" : "current");
	LOG_DBG("flags 0x%02x, IVI 0x%08x", flags, bt_mesh.iv_index);

#if defined(CONFIG_BT_MESH_PRIV_BEACONS)
	/* Invalidate private beacon to force regeneration: */
	sub->priv_beacon_ctx.idx = priv_random.idx - 1;
	priv_random.timestamp = 0;
#endif

	bt_mesh_beacon_auth(&keys->beacon, flags, keys->net_id, bt_mesh.iv_index,
			    sub->secure_beacon.auth);
}

static void subnet_evt(struct bt_mesh_subnet *sub, enum bt_mesh_key_evt evt)
{
	if (evt != BT_MESH_KEY_DELETED) {
		bt_mesh_beacon_update(sub);
	}
}

BT_MESH_SUBNET_CB_DEFINE(beacon) = {
	.evt_handler = subnet_evt,
};

void bt_mesh_beacon_init(void)
{
	k_work_init_delayable(&beacon_timer, beacon_send);

#if defined(CONFIG_BT_MESH_PRIV_BEACONS)
	private_random_update();
#endif
}

void bt_mesh_beacon_ivu_initiator(bool enable)
{
	atomic_set_bit_to(bt_mesh.flags, BT_MESH_IVU_INITIATOR, enable);

	/* Fire the beacon handler straight away if it's not already pending -
	 * in which case we'll fire according to the ongoing periodic sending.
	 * If beacons are disabled, the handler will exit early.
	 *
	 * An alternative solution would be to check whether beacons are enabled
	 * here, and cancel if not. As the cancel operation may fail, we would
	 * still have to implement an early exit mechanism, so we might as well
	 * just use this every time.
	 */
	beacon_send_sub_curr = NULL;
	k_work_schedule(&beacon_timer, K_NO_WAIT);
}

static void subnet_beacon_enable(struct bt_mesh_subnet *sub)
{
	sub->secure_beacon.last = 0U;
	sub->secure_beacon.cur = 0U;

#if defined(CONFIG_BT_MESH_PRIV_BEACONS)
	sub->priv_beacon.last = 0U;
	sub->priv_beacon.cur = 0U;
#endif

	bt_mesh_beacon_update(sub);
}

void bt_mesh_beacon_enable(void)
{
	if (bt_mesh_is_provisioned()) {
		bt_mesh_subnet_foreach(subnet_beacon_enable);
	}

	beacon_send_sub_curr = NULL;
	k_work_reschedule(&beacon_timer, K_NO_WAIT);
}

void bt_mesh_beacon_disable(void)
{
	/* If this fails, we'll do an early exit in the work handler. */
	beacon_send_sub_curr = NULL;
	(void)k_work_cancel_delayable(&beacon_timer);
}
