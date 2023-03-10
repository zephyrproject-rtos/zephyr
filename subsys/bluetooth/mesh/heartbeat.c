/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/bluetooth/mesh.h>
#include "net.h"
#include "rpl.h"
#include "access.h"
#include "lpn.h"
#include "settings.h"
#include "mesh.h"
#include "transport.h"
#include "heartbeat.h"
#include "foundation.h"

#define LOG_LEVEL CONFIG_BT_MESH_TRANS_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_hb);

/* Heartbeat Publication information for persistent storage. */
struct hb_pub_val {
	uint16_t dst;
	uint8_t  period;
	uint8_t  ttl;
	uint16_t feat;
	uint16_t net_idx:12,
		 indefinite:1;
};

static struct bt_mesh_hb_pub pub;
static struct bt_mesh_hb_sub sub;
static struct k_work_delayable sub_timer;
static struct k_work_delayable pub_timer;

static void notify_pub_sent(void)
{
	STRUCT_SECTION_FOREACH(bt_mesh_hb_cb, cb) {
		if (cb->pub_sent) {
			cb->pub_sent(&pub);
		}
	}
}

static int64_t sub_remaining(void)
{
	if (sub.dst == BT_MESH_ADDR_UNASSIGNED) {
		return 0U;
	}

	uint32_t rem_ms = k_ticks_to_ms_floor32(
		k_work_delayable_remaining_get(&sub_timer));

	return rem_ms / MSEC_PER_SEC;
}

static void hb_publish_end_cb(int err, void *cb_data)
{
	if (pub.period && pub.count > 1) {
		k_work_reschedule(&pub_timer, K_SECONDS(pub.period));
	}

	if (pub.count != 0xffff) {
		pub.count--;
	}

	if (!err) {
		notify_pub_sent();
	}
}

static void notify_recv(uint8_t hops, uint16_t feat)
{
	sub.remaining = sub_remaining();

	STRUCT_SECTION_FOREACH(bt_mesh_hb_cb, cb) {
		if (cb->recv) {
			cb->recv(&sub, hops, feat);
		}
	}
}

static void notify_sub_end(void)
{
	sub.remaining = 0;

	STRUCT_SECTION_FOREACH(bt_mesh_hb_cb, cb) {
		if (cb->sub_end) {
			cb->sub_end(&sub);
		}
	}
}

static void sub_end(struct k_work *work)
{
	notify_sub_end();
}

static int heartbeat_send(const struct bt_mesh_send_cb *cb, void *cb_data)
{
	uint16_t feat = 0U;
	struct __packed {
		uint8_t init_ttl;
		uint16_t feat;
	} hb;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = pub.net_idx,
		.app_idx = BT_MESH_KEY_UNUSED,
		.addr = pub.dst,
		.send_ttl = pub.ttl,
	};
	struct bt_mesh_net_tx tx = {
		.sub = bt_mesh_subnet_get(pub.net_idx),
		.ctx = &ctx,
		.src = bt_mesh_primary_addr(),
		.xmit = bt_mesh_net_transmit_get(),
	};

	/* Do nothing if heartbeat publication is not enabled or the subnet is
	 * removed.
	 */
	if (!tx.sub || pub.dst == BT_MESH_ADDR_UNASSIGNED) {
		return 0;
	}

	hb.init_ttl = pub.ttl;

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

	LOG_DBG("InitTTL %u feat 0x%04x", pub.ttl, feat);

	return bt_mesh_ctl_send(&tx, TRANS_CTL_OP_HEARTBEAT, &hb, sizeof(hb),
				cb, cb_data);
}

static void hb_publish_start_cb(uint16_t duration, int err, void *cb_data)
{
	if (err) {
		hb_publish_end_cb(err, cb_data);
	}
}

static void hb_publish(struct k_work *work)
{
	static const struct bt_mesh_send_cb publish_cb = {
		.start = hb_publish_start_cb,
		.end = hb_publish_end_cb,
	};
	struct bt_mesh_subnet *sub;
	int err;

	LOG_DBG("hb_pub.count: %u", pub.count);

	/* Fast exit if disabled or expired */
	if (pub.period == 0U || pub.count == 0U) {
		return;
	}

	sub = bt_mesh_subnet_get(pub.net_idx);
	if (!sub) {
		LOG_ERR("No matching subnet for idx 0x%02x", pub.net_idx);
		pub.dst = BT_MESH_ADDR_UNASSIGNED;
		return;
	}

	err = heartbeat_send(&publish_cb, NULL);
	if (err) {
		hb_publish_end_cb(err, NULL);
	}
}

