/*
 * Copyright (c) 2021 Xiaomi Corporation
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/debug/stack.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/net/buf.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/mesh.h>

#include "common/bt_str.h"

#include "host/hci_core.h"

#include "adv.h"
#include "net.h"
#include "proxy.h"
#include "solicitation.h"

#define LOG_LEVEL CONFIG_BT_MESH_ADV_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_adv_ext);

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
	/** The proxy has been start, but maybe pending. */
	ADV_FLAG_PROXY_START,
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
	const enum bt_mesh_adv_tag_bit tags;
	ATOMIC_DEFINE(flags, ADV_FLAGS_NUM);
	struct bt_le_ext_adv *instance;
	struct net_buf *buf;
	uint64_t timestamp;
	struct k_work_delayable work;
	struct bt_le_adv_param adv_param;
};

static void send_pending_adv(struct k_work *work);
static bool schedule_send(struct bt_mesh_ext_adv *adv);

static struct bt_mesh_ext_adv advs[] = {
	[0] = {
		.tags = (
#if !defined(CONFIG_BT_MESH_ADV_EXT_FRIEND_SEPARATE)
			BT_MESH_ADV_TAG_BIT_FRIEND |
#endif
#if !defined(CONFIG_BT_MESH_ADV_EXT_GATT_SEPARATE)
			BT_MESH_ADV_TAG_BIT_PROXY |
#endif /* !CONFIG_BT_MESH_ADV_EXT_GATT_SEPARATE */
#if defined(CONFIG_BT_MESH_ADV_EXT_RELAY_USING_MAIN_ADV_SET)
			BT_MESH_ADV_TAG_BIT_RELAY |
#endif /* CONFIG_BT_MESH_ADV_EXT_RELAY_USING_MAIN_ADV_SET */
#if defined(CONFIG_BT_MESH_PB_ADV)
			BT_MESH_ADV_TAG_BIT_PROV |
#endif /* CONFIG_BT_MESH_PB_ADV */
			BT_MESH_ADV_TAG_BIT_LOCAL
		),
		.work = Z_WORK_DELAYABLE_INITIALIZER(send_pending_adv),
	},
#if CONFIG_BT_MESH_RELAY_ADV_SETS
	[1 ... CONFIG_BT_MESH_RELAY_ADV_SETS] = {
		.tags = (
#if defined(CONFIG_BT_MESH_RELAY)
			BT_MESH_ADV_TAG_BIT_RELAY |
#endif /* CONFIG_BT_MESH_RELAY */
#if defined(CONFIG_BT_MESH_PB_ADV_USE_RELAY_SETS)
			BT_MESH_ADV_TAG_BIT_PROV |
#endif /* CONFIG_BT_MESH_PB_ADV_USE_RELAY_SETS */
		0),
		.work = Z_WORK_DELAYABLE_INITIALIZER(send_pending_adv),
	},
#endif /* CONFIG_BT_MESH_RELAY_ADV_SETS */
#if defined(CONFIG_BT_MESH_ADV_EXT_FRIEND_SEPARATE)
	{
		.tags = BT_MESH_ADV_TAG_BIT_FRIEND,
		.work = Z_WORK_DELAYABLE_INITIALIZER(send_pending_adv),
	},
#endif /* CONFIG_BT_MESH_ADV_EXT_FRIEND_SEPARATE */
#if defined(CONFIG_BT_MESH_ADV_EXT_GATT_SEPARATE)
	{
		.tags = BT_MESH_ADV_TAG_BIT_PROXY,
		.work = Z_WORK_DELAYABLE_INITIALIZER(send_pending_adv),
	},
#endif /* CONFIG_BT_MESH_ADV_EXT_GATT_SEPARATE */
};

BUILD_ASSERT(ARRAY_SIZE(advs) <= CONFIG_BT_EXT_ADV_MAX_ADV_SET,
	     "Insufficient adv instances");

static inline struct bt_mesh_ext_adv *relay_adv_get(void)
{
	if (!!(CONFIG_BT_MESH_RELAY_ADV_SETS)) {
		return &advs[1];
	} else {
		return &advs[0];
	}
}

static inline struct bt_mesh_ext_adv *gatt_adv_get(void)
{
	if (IS_ENABLED(CONFIG_BT_MESH_ADV_EXT_GATT_SEPARATE)) {
		return &advs[ARRAY_SIZE(advs) - 1];
	} else {
		return &advs[0];
	}
}

