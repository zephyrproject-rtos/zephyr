/*
 * Copyright 2023 NXP
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/assigned_numbers.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <zephyr/bluetooth/audio/pacs.h>
#include <zephyr/bluetooth/audio/pbp.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/audio/lc3.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/toolchain.h>

#include "bap_common.h"
#include "bap_stream_rx.h"
#include "bstests.h"
#include "common.h"

LOG_MODULE_REGISTER(pbp_public_broadcast_sink_test);

#if defined(CONFIG_BT_PBP)

extern enum bst_result_t bst_result;

static bool pbs_found;

static K_SEM_DEFINE(sem_pa_synced, 0U, 1U);
static K_SEM_DEFINE(sem_base_received, 0U, 1U);
static K_SEM_DEFINE(sem_syncable, 0U, 1U);
static K_SEM_DEFINE(sem_pa_sync_lost, 0U, 1U);

static struct bt_bap_broadcast_sink *broadcast_sink;
static struct bt_le_per_adv_sync *bcast_pa_sync;

static struct audio_test_stream test_streams[CONFIG_BT_BAP_BROADCAST_SNK_STREAM_COUNT];
static struct bt_bap_stream *streams_p[ARRAY_SIZE(test_streams)];

static const struct bt_audio_codec_cap codec = BT_AUDIO_CODEC_CAP_LC3(
	BT_AUDIO_CODEC_CAP_FREQ_16KHZ | BT_AUDIO_CODEC_CAP_FREQ_24KHZ |
		BT_AUDIO_CODEC_CAP_FREQ_48KHZ,
	BT_AUDIO_CODEC_CAP_DURATION_10, BT_AUDIO_CODEC_CAP_CHAN_COUNT_SUPPORT(1), 40u, 155u, 1u,
	BT_AUDIO_CONTEXT_TYPE_MEDIA);

/* Create a mask for the maximum BIS we can sync to using the number of streams
 * we have. We add an additional 1 since the bis indexes start from 1 and not
 * 0.
 */
static const uint32_t bis_index_mask = BIT_MASK(ARRAY_SIZE(test_streams) + 1U);
static uint32_t bis_index_bitfield;
static uint32_t broadcast_id;

static struct bt_pacs_cap cap = {
	.codec_cap = &codec,
};

static void broadcast_scan_recv(const struct bt_le_scan_recv_info *info,
				struct net_buf_simple *ad);

static struct bt_le_scan_cb broadcast_scan_cb = {
	.recv = broadcast_scan_recv
};

static void base_recv_cb(struct bt_bap_broadcast_sink *sink, const struct bt_bap_base *base,
			 size_t base_size)
{
	ARG_UNUSED(sink);
	ARG_UNUSED(base);
	ARG_UNUSED(base_size);

	k_sem_give(&sem_base_received);
}

static void syncable_cb(struct bt_bap_broadcast_sink *sink, const struct bt_iso_biginfo *biginfo)
{
	ARG_UNUSED(biginfo);

	LOG_INF("Broadcast sink %p is now syncable", sink);
	k_sem_give(&sem_syncable);
}

static struct bt_bap_broadcast_sink_cb broadcast_sink_cbs = {
	.base_recv = base_recv_cb,
	.syncable = syncable_cb,
};

static void started_cb(struct bt_bap_stream *stream)
{
	struct audio_test_stream *test_stream = audio_test_stream_from_bap_stream(stream);

	memset(&test_stream->last_info, 0, sizeof(test_stream->last_info));
	test_stream->rx_cnt = 0U;
	test_stream->valid_rx_cnt = 0U;
	UNSET_FLAG(test_stream->flag_audio_received);

	LOG_INF("Stream %p started", stream);
}

static void stopped_cb(struct bt_bap_stream *stream, uint8_t reason)
{
	LOG_INF("Stream %p stopped with reason 0x%02X", stream, reason);
}

static bool pa_decode_base(struct bt_data *data, void *user_data)
{
	const struct bt_bap_base *base = bt_bap_base_get_base_from_ad(data);
	uint32_t base_bis_index_bitfield = 0U;
	int err;

	ARG_UNUSED(user_data);

	/* Base is NULL if the data does not contain a valid BASE */
	if (base == NULL) {
		return true;
	}

	err = bt_bap_base_get_bis_indexes(base, &base_bis_index_bitfield);
	if (err != 0) {
		return false;
	}

	bis_index_bitfield = base_bis_index_bitfield & bis_index_mask;
	k_sem_give(&sem_base_received);

	return false;
}