int bt_mesh_hb_recv(struct bt_mesh_net_rx *rx, struct net_buf_simple *buf)
{
	uint8_t init_ttl, hops;
	uint16_t feat;

	if (buf->len < 3) {
		LOG_ERR("Too short heartbeat message");
		return -EINVAL;
	}

	init_ttl = (net_buf_simple_pull_u8(buf) & 0x7f);
	feat = net_buf_simple_pull_be16(buf);

	hops = (init_ttl - rx->ctx.recv_ttl + 1);

	if (rx->ctx.addr != sub.src || rx->ctx.recv_dst != sub.dst) {
		LOG_DBG("No subscription for received heartbeat");
		return 0;
	}

	if (!k_work_delayable_is_pending(&sub_timer)) {
		LOG_DBG("Heartbeat subscription inactive");
		return 0;
	}

	sub.min_hops = MIN(sub.min_hops, hops);
	sub.max_hops = MAX(sub.max_hops, hops);

	if (sub.count < 0xffff) {
		sub.count++;
	}

	LOG_DBG("src 0x%04x TTL %u InitTTL %u (%u hop%s) feat 0x%04x", rx->ctx.addr,
		rx->ctx.recv_ttl, init_ttl, hops, (hops == 1U) ? "" : "s", feat);

	notify_recv(hops, feat);

	return 0;
}

static void pub_disable(void)
{
	LOG_DBG("");

	pub.dst = BT_MESH_ADDR_UNASSIGNED;
	pub.count = 0U;
	pub.period = 0U;
	pub.ttl = 0U;
	pub.feat = 0U;
	pub.net_idx = 0U;

	/* Try to cancel, but it's OK if this still runs (or is
	 * running) as the handler will be a no-op if it hasn't
	 * already checked period for being non-zero.
	 */
	(void)k_work_cancel_delayable(&pub_timer);
}

uint8_t bt_mesh_hb_pub_set(struct bt_mesh_hb_pub *new_pub)
{
	if (!new_pub || new_pub->dst == BT_MESH_ADDR_UNASSIGNED) {
		pub_disable();

		if (IS_ENABLED(CONFIG_BT_SETTINGS) &&
		    bt_mesh_is_provisioned()) {
			bt_mesh_settings_store_schedule(
					BT_MESH_SETTINGS_HB_PUB_PENDING);
		}

		return STATUS_SUCCESS;
	}

	if (!bt_mesh_subnet_get(new_pub->net_idx)) {
		LOG_ERR("Unknown NetKey 0x%04x", new_pub->net_idx);
		return STATUS_INVALID_NETKEY;
	}

	new_pub->feat &= BT_MESH_FEAT_SUPPORTED;
	pub = *new_pub;

	if (!bt_mesh_is_provisioned()) {
		return STATUS_SUCCESS;
	}

	/* The first Heartbeat message shall be published as soon as possible
	 * after the Heartbeat Publication Period state has been configured for
	 * periodic publishing.
	 *
	 * If the new configuration disables publishing this flushes
	 * the work item.
	 */
	k_work_reschedule(&pub_timer, K_NO_WAIT);

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		bt_mesh_settings_store_schedule(
					BT_MESH_SETTINGS_HB_PUB_PENDING);
	}

	return STATUS_SUCCESS;
}

void bt_mesh_hb_pub_get(struct bt_mesh_hb_pub *get)
{
	*get = pub;
}

uint8_t bt_mesh_hb_sub_set(uint16_t src, uint16_t dst, uint32_t period)
{
	if (src != BT_MESH_ADDR_UNASSIGNED && !BT_MESH_ADDR_IS_UNICAST(src)) {
		LOG_WRN("Prohibited source address");
		return STATUS_INVALID_ADDRESS;
	}

	if (BT_MESH_ADDR_IS_VIRTUAL(dst) || BT_MESH_ADDR_IS_RFU(dst) ||
	    (BT_MESH_ADDR_IS_UNICAST(dst) && dst != bt_mesh_primary_addr())) {
		LOG_WRN("Prohibited destination address");
		return STATUS_INVALID_ADDRESS;
	}

	if (period > (1U << 16)) {
		LOG_WRN("Prohibited subscription period %u s", period);
		return STATUS_CANNOT_SET;
	}

	/* Only an explicit address change to unassigned should trigger clearing
	 * of the values according to MESH/NODE/CFG/HBS/BV-02-C.
	 */
	if (src == BT_MESH_ADDR_UNASSIGNED || dst == BT_MESH_ADDR_UNASSIGNED) {
		sub.src = BT_MESH_ADDR_UNASSIGNED;
		sub.dst = BT_MESH_ADDR_UNASSIGNED;
		sub.min_hops = 0U;
		sub.max_hops = 0U;
		sub.count = 0U;
		sub.period = 0U;
	} else if (period) {
		sub.src = src;
		sub.dst = dst;
		sub.min_hops = BT_MESH_TTL_MAX;
		sub.max_hops = 0U;
		sub.count = 0U;
		sub.period = period;
	} else {
		/* Clearing the period should stop heartbeat subscription
		 * without clearing the parameters, so we can still read them.
		 */
		sub.period = 0U;
	}

	/* Start the timer, which notifies immediately if the new
	 * configuration disables the subscription.
	 */
	k_work_reschedule(&sub_timer, K_SECONDS(sub.period));

	return STATUS_SUCCESS;
}

