/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>

#include "cap_initiator.h"

LOG_MODULE_REGISTER(cap_initiator_broadcast, LOG_LEVEL_INF);

static struct bt_cap_stream broadcast_stream;
uint64_t total_broadcast_tx_iso_packet_count; /* This value is exposed to test code */

static void broadcast_stream_started_cb(struct bt_bap_stream *stream)
{
	struct bt_cap_stream *cap_stream = CONTAINER_OF(stream, struct bt_cap_stream, bap_stream);
	int err;

	LOG_INF("Started broadcast stream %p", stream);
	total_broadcast_tx_iso_packet_count = 0U;

	err = cap_initiator_tx_register_stream(cap_stream);
	if (err != 0) {
		LOG_ERR("Failed to register %p for TX: %d", stream, err);
	}
}

static void broadcast_stream_stopped_cb(struct bt_bap_stream *stream, uint8_t reason)
{
	struct bt_cap_stream *cap_stream = CONTAINER_OF(stream, struct bt_cap_stream, bap_stream);
	int err;

	LOG_INF("Stopped broadcast stream %p with reason 0x%02X", stream, reason);

	err = cap_initiator_tx_unregister_stream(cap_stream);
	if (err != 0) {
		LOG_ERR("Failed to unregister %p for TX: %d", stream, err);
	}
}

static void broadcast_stream_sent_cb(struct bt_bap_stream *stream)
{
	/* Triggered every time we have sent an HCI data packet to the controller */

	if ((total_broadcast_tx_iso_packet_count % 100U) == 0U) {
		LOG_INF("Sent %llu HCI ISO data packets", total_broadcast_tx_iso_packet_count);
	}

	total_broadcast_tx_iso_packet_count++;
}

static int setup_extended_adv(struct bt_le_ext_adv **adv)
{
	int err;

	/* Create a non-connectable non-scannable advertising set */
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_NCONN, NULL, adv);
	if (err != 0) {
		LOG_ERR("Unable to create extended advertising set: %d", err);
		return err;
	}

	/* Set periodic advertising parameters */
	err = bt_le_per_adv_set_param(*adv, BT_LE_PER_ADV_PARAM(BT_GAP_PER_ADV_FAST_INT_MIN_2,
								BT_GAP_PER_ADV_FAST_INT_MAX_2,
								BT_LE_PER_ADV_OPT_NONE));
	if (err != 0) {
		LOG_ERR("Failed to set periodic advertising parameters: %d", err);
		return err;
	}

	return 0;
}

