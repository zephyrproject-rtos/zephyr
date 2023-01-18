/*
 * Copyright (c) 2021-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_AUDIO_BROADCAST_SOURCE)

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/audio.h>
#include "common.h"

/* When BROADCAST_ENQUEUE_COUNT > 1 we can enqueue enough buffers to ensure that
 * the controller is never idle
 */
#define BROADCAST_ENQUEUE_COUNT 2U
#define TOTAL_BUF_NEEDED (BROADCAST_ENQUEUE_COUNT * CONFIG_BT_AUDIO_BROADCAST_SRC_STREAM_COUNT)

BUILD_ASSERT(CONFIG_BT_ISO_TX_BUF_COUNT >= TOTAL_BUF_NEEDED,
	     "CONFIG_BT_ISO_TX_BUF_COUNT should be at least "
	     "BROADCAST_ENQUEUE_COUNT * CONFIG_BT_AUDIO_BROADCAST_SRC_STREAM_COUNT");

NET_BUF_POOL_FIXED_DEFINE(tx_pool,
			  TOTAL_BUF_NEEDED,
			  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU), 8, NULL);

extern enum bst_result_t bst_result;
static struct bt_audio_stream broadcast_source_streams[CONFIG_BT_AUDIO_BROADCAST_SRC_STREAM_COUNT];
static struct bt_audio_stream *streams[ARRAY_SIZE(broadcast_source_streams)];
static struct bt_audio_lc3_preset preset_16_2_1 =
	BT_AUDIO_LC3_BROADCAST_PRESET_16_2_1(BT_AUDIO_LOCATION_FRONT_LEFT,
					     BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
static struct bt_audio_lc3_preset preset_16_2_2 =
	BT_AUDIO_LC3_BROADCAST_PRESET_16_2_2(BT_AUDIO_LOCATION_FRONT_LEFT,
					     BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
CREATE_FLAG(flag_stopping);

static K_SEM_DEFINE(sem_started, 0U, ARRAY_SIZE(streams));
static K_SEM_DEFINE(sem_stopped, 0U, ARRAY_SIZE(streams));

static void started_cb(struct bt_audio_stream *stream)
{
	printk("Stream %p started\n", stream);
	k_sem_give(&sem_started);
}

static void stopped_cb(struct bt_audio_stream *stream)
{
	printk("Stream %p stopped\n", stream);
	k_sem_give(&sem_stopped);
}

static void sent_cb(struct bt_audio_stream *stream)
{
	static uint8_t mock_data[CONFIG_BT_ISO_TX_MTU];
	static bool mock_data_initialized;
	static uint16_t seq_num;
	struct net_buf *buf;
	int ret;

	if (TEST_FLAG(flag_stopping)) {
		return;
	}

	if (!mock_data_initialized) {
		for (size_t i = 0U; i < ARRAY_SIZE(mock_data); i++) {
			/* Initialize mock data */
			mock_data[i] = (uint8_t)i;
		}
		mock_data_initialized = true;
	}

	buf = net_buf_alloc(&tx_pool, K_FOREVER);
	if (buf == NULL) {
		printk("Could not allocate buffer when sending on %p\n",
		       stream);
		return;
	}

	net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);
	/* Use preset_16_2_1 as that is the config we end up using */
	net_buf_add_mem(buf, mock_data, preset_16_2_1.qos.sdu);
	ret = bt_audio_stream_send(stream, buf, seq_num++,
				   BT_ISO_TIMESTAMP_NONE);
	if (ret < 0) {
		/* This will end broadcasting on this stream. */
		printk("Unable to broadcast data on %p: %d\n", stream, ret);
		net_buf_unref(buf);
		return;
	}
}

static struct bt_audio_stream_ops stream_ops = {
	.started = started_cb,
	.stopped = stopped_cb,
	.sent = sent_cb
};

static int setup_broadcast_source(struct bt_audio_broadcast_source **source)
{
	struct bt_codec_data bis_codec_data = BT_CODEC_DATA(BT_CODEC_CONFIG_LC3_FREQ,
							    BT_CODEC_CONFIG_LC3_FREQ_16KHZ);
	struct bt_audio_broadcast_source_stream_param
		stream_params[ARRAY_SIZE(broadcast_source_streams)];
	struct bt_audio_broadcast_source_subgroup_param
		subgroup_params[CONFIG_BT_AUDIO_BROADCAST_SRC_SUBGROUP_COUNT];
	struct bt_audio_broadcast_source_create_param create_param;
	int err;

	(void)memset(broadcast_source_streams, 0,
		     sizeof(broadcast_source_streams));

	for (size_t i = 0; i < ARRAY_SIZE(stream_params); i++) {
		stream_params[i].stream = &broadcast_source_streams[i];
		bt_audio_stream_cb_register(stream_params[i].stream,
					    &stream_ops);
		stream_params[i].data_count = 1U;
		stream_params[i].data = &bis_codec_data;
	}

	for (size_t i = 0U; i < ARRAY_SIZE(subgroup_params); i++) {
		subgroup_params[i].params_count = 1U;
		subgroup_params[i].params = &stream_params[i];
		subgroup_params[i].codec = &preset_16_2_1.codec;
	}

	create_param.params_count = ARRAY_SIZE(subgroup_params);
	create_param.params = subgroup_params;
	create_param.qos = &preset_16_2_2.qos;
	create_param.packing = BT_ISO_PACKING_SEQUENTIAL;
	create_param.encryption = false;

	printk("Creating broadcast source with %zu subgroups and %zu streams\n",
	       ARRAY_SIZE(subgroup_params), ARRAY_SIZE(stream_params));
	err = bt_audio_broadcast_source_create(&create_param, source);
	if (err != 0) {
		printk("Unable to create broadcast source: %d\n", err);
		return err;
	}

	return 0;
}

static int setup_extended_adv(struct bt_audio_broadcast_source *source,
			      struct bt_le_ext_adv **adv)
{
	/* Broadcast Audio Streaming Endpoint advertising data */
	NET_BUF_SIMPLE_DEFINE(ad_buf,
			      BT_UUID_SIZE_16 + BT_AUDIO_BROADCAST_ID_SIZE);
	NET_BUF_SIMPLE_DEFINE(base_buf, 128);
	struct bt_data ext_ad;
	struct bt_data per_ad;
	uint32_t broadcast_id;
	int err;

	/* Create a non-connectable non-scannable advertising set */
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_NCONN_NAME, NULL, adv);
	if (err != 0) {
		printk("Unable to create extended advertising set: %d\n", err);
		return err;
	}

	/* Set periodic advertising parameters */
	err = bt_le_per_adv_set_param(*adv, BT_LE_PER_ADV_DEFAULT);
	if (err) {
		printk("Failed to set periodic advertising parameters: %d\n",
		       err);
		return err;
	}

	err = bt_audio_broadcast_source_get_id(source, &broadcast_id);
	if (err != 0) {
		printk("Unable to get broadcast ID: %d\n", err);
		return err;
	}

	/* Setup extended advertising data */
	net_buf_simple_add_le16(&ad_buf, BT_UUID_BROADCAST_AUDIO_VAL);
	net_buf_simple_add_le24(&ad_buf, broadcast_id);
	ext_ad.type = BT_DATA_SVC_DATA16;
	ext_ad.data_len = ad_buf.len;
	ext_ad.data = ad_buf.data;
	err = bt_le_ext_adv_set_data(*adv, &ext_ad, 1, NULL, 0);
	if (err != 0) {
		printk("Failed to set extended advertising data: %d\n", err);
		return err;
	}

	/* Setup periodic advertising data */
	err = bt_audio_broadcast_source_get_base(source, &base_buf);
	if (err != 0) {
		printk("Failed to get encoded BASE: %d\n", err);
		return err;
	}

	per_ad.type = BT_DATA_SVC_DATA16;
	per_ad.data_len = base_buf.len;
	per_ad.data = base_buf.data;
	err = bt_le_per_adv_set_data(*adv, &per_ad, 1);
	if (err != 0) {
		printk("Failed to set periodic advertising data: %d\n", err);
		return err;
	}

	/* Start extended advertising */
	err = bt_le_ext_adv_start(*adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err) {
		printk("Failed to start extended advertising: %d\n", err);
		return err;
	}

	/* Enable Periodic Advertising */
	err = bt_le_per_adv_start(*adv);
	if (err) {
		printk("Failed to enable periodic advertising: %d\n", err);
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

static void test_main(void)
{
	struct bt_codec_data new_metadata[1] =
		BT_CODEC_LC3_CONFIG_META(BT_AUDIO_CONTEXT_TYPE_ALERTS);
	struct bt_audio_broadcast_source *source;
	struct bt_le_ext_adv *adv;
	int err;

	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	err = setup_broadcast_source(&source);
	if (err != 0) {
		FAIL("Unable to setup broadcast source: %d\n", err);
		return;
	}

	err = setup_extended_adv(source, &adv);
	if (err != 0) {
		FAIL("Failed to setup extended advertising: %d\n", err);
		return;
	}

	printk("Reconfiguring broadcast source\n");
	err = bt_audio_broadcast_source_reconfig(source, &preset_16_2_1.codec,
						 &preset_16_2_1.qos);
	if (err != 0) {
		FAIL("Unable to reconfigure broadcast source: %d\n", err);
		return;
	}

	printk("Starting broadcast source\n");
	err = bt_audio_broadcast_source_start(source, adv);
	if (err != 0) {
		FAIL("Unable to start broadcast source: %d\n", err);
		return;
	}

	/* Wait for all to be started */
	printk("Waiting for streams to be started\n");
	for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
		k_sem_take(&sem_started, K_FOREVER);
	}

	/* Initialize sending */
	for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
		for (unsigned int j = 0U; j < BROADCAST_ENQUEUE_COUNT; j++) {
			sent_cb(streams[i]);
		}
	}

	/* Keeping running for a little while */
	k_sleep(K_SECONDS(15));

	/* Update metadata while streaming */
	printk("Updating metadata\n");
	err = bt_audio_broadcast_source_update_metadata(source, new_metadata,
							ARRAY_SIZE(new_metadata));
	if (err != 0) {
		FAIL("Failed to update metadata broadcast source: %d", err);
		return;
	}

	/* Keeping running for a little while */
	k_sleep(K_SECONDS(5));

	printk("Stopping broadcast source\n");
	SET_FLAG(flag_stopping);
	err = bt_audio_broadcast_source_stop(source);
	if (err != 0) {
		FAIL("Unable to stop broadcast source: %d\n", err);
		return;
	}

	/* Wait for all to be stopped */
	printk("Waiting for streams to be stopped\n");
	for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
		k_sem_take(&sem_stopped, K_FOREVER);
	}

	printk("Deleting broadcast source\n");
	err = bt_audio_broadcast_source_delete(source);
	if (err != 0) {
		FAIL("Unable to delete broadcast source: %d\n", err);
		return;
	}
	source = NULL;


	err = stop_extended_adv(adv);
	if (err != 0) {
		FAIL("Unable to stop extended advertising: %d\n", err);
		return;
	}
	adv = NULL;

	/* Recreate broadcast source to verify that it's possible */
	printk("Recreating broadcast source\n");
	err = setup_broadcast_source(&source);
	if (err != 0) {
		FAIL("Unable to setup broadcast source: %d\n", err);
		return;
	}

	printk("Deleting broadcast source\n");
	err = bt_audio_broadcast_source_delete(source);
	if (err != 0) {
		FAIL("Unable to delete broadcast source: %d\n", err);
		return;
	}
	source = NULL;

	PASS("Broadcast source passed\n");
}

static const struct bst_test_instance test_broadcast_source[] = {
	{
		.test_id = "broadcast_source",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_broadcast_source_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_broadcast_source);
}

#else /* CONFIG_BT_AUDIO_BROADCAST_SOURCE */

struct bst_test_list *test_broadcast_source_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_AUDIO_BROADCAST_SOURCE */
