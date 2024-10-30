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

#include "host/hci_core.h"

#include "net.h"
#include "foundation.h"
#include "beacon.h"
#include "prov.h"
#include "solicitation.h"

#define LOG_LEVEL CONFIG_BT_MESH_ADV_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_adv_legacy);

/* Pre-5.0 controllers enforce a minimum interval of 100ms
 * whereas 5.0+ controllers can go down to 20ms.
 */
#define ADV_INT_DEFAULT_MS 100
#define ADV_INT_FAST_MS    20

static struct k_thread adv_thread_data;
static K_KERNEL_STACK_DEFINE(adv_thread_stack, CONFIG_BT_MESH_ADV_STACK_SIZE);
static int32_t adv_timeout;

static bool is_mesh_suspended(void)
{
	return atomic_test_bit(bt_mesh.flags, BT_MESH_SUSPENDED);
}

static int bt_data_send(uint8_t num_events, uint16_t adv_int,
			const struct bt_data *ad, size_t ad_len,
			struct bt_mesh_adv_ctx *ctx)
{
	struct bt_le_adv_param param = {};
	uint64_t uptime = k_uptime_get();
	uint16_t duration;
	int err;
	const int32_t adv_int_min =
		((bt_dev.hci_version >= BT_HCI_VERSION_5_0) ?
		 ADV_INT_FAST_MS :
		 ADV_INT_DEFAULT_MS);

	adv_int = MAX(adv_int_min, adv_int);

	ARG_UNUSED(uptime);

	/* Zephyr Bluetooth Low Energy Controller for mesh stack uses
	 * pre-emptible continuous scanning, allowing advertising events to be
	 * transmitted without delay when advertising is enabled. No need to
	 * compensate with scan window duration.
	 * An advertising event could be delayed by upto one interval when
	 * advertising is stopped and started in quick succession, hence add
	 * advertising interval to the total advertising duration.
	 */
	duration = adv_int + num_events * (adv_int + 10);

	/* Zephyr Bluetooth Low Energy Controller built for nRF51x SoCs use
	 * CONFIG_BT_CTLR_LOW_LAT=y, and continuous scanning cannot be
	 * pre-empted, hence, scanning will block advertising events from
	 * being transmitted. Increase the advertising duration by the
	 * amount of scan window duration to compensate for the blocked
	 * advertising events.
	 */
	if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT)) {
		duration += BT_MESH_SCAN_WINDOW_MS;
	}

	LOG_DBG("count %u interval %ums duration %ums",
		num_events, adv_int, duration);

	if (IS_ENABLED(CONFIG_BT_MESH_DEBUG_USE_ID_ADDR)) {
		param.options = BT_LE_ADV_OPT_USE_IDENTITY;
	} else {
		param.options = 0U;
	}

	param.id = BT_ID_DEFAULT;
	param.interval_min = BT_MESH_ADV_SCAN_UNIT(adv_int);
	param.interval_max = param.interval_min;

	err = bt_le_adv_start(&param, ad, ad_len, NULL, 0);

	if (err) {
		LOG_ERR("Advertising failed: err %d", err);
		return err;
	}

	LOG_DBG("Advertising started. Sleeping %u ms", duration);

	if (ctx) {
		bt_mesh_adv_send_start(duration, err, ctx);
	}

	if (!is_mesh_suspended()) {
		k_sleep(K_MSEC(duration));
	}

	err = bt_le_adv_stop();
	if (err) {
		LOG_ERR("Stopping advertising failed: err %d", err);
		return err;
	}

	LOG_DBG("Advertising stopped (%u ms)", (uint32_t) k_uptime_delta(&uptime));

	return 0;
}

int bt_mesh_adv_bt_data_send(uint8_t num_events, uint16_t adv_int,
			     const struct bt_data *ad, size_t ad_len)
{
	return bt_data_send(num_events, adv_int, ad, ad_len, NULL);
}

static inline void adv_send(struct bt_mesh_adv *adv)
{
	uint16_t num_events = BT_MESH_TRANSMIT_COUNT(adv->ctx.xmit) + 1;
	uint16_t adv_int;
	struct bt_data ad;

	adv_int = BT_MESH_TRANSMIT_INT(adv->ctx.xmit);

	LOG_DBG("type %u len %u: %s", adv->ctx.type,
	       adv->b.len, bt_hex(adv->b.data, adv->b.len));

	ad.type = bt_mesh_adv_type[adv->ctx.type];
	ad.data_len = adv->b.len;
	ad.data = adv->b.data;

	bt_data_send(num_events, adv_int, &ad, 1, &adv->ctx);
}