static void broadcast_pa_recv(struct bt_le_per_adv_sync *sync,
			const struct bt_le_per_adv_sync_recv_info *info,
			struct net_buf_simple *buf)
{
	ARG_UNUSED(sync);
	ARG_UNUSED(info);

	bt_data_parse(buf, pa_decode_base, NULL);
}

static void broadcast_pa_synced(struct bt_le_per_adv_sync *sync,
			struct bt_le_per_adv_sync_synced_info *info)
{
	ARG_UNUSED(sync);
	ARG_UNUSED(info);

	LOG_INF("PA synced");
	k_sem_give(&sem_pa_synced);
}

static void broadcast_pa_terminated(struct bt_le_per_adv_sync *sync,
				    const struct bt_le_per_adv_sync_term_info *info)
{
	if (sync == bcast_pa_sync) {
		LOG_INF("PA sync %p lost with reason %u", sync, info->reason);
		bcast_pa_sync = NULL;

		k_sem_give(&sem_pa_sync_lost);
	}
}

static struct bt_bap_stream_ops stream_ops = {
	.started = started_cb,
	.stopped = stopped_cb,
	.recv = bap_stream_rx_recv_cb,
};

static struct bt_le_per_adv_sync_cb broadcast_sync_cb = {
	.synced = broadcast_pa_synced,
	.recv = broadcast_pa_recv,
	.term = broadcast_pa_terminated,
};

static int reset(void)
{
	int err;

	k_sem_reset(&sem_pa_synced);
	k_sem_reset(&sem_base_received);
	k_sem_reset(&sem_syncable);
	k_sem_reset(&sem_pa_sync_lost);

	broadcast_id = BT_BAP_INVALID_BROADCAST_ID;
	bis_index_bitfield = 0U;
	pbs_found = false;

	if (broadcast_sink != NULL) {
		err = bt_bap_broadcast_sink_delete(broadcast_sink);
		if (err != 0) {
			LOG_ERR("Deleting broadcast sink failed (err %d)", err);

			return err;
		}

		broadcast_sink = NULL;
	}

	return 0;
}

static int init(void)
{
	const struct bt_pacs_register_param pacs_param = {
		.snk_pac = true,
		.snk_loc = true,
		.src_pac = true,
		.src_loc = true,
	};
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth enable failed (err %d)\n", err);

		return err;
	}

	LOG_INF("Bluetooth initialized");

	err = bt_pacs_register(&pacs_param);
	if (err != 0) {
		FAIL("Could not register PACS (err %d)\n", err);
		return err;
	}

	bt_bap_broadcast_sink_register_cb(&broadcast_sink_cbs);
	bt_le_per_adv_sync_cb_register(&broadcast_sync_cb);

	err = bt_pacs_cap_register(BT_AUDIO_DIR_SINK, &cap);
	if (err != 0) {
		LOG_ERR("Capability register failed (err %d)", err);

		return err;
	}

	for (size_t i = 0U; i < ARRAY_SIZE(test_streams); i++) {
		streams_p[i] = bap_stream_from_audio_test_stream(&test_streams[i]);
		bt_bap_stream_cb_register(streams_p[i], &stream_ops);
	}

	return 0;
}

static void sync_broadcast_pa(const struct bt_le_scan_recv_info *info)
{
	struct bt_le_per_adv_sync_param param;
	int err;

	/* Unregister the callbacks to prevent broadcast_scan_recv to be called again */
	bt_le_scan_cb_unregister(&broadcast_scan_cb);
	err = bt_le_scan_stop();
	if (err != 0) {
		LOG_WRN("Could not stop scan: %d", err);
	}

	bt_addr_le_copy(&param.addr, info->addr);
	param.options = 0;
	param.sid = info->sid;
	param.skip = PA_SYNC_SKIP;
	param.timeout = interval_to_sync_timeout(info->interval);
	err = bt_le_per_adv_sync_create(&param, &bcast_pa_sync);
	if (err != 0) {
		LOG_ERR("Could not sync to PA: %d", err);
	}
}

