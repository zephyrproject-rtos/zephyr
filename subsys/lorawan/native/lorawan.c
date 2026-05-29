/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/lorawan/lorawan.h>
#include <string.h>

#include "lorawan.h"
#include "engine.h"
#include "radio.h"
#include "crypto/crypto.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lorawan_native, CONFIG_LORAWAN_LOG_LEVEL);

#define LWAN_MAX_APP_PAYLOAD		242
#define LWAN_DEFAULT_MAX_PAYLOAD	51
#define LWAN_MAX_CONF_TRIES		15

static struct {
	lorawan_battery_level_cb_t battery_cb;
	lorawan_dr_changed_cb_t dr_changed_cb;
} api_state;

struct lwan_ctx lwan_ctx = {
	.channel_count = LWAN_MAX_CHANNELS,
	.current_dr = LORAWAN_DR_0,
	.conf_tries = 1,
	.dl_callbacks = SYS_SLIST_STATIC_INIT(&lwan_ctx.dl_callbacks),
};

static const struct {
	bool enabled;
	enum lorawan_region id;
} region_table[] = {
	{ IS_ENABLED(CONFIG_LORAWAN_REGION_AS923), LORAWAN_REGION_AS923 },
	{ IS_ENABLED(CONFIG_LORAWAN_REGION_AU915), LORAWAN_REGION_AU915 },
	{ IS_ENABLED(CONFIG_LORAWAN_REGION_CN470), LORAWAN_REGION_CN470 },
	{ IS_ENABLED(CONFIG_LORAWAN_REGION_CN779), LORAWAN_REGION_CN779 },
	{ IS_ENABLED(CONFIG_LORAWAN_REGION_EU433), LORAWAN_REGION_EU433 },
	{ IS_ENABLED(CONFIG_LORAWAN_REGION_EU868), LORAWAN_REGION_EU868 },
	{ IS_ENABLED(CONFIG_LORAWAN_REGION_IN865), LORAWAN_REGION_IN865 },
	{ IS_ENABLED(CONFIG_LORAWAN_REGION_KR920), LORAWAN_REGION_KR920 },
	{ IS_ENABLED(CONFIG_LORAWAN_REGION_US915), LORAWAN_REGION_US915 },
	{ IS_ENABLED(CONFIG_LORAWAN_REGION_RU864), LORAWAN_REGION_RU864 },
};

static int lorawan_native_init(void)
{
	size_t count = 0;
	enum lorawan_region single = 0;

	for (size_t i = 0; i < ARRAY_SIZE(region_table); i++) {
		if (region_table[i].enabled) {
			single = region_table[i].id;
			count++;
		}
	}

	if (count == 1) {
		lwan_ctx.region = lwan_region_get(single);
	}

	return 0;
}
SYS_INIT(lorawan_native_init, POST_KERNEL, 0);

int lorawan_start(void)
{
	const struct device *lora_dev;
	int ret;

	if (atomic_test_and_set_bit(lwan_ctx.flags, LWAN_FLAG_STARTED)) {
		return -EALREADY;
	}

	lora_dev = DEVICE_DT_GET(DT_ALIAS(lora0));
	if (!device_is_ready(lora_dev)) {
		LOG_ERR("LoRa device not ready");
		ret = -ENODEV;
		goto fail;
	}

	ret = lwan_crypto_init();
	if (ret != 0) {
		LOG_ERR("Crypto init failed: %d", ret);
		goto fail;
	}

	ret = radio_init(lora_dev);
	if (ret != 0) {
		LOG_ERR("Radio init failed: %d", ret);
		goto fail;
	}

	if (lwan_ctx.region == NULL) {
		LOG_ERR("No region set. Call lorawan_set_region() "
			"when multiple regions are enabled.");
		ret = -EINVAL;
		goto fail;
	}

	ret = lwan_ctx.region->get_default_channels(
		lwan_ctx.channels, &lwan_ctx.channel_count);
	if (ret != 0) {
		LOG_ERR("Failed to get default channels: %d", ret);
		goto fail;
	}

	engine_init(&lwan_ctx);

	LOG_INF("Native LoRaWAN stack started");

	return 0;

fail:
	atomic_clear_bit(lwan_ctx.flags, LWAN_FLAG_STARTED);
	return ret;
}

int lorawan_join(const struct lorawan_join_config *config)
{
	struct lwan_join_req join_req;
	struct lwan_req msg;
	int ret;

	if (config == NULL) {
		return -EINVAL;
	}

	if (!atomic_test_bit(lwan_ctx.flags, LWAN_FLAG_STARTED)) {
		return -EPERM;
	}

	if (config->mode != LORAWAN_ACT_OTAA) {
		LOG_ERR("Only OTAA is supported");
		return -ENOTSUP;
	}

	if (config->dev_eui == NULL || config->otaa.join_eui == NULL ||
	    config->otaa.nwk_key == NULL) {
		return -EINVAL;
	}

	join_req = (struct lwan_join_req){
		.dev_eui = config->dev_eui,
		.join_eui = config->otaa.join_eui,
		.nwk_key = config->otaa.nwk_key,
		.dev_nonce = config->otaa.dev_nonce,
	};

	atomic_clear_bit(lwan_ctx.flags, LWAN_FLAG_JOINED);

	msg = LWAN_REQ(LWAN_REQ_JOIN, &join_req);

	ret = engine_post_req_wait(&msg);
	if (ret == 0) {
		LOG_INF("Successfully joined network");

		if (api_state.dr_changed_cb != NULL) {
			api_state.dr_changed_cb(lwan_ctx.current_dr);
		}
	} else {
		LOG_WRN("Join failed: %d", ret);
	}

	return ret;
}

