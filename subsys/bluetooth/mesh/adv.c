/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/debug/stack.h>
#include <zephyr/sys/util.h>

#include <zephyr/net_buf.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/mesh.h>

#include "common/bt_str.h"

#include "net.h"
#include "foundation.h"
#include "beacon.h"
#include "prov.h"
#include "proxy.h"
#include "pb_gatt_srv.h"
#include "solicitation.h"
#include "statistic.h"

#define LOG_LEVEL CONFIG_BT_MESH_ADV_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_adv);

/* Window and Interval are equal for continuous scanning */
#define MESH_SCAN_INTERVAL    BT_MESH_ADV_SCAN_UNIT(BT_MESH_SCAN_INTERVAL_MS)
#define MESH_SCAN_WINDOW      BT_MESH_ADV_SCAN_UNIT(BT_MESH_SCAN_WINDOW_MS)

const uint8_t bt_mesh_adv_type[BT_MESH_ADV_TYPES] = {
	[BT_MESH_ADV_PROV]   = BT_DATA_MESH_PROV,
	[BT_MESH_ADV_DATA]   = BT_DATA_MESH_MESSAGE,
	[BT_MESH_ADV_BEACON] = BT_DATA_MESH_BEACON,
	[BT_MESH_ADV_URI]    = BT_DATA_URI,
};

static bool active_scanning;
static K_FIFO_DEFINE(bt_mesh_adv_queue);
static K_FIFO_DEFINE(bt_mesh_relay_queue);
static K_FIFO_DEFINE(bt_mesh_friend_queue);

K_MEM_SLAB_DEFINE_STATIC(local_adv_pool, sizeof(struct bt_mesh_adv),
			 CONFIG_BT_MESH_ADV_BUF_COUNT, __alignof__(struct bt_mesh_adv));

#if defined(CONFIG_BT_MESH_RELAY_BUF_COUNT)
K_MEM_SLAB_DEFINE_STATIC(relay_adv_pool, sizeof(struct bt_mesh_adv),
			 CONFIG_BT_MESH_RELAY_BUF_COUNT, __alignof__(struct bt_mesh_adv));
#endif

#if defined(CONFIG_BT_MESH_ADV_EXT_FRIEND_SEPARATE)
K_MEM_SLAB_DEFINE_STATIC(friend_adv_pool, sizeof(struct bt_mesh_adv),
			 CONFIG_BT_MESH_FRIEND_LPN_COUNT, __alignof__(struct bt_mesh_adv));
#endif

void bt_mesh_adv_send_start(uint16_t duration, int err, struct bt_mesh_adv_ctx *ctx)
{
	if (!ctx->started) {
		ctx->started = 1;

		if (ctx->cb && ctx->cb->start) {
			ctx->cb->start(duration, err, ctx->cb_data);
		}

		if (err) {
			ctx->cb = NULL;
		} else if (IS_ENABLED(CONFIG_BT_MESH_STATISTIC)) {
			bt_mesh_stat_succeeded_count(ctx);
		}
	}
}

void bt_mesh_adv_send_end(int err, struct bt_mesh_adv_ctx const *ctx)
{
	if (ctx->started && ctx->cb && ctx->cb->end) {
		ctx->cb->end(err, ctx->cb_data);
	}
}

static struct bt_mesh_adv *adv_create_from_pool(struct k_mem_slab *buf_pool,
						enum bt_mesh_adv_type type,
						enum bt_mesh_adv_tag tag,
						uint8_t xmit, k_timeout_t timeout)
{
	struct bt_mesh_adv_ctx *ctx;
	struct bt_mesh_adv *adv;
	int err;

	if (atomic_test_bit(bt_mesh.flags, BT_MESH_SUSPENDED)) {
		LOG_WRN("Refusing to allocate buffer while suspended");
		return NULL;
	}

	err = k_mem_slab_alloc(buf_pool, (void **)&adv, timeout);
	if (err) {
		return NULL;
	}

	adv->__ref = 1;

	net_buf_simple_init_with_data(&adv->b, adv->__bufs, BT_MESH_ADV_DATA_SIZE);
	net_buf_simple_reset(&adv->b);

	ctx = &adv->ctx;

	(void)memset(ctx, 0, sizeof(*ctx));

	ctx->type         = type;
	ctx->tag          = tag;
	ctx->xmit         = xmit;

	return adv;
}

struct bt_mesh_adv *bt_mesh_adv_ref(struct bt_mesh_adv *adv)
{
	__ASSERT_NO_MSG(adv->__ref < UINT8_MAX);

	adv->__ref++;

	return adv;
}