static bool scan_check_and_sync_broadcast(struct bt_data *data, void *user_data)
{
	enum bt_pbp_announcement_feature source_features;
	struct bt_uuid_16 adv_uuid;
	uint8_t *tmp_meta;
	int ret;

	ARG_UNUSED(user_data);

	if (data->type != BT_DATA_SVC_DATA16) {
		return true;
	}

	if (!bt_uuid_create(&adv_uuid.uuid, data->data, BT_UUID_SIZE_16)) {
		return true;
	}

	if (!bt_uuid_cmp(&adv_uuid.uuid, BT_UUID_BROADCAST_AUDIO)) {
		/* Save broadcast_id */
		if (broadcast_id == BT_BAP_INVALID_BROADCAST_ID) {
			broadcast_id = sys_get_le24(data->data + BT_UUID_SIZE_16);
		}

		/* Found Broadcast Audio and Public Broadcast Announcement Services */
		if (pbs_found) {
			return false;
		}
	}

	ret = bt_pbp_parse_announcement(data, &source_features, &tmp_meta);
	if (ret >= 0) {
		LOG_INF("Found Suitable Public Broadcast Announcement with %d octets of metadata",
			ret);
		pbs_found = true;

		/* Continue parsing if Broadcast Audio Announcement Service was not found */
		if (broadcast_id == BT_BAP_INVALID_BROADCAST_ID) {
			return true;
		}

		return false;
	}

	/* Continue parsing */
	return true;
}

static void broadcast_scan_recv(const struct bt_le_scan_recv_info *info,
				struct net_buf_simple *ad)
{
	pbs_found = false;

	/* We are only interested in non-connectable periodic advertisers */
	if ((info->adv_props & BT_GAP_ADV_PROP_CONNECTABLE) ||
	     info->interval == 0) {
		return;
	}

	bt_data_parse(ad, scan_check_and_sync_broadcast, (void *)&broadcast_id);

	if ((broadcast_id != BT_BAP_INVALID_BROADCAST_ID) && pbs_found) {
		sync_broadcast_pa(info);
	}
}

static void wait_for_data(void)
{
	LOG_INF("Waiting for data");
	ARRAY_FOR_EACH_PTR(test_streams, test_stream) {
		if (audio_test_stream_is_streaming(test_stream)) {
			WAIT_FOR_FLAG(test_stream->flag_audio_received);
		}
	}
	LOG_INF("Data received");
}

static void test_main(void)
{
	int count = 0;
	int err;

	init();

	while (count < PBP_STREAMS_TO_SEND) {
		LOG_INF("Resetting for iteration %d", count);
		err = reset();
		if (err != 0) {
			LOG_ERR("Resetting failed: %d", err);
			break;
		}

		/* Register callbacks */
		bt_le_scan_cb_register(&broadcast_scan_cb);

		/* Start scanning */
		LOG_INF("Starting scan");
		err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL);
		if (err != 0) {
			LOG_ERR("Scan start failed (err %d)", err);
			break;
		}

		/* Wait for PA sync */
		LOG_INF("Waiting for PA Sync");
		err = k_sem_take(&sem_pa_synced, K_FOREVER);
		__ASSERT_NO_MSG(err == 0);

		/* Wait for BASE decode */
		LOG_INF("Waiting for BASE");
		err = k_sem_take(&sem_base_received, K_FOREVER);
		__ASSERT_NO_MSG(err == 0);

		/* Create broadcast sink */
		LOG_INF("Creating broadcast sink");
		err = bt_bap_broadcast_sink_create(bcast_pa_sync, broadcast_id, &broadcast_sink);
		if (err != 0) {
			LOG_ERR("Sink not created!");
			break;
		}

		LOG_INF("Waiting for syncable");

		err = k_sem_take(&sem_syncable, K_FOREVER);
		__ASSERT_NO_MSG(err == 0);

		/* Sync to broadcast source */
		LOG_INF("Syncing broadcast sink");
		err = bt_bap_broadcast_sink_sync(broadcast_sink, bis_index_bitfield,
						 streams_p, NULL);
		if (err != 0) {
			LOG_ERR("Unable to sync to broadcast source: %d", err);
			break;
		}

		wait_for_data();

		LOG_INF("Sending signal to broadcaster to stop");
		backchannel_sync_send_all(); /* let the broadcast source know it can stop */

		/* Wait for the stream to end */
		LOG_INF("Waiting for sync lost");
		err = k_sem_take(&sem_pa_sync_lost, K_FOREVER);
		__ASSERT_NO_MSG(err == 0);

		count++;
	}

	if (count == PBP_STREAMS_TO_SEND) {
		/* Pass if we synced only with the high quality broadcast */
		PASS("Public Broadcast sink passed\n");
	} else {
		FAIL("Public Broadcast sink failed (%d/%d)\n", count, PBP_STREAMS_TO_SEND);
	}
}

static const struct bst_test_instance test_public_broadcast_sink[] = {
	{
		.test_id = "public_broadcast_sink",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_public_broadcast_sink_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_public_broadcast_sink);
}

#else /* !CONFIG_BT_PBP */

struct bst_test_list *test_public_broadcast_sink_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_PBP */
