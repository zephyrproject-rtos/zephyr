/*
 * Copyright (c) 2021 Xiaomi Corporation
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/debug/stack.h>

#include <zephyr/net/buf.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/mesh.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_MESH_DEBUG_ADV)
#define LOG_MODULE_NAME bt_mesh_adv_ext
#include "common/log.h"

#include "host/hci_core.h"

#include "adv.h"
#include "net.h"
#include "proxy.h"

/* Convert from ms to 0.625ms units */
#define ADV_INT_FAST_MS    20

#ifndef CONFIG_BT_MESH_RELAY_ADV_SETS
#define CONFIG_BT_MESH_RELAY_ADV_SETS 0
#endif

enum {
	/** Controller is currently advertising */
	ADV_FLAG_ACTIVE,
	/** Advertising sending completed */
	ADV_FLAG_SENT,
	/** Currently performing proxy advertising */
	ADV_FLAG_PROXY,
	/** The send-call has been scheduled. */
	ADV_FLAG_SCHEDULED,
	/** The send-call has been pending. */
	ADV_FLAG_SCHEDULE_PENDING,
	/** Custom adv params have been set, we need to update the parameters on
	 *  the next send.
	 */
	ADV_FLAG_UPDATE_PARAMS,

	/* Number of adv flags. */
	ADV_FLAGS_NUM
};

struct bt_mesh_ext_adv {
	uint8_t tag;
	ATOMIC_DEFINE(flags, ADV_FLAGS_NUM);
	struct bt_le_ext_adv *instance;
	struct net_buf *buf;
	uint64_t timestamp;
	struct k_work_delayable work;
	struct bt_le_adv_param adv_param;
};

static void send_pending_adv(struct k_work *work);
static bool schedule_send(struct bt_mesh_ext_adv *adv);

static STRUCT_SECTION_ITERABLE(bt_mesh_ext_adv, adv_main) = {
	.tag = (BT_MESH_LOCAL_ADV |
#if !defined(CONFIG_BT_MESH_ADV_EXT_GATT_SEPARATE)
		BT_MESH_PROXY_ADV |
#endif /* !CONFIG_BT_MESH_ADV_EXT_GATT_SEPARATE */
		BT_MESH_RELAY_ADV),

	.work = Z_WORK_DELAYABLE_INITIALIZER(send_pending_adv),
};

#if CONFIG_BT_MESH_RELAY_ADV_SETS
static STRUCT_SECTION_ITERABLE(bt_mesh_ext_adv, adv_relay[CONFIG_BT_MESH_RELAY_ADV_SETS]) = {
	[0 ... CONFIG_BT_MESH_RELAY_ADV_SETS - 1] = {
		.tag = BT_MESH_RELAY_ADV,
		.work = Z_WORK_DELAYABLE_INITIALIZER(send_pending_adv),
	}
};
#endif /* CONFIG_BT_MESH_RELAY_ADV_SETS */

#if defined(CONFIG_BT_MESH_ADV_EXT_GATT_SEPARATE)
#define BT_MESH_ADV_COUNT			(1 + CONFIG_BT_MESH_RELAY_ADV_SETS + 1)
static STRUCT_SECTION_ITERABLE(bt_mesh_ext_adv, adv_gatt) = {
	.tag = BT_MESH_PROXY_ADV,
	.work = Z_WORK_DELAYABLE_INITIALIZER(send_pending_adv),
};
#else /* CONFIG_BT_MESH_ADV_EXT_GATT_SEPARATE */
#define BT_MESH_ADV_COUNT			(1 + CONFIG_BT_MESH_RELAY_ADV_SETS)
#endif /* CONFIG_BT_MESH_ADV_EXT_GATT_SEPARATE */

BUILD_ASSERT(CONFIG_BT_EXT_ADV_MAX_ADV_SET >= BT_MESH_ADV_COUNT,
	     "Insufficient adv instances");

static inline struct bt_mesh_ext_adv *relay_adv_get(void)
{
#if CONFIG_BT_MESH_RELAY_ADV_SETS
	return adv_relay;
#else /* !CONFIG_BT_MESH_RELAY_ADV_SETS */
	return &adv_main;
#endif /* CONFIG_BT_MESH_RELAY_ADV_SETS */
}

static inline struct bt_mesh_ext_adv *gatt_adv_get(void)
{
#if defined(CONFIG_BT_MESH_ADV_EXT_GATT_SEPARATE)
	return &adv_gatt;
#else /* !CONFIG_BT_MESH_ADV_EXT_GATT_SEPARATE */
	return &adv_main;
#endif /* CONFIG_BT_MESH_ADV_EXT_GATT_SEPARATE */
}