void bt_mesh_adv_unref(struct bt_mesh_adv *adv)
{
	__ASSERT_NO_MSG(adv->__ref > 0);

	if (--adv->__ref > 0) {
		return;
	}

	struct k_mem_slab *slab = &local_adv_pool;

#if (defined(CONFIG_BT_MESH_RELAY) || defined(CONFIG_BT_MESH_BRG_CFG_SRV))
	if (adv->ctx.tag == BT_MESH_ADV_TAG_RELAY) {
		slab = &relay_adv_pool;
	}
#endif

#if defined(CONFIG_BT_MESH_ADV_EXT_FRIEND_SEPARATE)
	if (adv->ctx.tag == BT_MESH_ADV_TAG_FRIEND) {
		slab = &friend_adv_pool;
	}
#endif

	k_mem_slab_free(slab, (void *)adv);
}

struct bt_mesh_adv *bt_mesh_adv_create(enum bt_mesh_adv_type type,
				       enum bt_mesh_adv_tag tag,
				       uint8_t xmit, k_timeout_t timeout)
{
#if (defined(CONFIG_BT_MESH_RELAY) || defined(CONFIG_BT_MESH_BRG_CFG_SRV))
	if (tag == BT_MESH_ADV_TAG_RELAY) {
		return adv_create_from_pool(&relay_adv_pool,
					    type, tag, xmit, timeout);
	}
#endif

#if defined(CONFIG_BT_MESH_ADV_EXT_FRIEND_SEPARATE)
	if (tag == BT_MESH_ADV_TAG_FRIEND) {
		return adv_create_from_pool(&friend_adv_pool,
					    type, tag, xmit, timeout);
	}
#endif

	return adv_create_from_pool(&local_adv_pool, type,
				    tag, xmit, timeout);
}

static struct bt_mesh_adv *process_events(struct k_poll_event *ev, int count)
{
	for (; count; ev++, count--) {
		LOG_DBG("ev->state %u", ev->state);

		switch (ev->state) {
		case K_POLL_STATE_FIFO_DATA_AVAILABLE:
			return k_fifo_get(ev->fifo, K_NO_WAIT);
		case K_POLL_STATE_NOT_READY:
		case K_POLL_STATE_CANCELLED:
			break;
		default:
			LOG_WRN("Unexpected k_poll event state %u", ev->state);
			break;
		}
	}

	return NULL;
}

struct bt_mesh_adv *bt_mesh_adv_get(k_timeout_t timeout)
{
	int err;
	struct k_poll_event events[] = {
		K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_FIFO_DATA_AVAILABLE,
						K_POLL_MODE_NOTIFY_ONLY,
						&bt_mesh_adv_queue,
						0),
#if (defined(CONFIG_BT_MESH_RELAY) || defined(CONFIG_BT_MESH_BRG_CFG_SRV)) && \
	(defined(CONFIG_BT_MESH_ADV_LEGACY) || \
	 defined(CONFIG_BT_MESH_ADV_EXT_RELAY_USING_MAIN_ADV_SET) || \
	 !(CONFIG_BT_MESH_RELAY_ADV_SETS))
		K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_FIFO_DATA_AVAILABLE,
						K_POLL_MODE_NOTIFY_ONLY,
						&bt_mesh_relay_queue,
						0),
#endif
	};

	err = k_poll(events, ARRAY_SIZE(events), timeout);
	if (err) {
		return NULL;
	}

	return process_events(events, ARRAY_SIZE(events));
}

struct bt_mesh_adv *bt_mesh_adv_get_by_tag(enum bt_mesh_adv_tag_bit tags, k_timeout_t timeout)
{
	if (IS_ENABLED(CONFIG_BT_MESH_ADV_EXT_FRIEND_SEPARATE) &&
	    tags & BT_MESH_ADV_TAG_BIT_FRIEND) {
		return k_fifo_get(&bt_mesh_friend_queue, timeout);
	}

	if ((IS_ENABLED(CONFIG_BT_MESH_RELAY) || IS_ENABLED(CONFIG_BT_MESH_BRG_CFG_SRV)) &&
	    !(tags & BT_MESH_ADV_TAG_BIT_LOCAL)) {
		return k_fifo_get(&bt_mesh_relay_queue, timeout);
	}

	return bt_mesh_adv_get(timeout);
}

void bt_mesh_adv_get_cancel(void)
{
	LOG_DBG("");

	k_fifo_cancel_wait(&bt_mesh_adv_queue);

	if ((IS_ENABLED(CONFIG_BT_MESH_RELAY) || IS_ENABLED(CONFIG_BT_MESH_BRG_CFG_SRV))) {
		k_fifo_cancel_wait(&bt_mesh_relay_queue);
	}

	if (IS_ENABLED(CONFIG_BT_MESH_ADV_EXT_FRIEND_SEPARATE)) {
		k_fifo_cancel_wait(&bt_mesh_friend_queue);
	}
}

