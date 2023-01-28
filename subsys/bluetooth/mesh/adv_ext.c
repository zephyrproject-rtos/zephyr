/*
 * Copyright (c) 2021 Xiaomi Corporation
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/debug/stack.h>

#include <zephyr/net/buf.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/mesh.h>

#include "common/bt_str.h"

#include "host/hci_core.h"

#include "adv.h"
#include "net.h"
#include "proxy.h"

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
	struct bt_mesh_adv *adv;
	uint64_t timestamp;
	struct k_work_delayable work;
	struct bt_le_adv_param adv_param;
};

static void send_pending_adv(struct k_work *work);
static bool schedule_send(struct bt_mesh_ext_adv *ext_adv);

static STRUCT_SECTION_ITERABLE(bt_mesh_ext_adv, adv_main) = {
	.tag = (
#if !defined(CONFIG_BT_MESH_ADV_EXT_FRIEND_SEPARATE)
		BT_MESH_FRIEND_ADV |
#endif
#if !defined(CONFIG_BT_MESH_ADV_EXT_GATT_SEPARATE)
		BT_MESH_PROXY_ADV |
#endif /* !CONFIG_BT_MESH_ADV_EXT_GATT_SEPARATE */
#if defined(CONFIG_BT_MESH_ADV_EXT_RELAY_USING_MAIN_ADV_SET)
		BT_MESH_RELAY_ADV |
#endif /* CONFIG_BT_MESH_ADV_EXT_RELAY_USING_MAIN_ADV_SET */
		BT_MESH_LOCAL_ADV),

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

#if defined(CONFIG_BT_MESH_ADV_EXT_FRIEND_SEPARATE)
#define ADV_EXT_FRIEND 1
static STRUCT_SECTION_ITERABLE(bt_mesh_ext_adv, adv_friend) = {
	.tag = BT_MESH_FRIEND_ADV,
	.work = Z_WORK_DELAYABLE_INITIALIZER(send_pending_adv),
};
#else /* CONFIG_BT_MESH_ADV_EXT_FRIEND_SEPARATE */
#define ADV_EXT_FRIEND 0
#endif /* CONFIG_BT_MESH_ADV_EXT_FRIEND_SEPARATE */

#if defined(CONFIG_BT_MESH_ADV_EXT_GATT_SEPARATE)
#define ADV_EXT_GATT 1
static STRUCT_SECTION_ITERABLE(bt_mesh_ext_adv, adv_gatt) = {
	.tag = BT_MESH_PROXY_ADV,
	.work = Z_WORK_DELAYABLE_INITIALIZER(send_pending_adv),
};
#else /* CONFIG_BT_MESH_ADV_EXT_GATT_SEPARATE */
#define ADV_EXT_GATT 0
#endif /* CONFIG_BT_MESH_ADV_EXT_GATT_SEPARATE */

#define BT_MESH_ADV_COUNT (1 + CONFIG_BT_MESH_RELAY_ADV_SETS + ADV_EXT_FRIEND + ADV_EXT_GATT)

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

static int adv_start(struct bt_mesh_ext_adv *ext_adv,
		     const struct bt_le_adv_param *param,
		     struct bt_le_ext_adv_start_param *start,
		     const struct bt_data *ad, size_t ad_len,
		     const struct bt_data *sd, size_t sd_len)
{
	int err;

	if (!ext_adv->instance) {
		LOG_ERR("Mesh advertiser not enabled");
		return -ENODEV;
	}

	if (atomic_test_and_set_bit(ext_adv->flags, ADV_FLAG_ACTIVE)) {
		LOG_ERR("Advertiser is busy");
		return -EBUSY;
	}

	if (atomic_test_bit(ext_adv->flags, ADV_FLAG_UPDATE_PARAMS)) {
		err = bt_le_ext_adv_update_param(ext_adv->instance, param);
		if (err) {
			LOG_ERR("Failed updating adv params: %d", err);
			atomic_clear_bit(ext_adv->flags, ADV_FLAG_ACTIVE);
			return err;
		}

		atomic_set_bit_to(ext_adv->flags, ADV_FLAG_UPDATE_PARAMS,
				  param != &ext_adv->adv_param);
	}

	err = bt_le_ext_adv_set_data(ext_adv->instance, ad, ad_len, sd, sd_len);
	if (err) {
		LOG_ERR("Failed setting adv data: %d", err);
		atomic_clear_bit(ext_adv->flags, ADV_FLAG_ACTIVE);
		return err;
	}

	ext_adv->timestamp = k_uptime_get();

	err = bt_le_ext_adv_start(ext_adv->instance, start);
	if (err) {
		LOG_ERR("Advertising failed: err %d", err);
		atomic_clear_bit(ext_adv->flags, ADV_FLAG_ACTIVE);
	}

	return err;
}

