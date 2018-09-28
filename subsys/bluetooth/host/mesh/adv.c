/*  Bluetooth Mesh */

/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/stack.h>
#include <misc/util.h>

#include <net/buf.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/mesh.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_MESH_DEBUG_ADV)
#include "common/log.h"

#include "../hci_core.h"

#include "adv.h"
#include "net.h"
#include "foundation.h"
#include "beacon.h"
#include "prov.h"
#include "proxy.h"

/* Convert from ms to 0.625ms units */
#define ADV_SCAN_UNIT(_ms) ((_ms) * 8 / 5)

/* Window and Interval are equal for continuous scanning */
#define MESH_SCAN_INTERVAL_MS 10
#define MESH_SCAN_WINDOW_MS   10
#define MESH_SCAN_INTERVAL    ADV_SCAN_UNIT(MESH_SCAN_INTERVAL_MS)
#define MESH_SCAN_WINDOW      ADV_SCAN_UNIT(MESH_SCAN_WINDOW_MS)

/* Pre-5.0 controllers enforce a minimum interval of 100ms
 * whereas 5.0+ controllers can go down to 20ms.
 */
#define ADV_INT_DEFAULT_MS 100
#define ADV_INT_FAST_MS    20

/* TinyCrypt PRNG consumes a lot of stack space, so we need to have
 * an increased call stack whenever it's used.
 */
#if defined(CONFIG_BT_HOST_CRYPTO)
#define ADV_STACK_SIZE 768
#else
#define ADV_STACK_SIZE 512
#endif

static K_FIFO_DEFINE(adv_queue);
static struct k_thread adv_thread_data;
static BT_STACK_NOINIT(adv_thread_stack, ADV_STACK_SIZE);

static const u8_t adv_type[] = {
	[BT_MESH_ADV_PROV]   = BT_DATA_MESH_PROV,
	[BT_MESH_ADV_DATA]   = BT_DATA_MESH_MESSAGE,
	[BT_MESH_ADV_BEACON] = BT_DATA_MESH_BEACON,
	[BT_MESH_ADV_URI]    = BT_DATA_URI,
};

NET_BUF_POOL_DEFINE(adv_buf_pool, CONFIG_BT_MESH_ADV_BUF_COUNT,
		    BT_MESH_ADV_DATA_SIZE, BT_MESH_ADV_USER_DATA_SIZE, NULL);

static struct bt_mesh_adv adv_pool[CONFIG_BT_MESH_ADV_BUF_COUNT];

static struct bt_mesh_adv *adv_alloc(int id)
{
	return &adv_pool[id];
}

static inline void adv_send_start(u16_t duration, int err,
				  const struct bt_mesh_send_cb *cb,
				  void *cb_data)
{
	if (cb && cb->start) {
		cb->start(duration, err, cb_data);
	}
}

static inline void adv_send_end(int err, const struct bt_mesh_send_cb *cb,
				void *cb_data)
{
	if (cb && cb->end) {
		cb->end(err, cb_data);
	}
}

static inline void adv_send(struct net_buf *buf)
{
	const s32_t adv_int_min = ((bt_dev.hci_version >= BT_HCI_VERSION_5_0) ?
				   ADV_INT_FAST_MS : ADV_INT_DEFAULT_MS);
	const struct bt_mesh_send_cb *cb = BT_MESH_ADV(buf)->cb;
	void *cb_data = BT_MESH_ADV(buf)->cb_data;
	struct bt_le_adv_param param;
	u16_t duration, adv_int;
	struct bt_data ad;
	int err;

	adv_int = max(adv_int_min,
		      BT_MESH_TRANSMIT_INT(BT_MESH_ADV(buf)->xmit));
	duration = (MESH_SCAN_WINDOW_MS +
		    ((BT_MESH_TRANSMIT_COUNT(BT_MESH_ADV(buf)->xmit) + 1) *
		     (adv_int + 10)));

	BT_DBG("type %u len %u: %s", BT_MESH_ADV(buf)->type,
	       buf->len, bt_hex(buf->data, buf->len));
	BT_DBG("count %u interval %ums duration %ums",
	       BT_MESH_TRANSMIT_COUNT(BT_MESH_ADV(buf)->xmit) + 1, adv_int,
	       duration);

	ad.type = adv_type[BT_MESH_ADV(buf)->type];
	ad.data_len = buf->len;
	ad.data = buf->data;

	if (IS_ENABLED(CONFIG_BT_MESH_DEBUG_USE_ID_ADDR)) {
		param.options = BT_LE_ADV_OPT_USE_IDENTITY;
	} else {
		param.options = 0;
	}

	param.id = BT_ID_DEFAULT;
	param.interval_min = ADV_SCAN_UNIT(adv_int);
	param.interval_max = param.interval_min;

	err = bt_le_adv_start(&param, &ad, 1, NULL, 0);
	net_buf_unref(buf);
	adv_send_start(duration, err, cb, cb_data);
	if (err) {
		BT_ERR("Advertising failed: err %d", err);
		return;
	}

	BT_DBG("Advertising started. Sleeping %u ms", duration);

	k_sleep(K_MSEC(duration));

	err = bt_le_adv_stop();
	adv_send_end(err, cb, cb_data);
	if (err) {
		BT_ERR("Stopping advertising failed: err %d", err);
		return;
	}

	BT_DBG("Advertising stopped");
}

