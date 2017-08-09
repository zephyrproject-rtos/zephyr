/*  Bluetooth Mesh */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <errno.h>
#include <misc/util.h>

#include <net/buf.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/mesh.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_MESH_DEBUG_BEACON)
#include "common/log.h"

#include "adv.h"
#include "mesh.h"
#include "net.h"
#include "prov.h"
#include "crypto.h"
#include "beacon.h"
#include "foundation.h"

#define UNPROVISIONED_INTERVAL     K_SECONDS(5)
#define PROVISIONED_INTERVAL       K_SECONDS(10)

#define BEACON_TYPE_UNPROVISIONED  0x00
#define BEACON_TYPE_SECURE         0x01

/* 3 transmissions, 20ms interval */
#define UNPROV_XMIT_COUNT          2
#define UNPROV_XMIT_INT            20

/* 1 transmission, 20ms interval */
#define PROV_XMIT_COUNT            0
#define PROV_XMIT_INT              20

static struct k_delayed_work beacon_timer;

static struct {
	u16_t net_idx;
	u8_t  data[21];
} beacon_cache[CONFIG_BT_MESH_SUBNET_COUNT];

static struct bt_mesh_subnet *cache_check(u8_t data[21])
{
	struct bt_mesh_subnet *sub;
	int i;

	for (i = 0; i < ARRAY_SIZE(beacon_cache); i++) {
		if (memcmp(beacon_cache[i].data, data, 21)) {
			continue;
		}

		sub = bt_mesh_subnet_get(beacon_cache[i].net_idx);
		if (sub) {
			BT_DBG("Match found in cache");
			return sub;
		}
	}

	return NULL;
}

static void cache_add(u8_t data[21], u16_t net_idx)
{
	memcpy(beacon_cache[net_idx].data, data, 21);
}

static void beacon_complete(struct net_buf *buf, int err)
{
	struct bt_mesh_subnet *sub;

	BT_DBG("err %d", err);

	sub = &bt_mesh.sub[BT_MESH_ADV(buf)->user_data[0]];
	sub->beacon_sent = k_uptime_get();
}

#define BEACON_INTERVAL(sub) K_SECONDS(10 * ((sub)->beacons_last + 1))

void bt_mesh_beacon_create(struct bt_mesh_subnet *sub,
			   struct net_buf_simple *buf)
{
	struct bt_mesh_subnet_keys *keys;
	u8_t flags;

	net_buf_simple_add_u8(buf, BEACON_TYPE_SECURE);

	if (sub->kr_flag) {
		flags = BT_MESH_NET_FLAG_KR;
		keys = &sub->keys[1];
	} else {
		flags = 0x00;
		keys = &sub->keys[0];
	}

	if (bt_mesh.iv_update) {
		flags |= BT_MESH_NET_FLAG_IVU;
	}

	net_buf_simple_add_u8(buf, flags);

	/* Network ID */
	net_buf_simple_add_mem(buf, keys->net_id, 8);

	/* IV Index */
	net_buf_simple_add_be32(buf, bt_mesh.iv_index);

	net_buf_simple_add_mem(buf, sub->auth, 8);

	BT_DBG("net_idx 0x%04x flags 0x%02x NetID %s", sub->net_idx,
	       flags, bt_hex(keys->net_id, 8));
	BT_DBG("IV Index 0x%08x Auth %s", bt_mesh.iv_index,
	       bt_hex(sub->auth, 8));
}