static int adv_send(struct bt_mesh_ext_adv *ext_adv, struct bt_mesh_adv *adv)
{
	struct bt_le_ext_adv_start_param start = {
		.num_events =
			BT_MESH_TRANSMIT_COUNT(adv->meta.xmit) + 1,
	};
	uint16_t duration, adv_int;
	struct bt_data ad;
	int err;

	adv_int = MAX(ADV_INT_FAST_MS,
		      BT_MESH_TRANSMIT_INT(adv->meta.xmit));
	/* Upper boundary estimate: */
	duration = start.num_events * (adv_int + 10);

	LOG_DBG("type %u len %u: %s", adv->meta.type, adv->b.len,
		bt_hex(adv->b.data, adv->b.len));
	LOG_DBG("count %u interval %ums duration %ums",
		BT_MESH_TRANSMIT_COUNT(adv->meta.xmit) + 1, adv_int, duration);

	ad.type = bt_mesh_adv_type[adv->meta.type];
	ad.data_len = adv->b.len;
	ad.data = adv->b.data;

	/* Only update advertising parameters if they're different */
	if (ext_adv->adv_param.interval_min != BT_MESH_ADV_SCAN_UNIT(adv_int)) {
		ext_adv->adv_param.interval_min = BT_MESH_ADV_SCAN_UNIT(adv_int);
		ext_adv->adv_param.interval_max = ext_adv->adv_param.interval_min;
		atomic_set_bit(ext_adv->flags, ADV_FLAG_UPDATE_PARAMS);
	}

	err = adv_start(ext_adv, &ext_adv->adv_param, &start, &ad, 1, NULL, 0);
	if (!err) {
		ext_adv->adv = bt_mesh_adv_ref(adv);
	}

	bt_mesh_adv_send_start(duration, err, &adv->meta);

	return err;
}

static const char *adv_tag_to_str(enum bt_mesh_adv_tag tag)
{
	if (tag & BT_MESH_LOCAL_ADV) {
		return "local adv";
	} else if (tag & BT_MESH_PROXY_ADV) {
		return "proxy adv";
	} else if (tag & BT_MESH_RELAY_ADV) {
		return "relay adv";
	} else if (tag & BT_MESH_FRIEND_ADV) {
		return "friend adv";
	} else {
		return "(unknown tag)";
	}
}

static void send_pending_adv(struct k_work *work)
{
	struct bt_mesh_ext_adv *ext_adv;
	struct bt_mesh_adv *adv;
	int err;

	ext_adv = CONTAINER_OF(work, struct bt_mesh_ext_adv, work.work);

	if (atomic_test_and_clear_bit(ext_adv->flags, ADV_FLAG_SENT)) {
		/* Calling k_uptime_delta on a timestamp moves it to the current time.
		 * This is essential here, as schedule_send() uses the end of the event
		 * as a reference to avoid sending the next advertisement too soon.
		 */
		int64_t duration = k_uptime_delta(&ext_adv->timestamp);

		LOG_DBG("Advertising stopped after %u ms for (%u) %s",
			(uint32_t)duration, ext_adv->tag, adv_tag_to_str(ext_adv->tag));

		atomic_clear_bit(ext_adv->flags, ADV_FLAG_ACTIVE);
		atomic_clear_bit(ext_adv->flags, ADV_FLAG_PROXY);

		if (ext_adv->adv) {
			bt_mesh_adv_unref(ext_adv->adv);
			ext_adv->adv = NULL;
		}

		(void)schedule_send(ext_adv);

		return;
	}

	atomic_clear_bit(ext_adv->flags, ADV_FLAG_SCHEDULED);

	while ((adv = bt_mesh_adv_get_by_tag(ext_adv->tag, K_NO_WAIT))) {
		/* busy == 0 means this was canceled */
		if (!adv->meta.busy) {
			bt_mesh_adv_unref(adv);
			continue;
		}

		adv->meta.busy = 0U;
		err = adv_send(ext_adv, adv);

		bt_mesh_adv_unref(adv);

		if (!err) {
			return; /* Wait for advertising to finish */
		}
	}

	if (!IS_ENABLED(CONFIG_BT_MESH_GATT_SERVER) ||
	    !(ext_adv->tag & BT_MESH_PROXY_ADV)) {
		return;
	}

	if (!bt_mesh_adv_gatt_send()) {
		atomic_set_bit(ext_adv->flags, ADV_FLAG_PROXY);
	}

	if (atomic_test_and_clear_bit(ext_adv->flags, ADV_FLAG_SCHEDULE_PENDING)) {
		schedule_send(ext_adv);
	}

}

