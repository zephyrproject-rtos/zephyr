/*
 * Copyright (c) 2022-2024 Nordic Semiconductor ASA
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/tmap.h>
#include <zephyr/bluetooth/byteorder.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>

#define BROADCAST_ENQUEUE_COUNT 2U

NET_BUF_POOL_FIXED_DEFINE(tx_pool,
			  (BROADCAST_ENQUEUE_COUNT * CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT),
			  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU), 8, NULL);

static K_SEM_DEFINE(sem_broadcast_started, 0, 1);
static K_SEM_DEFINE(sem_broadcast_stopped, 0, 1);

static struct bt_cap_stream broadcast_source_stream;
static struct bt_cap_stream *broadcast_stream;

static uint8_t bis_codec_data[] = {BT_AUDIO_CODEC_DATA(
	BT_AUDIO_CODEC_CFG_FREQ, BT_BYTES_LIST_LE16(BT_AUDIO_CODEC_CFG_FREQ_48KHZ))};

static const uint8_t new_metadata[] = {
	BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_STREAM_CONTEXT,
			    BT_BYTES_LIST_LE16(BT_AUDIO_CONTEXT_TYPE_MEDIA))
};

static struct bt_bap_lc3_preset broadcast_preset_48_2_1 =
	BT_BAP_LC3_UNICAST_PRESET_48_2_1(BT_AUDIO_LOCATION_FRONT_LEFT,
					BT_AUDIO_CONTEXT_TYPE_MEDIA);

struct bt_cap_initiator_broadcast_stream_param stream_params;
struct bt_cap_initiator_broadcast_subgroup_param subgroup_param;
struct bt_cap_initiator_broadcast_create_param create_param;
struct bt_cap_broadcast_source *broadcast_source;
static struct k_work_delayable audio_send_work;
struct bt_le_ext_adv *ext_adv;

static uint8_t tmap_addata[] = {
	BT_UUID_16_ENCODE(BT_UUID_TMAS_VAL), /* TMAS UUID */
	BT_BYTES_LIST_LE16(BT_TMAP_ROLE_BMS), /* TMAP Role */
};

static void broadcast_started_cb(struct bt_bap_stream *stream)
{
	printk("Stream %p started\n", stream);
	k_sem_give(&sem_broadcast_started);
}

static void broadcast_stopped_cb(struct bt_bap_stream *stream, uint8_t reason)
{
	printk("Stream %p stopped with reason 0x%02X\n", stream, reason);

	k_sem_give(&sem_broadcast_stopped);
}

static void broadcast_sent_cb(struct bt_bap_stream *stream)
{
	static uint8_t mock_data[CONFIG_BT_ISO_TX_MTU];
	static bool mock_data_initialized;
	static uint32_t seq_num;
	struct net_buf *buf;
	int ret;

	if (broadcast_preset_48_2_1.qos.sdu > CONFIG_BT_ISO_TX_MTU) {
		printk("Invalid SDU %u for the MTU: %d",
		       broadcast_preset_48_2_1.qos.sdu, CONFIG_BT_ISO_TX_MTU);
		return;
	}

	if (!mock_data_initialized) {
		for (size_t i = 0U; i < ARRAY_SIZE(mock_data); i++) {
			/* Initialize mock data */
			mock_data[i] = (uint8_t)i;
		}
		mock_data_initialized = true;
	}

	buf = net_buf_alloc(&tx_pool, K_NO_WAIT);
	if (buf == NULL) {
		printk("Could not allocate buffer when sending on %p\n", stream);

		/* Retry next SDU interval */
		k_work_schedule(&audio_send_work, K_USEC(broadcast_preset_48_2_1.qos.interval));
		return;
	}

	net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);
	net_buf_add_mem(buf, mock_data, broadcast_preset_48_2_1.qos.sdu);
	ret = bt_bap_stream_send(stream, buf, seq_num++);
	if (ret < 0) {
		net_buf_unref(buf);

		/* Retry next SDU interval */
		k_work_schedule(&audio_send_work, K_USEC(broadcast_preset_48_2_1.qos.interval));
		return;
	}
}