static int adv_start(struct bt_mesh_ext_adv *adv,
		     const struct bt_le_adv_param *param,
		     struct bt_le_ext_adv_start_param *start,
		     const struct bt_data *ad, size_t ad_len,
		     const struct bt_data *sd, size_t sd_len)
{
	int err;

	if (!adv->instance) {
		BT_ERR("Mesh advertiser not enabled");
		return -ENODEV;
	}

	if (atomic_test_and_set_bit(adv->flags, ADV_FLAG_ACTIVE)) {
		BT_ERR("Advertiser is busy");
		return -EBUSY;
	}

	if (atomic_test_bit(adv->flags, ADV_FLAG_UPDATE_PARAMS)) {
		err = bt_le_ext_adv_update_param(adv->instance, param);
		if (err) {
			BT_ERR("Failed updating adv params: %d", err);
			atomic_clear_bit(adv->flags, ADV_FLAG_ACTIVE);
			return err;
		}

		atomic_set_bit_to(adv->flags, ADV_FLAG_UPDATE_PARAMS,
				  param != &adv->adv_param);
	}

	err = bt_le_ext_adv_set_data(adv->instance, ad, ad_len, sd, sd_len);
	if (err) {
		BT_ERR("Failed setting adv data: %d", err);
		atomic_clear_bit(adv->flags, ADV_FLAG_ACTIVE);
		return err;
	}

	adv->timestamp = k_uptime_get();

	err = bt_le_ext_adv_start(adv->instance, start);
	if (err) {
		BT_ERR("Advertising failed: err %d", err);
		atomic_clear_bit(adv->flags, ADV_FLAG_ACTIVE);
	}

	return err;
}

static int buf_send(struct bt_mesh_ext_adv *adv, struct net_buf *buf)
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
	if (adv->adv_param.interval_min != BT_MESH_ADV_SCAN_UNIT(adv_int)) {
		adv->adv_param.interval_min = BT_MESH_ADV_SCAN_UNIT(adv_int);
		adv->adv_param.interval_max = adv->adv_param.interval_min;
		atomic_set_bit(adv->flags, ADV_FLAG_UPDATE_PARAMS);
	}

	err = adv_start(adv, &adv->adv_param, &start, &ad, 1, NULL, 0);
	if (!err) {
		adv->buf = net_buf_ref(buf);
	}

	bt_mesh_adv_send_start(duration, err, BT_MESH_ADV(buf));

	return err;
}

static void send_pending_adv(struct k_work *work)
{
	struct bt_mesh_ext_adv *adv;
	struct net_buf *buf;
	int err;

	adv = CONTAINER_OF(work, struct bt_mesh_ext_adv, work.work);

	if (atomic_test_and_clear_bit(adv->flags, ADV_FLAG_SENT)) {
		/* Calling k_uptime_delta on a timestamp moves it to the current time.
		 * This is essential here, as schedule_send() uses the end of the event
		 * as a reference to avoid sending the next advertisement too soon.
		 */
		int64_t duration = k_uptime_delta(&adv->timestamp);

		BT_DBG("Advertising stopped after %u ms", (uint32_t)duration);

		atomic_clear_bit(adv->flags, ADV_FLAG_ACTIVE);
		atomic_clear_bit(adv->flags, ADV_FLAG_PROXY);

		if (adv->buf) {
			net_buf_unref(adv->buf);
			adv->buf = NULL;
		}

		(void)schedule_send(adv);

		return;
	}

	atomic_clear_bit(adv->flags, ADV_FLAG_SCHEDULED);

	while ((buf = bt_mesh_adv_buf_get_by_tag(adv->tag, K_NO_WAIT))) {
		/* busy == 0 means this was canceled */
		if (!BT_MESH_ADV(buf)->busy) {
			net_buf_unref(buf);
			continue;
		}

		BT_MESH_ADV(buf)->busy = 0U;
		err = buf_send(adv, buf);

		net_buf_unref(buf);

		if (!err) {
			return; /* Wait for advertising to finish */
		}
	}

	if (!IS_ENABLED(CONFIG_BT_MESH_GATT_SERVER) ||
	    !(adv->tag & BT_MESH_PROXY_ADV)) {
		return;
	}

	if (!bt_mesh_adv_gatt_send()) {
		atomic_set_bit(adv->flags, ADV_FLAG_PROXY);
	}

	if (atomic_test_and_clear_bit(adv->flags, ADV_FLAG_SCHEDULE_PENDING)) {
		schedule_send(adv);
	}

}