static bool schedule_send(struct bt_mesh_ext_adv *ext_adv)
{
	uint64_t timestamp;
	int64_t delta;

	timestamp = ext_adv->timestamp;

	if (atomic_test_and_clear_bit(ext_adv->flags, ADV_FLAG_PROXY)) {
		(void)bt_le_ext_adv_stop(ext_adv->instance);
		atomic_clear_bit(ext_adv->flags, ADV_FLAG_ACTIVE);
	}

	if (atomic_test_bit(ext_adv->flags, ADV_FLAG_ACTIVE)) {
		atomic_set_bit(ext_adv->flags, ADV_FLAG_SCHEDULE_PENDING);
		return false;
	} else if (atomic_test_and_set_bit(ext_adv->flags, ADV_FLAG_SCHEDULED)) {
		return false;
	}

	atomic_clear_bit(ext_adv->flags, ADV_FLAG_SCHEDULE_PENDING);

	if (IS_ENABLED(CONFIG_BT_MESH_ADV_EXT_FRIEND_SEPARATE) &&
	    ext_adv->tag & BT_MESH_FRIEND_ADV) {
		k_work_reschedule(&ext_adv->work, K_NO_WAIT);
	} else {
		/* The controller will send the next advertisement immediately.
		 * Introduce a delay here to avoid sending the next mesh packet closer
		 * to the previous packet than what's permitted by the specification.
		 */
		delta = k_uptime_delta(&timestamp);
		k_work_reschedule(&ext_adv->work, K_MSEC(ADV_INT_FAST_MS - delta));
	}

	return true;
}

void bt_mesh_adv_gatt_update(void)
{
	(void)schedule_send(gatt_adv_get());
}

void bt_mesh_adv_local_ready(void)
{
	(void)schedule_send(&adv_main);
}

void bt_mesh_adv_relay_ready(void)
{
	struct bt_mesh_ext_adv *ext_adv = relay_adv_get();

	for (int i = 0; i < CONFIG_BT_MESH_RELAY_ADV_SETS; i++) {
		if (schedule_send(&ext_adv[i])) {
			return;
		}
	}

	/* Attempt to use the main adv set for the sending of relay messages. */
	if (IS_ENABLED(CONFIG_BT_MESH_ADV_EXT_RELAY_USING_MAIN_ADV_SET)) {
		(void)schedule_send(&adv_main);
	}
}

void bt_mesh_adv_friend_ready(void)
{
#if defined(CONFIG_BT_MESH_ADV_EXT_FRIEND_SEPARATE)
	(void)schedule_send(&adv_friend);
#endif
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
	STRUCT_SECTION_FOREACH(bt_mesh_ext_adv, ext_adv) {
		(void)memcpy(&ext_adv->adv_param, &adv_param, sizeof(adv_param));
	}
}

static struct bt_mesh_ext_adv *adv_instance_find(struct bt_le_ext_adv *instance)
{
	STRUCT_SECTION_FOREACH(bt_mesh_ext_adv, ext_adv) {
		if (ext_adv->instance == instance) {
			return ext_adv;
		}
	}

	return NULL;
}

static void adv_sent(struct bt_le_ext_adv *instance,
		     struct bt_le_ext_adv_sent_info *info)
{
	struct bt_mesh_ext_adv *ext_adv = adv_instance_find(instance);

	if (!ext_adv) {
		LOG_WRN("Unexpected adv instance");
		return;
	}

	if (!atomic_test_bit(ext_adv->flags, ADV_FLAG_ACTIVE)) {
		return;
	}

	atomic_set_bit(ext_adv->flags, ADV_FLAG_SENT);

	k_work_submit(&ext_adv->work.work);
}

#if defined(CONFIG_BT_MESH_GATT_SERVER)
static void connected(struct bt_le_ext_adv *instance,
		      struct bt_le_ext_adv_connected_info *info)
{
	struct bt_mesh_ext_adv *ext_adv = gatt_adv_get();

	if (atomic_test_and_clear_bit(ext_adv->flags, ADV_FLAG_PROXY)) {
		atomic_clear_bit(ext_adv->flags, ADV_FLAG_ACTIVE);
		(void)schedule_send(ext_adv);
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


	STRUCT_SECTION_FOREACH(bt_mesh_ext_adv, ext_adv) {
		err = bt_le_ext_adv_create(&ext_adv->adv_param, &adv_cb,
					   &ext_adv->instance);
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
	struct bt_mesh_ext_adv *ext_adv = gatt_adv_get();
	struct bt_le_ext_adv_start_param start = {
		/* Timeout is set in 10 ms steps, with 0 indicating "forever" */
		.timeout = (duration == SYS_FOREVER_MS) ? 0 : MAX(1, duration / 10),
	};

	LOG_DBG("Start advertising %d ms", duration);

	atomic_set_bit(ext_adv->flags, ADV_FLAG_UPDATE_PARAMS);

	return adv_start(ext_adv, param, &start, ad, ad_len, sd, sd_len);
}