static void adv_thread(void *p1, void *p2, void *p3)
{
	LOG_DBG("started");
	struct bt_mesh_adv *adv;

	while (!is_mesh_suspended()) {
		if (IS_ENABLED(CONFIG_BT_MESH_GATT_SERVER)) {
			adv = bt_mesh_adv_get(K_NO_WAIT);
			if (IS_ENABLED(CONFIG_BT_MESH_PROXY_SOLICITATION) && !adv) {
				(void)bt_mesh_sol_send();
			}

			while (!adv) {

				/* Adv timeout may be set by a call from proxy
				 * to bt_mesh_adv_gatt_start:
				 */
				adv_timeout = SYS_FOREVER_MS;
				(void)bt_mesh_adv_gatt_send();

				adv = bt_mesh_adv_get(SYS_TIMEOUT_MS(adv_timeout));
				bt_le_adv_stop();

				if (IS_ENABLED(CONFIG_BT_MESH_PROXY_SOLICITATION) && !adv) {
					(void)bt_mesh_sol_send();
				}
			}
		} else {
			adv = bt_mesh_adv_get(K_FOREVER);
		}

		if (!adv) {
			continue;
		}

		/* busy == 0 means this was canceled */
		if (adv->ctx.busy) {
			adv->ctx.busy = 0U;
			adv_send(adv);
		}

		struct bt_mesh_adv_ctx ctx = adv->ctx;

		adv->ctx.started = 0;
		bt_mesh_adv_unref(adv);
		bt_mesh_adv_send_end(0, &ctx);

		/* Give other threads a chance to run */
		k_yield();
	}

	/* Empty the advertising pool when advertising is disabled */
	while ((adv = bt_mesh_adv_get(K_NO_WAIT))) {
		bt_mesh_adv_send_start(0, -ENODEV, &adv->ctx);
		bt_mesh_adv_unref(adv);
	}
}

void bt_mesh_adv_local_ready(void)
{
	/* Will be handled automatically */
}

void bt_mesh_adv_relay_ready(void)
{
	/* Will be handled automatically */
}

void bt_mesh_adv_gatt_update(void)
{
	bt_mesh_adv_get_cancel();
}

int bt_mesh_adv_terminate(struct bt_mesh_adv *adv)
{
	ARG_UNUSED(adv);

	return 0;
}

void bt_mesh_adv_init(void)
{
	k_thread_create(&adv_thread_data, adv_thread_stack,
			K_KERNEL_STACK_SIZEOF(adv_thread_stack), adv_thread,
			NULL, NULL, NULL, K_PRIO_COOP(CONFIG_BT_MESH_ADV_PRIO),
			0, K_FOREVER);
	k_thread_name_set(&adv_thread_data, "BT Mesh adv");
}

int bt_mesh_adv_enable(void)
{
	/* The advertiser thread relies on BT_MESH_SUSPENDED flag. No point in starting the
	 * advertiser thread if the flag is not set.
	 */
	if (is_mesh_suspended()) {
		return -EINVAL;
	}

	k_thread_start(&adv_thread_data);
	return 0;
}

int bt_mesh_adv_disable(void)
{
	int err;

	/* k_thread_join will sleep forever if BT_MESH_SUSPENDED flag is not set. The advertiser
	 * thread will exit once the flag is set. The flag is set by the higher layer function. Here
	 * we need to check that the flag is dropped and ensure that the thread is stopped.
	 */
	if (!is_mesh_suspended()) {
		return -EINVAL;
	}

	err = k_thread_join(&adv_thread_data, K_FOREVER);
	LOG_DBG("Advertising disabled: %d", err);

	/* Since the thread will immediately stop after this function call and won’t perform any
	 * further operations, it’s safe to ignore the deadlock error (EDEADLK).
	 */
	return err == -EDEADLK ? 0 : err;
}

int bt_mesh_adv_gatt_start(const struct bt_le_adv_param *param, int32_t duration,
			   const struct bt_data *ad, size_t ad_len,
			   const struct bt_data *sd, size_t sd_len)
{
	adv_timeout = duration;
	return bt_le_adv_start(param, ad, ad_len, sd, sd_len);
}
