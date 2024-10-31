/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/pbp.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/byteorder.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

#include "bap_stream_tx.h"
#include "bstests.h"
#include "common.h"

#if defined(CONFIG_BT_PBP)
/* When BROADCAST_ENQUEUE_COUNT > 1 we can enqueue enough buffers to ensure that
 * the controller is never idle
 */
#define BROADCAST_ENQUEUE_COUNT 2U
#define BUF_NEEDED	(BROADCAST_ENQUEUE_COUNT * CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT)
/* PBS ASCII text */
#define PBS_DEMO                'P', 'B', 'P'
#define SEM_TIMEOUT K_SECONDS(2)

extern enum bst_result_t bst_result;

BUILD_ASSERT(CONFIG_BT_ISO_TX_BUF_COUNT >= BUF_NEEDED,
	     "CONFIG_BT_ISO_TX_BUF_COUNT should be at least "
	     "BROADCAST_ENQUEUE_COUNT * CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT");

NET_BUF_POOL_FIXED_DEFINE(tx_pool,
			  BUF_NEEDED,
			  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static const uint8_t pba_metadata[] = {
	BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_PROGRAM_INFO, PBS_DEMO)};

static uint8_t bis_codec_data[] = {
	BT_AUDIO_CODEC_DATA(BT_AUDIO_CODEC_CFG_FREQ,
			BT_BYTES_LIST_LE16(BT_AUDIO_CODEC_CFG_FREQ_48KHZ))};

static struct audio_test_stream broadcast_source_stream;
static struct bt_cap_stream *broadcast_stream;

static struct bt_cap_initiator_broadcast_stream_param stream_params;
static struct bt_cap_initiator_broadcast_subgroup_param subgroup_param;
static struct bt_cap_initiator_broadcast_create_param create_param;
static struct bt_cap_broadcast_source *broadcast_source;

static struct bt_bap_lc3_preset broadcast_preset_48_2_1 =
	BT_BAP_LC3_UNICAST_PRESET_48_2_1(BT_AUDIO_LOCATION_FRONT_LEFT,
					 BT_AUDIO_CONTEXT_TYPE_MEDIA);

static K_SEM_DEFINE(sem_started, 0U, 1);
static K_SEM_DEFINE(sem_stopped, 0U, 1);

static struct bt_le_ext_adv *adv;

static void started_cb(struct bt_bap_stream *stream)
{
	int err;

	printk("Stream %p started\n", stream);

	err = stream_tx_register(stream);
	if (err != 0) {
		FAIL("Failed to register stream %p for TX: %d\n", stream, err);
		return;
	}

	k_sem_give(&sem_started);
}

static void stopped_cb(struct bt_bap_stream *stream, uint8_t reason)
{
	int err;

	printk("Stream %p stopped with reason 0x%02X\n", stream, reason);

	err = stream_tx_unregister(stream);
	if (err != 0) {
		FAIL("Failed to unregister stream %p for TX: %d\n", stream, err);
		return;
	}

	k_sem_give(&sem_stopped);
}

static int setup_extended_adv_data(struct bt_cap_broadcast_source *source,
				   struct bt_le_ext_adv *adv)
{
	/* Broadcast Audio Streaming Endpoint advertising data */
	NET_BUF_SIMPLE_DEFINE(ad_buf, BT_UUID_SIZE_16 + BT_AUDIO_BROADCAST_ID_SIZE);
	NET_BUF_SIMPLE_DEFINE(pbp_ad_buf, BT_PBP_MIN_PBA_SIZE + ARRAY_SIZE(pba_metadata));
	NET_BUF_SIMPLE_DEFINE(base_buf, 128);
	static enum bt_pbp_announcement_feature pba_params;
	struct bt_data ext_ad[2];
	struct bt_data per_ad;
	uint32_t broadcast_id;
	int err;

	err = bt_rand(&broadcast_id, BT_AUDIO_BROADCAST_ID_SIZE);
	if (err) {
		FAIL("Unable to generate broadcast ID: %d\n", err);
		return err;
	}

	/* Broadcast Audio Announcements */
	net_buf_simple_add_le16(&ad_buf, BT_UUID_BROADCAST_AUDIO_VAL);
	net_buf_simple_add_le24(&ad_buf, broadcast_id);
	ext_ad[0].type = BT_DATA_SVC_DATA16;
	ext_ad[0].data_len = ad_buf.len;
	ext_ad[0].data = ad_buf.data;

