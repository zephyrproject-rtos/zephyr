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
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>

#include "bap_stream_rx.h"
#include "bstests.h"
#include "common.h"

#if defined(CONFIG_BT_PBP)
#define SEM_TIMEOUT K_SECONDS(30)

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
	k_sem_give(&sem_base_received);
}

static void syncable_cb(struct bt_bap_broadcast_sink *sink, const struct bt_iso_biginfo *biginfo)
{
	printk("Broadcast sink %p is now syncable\n", sink);
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

	printk("Stream %p started\n", stream);
}

static void stopped_cb(struct bt_bap_stream *stream, uint8_t reason)
{
	printk("Stream %p stopped with reason 0x%02X\n", stream, reason);
}

static bool pa_decode_base(struct bt_data *data, void *user_data)
{
	const struct bt_bap_base *base = bt_bap_base_get_base_from_ad(data);
	uint32_t base_bis_index_bitfield = 0U;
	int err;

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
	bt_data_parse(buf, pa_decode_base, NULL);
}

static void broadcast_pa_synced(struct bt_le_per_adv_sync *sync,
			struct bt_le_per_adv_sync_synced_info *info)
{
	printk("PA synced\n");
	k_sem_give(&sem_pa_synced);
}

static void broadcast_pa_terminated(struct bt_le_per_adv_sync *sync,
				    const struct bt_le_per_adv_sync_term_info *info)
{
	if (sync == bcast_pa_sync) {
		printk("PA sync %p lost with reason %u\n", sync, info->reason);
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
	UNSET_FLAG(flag_audio_received);

	broadcast_id = BT_BAP_INVALID_BROADCAST_ID;
	bis_index_bitfield = 0U;
	pbs_found = false;

	if (broadcast_sink != NULL) {
		err = bt_bap_broadcast_sink_delete(broadcast_sink);
		if (err) {
			printk("Deleting broadcast sink failed (err %d)\n", err);

			return err;
		}

		broadcast_sink = NULL;
	}

	return 0;
}

static int init(void)
{
	const struct bt_bap_pacs_register_param pacs_param = {
		.snk_pac = true,
		.snk_loc = true,
		.src_pac = true,
		.src_loc = true
	};
	int err;

	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth enable failed (err %d)\n", err);

		return err;
	}

	printk("Bluetooth initialized\n");

	err = bt_pacs_register(&pacs_param);
	if (err) {
		FAIL("Could not register PACS (err %d)\n", err);
		return err;
	}

	bt_bap_broadcast_sink_register_cb(&broadcast_sink_cbs);
	bt_le_per_adv_sync_cb_register(&broadcast_sync_cb);

	err = bt_pacs_cap_register(BT_AUDIO_DIR_SINK, &cap);
	if (err) {
		printk("Capability register failed (err %d)\n", err);

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
		printk("Could not stop scan: %d\n", err);
	}

	bt_addr_le_copy(&param.addr, info->addr);
	param.options = 0;
	param.sid = info->sid;
	param.skip = PA_SYNC_SKIP;
	param.timeout = interval_to_sync_timeout(info->interval);
	err = bt_le_per_adv_sync_create(&param, &bcast_pa_sync);
	if (err != 0) {
		printk("Could not sync to PA: %d\n", err);
	}
}

static bool scan_check_and_sync_broadcast(struct bt_data *data, void *user_data)
{
	enum bt_pbp_announcement_feature source_features;
	struct bt_uuid_16 adv_uuid;
	uint8_t *tmp_meta;
	int ret;

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
		printk("Found Suitable Public Broadcast Announcement with %d octets of metadata\n",
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

static void test_main(void)
{
	int count = 0;
	int err;

	init();

	while (count < PBP_STREAMS_TO_SEND) {
		printk("Resetting for iteration %d\n", count);
		err = reset();
		if (err != 0) {
			printk("Resetting failed: %d\n", err);
			break;
		}

		/* Register callbacks */
		bt_le_scan_cb_register(&broadcast_scan_cb);

		/* Start scanning */
		printk("Starting scan\n");
		err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL);
		if (err) {
			printk("Scan start failed (err %d)\n", err);
			break;
		}

		/* Wait for PA sync */
		printk("Waiting for PA Sync\n");
		err = k_sem_take(&sem_pa_synced, SEM_TIMEOUT);
		if (err != 0) {
			printk("sem_pa_synced timed out\n");
			break;
		}

		/* Wait for BASE decode */
		printk("Waiting for BASE\n");
		err = k_sem_take(&sem_base_received, SEM_TIMEOUT);
		if (err != 0) {
			printk("sem_base_received timed out\n");
			break;
		}

		/* Create broadcast sink */
		printk("Creating broadcast sink\n");
		err = bt_bap_broadcast_sink_create(bcast_pa_sync, broadcast_id, &broadcast_sink);
		if (err != 0) {
			printk("Sink not created!\n");
			break;
		}

		printk("Waiting for syncable\n");
		err = k_sem_take(&sem_syncable, SEM_TIMEOUT);
		if (err != 0) {
			printk("sem_syncable timed out\n");
			break;
		}

		/* Sync to broadcast source */
		printk("Syncing broadcast sink\n");
		err = bt_bap_broadcast_sink_sync(broadcast_sink, bis_index_bitfield,
						 streams_p, NULL);
		if (err != 0) {
			printk("Unable to sync to broadcast source: %d\n", err);
			break;
		}

		/* Wait for data */
		printk("Waiting for data\n");
		WAIT_FOR_FLAG(flag_audio_received);

		printk("Sending signal to broadcaster to stop\n");
		backchannel_sync_send_all(); /* let the broadcast source know it can stop */

		/* Wait for the stream to end */
		printk("Waiting for sync lost\n");
		k_sem_take(&sem_pa_sync_lost, SEM_TIMEOUT);

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