static int adv_start(struct bt_mesh_ext_adv *adv,
		     const struct bt_le_adv_param *param,
		     struct bt_le_ext_adv_start_param *start,
		     const struct bt_data *ad, size_t ad_len,
		     const struct bt_data *sd, size_t sd_len)
{
	int err;

	if (!adv->instance) {
		LOG_ERR("Mesh advertiser not enabled");
		return -ENODEV;
	}

	if (atomic_test_and_set_bit(adv->flags, ADV_FLAG_ACTIVE)) {
		LOG_ERR("Advertiser is busy");
		return -EBUSY;
	}

	if (atomic_test_bit(adv->flags, ADV_FLAG_UPDATE_PARAMS)) {
		err = bt_le_ext_adv_update_param(adv->instance, param);
		if (err) {
			LOG_ERR("Failed updating adv params: %d", err);
			atomic_clear_bit(adv->flags, ADV_FLAG_ACTIVE);
			return err;
		}

		atomic_set_bit_to(adv->flags, ADV_FLAG_UPDATE_PARAMS,
				  param != &adv->adv_param);
	}

	err = bt_le_ext_adv_set_data(adv->instance, ad, ad_len, sd, sd_len);
	if (err) {
		LOG_ERR("Failed setting adv data: %d", err);
		atomic_clear_bit(adv->flags, ADV_FLAG_ACTIVE);
		return err;
	}

	adv->timestamp = k_uptime_get();

	err = bt_le_ext_adv_start(adv->instance, start);
	if (err) {
		LOG_ERR("Advertising failed: err %d", err);
		atomic_clear_bit(adv->flags, ADV_FLAG_ACTIVE);
	}

	return err;
}

static int bt_data_send(struct bt_mesh_ext_adv *adv, uint8_t num_events, uint16_t adv_interval,
			const struct bt_data *ad, size_t ad_len)
{
	struct bt_le_ext_adv_start_param start = {
		.num_events = num_events,
	};

	adv_interval = MAX(ADV_INT_FAST_MS, adv_interval);

	/* Only update advertising parameters if they're different */
	if (adv->adv_param.interval_min != BT_MESH_ADV_SCAN_UNIT(adv_interval)) {
		adv->adv_param.interval_min = BT_MESH_ADV_SCAN_UNIT(adv_interval);
		adv->adv_param.interval_max = adv->adv_param.interval_min;
		atomic_set_bit(adv->flags, ADV_FLAG_UPDATE_PARAMS);
	}

	return adv_start(adv, &adv->adv_param, &start, ad, ad_len, NULL, 0);
}

static int buf_send(struct bt_mesh_ext_adv *adv, struct net_buf *buf)
{
	uint8_t num_events = BT_MESH_TRANSMIT_COUNT(BT_MESH_ADV(buf)->xmit) + 1;
	uint16_t duration, adv_int;
	struct bt_data ad;
	int err;

	adv_int = BT_MESH_TRANSMIT_INT(BT_MESH_ADV(buf)->xmit);
	/* Upper boundary estimate: */
	duration = num_events * (adv_int + 10);

	LOG_DBG("type %u len %u: %s", BT_MESH_ADV(buf)->type,
	       buf->len, bt_hex(buf->data, buf->len));
	LOG_DBG("count %u interval %ums duration %ums",
	       num_events, adv_int, duration);

	ad.type = bt_mesh_adv_type[BT_MESH_ADV(buf)->type];
	ad.data_len = buf->len;
	ad.data = buf->data;

	err = bt_data_send(adv, num_events, adv_int, &ad, 1);
	if (!err) {
		adv->buf = net_buf_ref(buf);
	}

	bt_mesh_adv_send_start(duration, err, BT_MESH_ADV(buf));

	return err;
}

static const char * const adv_tag_to_str[] = {
	[BT_MESH_ADV_TAG_LOCAL]  = "local adv",
	[BT_MESH_ADV_TAG_RELAY]  = "relay adv",
	[BT_MESH_ADV_TAG_PROXY]  = "proxy adv",
	[BT_MESH_ADV_TAG_FRIEND] = "friend adv",
	[BT_MESH_ADV_TAG_PROV]   = "prov adv",
};

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

		LOG_DBG("Advertising stopped after %u ms for %s", (uint32_t)duration,
		       adv->buf ? adv_tag_to_str[BT_MESH_ADV(adv->buf)->tag] :
				  adv_tag_to_str[BT_MESH_ADV_TAG_PROXY]);

		atomic_clear_bit(adv->flags, ADV_FLAG_ACTIVE);
		atomic_clear_bit(adv->flags, ADV_FLAG_PROXY);
		atomic_clear_bit(adv->flags, ADV_FLAG_PROXY_START);

		if (adv->buf) {
			net_buf_unref(adv->buf);
			adv->buf = NULL;
		}

		(void)schedule_send(adv);

		return;
	}

	atomic_clear_bit(adv->flags, ADV_FLAG_SCHEDULED);

	while ((buf = bt_mesh_adv_buf_get_by_tag(adv->tags, K_NO_WAIT))) {
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
	    !(adv->tags & BT_MESH_ADV_TAG_BIT_PROXY)) {
		return;
	}

	if (IS_ENABLED(CONFIG_BT_MESH_PROXY_SOLICITATION) &&
	    !bt_mesh_sol_send()) {
		return;
	}

	atomic_set_bit(adv->flags, ADV_FLAG_PROXY_START);

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

	timestamp = adv->timestamp;

	if (atomic_test_and_clear_bit(adv->flags, ADV_FLAG_PROXY)) {
		atomic_clear_bit(adv->flags, ADV_FLAG_PROXY_START);
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

	if ((IS_ENABLED(CONFIG_BT_MESH_ADV_EXT_FRIEND_SEPARATE) &&
	     adv->tags & BT_MESH_ADV_TAG_BIT_FRIEND) ||
	    (CONFIG_BT_MESH_RELAY_ADV_SETS > 0 && adv->tags & BT_MESH_ADV_TAG_BIT_RELAY)) {
		k_work_reschedule(&adv->work, K_NO_WAIT);
	} else {
		/* The controller will send the next advertisement immediately.
		 * Introduce a delay here to avoid sending the next mesh packet closer
		 * to the previous packet than what's permitted by the specification.
		 */
		delta = k_uptime_delta(&timestamp);
		k_work_reschedule(&adv->work, K_MSEC(ADV_INT_FAST_MS - delta));
	}

	return true;
}