	/**
	 * Create a Public Broadcast Announcement
	 * Cycle between high and standard quality public broadcast audio.
	 */
	if (pba_params & BT_PBP_ANNOUNCEMENT_FEATURE_HIGH_QUALITY) {
		pba_params = 0;
		pba_params |= BT_PBP_ANNOUNCEMENT_FEATURE_STANDARD_QUALITY;
		printk("Starting stream with standard quality!\n");
	} else {
		pba_params = 0;
		pba_params |= BT_PBP_ANNOUNCEMENT_FEATURE_HIGH_QUALITY;
		printk("Starting stream with high quality!\n");
	}

	err = bt_pbp_get_announcement(pba_metadata, ARRAY_SIZE(pba_metadata), pba_params,
				      &pbp_ad_buf);
	if (err != 0) {
		printk("Failed to create public broadcast announcement!: %d\n", err);

		return err;
	}
	ext_ad[1].type = BT_DATA_SVC_DATA16;
	ext_ad[1].data_len = pbp_ad_buf.len;
	ext_ad[1].data = pbp_ad_buf.data;

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

static int setup_extended_adv(struct bt_le_ext_adv **adv)
{
	int err;

	/* Create a non-connectable advertising set */
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_NCONN, NULL, adv);
	if (err != 0) {
		printk("Unable to create extended advertising set: %d\n", err);

		return err;
	}

	/* Set periodic advertising parameters */
	err = bt_le_per_adv_set_param(*adv, BT_LE_PER_ADV_DEFAULT);
	if (err) {
		printk("Failed to set periodic advertising parameters: %d\n", err);

		return err;
	}

	return 0;
}

static int stop_extended_adv(struct bt_le_ext_adv *adv)
{
	int err;

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

static struct bt_bap_stream_ops broadcast_stream_ops = {
	.started = started_cb,
	.stopped = stopped_cb,
	.sent = stream_tx_sent_cb,
};

static void test_main(void)
{
	int err;
	int count = 0;

	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth enable failed (err %d)\n", err);

		return;
	}

	printk("Bluetooth initialized\n");
	stream_tx_init();

	broadcast_stream = &broadcast_source_stream.stream;
	bt_bap_stream_cb_register(&broadcast_stream->bap_stream, &broadcast_stream_ops);

	stream_params.stream = broadcast_stream;
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

	while (count < PBP_STREAMS_TO_SEND) {
		k_sem_reset(&sem_started);
		k_sem_reset(&sem_stopped);

		err = setup_extended_adv(&adv);
		if (err != 0) {
			printk("Unable to setup extended advertiser: %d\n", err);
			FAIL("Public Broadcast source failed\n");
		}

		err = bt_cap_initiator_broadcast_audio_create(&create_param, &broadcast_source);
		if (err != 0) {
			printk("Unable to create broadcast source: %d\n", err);
			FAIL("Public Broadcast source failed\n");
		}

		err = bt_cap_initiator_broadcast_audio_start(broadcast_source, adv);
		if (err != 0) {
			printk("Unable to start broadcast source: %d\n", err);
			FAIL("Public Broadcast source failed\n");
		}

		err = setup_extended_adv_data(broadcast_source, adv);
		if (err != 0) {
			printk("Unable to setup extended advertising data: %d\n", err);
			FAIL("Public Broadcast source failed\n");
		}

		err = start_extended_adv(adv);
		if (err != 0) {
			printk("Unable to start extended advertiser: %d\n", err);
			FAIL("Public Broadcast source failed\n");
		}

		k_sem_take(&sem_started, SEM_TIMEOUT);

		/* Wait for other devices to let us know when we can stop the source */
		printk("Waiting for signal from receiver to stop\n"); backchannel_sync_wait_any();

		err = bt_cap_initiator_broadcast_audio_stop(broadcast_source);
		if (err != 0) {
			printk("Failed to stop broadcast source: %d\n", err);
			FAIL("Public Broadcast source failed\n");
		}

		k_sem_take(&sem_stopped, SEM_TIMEOUT);
		err = bt_cap_initiator_broadcast_audio_delete(broadcast_source);
		if (err != 0) {
			printk("Failed to stop broadcast source: %d\n", err);
			FAIL("Public Broadcast source failed\n");
		}

		broadcast_source = NULL;

		err = stop_extended_adv(adv);
		if (err != 0) {
			printk("Failed to stop and delete extended advertising: %d\n", err);
			FAIL("Public Broadcast source failed\n");
		}

		count++;
	}

	PASS("Public Broadcast source passed\n");
}

static const struct bst_test_instance test_pbp_broadcaster[] = {
	{
		.test_id = "public_broadcast_source",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_public_broadcast_source_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_pbp_broadcaster);
}

#else /* CONFIG_BT_PBP */

struct bst_test_list *test_public_broadcast_source_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_PBP */
