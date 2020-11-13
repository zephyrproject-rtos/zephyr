/*  Bluetooth Mesh */

/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <debug/stack.h>

#include <net/buf.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/mesh.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_MESH_DEBUG_ADV)
#define LOG_MODULE_NAME bt_mesh_adv_ext
#include "common/log.h"

#include "host/hci_core.h"

#include "adv.h"
#include "net.h"
#include "proxy.h"

/* Convert from ms to 0.625ms units */
#define ADV_INT_FAST_MS    20

static struct bt_le_adv_param adv_param = {
	.id = BT_ID_DEFAULT,
	.interval_min = BT_MESH_ADV_SCAN_UNIT(ADV_INT_FAST_MS),
	.interval_max = BT_MESH_ADV_SCAN_UNIT(ADV_INT_FAST_MS),
#if defined(CONFIG_BT_MESH_DEBUG_USE_ID_ADDR)
	.options = BT_LE_ADV_OPT_USE_IDENTITY,
#endif
};

enum {
	/** Controller is currently advertising */
	ADV_FLAG_ACTIVE,
	/** Currently performing proxy advertising */
	ADV_FLAG_PROXY,
	/** The send-call has been scheduled. */
	ADV_FLAG_SCHEDULED,
	/** Custom adv params have been set, we need to update the parameters on
	 *  the next send.
	 */
	ADV_FLAG_UPDATE_PARAMS,

	/* Number of adv flags. */
	ADV_FLAGS_NUM
};

static struct {
	ATOMIC_DEFINE(flags, ADV_FLAGS_NUM);
	struct bt_le_ext_adv *instance;
	const struct bt_mesh_send_cb *cb;
	void *cb_data;
	uint64_t timestamp;
	struct k_delayed_work work;
} adv;

static int adv_start(const struct bt_le_adv_param *param,
		     struct bt_le_ext_adv_start_param *start,
		     const struct bt_data *ad, size_t ad_len,
		     const struct bt_data *sd, size_t sd_len)
{
	int err;

	if (!adv.instance) {
		BT_ERR("Mesh advertiser not enabled");
		return -ENODEV;
	}

	if (atomic_test_and_set_bit(adv.flags, ADV_FLAG_ACTIVE)) {
		BT_ERR("Advertiser is busy");
		return -EBUSY;
	}

	if (atomic_test_bit(adv.flags, ADV_FLAG_UPDATE_PARAMS)) {
		err = bt_le_ext_adv_update_param(adv.instance, param);
		if (err) {
			BT_ERR("Failed updating adv params: %d", err);
			atomic_clear_bit(adv.flags, ADV_FLAG_ACTIVE);
			return err;
		}

		atomic_set_bit_to(adv.flags, ADV_FLAG_UPDATE_PARAMS,
				  param != &adv_param);
	}

	err = bt_le_ext_adv_set_data(adv.instance, ad, ad_len, sd, sd_len);
	if (err) {
		BT_ERR("Failed setting adv data: %d", err);
		atomic_clear_bit(adv.flags, ADV_FLAG_ACTIVE);
		return err;
	}

	adv.timestamp = k_uptime_get();

	err = bt_le_ext_adv_start(adv.instance, start);
	if (err) {
		BT_ERR("Advertising failed: err %d", err);
		atomic_clear_bit(adv.flags, ADV_FLAG_ACTIVE);
	}

	return err;
}

static int buf_send(struct net_buf *buf)
{
	struct bt_le_ext_adv_start_param start = {
		.num_events =
			BT_MESH_TRANSMIT_COUNT(BT_MESH_ADV(buf)->xmit) + 1,
	};
	uint16_t duration, adv_int;
	struct bt_data ad;
	int err;

	adv_int = MAX(ADV_INT_FAST_MS,
		      BT_MESH_TRANSMIT_INT(BT_MESH_ADV(buf)->xmit));
	/* Upper boundary estimate: */
	duration = start.num_events * (adv_int + 10);

	BT_DBG("type %u len %u: %s", BT_MESH_ADV(buf)->type,
	       buf->len, bt_hex(buf->data, buf->len));
	BT_DBG("count %u interval %ums duration %ums",
	       BT_MESH_TRANSMIT_COUNT(BT_MESH_ADV(buf)->xmit) + 1, adv_int,
	       duration);

	ad.type = bt_mesh_adv_type[BT_MESH_ADV(buf)->type];
	ad.data_len = buf->len;
	ad.data = buf->data;

	/* Only update advertising parameters if they're different */
	if (adv_param.interval_min != BT_MESH_ADV_SCAN_UNIT(adv_int)) {
		adv_param.interval_min = BT_MESH_ADV_SCAN_UNIT(adv_int);
		adv_param.interval_max = adv_param.interval_min;
		atomic_set_bit(adv.flags, ADV_FLAG_UPDATE_PARAMS);
	}

	adv.cb = BT_MESH_ADV(buf)->cb;
	adv.cb_data = BT_MESH_ADV(buf)->cb_data;

	err = adv_start(&adv_param, &start, &ad, 1, NULL, 0);
	net_buf_unref(buf);
	bt_mesh_adv_send_start(duration, err, adv.cb, adv.cb_data);

	return err;
}