void bt_mesh_hb_sub_reset_count(void)
{
	sub.count = 0;
}

void bt_mesh_hb_sub_get(struct bt_mesh_hb_sub *get)
{
	*get = sub;
	get->remaining = sub_remaining();
}

static void hb_unsolicited_pub_end_cb(int err, void *cb_data)
{
	if (!err) {
		notify_pub_sent();
	}
}

void bt_mesh_hb_feature_changed(uint16_t features)
{
	static const struct bt_mesh_send_cb pub_cb = {
		.end = hb_unsolicited_pub_end_cb,
	};

	if (pub.dst == BT_MESH_ADDR_UNASSIGNED) {
		return;
	}

	if (!(pub.feat & features)) {
		return;
	}

	heartbeat_send(&pub_cb, NULL);
}

void bt_mesh_hb_init(void)
{
	pub.net_idx = BT_MESH_KEY_UNUSED;
	k_work_init_delayable(&pub_timer, hb_publish);
	k_work_init_delayable(&sub_timer, sub_end);
}

void bt_mesh_hb_start(void)
{
	if (pub.count && pub.period) {
		LOG_DBG("Starting heartbeat publication");
		k_work_reschedule(&pub_timer, K_NO_WAIT);
	}
}

void bt_mesh_hb_suspend(void)
{
	/* Best-effort suspend.  This cannot guarantee that an
	 * in-progress publish will not complete.
	 */
	(void)k_work_cancel_delayable(&pub_timer);
}

void bt_mesh_hb_resume(void)
{
	if (pub.period && pub.count) {
		LOG_DBG("Starting heartbeat publication");
		k_work_reschedule(&pub_timer, K_NO_WAIT);
	}
}

static int hb_pub_set(const char *name, size_t len_rd,
		      settings_read_cb read_cb, void *cb_arg)
{
	struct bt_mesh_hb_pub pub;
	struct hb_pub_val hb_val;
	int err;

	err = bt_mesh_settings_set(read_cb, cb_arg, &hb_val, sizeof(hb_val));
	if (err) {
		LOG_ERR("Failed to set \'hb_val\'");
		return err;
	}

	pub.dst = hb_val.dst;
	pub.period = bt_mesh_hb_pwr2(hb_val.period);
	pub.ttl = hb_val.ttl;
	pub.feat = hb_val.feat;
	pub.net_idx = hb_val.net_idx;

	if (hb_val.indefinite) {
		pub.count = 0xffff;
	} else {
		pub.count = 0U;
	}

	(void)bt_mesh_hb_pub_set(&pub);

	LOG_DBG("Restored heartbeat publication");

	return 0;
}

BT_MESH_SETTINGS_DEFINE(pub, "HBPub", hb_pub_set);

void bt_mesh_hb_pub_pending_store(void)
{
	struct bt_mesh_hb_pub pub;
	struct hb_pub_val val;
	int err;

	bt_mesh_hb_pub_get(&pub);
	if (pub.dst == BT_MESH_ADDR_UNASSIGNED) {
		err = settings_delete("bt/mesh/HBPub");
	} else {
		val.indefinite = (pub.count == 0xffff);
		val.dst = pub.dst;
		val.period = bt_mesh_hb_log(pub.period);
		val.ttl = pub.ttl;
		val.feat = pub.feat;
		val.net_idx = pub.net_idx;

		err = settings_save_one("bt/mesh/HBPub", &val, sizeof(val));
	}

	if (err) {
		LOG_ERR("Failed to store Heartbeat Publication");
	} else {
		LOG_DBG("Stored Heartbeat Publication");
	}
}
