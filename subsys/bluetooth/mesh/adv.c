/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/debug/stack.h>
#include <zephyr/sys/util.h>

#include <zephyr/net/buf.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/mesh.h>

#include "common/bt_str.h"

#include "adv.h"
#include "net.h"
#include "foundation.h"
#include "beacon.h"
#include "host/ecc.h"
#include "prov.h"
#include "proxy.h"
#include "pb_gatt_srv.h"

#define LOG_LEVEL CONFIG_BT_MESH_ADV_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_adv);

/* Window and Interval are equal for continuous scanning */
#define MESH_SCAN_INTERVAL    BT_MESH_ADV_SCAN_UNIT(BT_MESH_SCAN_INTERVAL_MS)
#define MESH_SCAN_WINDOW      BT_MESH_ADV_SCAN_UNIT(BT_MESH_SCAN_WINDOW_MS)

#ifndef CONFIG_BT_MESH_RELAY_BUF_COUNT
#define CONFIG_BT_MESH_RELAY_BUF_COUNT 0
#endif

const uint8_t bt_mesh_adv_type[BT_MESH_ADV_TYPES] = {
	[BT_MESH_ADV_PROV]   = BT_DATA_MESH_PROV,
	[BT_MESH_ADV_DATA]   = BT_DATA_MESH_MESSAGE,
	[BT_MESH_ADV_BEACON] = BT_DATA_MESH_BEACON,
	[BT_MESH_ADV_URI]    = BT_DATA_URI,
};

static K_QUEUE_DEFINE(bt_mesh_adv_queue);
static K_QUEUE_DEFINE(bt_mesh_relay_queue);
static K_QUEUE_DEFINE(bt_mesh_friend_queue);

K_MEM_SLAB_DEFINE(adv_buf_pool, sizeof(struct bt_mesh_buf),
		  CONFIG_BT_MESH_ADV_BUF_COUNT, __alignof__(struct bt_mesh_buf));

K_MEM_SLAB_DEFINE(relay_buf_pool, sizeof(struct bt_mesh_buf),
		  CONFIG_BT_MESH_RELAY_BUF_COUNT, __alignof__(struct bt_mesh_buf));

K_MEM_SLAB_DEFINE(friend_buf_pool, sizeof(struct bt_mesh_buf),
		  CONFIG_BT_MESH_FRIEND_LPN_COUNT, __alignof__(struct bt_mesh_buf));

static struct bt_mesh_buf *adv_create_from_pool(struct k_mem_slab *buf_pool,
						enum bt_mesh_adv_type type,
						enum bt_mesh_adv_tag tag,
						uint8_t xmit, uint8_t prio,
						k_timeout_t timeout)
{
	struct bt_mesh_adv *adv;
	struct bt_mesh_buf *buf;
	int err;

	if (atomic_test_bit(bt_mesh.flags, BT_MESH_SUSPENDED)) {
		LOG_WRN("Refusing to allocate buffer while suspended");
		return NULL;
	}

	err = k_mem_slab_alloc(buf_pool, (void **)&buf, timeout);
	if (err) {
		return NULL;
	}

	buf->ref = 1;

	net_buf_simple_init_with_data(&buf->b, buf->_bufs, BT_MESH_ADV_DATA_SIZE);
	net_buf_simple_reset(&buf->b);

	adv = &buf->adv;

	(void)memset(adv, 0, sizeof(*adv));

	adv->type         = type;
	adv->tag          = tag;
	adv->xmit         = xmit;
	adv->prio         = prio;

	return buf;
}

struct bt_mesh_buf *bt_mesh_buf_ref(struct bt_mesh_buf *buf)
{
	buf->ref++;

	return buf;
}

void bt_mesh_buf_unref(struct bt_mesh_buf *buf)
{
	if (--buf->ref > 0) {
		return;
	}

	struct k_mem_slab *slab;
	struct bt_mesh_adv adv = *(&buf->adv);

	if (IS_ENABLED(CONFIG_BT_MESH_RELAY) &&
	    adv.tag == BT_MESH_RELAY_ADV) {
		slab = &relay_buf_pool;
	} else if (IS_ENABLED(CONFIG_BT_MESH_ADV_EXT_FRIEND_SEPARATE) &&
		   adv.tag == BT_MESH_FRIEND_ADV) {
		slab = &friend_buf_pool;
	} else {
		slab = &adv_buf_pool;
	}

	k_mem_slab_free(slab, (void **)&buf);
	bt_mesh_adv_send_end(0, &adv);
}

