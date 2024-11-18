/*
 * Copyright (c) 2021-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <zephyr/bluetooth/audio/lc3.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/byteorder.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/toolchain.h>

#include "bap_common.h"
#include "bstests.h"
#include "common.h"

#define SUPPORTED_CHAN_COUNTS          BT_AUDIO_CODEC_CAP_CHAN_COUNT_SUPPORT(1, 2)
#define SUPPORTED_MIN_OCTETS_PER_FRAME 30
#define SUPPORTED_MAX_OCTETS_PER_FRAME 155
#define SUPPORTED_MAX_FRAMES_PER_SDU   1

#if defined(CONFIG_BT_BAP_BROADCAST_SOURCE)
/* When BROADCAST_ENQUEUE_COUNT > 1 we can enqueue enough buffers to ensure that
 * the controller is never idle
 */
#define BROADCAST_ENQUEUE_COUNT 2U
#define TOTAL_BUF_NEEDED	(BROADCAST_ENQUEUE_COUNT * CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT)

BUILD_ASSERT(CONFIG_BT_ISO_TX_BUF_COUNT >= TOTAL_BUF_NEEDED,
	     "CONFIG_BT_ISO_TX_BUF_COUNT should be at least "
	     "BROADCAST_ENQUEUE_COUNT * CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT");