static void audio_timer_timeout(struct k_work *work)
{
	broadcast_sent_cb(&broadcast_stream->bap_stream);
}

static struct bt_bap_stream_ops broadcast_stream_ops = {
	.started = broadcast_started_cb,
	.stopped = broadcast_stopped_cb,
	.sent = broadcast_sent_cb
};

static const struct bt_data ad[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static int setup_extended_adv(struct bt_le_ext_adv **adv)
{
	int err;

	/* Create a non-connectable advertising set */
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_NCONN, NULL, adv);
	if (err != 0) {
		printk("Unable to create extended advertising set: %d\n", err);
		return err;
	}

	/* Set advertising data to have complete local name set */
	err = bt_le_ext_adv_set_data(*adv, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("Failed to set advertising data (err %d)\n", err);
		return 0;
	}

	/* Set periodic advertising parameters */
	err = bt_le_per_adv_set_param(*adv, BT_LE_PER_ADV_DEFAULT);
	if (err) {
		printk("Failed to set periodic advertising parameters: %d\n",
		       err);
		return err;
	}

	return 0;
}

static int setup_extended_adv_data(struct bt_cap_broadcast_source *source,
				   struct bt_le_ext_adv *adv)
{
	/* Broadcast Audio Streaming Endpoint advertising data */
	NET_BUF_SIMPLE_DEFINE(ad_buf,
			      BT_UUID_SIZE_16 + BT_AUDIO_BROADCAST_ID_SIZE);
	NET_BUF_SIMPLE_DEFINE(base_buf, 128);
	struct bt_data ext_ad[2];
	struct bt_data per_ad;
	uint32_t broadcast_id;
	int err;

	err = bt_cap_initiator_broadcast_get_id(source, &broadcast_id);
	if (err != 0) {
		printk("Unable to get broadcast ID: %d\n", err);
		return err;
	}

	/* Setup extended advertising data */
	ext_ad[0].type = BT_DATA_SVC_DATA16;
	ext_ad[0].data_len = ARRAY_SIZE(tmap_addata);
	ext_ad[0].data = tmap_addata;
	/* Broadcast Audio Announcement */
	net_buf_simple_add_le16(&ad_buf, BT_UUID_BROADCAST_AUDIO_VAL);
	net_buf_simple_add_le24(&ad_buf, broadcast_id);
	ext_ad[1].type = BT_DATA_SVC_DATA16;
	ext_ad[1].data_len = ad_buf.len + sizeof(ext_ad[1].type);
	ext_ad[1].data = ad_buf.data;

	err = bt_le_ext_adv_set_data(adv, ext_ad, ARRAY_SIZE(ext_ad), NULL, 0);
	if (err != 0) {
		printk("Failed to set extended advertising data: %d\n", err);
		return err;
	}

	/* Setup periodic advertising data */
	err = bt_cap_initiator_broadcast_get_base(source, &base_buf);
	if (err != 0) {
		printk("Failed to get encoded BASE: %d\n", err);
		return err;
	}

	per_ad.type = BT_DATA_SVC_DATA16;
	per_ad.data_len = base_buf.len;
	per_ad.data = base_buf.data;
	err = bt_le_per_adv_set_data(adv, &per_ad, 1);
	if (err != 0) {
		printk("Failed to set periodic advertising data: %d\n", err);
		return err;
	}

	return 0;
}

static int start_extended_adv(struct bt_le_ext_adv *adv)
{
	int err;

	/* Start extended advertising */
	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err) {
		printk("Failed to start extended advertising: %d\n", err);
		return err;
	}

	/* Enable Periodic Advertising */
	err = bt_le_per_adv_start(adv);
	if (err) {
		printk("Failed to enable periodic advertising: %d\n", err);
		return err;
	}

	return 0;
}