void bt_mesh_adv_gatt_update(void)
{
	(void)schedule_send(gatt_adv_get());
}

void bt_mesh_adv_buf_local_ready(void)
{
	(void)schedule_send(advs);
}

void bt_mesh_adv_buf_relay_ready(void)
{
	struct bt_mesh_ext_adv *adv = relay_adv_get();

	for (int i = 0; i < CONFIG_BT_MESH_RELAY_ADV_SETS; i++) {
		if (schedule_send(&adv[i])) {
			return;
		}
	}

	/* Attempt to use the main adv set for the sending of relay messages. */
	if (IS_ENABLED(CONFIG_BT_MESH_ADV_EXT_RELAY_USING_MAIN_ADV_SET)) {
		(void)schedule_send(advs);
	}
}

void bt_mesh_adv_buf_friend_ready(void)
{
	if (IS_ENABLED(CONFIG_BT_MESH_ADV_EXT_FRIEND_SEPARATE)) {
		schedule_send(&advs[1 + CONFIG_BT_MESH_RELAY_ADV_SETS]);
	} else {
		schedule_send(&advs[0]);
	}
}

void bt_mesh_adv_buf_terminate(const struct net_buf *buf)
{
	int err;

	for (int i = 0; i < ARRAY_SIZE(advs); i++) {
		struct bt_mesh_ext_adv *adv = &advs[i];

		if (adv->buf != buf) {
			continue;
		}

		if (!atomic_test_bit(adv->flags, ADV_FLAG_ACTIVE)) {
			return;
		}

		err = bt_le_ext_adv_stop(adv->instance);
		if (err) {
			LOG_ERR("Failed to stop adv %d", err);
			return;
		}

		/* Do not call `cb:end`, since this user action */
		BT_MESH_ADV(adv->buf)->cb = NULL;

		atomic_set_bit(adv->flags, ADV_FLAG_SENT);

		k_work_submit(&adv->work.work);

		return;
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

	for (int i = 0; i < ARRAY_SIZE(advs); i++) {
		(void)memcpy(&advs[i].adv_param, &adv_param, sizeof(adv_param));
	}
}

static struct bt_mesh_ext_adv *adv_instance_find(struct bt_le_ext_adv *instance)
{
	for (int i = 0; i < ARRAY_SIZE(advs); i++) {
		if (advs[i].instance == instance) {
			return &advs[i];
		}
	}

	return NULL;
}

static void adv_sent(struct bt_le_ext_adv *instance,
		     struct bt_le_ext_adv_sent_info *info)
{
	struct bt_mesh_ext_adv *adv = adv_instance_find(instance);

	if (!adv) {
		LOG_WRN("Unexpected adv instance");
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

	if (atomic_test_and_clear_bit(adv->flags, ADV_FLAG_PROXY_START)) {
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

	if (advs[0].instance) {
		/* Already initialized */
		return 0;
	}

	for (int i = 0; i < ARRAY_SIZE(advs); i++) {
		err = bt_le_ext_adv_create(&advs[i].adv_param, &adv_cb,
					   &advs[i].instance);
		if (err) {
			return err;
		}
	}

	return 0;
}

int bt_mesh_adv_disable(void)
{
	int err;
	struct k_work_sync sync;

	for (int i = 0; i < ARRAY_SIZE(advs); i++) {
		k_work_flush_delayable(&advs[i].work, &sync);

		err = bt_le_ext_adv_stop(advs[i].instance);
		if (err) {
			LOG_ERR("Failed to stop adv %d", err);
			return err;
		}

		/* `adv_sent` is called to finish transmission of an adv buffer that was pushed to
		 * the host before the advertiser was stopped, but did not finish.
		 */
		adv_sent(advs[i].instance, NULL);

		err = bt_le_ext_adv_delete(advs[i].instance);
		if (err) {
			LOG_ERR("Failed to delete adv %d", err);
			return err;
		}
		advs[i].instance = NULL;
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
		.timeout = (duration == SYS_FOREVER_MS) ? 0 : MAX(1, duration / 10),
	};

	LOG_DBG("Start advertising %d ms", duration);

	atomic_set_bit(adv->flags, ADV_FLAG_UPDATE_PARAMS);

	return adv_start(adv, param, &start, ad, ad_len, sd, sd_len);
}

int bt_mesh_adv_bt_data_send(uint8_t num_events, uint16_t adv_interval,
			     const struct bt_data *ad, size_t ad_len)
{
	return bt_data_send(advs, num_events, adv_interval, ad, ad_len);
}