static int broadcast_create(struct bt_cap_broadcast_source **broadcast_source)
{
	/** For simplicity we use the mandatory configuration 16_2_1 */
	static struct bt_bap_lc3_preset broadcast_preset_16_2_1 =
		BT_BAP_LC3_BROADCAST_PRESET_16_2_1(BT_AUDIO_LOCATION_MONO_AUDIO,
						   BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
	struct bt_cap_initiator_broadcast_stream_param stream_params = {
		.stream = &broadcast_stream,
	};
	struct bt_cap_initiator_broadcast_subgroup_param subgroup_param = {
		.codec_cfg = &broadcast_preset_16_2_1.codec_cfg,
		.stream_params = &stream_params,
		.stream_count = 1U,
	};
	const struct bt_cap_initiator_broadcast_create_param create_param = {
		.qos = &broadcast_preset_16_2_1.qos,
		.subgroup_params = &subgroup_param,
		.subgroup_count = 1U,
	};
	int err;

	LOG_INF("Creating broadcast source");

	err = bt_cap_initiator_broadcast_audio_create(&create_param, broadcast_source);
	if (err != 0) {
		LOG_ERR("Unable to start broadcast source: %d", err);
		return err;
	}

	return 0;
}

static int setup_extended_adv_data(struct bt_cap_broadcast_source *source,
				   struct bt_le_ext_adv *adv)
{
	/* Broadcast Audio Streaming Endpoint advertising data */
	NET_BUF_SIMPLE_DEFINE(ad_buf, BT_UUID_SIZE_16 + BT_AUDIO_BROADCAST_ID_SIZE);
	NET_BUF_SIMPLE_DEFINE(base_buf, 64);
	struct bt_data ext_ad;
	struct bt_data per_ad;
	uint32_t broadcast_id;
	int err;

	err = bt_cap_initiator_broadcast_get_id(source, &broadcast_id);
	if (err != 0) {
		LOG_ERR("Unable to get broadcast ID: %d", err);
		return err;
	}

	/* Setup extended advertising data */
	net_buf_simple_add_le16(&ad_buf, BT_UUID_BROADCAST_AUDIO_VAL);
	net_buf_simple_add_le24(&ad_buf, broadcast_id);
	ext_ad.type = BT_DATA_SVC_DATA16;
	ext_ad.data_len = ad_buf.len;
	ext_ad.data = ad_buf.data;
	err = bt_le_ext_adv_set_data(adv, &ext_ad, 1, NULL, 0);
	if (err != 0) {
		LOG_ERR("Failed to set extended advertising data: %d", err);
		return err;
	}

	/* Setup periodic advertising data */
	err = bt_cap_initiator_broadcast_get_base(source, &base_buf);
	if (err != 0) {
		LOG_ERR("Failed to get encoded BASE: %d", err);
		return err;
	}

	per_ad.type = BT_DATA_SVC_DATA16;
	per_ad.data_len = base_buf.len;
	per_ad.data = base_buf.data;
	err = bt_le_per_adv_set_data(adv, &per_ad, 1);
	if (err != 0) {
		LOG_ERR("Failed to set periodic advertising data: %d", err);
		return err;
	}

	return 0;
}

static int start_extended_adv(struct bt_le_ext_adv *adv)
{
	int err;

	/* Start extended advertising */
	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err != 0) {
		LOG_ERR("Failed to start extended advertising: %d", err);
		return err;
	}

	/* Enable Periodic Advertising */
	err = bt_le_per_adv_start(adv);
	if (err != 0) {
		LOG_ERR("Failed to enable periodic advertising: %d", err);
		return err;
	}

	return 0;
}

static int broadcast_start(struct bt_cap_broadcast_source *broadcast_source,
			   struct bt_le_ext_adv *adv)
{
	int err;

	err = bt_cap_initiator_broadcast_audio_start(broadcast_source, adv);
	if (err != 0) {
		LOG_ERR("Unable to start broadcast source: %d", err);
		return err;
	}

	LOG_INF("Broadcast source started");

	return 0;
}

static int init_cap_initiator(void)
{
	static struct bt_bap_stream_ops broadcast_stream_ops = {
		.started = broadcast_stream_started_cb,
		.stopped = broadcast_stream_stopped_cb,
		.sent = broadcast_stream_sent_cb,
	};

	bt_cap_stream_ops_register(&broadcast_stream, &broadcast_stream_ops);

	cap_initiator_tx_init();

	return 0;
}

int cap_initiator_broadcast(void)
{
	struct bt_cap_broadcast_source *broadcast_source;
	struct bt_le_ext_adv *adv;
	int err;

	err = init_cap_initiator();
	if (err != 0) {
		return err;
	}

	LOG_INF("CAP initiator broadcast initialized");

	err = setup_extended_adv(&adv);
	if (err != 0) {
		return err;
	}

	err = broadcast_create(&broadcast_source);
	if (err != 0) {
		return err;
	}

	err = broadcast_start(broadcast_source, adv);
	if (err != 0) {
		return err;
	}

	err = setup_extended_adv_data(broadcast_source, adv);
	if (err != 0) {
		return err;
	}

	err = start_extended_adv(adv);
	if (err != 0) {
		return err;
	}

	return 0;
}