struct bt_mesh_buf *bt_mesh_adv_relay_create(uint8_t prio, uint8_t xmit)
{
	void *lowest = NULL, *prev = NULL, *curr;
	uint8_t prio_cur = prio;
	struct bt_mesh_buf *buf;

	buf = adv_create_from_pool(&relay_buf_pool,
				   BT_MESH_ADV_DATA, BT_MESH_RELAY_ADV,
				   xmit, prio, K_NO_WAIT);
	if (buf) {
		return buf;
	}

	if (!IS_ENABLED(CONFIG_BT_MESH_RELAY_PRIORITY) || !prio_cur) {
		return NULL;
	}

	K_QUEUE_FOR_EACH(&bt_mesh_relay_queue, curr) {
		buf = CONTAINER_OF(curr, struct bt_mesh_buf, node);

		if (buf->adv.prio < prio_cur) {
			prio_cur = buf->adv.prio;
			lowest = curr;
		}

		prev = curr;
	}

	if (!lowest) {
		return NULL;
	}

	k_queue_remove(&bt_mesh_relay_queue, lowest);

	buf = CONTAINER_OF(lowest, struct bt_mesh_buf, node);

	bt_mesh_adv_send_start(0, -ECANCELED, &buf->adv);
	bt_mesh_buf_unref(buf);

	return adv_create_from_pool(&relay_buf_pool,
				    BT_MESH_ADV_DATA, BT_MESH_RELAY_ADV,
				    xmit, prio, K_NO_WAIT);
}

struct bt_mesh_buf *bt_mesh_adv_main_create(enum bt_mesh_adv_type type,
					    uint8_t xmit, k_timeout_t timeout)
{
	LOG_DBG("");

	return adv_create_from_pool(&adv_buf_pool,
				    type, BT_MESH_LOCAL_ADV,
				    xmit, 0, timeout);
}

struct bt_mesh_buf *bt_mesh_adv_frnd_create(uint8_t xmit, k_timeout_t timeout)
{
	LOG_DBG("");

	return adv_create_from_pool(&friend_buf_pool,
				    BT_MESH_ADV_DATA, BT_MESH_FRIEND_ADV,
				    xmit, 0, timeout);
}

#if CONFIG_BT_MESH_RELAY_ADV_SETS || CONFIG_BT_MESH_ADV_EXT_FRIEND_SEPARATE
static struct bt_mesh_buf *process_events(struct k_poll_event *ev, int count)
{
	for (; count; ev++, count--) {
		LOG_DBG("ev->state %u", ev->state);

		switch (ev->state) {
		case K_POLL_STATE_FIFO_DATA_AVAILABLE:
			return k_queue_get(ev->queue, K_NO_WAIT);
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

struct bt_mesh_buf *bt_mesh_adv_buf_get(k_timeout_t timeout)
{
	int err;
	struct k_poll_event events[] = {
		K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_FIFO_DATA_AVAILABLE,
						K_POLL_MODE_NOTIFY_ONLY,
						&bt_mesh_adv_queue,
						0),
#if defined(CONFIG_BT_MESH_ADV_EXT_RELAY_USING_MAIN_ADV_SET)
		K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_FIFO_DATA_AVAILABLE,
						K_POLL_MODE_NOTIFY_ONLY,
						&bt_mesh_relay_queue,
						0),
#endif /* CONFIG_BT_MESH_ADV_EXT_RELAY_USING_MAIN_ADV_SET */
	};

	err = k_poll(events, ARRAY_SIZE(events), timeout);
	if (err) {
		return NULL;
	}

	return process_events(events, ARRAY_SIZE(events));
}

struct bt_mesh_buf *bt_mesh_adv_buf_relay_get(k_timeout_t timeout)
{
	return k_queue_get(&bt_mesh_relay_queue, timeout);
}

struct bt_mesh_buf *bt_mesh_adv_buf_get_by_tag(uint8_t tag, k_timeout_t timeout)
{
	if (IS_ENABLED(CONFIG_BT_MESH_ADV_EXT_FRIEND_SEPARATE) &&
	    tag & BT_MESH_FRIEND_ADV) {
		return k_queue_get(&bt_mesh_friend_queue, timeout);
	} else if (tag & BT_MESH_LOCAL_ADV) {
		return bt_mesh_adv_buf_get(timeout);
	} else if (tag & BT_MESH_RELAY_ADV) {
		return bt_mesh_adv_buf_relay_get(timeout);
	} else {
		return NULL;
	}
}

static void relay_send(struct bt_mesh_buf *buf)
{
	uint8_t prio = buf->adv.prio;
	void *curr, *prev = NULL;

	if (!IS_ENABLED(CONFIG_BT_MESH_RELAY_PRIORITY) || !prio) {
		k_queue_append(&bt_mesh_relay_queue, bt_mesh_buf_ref(buf));
		bt_mesh_adv_buf_relay_ready();
		return;
	}

	K_QUEUE_FOR_EACH(&bt_mesh_relay_queue, curr) {
		struct bt_mesh_buf *buf_curr = CONTAINER_OF(curr, struct bt_mesh_buf, node);

		if (buf_curr->adv.prio < prio) {
			break;
		}

		prev = curr;
	}

	/* The messages with the highest priority are always placed at the head, and
	 * the messages with the same priority are arranged in chronological order.
	 */
	k_queue_insert(&bt_mesh_relay_queue, prev, bt_mesh_buf_ref(buf));

	bt_mesh_adv_buf_relay_ready();
}
#else /* !(CONFIG_BT_MESH_RELAY_ADV_SETS || CONFIG_BT_MESH_ADV_EXT_FRIEND_SEPARATE) */
struct bt_mesh_buf *bt_mesh_adv_buf_get(k_timeout_t timeout)
{
	return k_queue_get(&bt_mesh_adv_queue, timeout);
}

struct bt_mesh_buf *bt_mesh_adv_buf_get_by_tag(uint8_t tag, k_timeout_t timeout)
{
	ARG_UNUSED(tag);