void bt_mesh_adv_send(struct bt_mesh_adv *adv, const struct bt_mesh_send_cb *cb,
		      void *cb_data)
{
	LOG_DBG("type 0x%02x len %u: %s", adv->ctx.type, adv->b.len,
		bt_hex(adv->b.data, adv->b.len));

	if (atomic_test_bit(bt_mesh.flags, BT_MESH_SUSPENDED)) {
		LOG_WRN("Sending advertisement while suspended");
	}

	adv->ctx.cb = cb;
	adv->ctx.cb_data = cb_data;
	adv->ctx.busy = 1U;

	if (IS_ENABLED(CONFIG_BT_MESH_STATISTIC)) {
		bt_mesh_stat_planned_count(&adv->ctx);
	}

	if (IS_ENABLED(CONFIG_BT_MESH_ADV_EXT_FRIEND_SEPARATE) &&
	    adv->ctx.tag == BT_MESH_ADV_TAG_FRIEND) {
		k_fifo_put(&bt_mesh_friend_queue, bt_mesh_adv_ref(adv));
		bt_mesh_adv_friend_ready();
		return;
	}

	if (((IS_ENABLED(CONFIG_BT_MESH_RELAY) || IS_ENABLED(CONFIG_BT_MESH_BRG_CFG_SRV)) &&
	    adv->ctx.tag == BT_MESH_ADV_TAG_RELAY) ||
	    (IS_ENABLED(CONFIG_BT_MESH_PB_ADV_USE_RELAY_SETS) &&
	     adv->ctx.tag == BT_MESH_ADV_TAG_PROV)) {
		k_fifo_put(&bt_mesh_relay_queue, bt_mesh_adv_ref(adv));
		bt_mesh_adv_relay_ready();
		return;
	}

	k_fifo_put(&bt_mesh_adv_queue, bt_mesh_adv_ref(adv));
	bt_mesh_adv_local_ready();
}

int bt_mesh_adv_gatt_send(void)
{
	if (bt_mesh_is_provisioned()) {
		if (IS_ENABLED(CONFIG_BT_MESH_GATT_PROXY)) {
			LOG_DBG("Proxy Advertising");
			return bt_mesh_proxy_adv_start();
		}
	} else if (IS_ENABLED(CONFIG_BT_MESH_PB_GATT)) {
		LOG_DBG("PB-GATT Advertising");
		return bt_mesh_pb_gatt_srv_adv_start();
	}

	return -ENOTSUP;
}

static void bt_mesh_scan_cb(const bt_addr_le_t *addr, int8_t rssi,
			    uint8_t adv_type, struct net_buf_simple *buf)
{
	if (adv_type != BT_GAP_ADV_TYPE_ADV_NONCONN_IND) {
		return;
	}

	LOG_DBG("len %u: %s", buf->len, bt_hex(buf->data, buf->len));

	while (buf->len > 1) {
		struct net_buf_simple_state state;
		uint8_t len, type;

		len = net_buf_simple_pull_u8(buf);
		/* Check for early termination */
		if (len == 0U) {
			return;
		}

		if (len > buf->len) {
			LOG_WRN("AD malformed");
			return;
		}

		net_buf_simple_save(buf, &state);

		type = net_buf_simple_pull_u8(buf);

		buf->len = len - 1;

		switch (type) {
		case BT_DATA_MESH_MESSAGE:
			bt_mesh_net_recv(buf, rssi, BT_MESH_NET_IF_ADV);
			break;
#if defined(CONFIG_BT_MESH_PB_ADV)
		case BT_DATA_MESH_PROV:
			bt_mesh_pb_adv_recv(buf);
			break;
#endif
		case BT_DATA_MESH_BEACON:
			bt_mesh_beacon_recv(buf);
			break;
		case BT_DATA_UUID16_SOME:
			/* Fall through */
		case BT_DATA_UUID16_ALL:
			if (IS_ENABLED(CONFIG_BT_MESH_OD_PRIV_PROXY_SRV)) {
				/* Restore buffer with Solicitation PDU */
				net_buf_simple_restore(buf, &state);
				bt_mesh_sol_recv(buf, len - 1);
			}
			break;
		default:
			break;
		}

		net_buf_simple_restore(buf, &state);
		net_buf_simple_pull(buf, len);
	}
}

int bt_mesh_scan_active_set(bool active)
{
	if (active_scanning == active) {
		return 0;
	}

	active_scanning = active;
	bt_mesh_scan_disable();
	return bt_mesh_scan_enable();
}

int bt_mesh_scan_enable(void)
{
	struct bt_le_scan_param scan_param = {
		.type = active_scanning ? BT_LE_SCAN_TYPE_ACTIVE :
					  BT_LE_SCAN_TYPE_PASSIVE,
		.interval = MESH_SCAN_INTERVAL,
		.window = MESH_SCAN_WINDOW
	};
	int err;

	LOG_DBG("");

	err = bt_le_scan_start(&scan_param, bt_mesh_scan_cb);
	if (err && err != -EALREADY) {
		LOG_ERR("starting scan failed (err %d)", err);
		return err;
	}

	return 0;
}

int bt_mesh_scan_disable(void)
{
	int err;

	LOG_DBG("");

	err = bt_le_scan_stop();
	if (err && err != -EALREADY) {
		LOG_ERR("stopping scan failed (err %d)", err);
		return err;
	}

	return 0;
}