int lorawan_send(uint8_t port, uint8_t *data, uint8_t len,
		 enum lorawan_message_type type)
{
	struct lwan_send_req send_req;
	struct lwan_req msg;

	if (!atomic_test_bit(lwan_ctx.flags, LWAN_FLAG_STARTED)) {
		return -EPERM;
	}

	if (!atomic_test_bit(lwan_ctx.flags, LWAN_FLAG_JOINED)) {
		return -ENETDOWN;
	}

	if (len > 0 && data == NULL) {
		return -EINVAL;
	}

	if (len > LWAN_MAX_APP_PAYLOAD) {
		return -EMSGSIZE;
	}

	send_req = (struct lwan_send_req){
		.data = data,
		.len = len,
		.port = port,
		.type = type,
	};

	msg = LWAN_REQ(LWAN_REQ_SEND, &send_req);

	return engine_post_req_wait(&msg);
}

int lorawan_set_region(enum lorawan_region region)
{
	const struct lwan_region_ops *ops;

	if (atomic_test_bit(lwan_ctx.flags, LWAN_FLAG_STARTED)) {
		return -EALREADY;
	}

	ops = lwan_region_get(region);
	if (ops == NULL) {
		return -ENOTSUP;
	}

	lwan_ctx.region = ops;
	return 0;
}

int lorawan_set_class(enum lorawan_class dev_class)
{
	if (dev_class != LORAWAN_CLASS_A) {
		return -ENOTSUP;
	}

	return 0;
}

void lorawan_enable_adr(bool enable)
{
	struct lwan_enable_adr_req adr_req = {
		.enable = enable,
	};
	struct lwan_req msg = LWAN_REQ(LWAN_REQ_ENABLE_ADR, &adr_req);

	if (!atomic_test_bit(lwan_ctx.flags, LWAN_FLAG_STARTED)) {
		LOG_WRN("Stack not started; ADR setting ignored");
		return;
	}

	(void)engine_post_req_wait(&msg);
}

int lorawan_set_datarate(enum lorawan_datarate dr)
{
	struct lwan_set_datarate_req dr_req = {
		.dr = dr,
	};
	struct lwan_req msg = LWAN_REQ(LWAN_REQ_SET_DATARATE, &dr_req);

	if (!atomic_test_bit(lwan_ctx.flags, LWAN_FLAG_STARTED)) {
		return -EPERM;
	}

	return engine_post_req_wait(&msg);
}

enum lorawan_datarate lorawan_get_min_datarate(void)
{
	return LORAWAN_DR_0;
}

void lorawan_get_payload_sizes(uint8_t *max_next_payload_size,
			       uint8_t *max_payload_size)
{
	struct lwan_dr_params dr_params;
	int8_t power;
	uint8_t payload;

	if (lwan_ctx.region != NULL &&
	    lwan_ctx.region->get_tx_params((uint8_t)lwan_ctx.current_dr,
					    lwan_ctx.mac.tx_power_idx,
					    &dr_params, &power) == 0) {
		payload = dr_params.max_payload;
	} else {
		payload = LWAN_DEFAULT_MAX_PAYLOAD;
	}

	if (max_next_payload_size != NULL) {
		*max_next_payload_size = payload;
	}

	if (max_payload_size != NULL) {
		*max_payload_size = payload;
	}
}

int lorawan_set_conf_msg_tries(uint8_t tries)
{
	struct lwan_set_conf_msg_tries_req tries_req = {
		.tries = tries,
	};
	struct lwan_req msg = LWAN_REQ(LWAN_REQ_SET_CONF_MSG_TRIES, &tries_req);

	if (tries == 0 || tries > LWAN_MAX_CONF_TRIES) {
		return -EINVAL;
	}

	if (!atomic_test_bit(lwan_ctx.flags, LWAN_FLAG_STARTED)) {
		return -EPERM;
	}

	return engine_post_req_wait(&msg);
}

int lorawan_set_channels_mask(uint16_t *channels_mask,
			      size_t channels_mask_size)
{
	struct lwan_set_channels_mask_req mask_req = {
		.channels_mask = channels_mask,
		.channels_mask_size = channels_mask_size,
	};
	struct lwan_req msg = LWAN_REQ(LWAN_REQ_SET_CHANNELS_MASK, &mask_req);

	if (channels_mask == NULL || channels_mask_size == 0) {
		return -EINVAL;
	}

	if (!atomic_test_bit(lwan_ctx.flags, LWAN_FLAG_STARTED)) {
		return -EPERM;
	}

	return engine_post_req_wait(&msg);
}

void lorawan_register_battery_level_callback(lorawan_battery_level_cb_t cb)
{
	api_state.battery_cb = cb;
}

void lorawan_register_downlink_callback(struct lorawan_downlink_cb *cb)
{
	sys_slist_append(&lwan_ctx.dl_callbacks, &cb->node);
}

void lorawan_register_dr_changed_callback(lorawan_dr_changed_cb_t cb)
{
	api_state.dr_changed_cb = cb;
}

void lorawan_register_link_check_ans_callback(lorawan_link_check_ans_cb_t cb)
{
	ARG_UNUSED(cb);
}

int lorawan_request_device_time(bool force_request)
{
	ARG_UNUSED(force_request);

	return -ENOTSUP;
}

int lorawan_device_time_get(uint32_t *gps_time)
{
	ARG_UNUSED(gps_time);

	return -ENOTSUP;
}

int lorawan_request_link_check(bool force_request)
{
	ARG_UNUSED(force_request);

	return -ENOTSUP;
}