	return bt_mesh_adv_buf_get(timeout);
}
#endif /* CONFIG_BT_MESH_RELAY_ADV_SETS || CONFIG_BT_MESH_ADV_EXT_FRIEND_SEPARATE */

void bt_mesh_adv_buf_get_cancel(void)
{
	LOG_DBG("");

	k_queue_cancel_wait(&bt_mesh_adv_queue);

#if CONFIG_BT_MESH_RELAY_ADV_SETS
	k_queue_cancel_wait(&bt_mesh_relay_queue);
#endif /* CONFIG_BT_MESH_RELAY_ADV_SETS */

	if (IS_ENABLED(CONFIG_BT_MESH_ADV_EXT_FRIEND_SEPARATE)) {
		k_fifo_cancel_wait(&bt_mesh_friend_queue);
	}
}

void bt_mesh_adv_send(struct bt_mesh_buf *buf, const struct bt_mesh_send_cb *cb,
		      void *cb_data)
{
	LOG_DBG("type 0x%02x len %u: %s", buf->adv.type, buf->b.len,
		bt_hex(buf->b.data, buf->b.len));

	buf->adv.cb = cb;
	buf->adv.cb_data = cb_data;
	buf->adv.busy = 1U;

	if (IS_ENABLED(CONFIG_BT_MESH_ADV_EXT_FRIEND_SEPARATE) &&
	    buf->adv.tag == BT_MESH_FRIEND_ADV) {
		k_queue_append(&bt_mesh_friend_queue, bt_mesh_buf_ref(buf));
		bt_mesh_adv_buf_friend_ready();
		return;
	}

#if CONFIG_BT_MESH_RELAY_ADV_SETS
	if (buf->adv.tag == BT_MESH_RELAY_ADV) {
		relay_send(buf);
		return;
	}
#endif /* CONFIG_BT_MESH_RELAY_ADV_SETS */

	k_queue_append(&bt_mesh_adv_queue, bt_mesh_buf_ref(buf));
	bt_mesh_adv_buf_local_ready();
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
		default:
			break;
		}

		net_buf_simple_restore(buf, &state);
		net_buf_simple_pull(buf, len);
	}
}

int bt_mesh_scan_enable(void)
{
	struct bt_le_scan_param scan_param = {
			.type       = BT_HCI_LE_SCAN_PASSIVE,
			.options    = BT_LE_SCAN_OPT_NONE,
			.interval   = MESH_SCAN_INTERVAL,
			.window     = MESH_SCAN_WINDOW };
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