NET_BUF_POOL_FIXED_DEFINE(tx_pool,
			  TOTAL_BUF_NEEDED,
			  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

extern enum bst_result_t bst_result;
static struct audio_test_stream broadcast_source_streams[CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT];
static struct bt_bap_lc3_preset preset_16_2_1 = BT_BAP_LC3_BROADCAST_PRESET_16_2_1(
	BT_AUDIO_LOCATION_FRONT_LEFT, BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
static struct bt_bap_lc3_preset preset_16_1_1 = BT_BAP_LC3_BROADCAST_PRESET_16_1_1(
	BT_AUDIO_LOCATION_FRONT_LEFT, BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);

static uint8_t bis_codec_data[] = {
	BT_AUDIO_CODEC_DATA(BT_AUDIO_CODEC_CFG_CHAN_ALLOC,
			    BT_BYTES_LIST_LE32(BT_AUDIO_LOCATION_FRONT_CENTER)),
};

static K_SEM_DEFINE(sem_started, 0U, ARRAY_SIZE(broadcast_source_streams));
static K_SEM_DEFINE(sem_stopped, 0U, ARRAY_SIZE(broadcast_source_streams));

static void validate_stream_codec_cfg(const struct bt_bap_stream *stream)
{
	const struct bt_audio_codec_cfg *codec_cfg = stream->codec_cfg;
	const struct bt_audio_codec_cfg *exp_codec_cfg = &preset_16_1_1.codec_cfg;
	enum bt_audio_location chan_allocation;
	uint8_t frames_blocks_per_sdu;
	size_t min_sdu_size_required;
	uint16_t octets_per_frame;
	uint8_t chan_cnt;
	int ret;
	int exp_ret;

	ret = bt_audio_codec_cfg_get_freq(codec_cfg);
	exp_ret = bt_audio_codec_cfg_get_freq(exp_codec_cfg);
	if (ret >= 0) {
		const int freq = bt_audio_codec_cfg_freq_to_freq_hz(ret);
		const int exp_freq = bt_audio_codec_cfg_freq_to_freq_hz(exp_ret);

		if (freq != exp_freq) {
			FAIL("Invalid frequency: %d Expected: %d\n", freq, exp_freq);

			return;
		}
	} else {
		FAIL("Could not get frequency: %d\n", ret);

		return;
	}

	ret = bt_audio_codec_cfg_get_frame_dur(codec_cfg);
	exp_ret = bt_audio_codec_cfg_get_frame_dur(exp_codec_cfg);
	if (ret >= 0) {
		const int frm_dur_us = bt_audio_codec_cfg_frame_dur_to_frame_dur_us(ret);
		const int exp_frm_dur_us = bt_audio_codec_cfg_frame_dur_to_frame_dur_us(exp_ret);

		if (frm_dur_us != exp_frm_dur_us) {
			FAIL("Invalid frame duration: %d Exp: %d\n", frm_dur_us, exp_frm_dur_us);

			return;
		}
	} else {
		FAIL("Could not get frame duration: %d\n", ret);

		return;
	}

	/* The broadcast source sets the channel allocation in the BIS to
	 * BT_AUDIO_LOCATION_FRONT_CENTER
	 */
	ret = bt_audio_codec_cfg_get_chan_allocation(codec_cfg, &chan_allocation, true);
	if (ret == 0) {
		if (chan_allocation != BT_AUDIO_LOCATION_FRONT_CENTER) {
			FAIL("Unexpected channel allocation: 0x%08X", chan_allocation);

			return;
		}

		chan_cnt = bt_audio_get_chan_count(chan_allocation);
	} else {
		FAIL("Could not get subgroup channel allocation: %d\n", ret);

		return;
	}

	if (chan_cnt == 0 || (BIT(chan_cnt - 1) & SUPPORTED_CHAN_COUNTS) == 0) {
		FAIL("Unsupported channel count: %u\n", chan_cnt);

		return;
	}

	ret = bt_audio_codec_cfg_get_octets_per_frame(codec_cfg);
	if (ret > 0) {
		octets_per_frame = (uint16_t)ret;
	} else {
		FAIL("Could not get subgroup octets per frame: %d\n", ret);

		return;
	}

	if (!IN_RANGE(octets_per_frame, SUPPORTED_MIN_OCTETS_PER_FRAME,
		      SUPPORTED_MAX_OCTETS_PER_FRAME)) {
		FAIL("Unsupported octets per frame: %u\n", octets_per_frame);

		return;
	}

	ret = bt_audio_codec_cfg_get_frame_blocks_per_sdu(codec_cfg, true);
	if (ret > 0) {
		frames_blocks_per_sdu = (uint8_t)ret;
	} else {
		FAIL("Could not get frame blocks per SDU: %d\n", ret);

		return;
	}

	/* An SDU can consist of X frame blocks, each with Y frames (one per channel) of size Z in
	 * them. The minimum SDU size required for this is X * Y * Z.
	 */
	min_sdu_size_required = chan_cnt * octets_per_frame * frames_blocks_per_sdu;
	if (min_sdu_size_required > stream->qos->sdu) {
		FAIL("With %zu channels and %u octets per frame and %u frames per block, SDUs "
		     "shall be at minimum %zu, but the stream has been configured for %u\n",
		     chan_cnt, octets_per_frame, frames_blocks_per_sdu, min_sdu_size_required,
		     stream->qos->sdu);

		return;
	}
}

static void started_cb(struct bt_bap_stream *stream)
{
	struct audio_test_stream *test_stream = audio_test_stream_from_bap_stream(stream);
	struct bt_bap_ep_info info;
	int err;

	test_stream->seq_num = 0U;
	test_stream->tx_cnt = 0U;

	err = bt_bap_ep_get_info(stream->ep, &info);
	if (err != 0) {
		FAIL("Failed to get EP info: %d\n", err);
		return;
	}

	if (info.state != BT_BAP_EP_STATE_STREAMING) {
		FAIL("Unexpected EP state: %d\n", info.state);
		return;
	}

	if (info.dir != BT_AUDIO_DIR_SOURCE) {
		FAIL("Unexpected info.dir: %d\n", info.dir);
		return;
	}

	if (!info.can_send) {
		FAIL("info.can_send is false\n");
		return;
	}

	if (info.can_recv) {
		FAIL("info.can_recv is true\n");
		return;
	}

	if (info.paired_ep != NULL) {
		FAIL("Unexpected info.paired_ep: %p\n", info.paired_ep);
		return;
	}

	printk("Stream %p started\n", stream);
	validate_stream_codec_cfg(stream);
	k_sem_give(&sem_started);
}

static void stopped_cb(struct bt_bap_stream *stream, uint8_t reason)
{
	printk("Stream %p stopped with reason 0x%02X\n", stream, reason);
	k_sem_give(&sem_stopped);
}

static void stream_sent_cb(struct bt_bap_stream *stream)
{
	struct audio_test_stream *test_stream = audio_test_stream_from_bap_stream(stream);
	struct net_buf *buf;
	int ret;

	if (!test_stream->tx_active) {
		return;
	}

	if ((test_stream->tx_cnt % 100U) == 0U) {
		printk("Sent with seq_num %u\n", test_stream->seq_num);
	}

	buf = net_buf_alloc(&tx_pool, K_FOREVER);
	if (buf == NULL) {
		printk("Could not allocate buffer when sending on %p\n",
		       stream);
		return;
	}

	net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);
	net_buf_add_mem(buf, mock_iso_data, test_stream->tx_sdu_size);
	ret = bt_bap_stream_send(stream, buf, test_stream->seq_num++);
	if (ret < 0) {
		/* This will end broadcasting on this stream. */
		net_buf_unref(buf);

		/* Only fail if tx is active (may fail if we are disabling the stream) */
		if (test_stream->tx_active) {
			FAIL("Unable to broadcast data on %p: %d\n", stream, ret);
		}

		return;
	}

	test_stream->tx_cnt++;
}

static struct bt_bap_stream_ops stream_ops = {
	.started = started_cb,
	.stopped = stopped_cb,
	.sent = stream_sent_cb,
};

static int setup_broadcast_source(struct bt_bap_broadcast_source **source, bool encryption)
{
	struct bt_bap_broadcast_source_stream_param
		stream_params[ARRAY_SIZE(broadcast_source_streams)];
	struct bt_bap_broadcast_source_subgroup_param
		subgroup_params[CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT];
	struct bt_bap_broadcast_source_param create_param;
	int err;

	(void)memset(broadcast_source_streams, 0,
		     sizeof(broadcast_source_streams));

	for (size_t i = 0; i < ARRAY_SIZE(stream_params); i++) {
		stream_params[i].stream =
			bap_stream_from_audio_test_stream(&broadcast_source_streams[i]);
		bt_bap_stream_cb_register(stream_params[i].stream,
					    &stream_ops);
#if CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE > 0
		stream_params[i].data_len = ARRAY_SIZE(bis_codec_data);
		stream_params[i].data = bis_codec_data;
#endif /* CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE > 0 */
	}

	for (size_t i = 0U; i < ARRAY_SIZE(subgroup_params); i++) {
		subgroup_params[i].params_count = ARRAY_SIZE(stream_params);
		subgroup_params[i].params = &stream_params[i];
		subgroup_params[i].codec_cfg = &preset_16_1_1.codec_cfg;
	}

	create_param.params_count = ARRAY_SIZE(subgroup_params);
	create_param.params = subgroup_params;
	create_param.qos = &preset_16_2_1.qos;
	create_param.packing = BT_ISO_PACKING_SEQUENTIAL;
	create_param.encryption = encryption;
	if (encryption) {
		memcpy(create_param.broadcast_code, BROADCAST_CODE, sizeof(BROADCAST_CODE));
	}

	printk("Creating broadcast source with %zu subgroups and %zu streams\n",
	       ARRAY_SIZE(subgroup_params), ARRAY_SIZE(stream_params));
	err = bt_bap_broadcast_source_create(&create_param, source);
	if (err != 0) {
		printk("Unable to create broadcast source: %d\n", err);
		return err;
	}

	for (size_t i = 0U; i < ARRAY_SIZE(broadcast_source_streams); i++) {
		struct audio_test_stream *test_stream = &broadcast_source_streams[i];

		test_stream->tx_sdu_size = preset_16_1_1.qos.sdu;
	}

	return 0;
}

static void test_broadcast_source_get_base(struct bt_bap_broadcast_source *source,
					   struct net_buf_simple *base_buf)
{
	int err;

	err = bt_bap_broadcast_source_get_base(source, base_buf);
	if (err != 0) {
		FAIL("Failed to get encoded BASE: %d\n", err);
		return;
	}
}

static int setup_extended_adv(struct bt_bap_broadcast_source *source, struct bt_le_ext_adv **adv)
{
	/* Broadcast Audio Streaming Endpoint advertising data */
	NET_BUF_SIMPLE_DEFINE(ad_buf,
			      BT_UUID_SIZE_16 + BT_AUDIO_BROADCAST_ID_SIZE);
	struct bt_le_adv_param adv_param = BT_LE_ADV_PARAM_INIT(
		BT_LE_ADV_OPT_EXT_ADV, 0x80, 0x80, NULL);
	NET_BUF_SIMPLE_DEFINE(base_buf, 128);
	struct bt_data ext_ad;
	struct bt_data per_ad;
	uint32_t broadcast_id;
	int err;

	/* Create a non-connectable advertising set */
	err = bt_le_ext_adv_create(&adv_param, NULL, adv);
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

	err = bt_rand(&broadcast_id, BT_AUDIO_BROADCAST_ID_SIZE);
	if (err) {
		printk("Unable to generate broadcast ID: %d\n", err);
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
	test_broadcast_source_get_base(source, &base_buf);

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

static void test_broadcast_source_reconfig(struct bt_bap_broadcast_source *source)
{
	struct bt_bap_broadcast_source_stream_param
		stream_params[ARRAY_SIZE(broadcast_source_streams)];
	struct bt_bap_broadcast_source_subgroup_param
		subgroup_params[CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT];
	struct bt_bap_broadcast_source_param reconfig_param;
	int err;

	for (size_t i = 0; i < ARRAY_SIZE(stream_params); i++) {
		stream_params[i].stream =
			bap_stream_from_audio_test_stream(&broadcast_source_streams[i]);
		stream_params[i].data_len = ARRAY_SIZE(bis_codec_data);
		stream_params[i].data = bis_codec_data;
	}

	for (size_t i = 0U; i < ARRAY_SIZE(subgroup_params); i++) {
		subgroup_params[i].params_count = 1U;
		subgroup_params[i].params = &stream_params[i];
		subgroup_params[i].codec_cfg = &preset_16_1_1.codec_cfg;
	}

	reconfig_param.params_count = ARRAY_SIZE(subgroup_params);
	reconfig_param.params = subgroup_params;
	reconfig_param.qos = &preset_16_1_1.qos;
	reconfig_param.packing = BT_ISO_PACKING_SEQUENTIAL;
	reconfig_param.encryption = false;

	printk("Reconfiguring broadcast source\n");
	err = bt_bap_broadcast_source_reconfig(source, &reconfig_param);
	if (err != 0) {
		FAIL("Unable to reconfigure broadcast source: %d\n", err);
		return;
	}

	for (size_t i = 0U; i < ARRAY_SIZE(broadcast_source_streams); i++) {
		struct audio_test_stream *test_stream = &broadcast_source_streams[i];

		test_stream->tx_sdu_size = preset_16_1_1.qos.sdu;
	}
}

static void test_broadcast_source_start(struct bt_bap_broadcast_source *source,
					struct bt_le_ext_adv *adv)
{
	int err;

	printk("Starting broadcast source\n");
	err = bt_bap_broadcast_source_start(source, adv);
	if (err != 0) {
		FAIL("Unable to start broadcast source: %d\n", err);
		return;
	}

	/* Wait for all to be started */
	printk("Waiting for streams to be started\n");
	for (size_t i = 0U; i < ARRAY_SIZE(broadcast_source_streams); i++) {
		k_sem_take(&sem_started, K_FOREVER);
	}
}

static void test_broadcast_source_update_metadata(struct bt_bap_broadcast_source *source,
						  struct bt_le_ext_adv *adv)
{
	uint8_t new_metadata[] = BT_AUDIO_CODEC_CFG_LC3_META(BT_AUDIO_CONTEXT_TYPE_ALERTS);
	struct bt_data per_ad;
	int err;

	NET_BUF_SIMPLE_DEFINE(base_buf, 128);

	printk("Updating metadata\n");
	err = bt_bap_broadcast_source_update_metadata(source, new_metadata,
						      ARRAY_SIZE(new_metadata));
	if (err != 0) {
		FAIL("Failed to update metadata broadcast source: %d\n", err);
		return;
	}

	/* Get the new BASE */
	test_broadcast_source_get_base(source, &base_buf);

	/* Update the periodic advertising data with the new BASE */
	per_ad.type = BT_DATA_SVC_DATA16;
	per_ad.data_len = base_buf.len;
	per_ad.data = base_buf.data;
	err = bt_le_per_adv_set_data(adv, &per_ad, 1);
	if (err != 0) {
		FAIL("Failed to set periodic advertising data: %d\n", err);
	}
}

static void test_broadcast_source_stop(struct bt_bap_broadcast_source *source)
{
	int err;

	printk("Stopping broadcast source\n");

	for (size_t i = 0U; i < ARRAY_SIZE(broadcast_source_streams); i++) {
		broadcast_source_streams[i].tx_active = false;
	}

	err = bt_bap_broadcast_source_stop(source);
	if (err != 0) {
		FAIL("Unable to stop broadcast source: %d\n", err);
		return;
	}

	/* Wait for all to be stopped */
	printk("Waiting for streams to be stopped\n");
	for (size_t i = 0U; i < ARRAY_SIZE(broadcast_source_streams); i++) {
		k_sem_take(&sem_stopped, K_FOREVER);
	}
}

static void test_broadcast_source_delete(struct bt_bap_broadcast_source *source)
{
	int err;

	printk("Deleting broadcast source\n");

	err = bt_bap_broadcast_source_delete(source);
	if (err != 0) {
		FAIL("Unable to stop broadcast source: %d\n", err);
		return;
	}
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
	struct bt_bap_broadcast_source *source;
	struct bt_le_ext_adv *adv;
	int err;

	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	err = setup_broadcast_source(&source, false);
	if (err != 0) {
		FAIL("Unable to setup broadcast source: %d\n", err);
		return;
	}

	err = setup_extended_adv(source, &adv);
	if (err != 0) {
		FAIL("Failed to setup extended advertising: %d\n", err);
		return;
	}

	test_broadcast_source_reconfig(source);

	test_broadcast_source_start(source, adv);

	/* Initialize sending */
	printk("Sending data\n");
	for (size_t i = 0U; i < ARRAY_SIZE(broadcast_source_streams); i++) {
		for (unsigned int j = 0U; j < BROADCAST_ENQUEUE_COUNT; j++) {
			struct audio_test_stream *test_stream = &broadcast_source_streams[i];

			test_stream->tx_active = true;
			stream_sent_cb(&test_stream->stream.bap_stream);
		}
	}

	/* Wait for other devices to have received what they wanted */
	backchannel_sync_wait_any();

	/* Update metadata while streaming */
	test_broadcast_source_update_metadata(source, adv);

	/* Wait for other devices to have received what they wanted */
	backchannel_sync_wait_any();

	/* Wait for other devices to let us know when we can stop the source */
	backchannel_sync_wait_any();

	test_broadcast_source_stop(source);

	test_broadcast_source_delete(source);
	source = NULL;

	err = stop_extended_adv(adv);
	if (err != 0) {
		FAIL("Unable to stop extended advertising: %d\n", err);
		return;
	}
	adv = NULL;

	/* Recreate broadcast source to verify that it's possible */
	printk("Recreating broadcast source\n");
	err = setup_broadcast_source(&source, false);
	if (err != 0) {
		FAIL("Unable to setup broadcast source: %d\n", err);
		return;
	}

	printk("Deleting broadcast source\n");
	test_broadcast_source_delete(source);
	source = NULL;

	PASS("Broadcast source passed\n");
}

static void test_main_encrypted(void)
{
	struct bt_bap_broadcast_source *source;
	struct bt_le_ext_adv *adv;
	int err;

	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	err = setup_broadcast_source(&source, true);
	if (err != 0) {
		FAIL("Unable to setup broadcast source: %d\n", err);
		return;
	}

	err = setup_extended_adv(source, &adv);
	if (err != 0) {
		FAIL("Failed to setup extended advertising: %d\n", err);
		return;
	}

	test_broadcast_source_start(source, adv);

	/* Initialize sending */
	printk("Sending data\n");
	for (size_t i = 0U; i < ARRAY_SIZE(broadcast_source_streams); i++) {
		for (unsigned int j = 0U; j < BROADCAST_ENQUEUE_COUNT; j++) {
			struct audio_test_stream *test_stream = &broadcast_source_streams[i];

			test_stream->tx_active = true;
			stream_sent_cb(&test_stream->stream.bap_stream);
		}
	}

	/* Wait for other devices to have received data */
	backchannel_sync_wait_any();

	/* Wait for other devices to let us know when we can stop the source */
	backchannel_sync_wait_any();

	test_broadcast_source_stop(source);

	test_broadcast_source_delete(source);
	source = NULL;

	err = stop_extended_adv(adv);
	if (err != 0) {
		FAIL("Unable to stop extended advertising: %d\n", err);
		return;
	}
	adv = NULL;

	PASS("Broadcast source encrypted passed\n");
}

static const struct bst_test_instance test_broadcast_source[] = {
	{
		.test_id = "broadcast_source",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main,
	},
	{
		.test_id = "broadcast_source_encrypted",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main_encrypted,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_broadcast_source_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_broadcast_source);
}

#else /* CONFIG_BT_BAP_BROADCAST_SOURCE */

struct bst_test_list *test_broadcast_source_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_BAP_BROADCAST_SOURCE */