static void adv_stack_dump(const struct k_thread *thread, void *user_data)
{
#if defined(CONFIG_THREAD_STACK_INFO)
	stack_analyze((char *)user_data, (char *)thread->stack_info.start,
						thread->stack_info.size);
#endif
}

static void adv_thread(void *p1, void *p2, void *p3)
{
	BT_DBG("started");

	while (1) {
		struct net_buf *buf;

		if (IS_ENABLED(CONFIG_BT_MESH_PROXY)) {
			buf = net_buf_get(&adv_queue, K_NO_WAIT);
			while (!buf) {
				s32_t timeout;

				timeout = bt_mesh_proxy_adv_start();
				BT_DBG("Proxy Advertising up to %d ms",
				       timeout);
				buf = net_buf_get(&adv_queue, timeout);
				bt_mesh_proxy_adv_stop();
			}
		} else {
			buf = net_buf_get(&adv_queue, K_FOREVER);
		}

		if (!buf) {
			continue;
		}

		/* busy == 0 means this was canceled */
		if (BT_MESH_ADV(buf)->busy) {
			BT_MESH_ADV(buf)->busy = 0;
			adv_send(buf);
		}

		STACK_ANALYZE("adv stack", adv_thread_stack);
		k_thread_foreach(adv_stack_dump, "BT_MESH");

		/* Give other threads a chance to run */
		k_yield();
	}
}

void bt_mesh_adv_update(void)
{
	BT_DBG("");

	k_fifo_cancel_wait(&adv_queue);
}

struct net_buf *bt_mesh_adv_create_from_pool(struct net_buf_pool *pool,
					     bt_mesh_adv_alloc_t get_id,
					     enum bt_mesh_adv_type type,
					     u8_t xmit, s32_t timeout)
{
	struct bt_mesh_adv *adv;
	struct net_buf *buf;

	buf = net_buf_alloc(pool, timeout);
	if (!buf) {
		return NULL;
	}

	adv = get_id(net_buf_id(buf));
	BT_MESH_ADV(buf) = adv;

	(void)memset(adv, 0, sizeof(*adv));

	adv->type         = type;
	adv->xmit         = xmit;

	return buf;
}

struct net_buf *bt_mesh_adv_create(enum bt_mesh_adv_type type, u8_t xmit,
				   s32_t timeout)
{
	return bt_mesh_adv_create_from_pool(&adv_buf_pool, adv_alloc, type,
					    xmit, timeout);
}

void bt_mesh_adv_send(struct net_buf *buf, const struct bt_mesh_send_cb *cb,
		      void *cb_data)
{
	BT_DBG("type 0x%02x len %u: %s", BT_MESH_ADV(buf)->type, buf->len,
	       bt_hex(buf->data, buf->len));

	BT_MESH_ADV(buf)->cb = cb;
	BT_MESH_ADV(buf)->cb_data = cb_data;
	BT_MESH_ADV(buf)->busy = 1;

	net_buf_put(&adv_queue, net_buf_ref(buf));
}

static void bt_mesh_scan_cb(const bt_addr_le_t *addr, s8_t rssi,
			    u8_t adv_type, struct net_buf_simple *buf)
{
	if (adv_type != BT_LE_ADV_NONCONN_IND) {
		return;
	}

	BT_DBG("len %u: %s", buf->len, bt_hex(buf->data, buf->len));

	while (buf->len > 1) {
		struct net_buf_simple_state state;
		u8_t len, type;

		len = net_buf_simple_pull_u8(buf);
		/* Check for early termination */
		if (len == 0) {
			return;
		}

		if (len > buf->len) {
			BT_WARN("AD malformed");
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

void bt_mesh_adv_init(void)
{
	k_thread_create(&adv_thread_data, adv_thread_stack,
			K_THREAD_STACK_SIZEOF(adv_thread_stack), adv_thread,
			NULL, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);
}

int bt_mesh_scan_enable(void)
{
	struct bt_le_scan_param scan_param = {
			.type       = BT_HCI_LE_SCAN_PASSIVE,
			.filter_dup = BT_HCI_LE_SCAN_FILTER_DUP_DISABLE,
			.interval   = MESH_SCAN_INTERVAL,
			.window     = MESH_SCAN_WINDOW };

	BT_DBG("");

	return bt_le_scan_start(&scan_param, bt_mesh_scan_cb);
}

int bt_mesh_scan_disable(void)
{
	BT_DBG("");

	return bt_le_scan_stop();
}