static void send_pending_adv(struct k_work *work)
{
	struct net_buf *buf;
	int err;

	atomic_clear_bit(adv.flags, ADV_FLAG_SCHEDULED);

	while ((buf = net_buf_get(&bt_mesh_adv_queue, K_NO_WAIT))) {
		/* busy == 0 means this was canceled */
		if (!BT_MESH_ADV(buf)->busy) {
			net_buf_unref(buf);
			continue;
		}

		BT_MESH_ADV(buf)->busy = 0U;
		err = buf_send(buf);
		if (!err) {
			return; /* Wait for advertising to finish */
		}
	}

	/* No more pending buffers */
	if (IS_ENABLED(CONFIG_BT_MESH_PROXY)) {
		BT_DBG("Proxy Advertising");
		err = bt_mesh_proxy_adv_start();
		if (!err) {
			atomic_set_bit(adv.flags, ADV_FLAG_PROXY);
		}
	}
}

static void schedule_send(void)
{
	uint64_t timestamp = adv.timestamp;
	int64_t delta;

	if (atomic_test_and_clear_bit(adv.flags, ADV_FLAG_PROXY)) {
		bt_le_ext_adv_stop(adv.instance);
		atomic_clear_bit(adv.flags, ADV_FLAG_ACTIVE);
	}

	if (atomic_test_bit(adv.flags, ADV_FLAG_ACTIVE) ||
	    atomic_test_and_set_bit(adv.flags, ADV_FLAG_SCHEDULED)) {
		return;
	}

	/* The controller will send the next advertisement immediately.
	 * Introduce a delay here to avoid sending the next mesh packet closer
	 * to the previous packet than what's permitted by the specification.
	 */
	delta = k_uptime_delta(&timestamp);
	k_delayed_work_submit(&adv.work, K_MSEC(ADV_INT_FAST_MS - delta));
}

void bt_mesh_adv_update(void)
{
	BT_DBG("");

	schedule_send();
}

void bt_mesh_adv_buf_ready(void)
{
	schedule_send();
}

void bt_mesh_adv_init(void)
{
	k_delayed_work_init(&adv.work, send_pending_adv);
}

static void adv_sent(struct bt_le_ext_adv *instance,
		     struct bt_le_ext_adv_sent_info *info)
{
	/* Calling k_uptime_delta on a timestamp moves it to the current time.
	 * This is essential here, as schedule_send() uses the end of the event
	 * as a reference to avoid sending the next advertisement too soon.
	 */
	int64_t duration = k_uptime_delta(&adv.timestamp);

	BT_DBG("Advertising stopped after %u ms", (uint32_t)duration);

	atomic_clear_bit(adv.flags, ADV_FLAG_ACTIVE);

	if (!atomic_test_and_clear_bit(adv.flags, ADV_FLAG_PROXY)) {
		bt_mesh_adv_send_end(0, adv.cb, adv.cb_data);
	}

	schedule_send();
}

static void connected(struct bt_le_ext_adv *instance,
		      struct bt_le_ext_adv_connected_info *info)
{
	if (atomic_test_and_clear_bit(adv.flags, ADV_FLAG_PROXY)) {
		atomic_clear_bit(adv.flags, ADV_FLAG_ACTIVE);
		schedule_send();
	}
}

int bt_mesh_adv_enable(void)
{
	static const struct bt_le_ext_adv_cb adv_cb = {
		.sent = adv_sent,
		.connected = connected,
	};

	if (adv.instance) {
		/* Already initialized */
		return 0;
	}

	return bt_le_ext_adv_create(&adv_param, &adv_cb, &adv.instance);
}

int bt_mesh_adv_start(const struct bt_le_adv_param *param, int32_t duration,
		      const struct bt_data *ad, size_t ad_len,
		      const struct bt_data *sd, size_t sd_len)
{
	struct bt_le_ext_adv_start_param start = {
		/* Timeout is set in 10 ms steps, with 0 indicating "forever" */
		.timeout = (duration == SYS_FOREVER_MS) ? 0 : (duration / 10),
	};

	BT_DBG("Start advertising %d ms", duration);

	atomic_set_bit(adv.flags, ADV_FLAG_UPDATE_PARAMS);

	return adv_start(param, &start, ad, ad_len, sd, sd_len);
}