static int secure_beacon_send(void)
{
	s64_t threshold;
	int i;

	BT_DBG("");

	/* If the interval has passed or is within 5 seconds from now
	 * send a beacon.
	 */
	threshold = k_uptime_get() + K_SECONDS(5);

	for (i = 0; i < ARRAY_SIZE(bt_mesh.sub); i++) {
		struct bt_mesh_subnet *sub = &bt_mesh.sub[i];
		struct net_buf *buf;

		if (sub->net_idx == BT_MESH_KEY_UNUSED) {
			continue;
		}

		if (sub->beacon_sent + BEACON_INTERVAL(sub) > threshold) {
			continue;
		}

		buf = bt_mesh_adv_create(BT_MESH_ADV_BEACON, PROV_XMIT_COUNT,
					 PROV_XMIT_INT, K_NO_WAIT);
		if (!buf) {
			BT_ERR("Unable to allocate beacon buffer");
			return -ENOBUFS;
		}

		BT_MESH_ADV(buf)->user_data[0] = i;

		bt_mesh_beacon_create(sub, &buf->b);

		bt_mesh_adv_send(buf, beacon_complete);
		net_buf_unref(buf);
	}

	return 0;
}

static int unprovisioned_beacon_send(void)
{
#if defined(CONFIG_BT_MESH_PB_ADV)
	struct net_buf *buf;

	BT_DBG("");

	buf = bt_mesh_adv_create(BT_MESH_ADV_BEACON, UNPROV_XMIT_COUNT,
				 UNPROV_XMIT_INT, K_NO_WAIT);
	if (!buf) {
		BT_ERR("Unable to allocate beacon buffer");
		return -ENOBUFS;
	}

	net_buf_add_u8(buf, BEACON_TYPE_UNPROVISIONED);
	net_buf_add_mem(buf, bt_mesh_prov_get_uuid(), 16);

	/* OOB Info (2 bytes) + URI Hash (4 bytes) */
	memset(net_buf_add(buf, 2 + 4), 0, 2 + 4);

	bt_mesh_adv_send(buf, NULL);
	net_buf_unref(buf);

#endif /* CONFIG_BT_MESH_PB_ADV */
	return 0;
}

static void update_beacon_observation(void)
{
	static bool first_half;
	int i;

	/* Observation period is 20 seconds, whereas the beacon timer
	 * runs every 10 seconds. We process what's happened during the
	 * window only after the seconnd half.
	 */
	first_half = !first_half;
	if (first_half) {
		return;
	}

	for (i = 0; i < ARRAY_SIZE(bt_mesh.sub); i++) {
		struct bt_mesh_subnet *sub = &bt_mesh.sub[i];

		if (sub->net_idx == BT_MESH_KEY_UNUSED) {
			continue;
		}

		sub->beacons_last = sub->beacons_cur;
		sub->beacons_cur = 0;
	}
}

static void beacon_send(struct k_work *work)
{
	/* Don't send anything if we have an active provisioning link */
	if (IS_ENABLED(CONFIG_BT_MESH_PROV) && bt_prov_active()) {
		k_delayed_work_submit(&beacon_timer, UNPROVISIONED_INTERVAL);
		return;
	}

	BT_DBG("");

	if (bt_mesh_is_provisioned()) {
		update_beacon_observation();
		secure_beacon_send();

		/* Only resubmit if beaconing is still enabled */
		if (bt_mesh_beacon_get() == BT_MESH_BEACON_ENABLED ||
		    bt_mesh.ivu_initiator) {
			k_delayed_work_submit(&beacon_timer,
					      PROVISIONED_INTERVAL);
		}
	} else {
		unprovisioned_beacon_send();
		k_delayed_work_submit(&beacon_timer, UNPROVISIONED_INTERVAL);
	}

}

