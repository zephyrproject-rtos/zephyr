/*
 * Copyright (c) 2021-2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
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
#include "bap_stream_tx.h"
#include "bstests.h"
#include "common.h"

#define SUPPORTED_CHAN_COUNTS          BT_AUDIO_CODEC_CAP_CHAN_COUNT_SUPPORT(1, 2)
#define SUPPORTED_MIN_OCTETS_PER_FRAME 30
#define SUPPORTED_MAX_OCTETS_PER_FRAME 155
#define SUPPORTED_MAX_FRAMES_PER_SDU   1

#if defined(CONFIG_BT_BAP_BROADCAST_SOURCE)
CREATE_FLAG(flag_source_started);

static struct audio_test_stream broadcast_source_streams[CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT];
/* We always default to the mandatory-to-support preset_16_2_1 */
static struct bt_bap_lc3_preset preset_16_2_1 = BT_BAP_LC3_BROADCAST_PRESET_16_2_1(
	BT_AUDIO_LOCATION_FRONT_LEFT, BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
static struct bt_bap_lc3_preset preset_16_1_1 = BT_BAP_LC3_BROADCAST_PRESET_16_1_1(
	BT_AUDIO_LOCATION_FRONT_LEFT, BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
static struct bt_audio_codec_cfg *codec_cfg = &preset_16_2_1.codec_cfg;

static uint8_t bis_codec_data[] = {
	BT_AUDIO_CODEC_DATA(BT_AUDIO_CODEC_CFG_CHAN_ALLOC,
			    BT_BYTES_LIST_LE32(BT_AUDIO_LOCATION_FRONT_CENTER)),
};
static unsigned long subgroup_cnt_arg = 1;
static unsigned long streams_per_subgroup_cnt_arg = 1;

static K_SEM_DEFINE(sem_stream_started, 0U, ARRAY_SIZE(broadcast_source_streams));
static K_SEM_DEFINE(sem_stream_stopped, 0U, ARRAY_SIZE(broadcast_source_streams));

static void validate_stream_codec_cfg(const struct bt_bap_stream *stream)
{
	const struct bt_audio_codec_cfg *stream_codec_cfg = stream->codec_cfg;
	const struct bt_audio_codec_cfg *exp_codec_cfg = codec_cfg;
	enum bt_audio_location chan_allocation;
	uint8_t frames_blocks_per_sdu;
	size_t min_sdu_size_required;
	uint16_t octets_per_frame;
	uint8_t chan_cnt;
	int ret;
	int exp_ret;

	if (stream_codec_cfg->id != BT_HCI_CODING_FORMAT_LC3) {
		/* We can only validate LC3 codecs */
		return;
	}

	ret = bt_audio_codec_cfg_get_freq(stream_codec_cfg);
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

	ret = bt_audio_codec_cfg_get_frame_dur(stream_codec_cfg);
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
	ret = bt_audio_codec_cfg_get_chan_allocation(stream_codec_cfg, &chan_allocation, true);
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

	ret = bt_audio_codec_cfg_get_octets_per_frame(stream_codec_cfg);
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

	ret = bt_audio_codec_cfg_get_frame_blocks_per_sdu(stream_codec_cfg, true);
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

static void stream_started_cb(struct bt_bap_stream *stream)
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

	err = bap_stream_tx_register(stream);
	if (err != 0) {
		FAIL("Failed to register stream %p for TX: %d\n", stream, err);
		return;
	}

	printk("Stream %p started\n", stream);
	validate_stream_codec_cfg(stream);
	k_sem_give(&sem_stream_started);
}

static void steam_stopped_cb(struct bt_bap_stream *stream, uint8_t reason)
{
	int err;

	printk("Stream %p stopped with reason 0x%02X\n", stream, reason);

	err = bap_stream_tx_unregister(stream);
	if (err != 0) {
		FAIL("Failed to unregister stream %p for TX: %d\n", stream, err);
		return;
	}

	k_sem_give(&sem_stream_stopped);
}

static struct bt_bap_stream_ops stream_ops = {
	.started = stream_started_cb,
	.stopped = steam_stopped_cb,
	.sent = bap_stream_tx_sent_cb,
};

static void source_started_cb(struct bt_bap_broadcast_source *source)
{
	printk("Broadcast source %p started\n", source);
	SET_FLAG(flag_source_started);
}

static void source_stopped_cb(struct bt_bap_broadcast_source *source, uint8_t reason)
{
	printk("Broadcast source %p stopped with reason 0x%02X\n", source, reason);
	UNSET_FLAG(flag_source_started);
}

static int setup_broadcast_source(struct bt_bap_broadcast_source **source, bool encryption)
{
	struct bt_bap_broadcast_source_stream_param
		stream_params[ARRAY_SIZE(broadcast_source_streams)];
	struct bt_bap_broadcast_source_subgroup_param
		subgroup_params[CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT];
	const unsigned long stream_cnt = subgroup_cnt_arg * streams_per_subgroup_cnt_arg;
	struct bt_bap_broadcast_source_param create_param;
	int err;

	if (stream_cnt > ARRAY_SIZE(stream_params)) {
		printk("Unable to create broadcast source with %lu subgroups with %lu streams each "
		       "(%lu total)\n",
		       subgroup_cnt_arg, streams_per_subgroup_cnt_arg, stream_cnt);
		return -ENOMEM;
	}

	(void)memset(broadcast_source_streams, 0,
		     sizeof(broadcast_source_streams));

	for (size_t i = 0; i < stream_cnt; i++) {
		stream_params[i].stream =
			bap_stream_from_audio_test_stream(&broadcast_source_streams[i]);
		bt_bap_stream_cb_register(stream_params[i].stream,
					    &stream_ops);
#if CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE > 0
		stream_params[i].data_len = ARRAY_SIZE(bis_codec_data);
		stream_params[i].data = bis_codec_data;
#endif /* CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE > 0 */
	}

	for (size_t i = 0U; i < subgroup_cnt_arg; i++) {
		subgroup_params[i].params_count = streams_per_subgroup_cnt_arg;
		subgroup_params[i].params = &stream_params[i * streams_per_subgroup_cnt_arg];
		subgroup_params[i].codec_cfg = codec_cfg;
	}

	create_param.params_count = subgroup_cnt_arg;
	create_param.params = subgroup_params;
	create_param.qos = &preset_16_2_1.qos;
	create_param.packing = BT_ISO_PACKING_SEQUENTIAL;
	create_param.encryption = encryption;
	if (encryption) {
		memcpy(create_param.broadcast_code, BROADCAST_CODE, sizeof(BROADCAST_CODE));
	}

	printk("Creating broadcast source with %lu subgroups and %lu streams\n", subgroup_cnt_arg,
	       stream_cnt);
	err = bt_bap_broadcast_source_create(&create_param, source);
	if (err != 0) {
		printk("Unable to create broadcast source: %d\n", err);
		return err;
	}

	for (size_t i = 0U; i < stream_cnt; i++) {
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
	NET_BUF_SIMPLE_DEFINE(ad_buf, BT_UUID_SIZE_16 + BT_AUDIO_BROADCAST_ID_SIZE);
	NET_BUF_SIMPLE_DEFINE(base_buf, 128);
	struct bt_data ext_ad;
	struct bt_data per_ad;
	uint32_t broadcast_id;
	int err;

	setup_broadcast_adv(adv);

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

static void test_broadcast_source_reconfig(struct bt_bap_broadcast_source *source,
					   struct bt_le_ext_adv *adv)
{
	struct bt_bap_broadcast_source_stream_param
		stream_params[ARRAY_SIZE(broadcast_source_streams)];
	struct bt_bap_broadcast_source_subgroup_param
		subgroup_params[CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT];
	const unsigned long stream_cnt = subgroup_cnt_arg * streams_per_subgroup_cnt_arg;
	struct bt_bap_broadcast_source_param reconfig_param;
	NET_BUF_SIMPLE_DEFINE(base_buf, 128);
	struct bt_data per_ad;
	int err;

	for (size_t i = 0; i < stream_cnt; i++) {
		stream_params[i].stream =
			bap_stream_from_audio_test_stream(&broadcast_source_streams[i]);
		stream_params[i].data_len = ARRAY_SIZE(bis_codec_data);
		stream_params[i].data = bis_codec_data;
	}

	codec_cfg = &preset_16_1_1.codec_cfg;
	for (size_t i = 0U; i < subgroup_cnt_arg; i++) {
		subgroup_params[i].params_count = streams_per_subgroup_cnt_arg;
		subgroup_params[i].params = &stream_params[i * streams_per_subgroup_cnt_arg];
		subgroup_params[i].codec_cfg = codec_cfg; /* update the cfg 16_1_1 */
	}

	reconfig_param.params_count = subgroup_cnt_arg;
	reconfig_param.params = subgroup_params;
	reconfig_param.qos = &preset_16_1_1.qos; /* update the QoS from 16_2_1 to 16_1_1 */
	reconfig_param.packing = BT_ISO_PACKING_SEQUENTIAL;
	reconfig_param.encryption = false;

	printk("Reconfiguring broadcast source\n");
	err = bt_bap_broadcast_source_reconfig(source, &reconfig_param);
	if (err != 0) {
		FAIL("Unable to reconfigure broadcast source: %d\n", err);
		return;
	}

	for (size_t i = 0U; i < stream_cnt; i++) {
		struct audio_test_stream *test_stream = &broadcast_source_streams[i];

		test_stream->tx_sdu_size = preset_16_1_1.qos.sdu;
	}

	/* Update the BASE */
	test_broadcast_source_get_base(source, &base_buf);

	per_ad.type = BT_DATA_SVC_DATA16;
	per_ad.data_len = base_buf.len;
	per_ad.data = base_buf.data;
	err = bt_le_per_adv_set_data(adv, &per_ad, 1);
	if (err != 0) {
		FAIL("Failed to set periodic advertising data: %d\n", err);
	}
}

static void test_broadcast_source_start(struct bt_bap_broadcast_source *source,
					struct bt_le_ext_adv *adv)
{
	const unsigned long stream_cnt = subgroup_cnt_arg * streams_per_subgroup_cnt_arg;
	int err;

	printk("Starting broadcast source\n");
	err = bt_bap_broadcast_source_start(source, adv);
	if (err != 0) {
		FAIL("Unable to start broadcast source: %d\n", err);
		return;
	}

	/* Wait for all to be started */
	printk("Waiting for %lu streams to be started\n", stream_cnt);
	for (size_t i = 0U; i < stream_cnt; i++) {
		k_sem_take(&sem_stream_started, K_FOREVER);
	}

	WAIT_FOR_FLAG(flag_source_started);
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
	const unsigned long stream_cnt = subgroup_cnt_arg * streams_per_subgroup_cnt_arg;
	int err;

	printk("Stopping broadcast source\n");

	err = bt_bap_broadcast_source_stop(source);
	if (err != 0) {
		FAIL("Unable to stop broadcast source: %d\n", err);
		return;
	}

	/* Wait for all to be stopped */
	printk("Waiting for %lu streams to be stopped\n", stream_cnt);
	for (size_t i = 0U; i < stream_cnt; i++) {
		k_sem_take(&sem_stream_stopped, K_FOREVER);
	}

	WAIT_FOR_UNSET_FLAG(flag_source_started);
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

static void init(void)
{
	static struct bt_bap_broadcast_source_cb broadcast_source_cb = {
		.started = source_started_cb,
		.stopped = source_stopped_cb,
	};
	int err;

	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");
	bap_stream_tx_init();

	err = bt_bap_broadcast_source_register_cb(&broadcast_source_cb);
	if (err != 0) {
		FAIL("Failed to register broadcast source callbacks (err %d)\n", err);
		return;
	}
}

static void test_main(void)
{
	struct bt_bap_broadcast_source *source;
	struct bt_le_ext_adv *adv;
	int err;

	init();

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

	test_broadcast_source_start(source, adv);

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

static void test_main_update(void)
{
	struct bt_bap_broadcast_source *source;
	struct bt_le_ext_adv *adv;
	int err;

	init();

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

	test_broadcast_source_reconfig(source, adv);

	test_broadcast_source_start(source, adv);

	/* Wait for other devices to have received data */
	backchannel_sync_wait_any();

	/* Update metadata while streaming */
	test_broadcast_source_update_metadata(source, adv);

	/* Wait for other devices to have received metadata update */
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

	PASS("Broadcast source passed\n");
}

static void test_main_encrypted(void)
{
	struct bt_bap_broadcast_source *source;
	struct bt_le_ext_adv *adv;
	int err;

	init();

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

static void test_args(int argc, char *argv[])
{
	for (size_t argn = 0; argn < argc; argn++) {
		const char *arg = argv[argn];

		if (strcmp(arg, "subgroup_cnt") == 0) {
			arg = argv[++argn];
			subgroup_cnt_arg = strtoul(arg, NULL, 10);

			if (!IN_RANGE(subgroup_cnt_arg, 1,
				      CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT)) {
				FAIL("Invalid number of subgroups: %lu\n", subgroup_cnt_arg);
			}
		} else if (strcmp(arg, "streams_per_subgroup_cnt") == 0) {
			arg = argv[++argn];
			streams_per_subgroup_cnt_arg = strtoul(arg, NULL, 10);

			if (!IN_RANGE(streams_per_subgroup_cnt_arg, 1,
				      CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT)) {
				FAIL("Invalid number of streams per subgroup: %lu\n",
				     streams_per_subgroup_cnt_arg);
			}
		} else if (strcmp(arg, "vs_codec") == 0) {
			codec_cfg = &vs_codec_cfg;
		} else if (strcmp(arg, "lc3_codec") == 0) {
			codec_cfg = &preset_16_2_1.codec_cfg;
		} else {
			FAIL("Invalid arg: %s\n", arg);
		}
	}
}

static const struct bst_test_instance test_broadcast_source[] = {
	{
		.test_id = "broadcast_source",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main,
		.test_args_f = test_args,
	},
	{
		.test_id = "broadcast_source_update",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main_update,
		.test_args_f = test_args,
	},
	{
		.test_id = "broadcast_source_encrypted",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main_encrypted,
		.test_args_f = test_args,
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