static bool schedule_send(struct bt_mesh_ext_adv *adv)
{
	uint64_t timestamp;
	int64_t delta;

	if (!adv) {
		return false;
	}

	timestamp = adv->timestamp;

	if (atomic_test_and_clear_bit(adv->flags, ADV_FLAG_PROXY)) {
		(void)bt_le_ext_adv_stop(adv->instance);
		atomic_clear_bit(adv->flags, ADV_FLAG_ACTIVE);
	}

	if (atomic_test_bit(adv->flags, ADV_FLAG_ACTIVE)) {
		atomic_set_bit(adv->flags, ADV_FLAG_SCHEDULE_PENDING);
		return false;
	} else if (atomic_test_and_set_bit(adv->flags, ADV_FLAG_SCHEDULED)) {
		return false;
	}

	atomic_clear_bit(adv->flags, ADV_FLAG_SCHEDULE_PENDING);

	/* The controller will send the next advertisement immediately.
	 * Introduce a delay here to avoid sending the next mesh packet closer
	 * to the previous packet than what's permitted by the specification.
	 */
	delta = k_uptime_delta(&timestamp);
	k_work_reschedule(&adv->work, K_MSEC(ADV_INT_FAST_MS - delta));

	return true;
}

void bt_mesh_adv_gatt_update(void)
{
	(void)schedule_send(gatt_adv_get());
}

void bt_mesh_adv_buf_local_ready(void)
{
	(void)schedule_send(&adv_main);
}

void bt_mesh_adv_buf_relay_ready(void)
{
	struct bt_mesh_ext_adv *adv = relay_adv_get();

	for (int i = 0; i < CONFIG_BT_MESH_RELAY_ADV_SETS; i++) {
		if (schedule_send(&adv[i])) {
			return;
		}
	}
}

void bt_mesh_adv_init(void)
{
	struct bt_le_adv_param adv_param = {
		.id = BT_ID_DEFAULT,
		.interval_min = BT_MESH_ADV_SCAN_UNIT(ADV_INT_FAST_MS),
		.interval_max = BT_MESH_ADV_SCAN_UNIT(ADV_INT_FAST_MS),
#if defined(CONFIG_BT_MESH_DEBUG_USE_ID_ADDR)
		.options = BT_LE_ADV_OPT_USE_IDENTITY,
#endif
};
	STRUCT_SECTION_FOREACH(bt_mesh_ext_adv, adv) {
		(void)memcpy(&adv->adv_param, &adv_param, sizeof(adv_param));
	}
}

static struct bt_mesh_ext_adv *adv_instance_find(struct bt_le_ext_adv *instance)
{
	STRUCT_SECTION_FOREACH(bt_mesh_ext_adv, adv) {
		if (adv->instance == instance) {
			return adv;
		}
	}

	return NULL;
}

static void adv_sent(struct bt_le_ext_adv *instance,
		     struct bt_le_ext_adv_sent_info *info)
{
	struct bt_mesh_ext_adv *adv = adv_instance_find(instance);

	if (!adv) {
		BT_WARN("Unexpected adv instance");
		return;
	}

	if (!atomic_test_bit(adv->flags, ADV_FLAG_ACTIVE)) {
		return;
	}

	atomic_set_bit(adv->flags, ADV_FLAG_SENT);

	k_work_submit(&adv->work.work);
}

#if defined(CONFIG_BT_MESH_GATT_SERVER)
static void connected(struct bt_le_ext_adv *instance,
		      struct bt_le_ext_adv_connected_info *info)
{
	struct bt_mesh_ext_adv *adv = gatt_adv_get();

	if (atomic_test_and_clear_bit(adv->flags, ADV_FLAG_PROXY)) {
		atomic_clear_bit(adv->flags, ADV_FLAG_ACTIVE);
		(void)schedule_send(adv);
	}
}
#endif /* CONFIG_BT_MESH_GATT_SERVER */

int bt_mesh_adv_enable(void)
{
	int err;

	static const struct bt_le_ext_adv_cb adv_cb = {
		.sent = adv_sent,
#if defined(CONFIG_BT_MESH_GATT_SERVER)
		.connected = connected,
#endif /* CONFIG_BT_MESH_GATT_SERVER */
	};

	if (adv_main.instance) {
		/* Already initialized */
		return 0;
	}


	STRUCT_SECTION_FOREACH(bt_mesh_ext_adv, adv) {
		err = bt_le_ext_adv_create(&adv->adv_param, &adv_cb,
					   &adv->instance);
		if (err) {
			return err;
		}
	}

	return 0;
}

int bt_mesh_adv_gatt_start(const struct bt_le_adv_param *param,
			   int32_t duration,
			   const struct bt_data *ad, size_t ad_len,
			   const struct bt_data *sd, size_t sd_len)
{
	struct bt_mesh_ext_adv *adv = gatt_adv_get();
	struct bt_le_ext_adv_start_param start = {
		/* Timeout is set in 10 ms steps, with 0 indicating "forever" */
		.timeout = (duration == SYS_FOREVER_MS) ? 0 : (duration / 10),
	};

	BT_DBG("Start advertising %d ms", duration);

	atomic_set_bit(adv->flags, ADV_FLAG_UPDATE_PARAMS);

	return adv_start(adv, param, &start, ad, ad_len, sd, sd_len);
}
