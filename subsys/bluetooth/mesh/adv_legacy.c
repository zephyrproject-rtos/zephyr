/*  Bluetooth Mesh */

/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <debug/stack.h>
#include <sys/util.h>

#include <net/buf.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/mesh.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_MESH_DEBUG_ADV)
#define LOG_MODULE_NAME bt_mesh_adv_legacy
#include "common/log.h"

#include "host/hci_core.h"

#include "adv.h"
#include "net.h"
#include "foundation.h"
#include "beacon.h"
#include "prov.h"
#include "proxy.h"

/* Pre-5.0 controllers enforce a minimum interval of 100ms
 * whereas 5.0+ controllers can go down to 20ms.
 */
#define ADV_INT_DEFAULT_MS 100
#define ADV_INT_FAST_MS    20

static struct k_thread adv_thread_data;
static K_KERNEL_STACK_DEFINE(adv_thread_stack, CONFIG_BT_MESH_ADV_STACK_SIZE);
static int32_t adv_timeout;

static inline void adv_send(struct net_buf *buf)
{
	const int32_t adv_int_min =
		((bt_dev.hci_version >= BT_HCI_VERSION_5_0) ?
			       ADV_INT_FAST_MS :
			       ADV_INT_DEFAULT_MS);
	const struct bt_mesh_send_cb *cb = BT_MESH_ADV(buf)->cb;
	void *cb_data = BT_MESH_ADV(buf)->cb_data;
	struct bt_le_adv_param param = {};
	uint16_t duration, adv_int;
	struct bt_data ad;
	int err;

	adv_int = MAX(adv_int_min,
		      BT_MESH_TRANSMIT_INT(BT_MESH_ADV(buf)->xmit));
	duration = (BT_MESH_SCAN_WINDOW_MS +
		    ((BT_MESH_TRANSMIT_COUNT(BT_MESH_ADV(buf)->xmit) + 1) *
		     (adv_int + 10)));

	BT_DBG("type %u len %u: %s", BT_MESH_ADV(buf)->type,
	       buf->len, bt_hex(buf->data, buf->len));
	BT_DBG("count %u interval %ums duration %ums",
	       BT_MESH_TRANSMIT_COUNT(BT_MESH_ADV(buf)->xmit) + 1, adv_int,
	       duration);

	ad.type = bt_mesh_adv_type[BT_MESH_ADV(buf)->type];
	ad.data_len = buf->len;
	ad.data = buf->data;

	if (IS_ENABLED(CONFIG_BT_MESH_DEBUG_USE_ID_ADDR)) {
		param.options = BT_LE_ADV_OPT_USE_IDENTITY;
	} else {
		param.options = 0U;
	}

	param.id = BT_ID_DEFAULT;
	param.interval_min = BT_MESH_ADV_SCAN_UNIT(adv_int);
	param.interval_max = param.interval_min;

	uint64_t time = k_uptime_get();

	err = bt_le_adv_start(&param, &ad, 1, NULL, 0);
	net_buf_unref(buf);
	bt_mesh_adv_send_start(duration, err, cb, cb_data);
	if (err) {
		BT_ERR("Advertising failed: err %d", err);
		return;
	}

	BT_DBG("Advertising started. Sleeping %u ms", duration);

	k_sleep(K_MSEC(duration));

	err = bt_le_adv_stop();
	bt_mesh_adv_send_end(err, cb, cb_data);
	if (err) {
		BT_ERR("Stopping advertising failed: err %d", err);
		return;
	}

	BT_DBG("Advertising stopped (%u ms)", (uint32_t) k_uptime_delta(&time));
}

static void adv_thread(void *p1, void *p2, void *p3)
{
	BT_DBG("started");

	while (1) {
		struct net_buf *buf;

		if (IS_ENABLED(CONFIG_BT_MESH_PROXY)) {
			buf = net_buf_get(&bt_mesh_adv_queue, K_NO_WAIT);
			while (!buf) {

				/* Adv timeout may be set by a call from proxy
				 * to bt_mesh_adv_start:
				 */
				adv_timeout = SYS_FOREVER_MS;
				bt_mesh_proxy_adv_start();
				BT_DBG("Proxy Advertising");

				buf = net_buf_get(&bt_mesh_adv_queue,
						  SYS_TIMEOUT_MS(adv_timeout));
				bt_le_adv_stop();
			}
		} else {
			buf = net_buf_get(&bt_mesh_adv_queue, K_FOREVER);
		}

		if (!buf) {
			continue;
		}

		/* busy == 0 means this was canceled */
		if (BT_MESH_ADV(buf)->busy) {
			BT_MESH_ADV(buf)->busy = 0U;
			adv_send(buf);
		} else {
			net_buf_unref(buf);
		}

		/* Give other threads a chance to run */
		k_yield();
	}
}

void bt_mesh_adv_update(void)
{
	BT_DBG("");

	k_fifo_cancel_wait(&bt_mesh_adv_queue);
}

void bt_mesh_adv_buf_ready(void)
{
	/* Will be handled automatically */
}

void bt_mesh_adv_init(void)
{
	k_thread_create(&adv_thread_data, adv_thread_stack,
			K_KERNEL_STACK_SIZEOF(adv_thread_stack), adv_thread,
			NULL, NULL, NULL, K_PRIO_COOP(7), 0, K_FOREVER);
	k_thread_name_set(&adv_thread_data, "BT Mesh adv");
}

int bt_mesh_adv_enable(void)
{
	k_thread_start(&adv_thread_data);
	return 0;
}

int bt_mesh_adv_start(const struct bt_le_adv_param *param, int32_t duration,
		      const struct bt_data *ad, size_t ad_len,
		      const struct bt_data *sd, size_t sd_len)
{
	adv_timeout = duration;
	return bt_le_adv_start(param, ad, ad_len, sd, sd_len);
}