static void secure_beacon_recv(struct net_buf_simple *buf)
{
	u8_t *data, *net_id, *auth;
	struct bt_mesh_subnet *sub;
	u32_t iv_index;
	u8_t flags;
	bool new_key;

	if (buf->len < 21) {
		BT_ERR("Too short secure beacon (len %u)", buf->len);
		return;
	}

	sub = cache_check(buf->data);
	if (sub) {
		/* We've seen this beacon before - just update the stats */
		goto update_stats;
	}

	/* So we can add to the cache if auth matches */
	data = buf->data;

	flags = net_buf_simple_pull_u8(buf);
	net_id = buf->data;
	net_buf_simple_pull(buf, 8);
	iv_index = net_buf_simple_pull_be32(buf);
	auth = buf->data;

	BT_DBG("flags 0x%02x id %s iv_index 0x%08x",
	       flags, bt_hex(net_id, 8), iv_index);

	sub = bt_mesh_subnet_find(net_id, flags, iv_index, auth, &new_key);
	if (!sub) {
		BT_DBG("No subnet that matched beacon");
		return;
	}

	if (sub->kr_phase == BT_MESH_KR_PHASE_2 && !new_key) {
		BT_WARN("Ignoring Phase 2 KR Update secured using old key");
		return;
	}

	cache_add(data, sub->net_idx);

	/* If we have NetKey0 accept initiation only from it */
	if (bt_mesh_subnet_get(BT_MESH_KEY_PRIMARY) &&
	    sub->net_idx != BT_MESH_KEY_PRIMARY) {
		BT_WARN("Ignoring secure beacon on non-primary subnet");
		goto update_stats;
	}

	BT_DBG("net_idx 0x%04x iv_index 0x%08x, current iv_index 0x%08x",
	       sub->net_idx, iv_index, bt_mesh.iv_index);

	if (bt_mesh.ivu_initiator &&
	    bt_mesh.iv_update == BT_MESH_IV_UPDATE(flags)) {
		bt_mesh_beacon_ivu_initiator(false);
	}

	bt_mesh_iv_update(iv_index, BT_MESH_IV_UPDATE(flags));

	if (bt_mesh_kr_update(sub, BT_MESH_KEY_REFRESH(flags), new_key)) {
		bt_mesh_net_beacon_update(sub);
	}

update_stats:
	if (bt_mesh_beacon_get() == BT_MESH_BEACON_ENABLED &&
	    sub->beacons_cur < 0xff) {
		sub->beacons_cur++;
	}
}

void bt_mesh_beacon_recv(struct net_buf_simple *buf)
{
	u8_t type;

	BT_DBG("%u bytes: %s", buf->len, bt_hex(buf->data, buf->len));

	if (buf->len < 1) {
		BT_ERR("Too short beacon");
		return;
	}

	type = net_buf_simple_pull_u8(buf);
	switch (type) {
	case BEACON_TYPE_UNPROVISIONED:
		BT_DBG("Ignoring unprovisioned device beacon");
		break;
	case BEACON_TYPE_SECURE:
		secure_beacon_recv(buf);
		break;
	default:
		BT_WARN("Unknown beacon type 0x%02x", type);
		break;
	}
}

void bt_mesh_beacon_init(void)
{
	k_delayed_work_init(&beacon_timer, beacon_send);

	/* Start beaconing since we're unprovisioned */
	k_work_submit(&beacon_timer.work);
}

void bt_mesh_beacon_ivu_initiator(bool enable)
{
	bt_mesh.ivu_initiator = enable;

	if (enable) {
		k_work_submit(&beacon_timer.work);
	} else if (bt_mesh_beacon_get() == BT_MESH_BEACON_DISABLED) {
		k_delayed_work_cancel(&beacon_timer);
	}
}

void bt_mesh_beacon_enable(void)
{
	int i;

	if (!bt_mesh_is_provisioned()) {
		k_work_submit(&beacon_timer.work);
		return;
	}

	for (i = 0; i < ARRAY_SIZE(bt_mesh.sub); i++) {
		struct bt_mesh_subnet *sub = &bt_mesh.sub[i];

		if (sub->net_idx == BT_MESH_KEY_UNUSED) {
			continue;
		}

		sub->beacons_last = 0;
		sub->beacons_cur = 0;

		bt_mesh_net_beacon_update(sub);
	}

	k_work_submit(&beacon_timer.work);
}

void bt_mesh_beacon_disable(void)
{
	if (!bt_mesh.ivu_initiator) {
		k_delayed_work_cancel(&beacon_timer);
	}
}