static int stop_and_delete_extended_adv(struct bt_le_ext_adv *adv)
{
	int err;

	/* Stop extended advertising */
	err = bt_le_per_adv_stop(adv);
	if (err) {
		printk("Failed to stop periodic advertising: %d\n", err);
		return err;
	}

	err = bt_le_ext_adv_stop(adv);
	if (err) {
		printk("Failed to stop extended advertising: %d\n", err);
		return err;
	}

	err = bt_le_ext_adv_delete(adv);
	if (err) {
		printk("Failed to delete extended advertising: %d\n", err);
		return err;
	}

	return 0;
}

static int reset(void)
{
	k_sem_reset(&sem_broadcast_started);
	k_sem_reset(&sem_broadcast_stopped);

	return 0;
}

int cap_initiator_init(void)
{
	broadcast_stream = &broadcast_source_stream;
	bt_bap_stream_cb_register(&broadcast_stream->bap_stream, &broadcast_stream_ops);
	k_work_init_delayable(&audio_send_work, audio_timer_timeout);

	return 0;
}

void cap_initiator_setup(void)
{
	int err;

	stream_params.stream = &broadcast_source_stream;
	stream_params.data_len = ARRAY_SIZE(bis_codec_data);
	stream_params.data = bis_codec_data;

	subgroup_param.stream_count = 1U;
	subgroup_param.stream_params = &stream_params;
	subgroup_param.codec_cfg = &broadcast_preset_48_2_1.codec_cfg;

	create_param.subgroup_count = 1U;
	create_param.subgroup_params = &subgroup_param;
	create_param.qos = &broadcast_preset_48_2_1.qos;
	create_param.packing = BT_ISO_PACKING_SEQUENTIAL;
	create_param.encryption = false;

	while (true) {
		err = reset();
		if (err != 0) {
			printk("Resetting failed: %d - Aborting\n", err);
			return;
		}
		printk("Creating broadcast source\n");

		err = setup_extended_adv(&ext_adv);
		if (err != 0) {
			printk("Unable to setup extended advertiser: %d\n", err);
			return;
		}

		err = bt_cap_initiator_broadcast_audio_create(&create_param, &broadcast_source);
		if (err != 0) {
			printk("Unable to create broadcast source: %d\n", err);
			return;
		}

		err = bt_cap_initiator_broadcast_audio_start(broadcast_source, ext_adv);
		if (err != 0) {
			printk("Unable to start broadcast source: %d\n", err);
			return;
		}

		err = setup_extended_adv_data(broadcast_source, ext_adv);
		if (err != 0) {
			printk("Unable to setup extended advertising data: %d\n", err);
			return;
		}

		err = start_extended_adv(ext_adv);
		if (err != 0) {
			printk("Unable to start extended advertiser: %d\n", err);
			return;
		}
		k_sem_take(&sem_broadcast_started, K_FOREVER);

		/* Initialize sending */
		for (unsigned int j = 0U; j < BROADCAST_ENQUEUE_COUNT; j++) {
			broadcast_sent_cb(&broadcast_stream->bap_stream);
		}

		/* Run for a little while */
		k_sleep(K_SECONDS(10));

		err = bt_cap_initiator_broadcast_audio_update(broadcast_source,
							      new_metadata,
							      ARRAY_SIZE(new_metadata));
		if (err != 0) {
			printk("Failed to update broadcast source metadata: %d\n", err);
			return;
		}

		/* Run for a little while */
		k_sleep(K_SECONDS(10));

		err = bt_cap_initiator_broadcast_audio_stop(broadcast_source);
		if (err != 0) {
			printk("Failed to stop broadcast source: %d\n", err);
			return;
		}
		k_sem_take(&sem_broadcast_stopped, K_FOREVER);

		err = bt_cap_initiator_broadcast_audio_delete(broadcast_source);
		if (err != 0) {
			printk("Failed to stop broadcast source: %d\n", err);
			return;
		}
		broadcast_source = NULL;

		err = stop_and_delete_extended_adv(ext_adv);
		if (err != 0) {
			printk("Failed to stop and delete extended advertising: %d\n", err);
			return;
		}
	}
}
