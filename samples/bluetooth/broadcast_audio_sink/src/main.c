/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/capabilities.h>

#define SEM_TIMEOUT K_SECONDS(10)

static K_SEM_DEFINE(sem_broadcaster_found, 0U, 1U);
static K_SEM_DEFINE(sem_pa_synced, 0U, 1U);
static K_SEM_DEFINE(sem_base_received, 0U, 1U);
static K_SEM_DEFINE(sem_syncable, 0U, 1U);
static K_SEM_DEFINE(sem_pa_sync_lost, 0U, 1U);

static struct bt_audio_broadcast_sink *broadcast_sink;
static struct bt_audio_stream streams[CONFIG_BT_AUDIO_BROADCAST_SNK_STREAM_COUNT];

static struct bt_codec codec = BT_CODEC_LC3_CONFIG_16_2(BT_AUDIO_LOCATION_FRONT_LEFT,
							BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);

/* Create a mask for the maximum BIS we can sync to using the number of streams
 * we have. We add an additional 1 since the bis indexes start from 1 and not
 * 0.
 */
static const uint32_t bis_index_mask = BIT_MASK(ARRAY_SIZE(streams) + 1U);
static uint32_t bis_index_bitfield;

static void stream_started_cb(struct bt_audio_stream *stream)
{
	printk("Stream %p started\n", stream);
}

static void stream_stopped_cb(struct bt_audio_stream *stream)
{
	printk("Stream %p stopped\n", stream);
}

static void stream_recv_cb(struct bt_audio_stream *stream,
			   const struct bt_iso_recv_info *info,
			   struct net_buf *buf)
{
	static uint32_t recv_cnt;

	recv_cnt++;
	if ((recv_cnt % 1000U) == 0U) {
		printk("Received %u total ISO packets\n", recv_cnt);
	}
}

static struct bt_audio_stream_ops stream_ops = {
	.started = stream_started_cb,
	.stopped = stream_stopped_cb,
	.recv = stream_recv_cb
};

static bool scan_recv_cb(const struct bt_le_scan_recv_info *info,
			 uint32_t broadcast_id)
{
	k_sem_give(&sem_broadcaster_found);

	return true;
}

static void scan_term_cb(int err)
{
	if (err != 0) {
		printk("Scan terminated with error: %d\n", err);
	}
}

static void pa_synced_cb(struct bt_audio_broadcast_sink *sink,
			 struct bt_le_per_adv_sync *sync,
			 uint32_t broadcast_id)
{
	if (broadcast_sink != NULL) {
		printk("Unexpected PA sync\n");
		return;
	}

	printk("PA synced for broadcast sink %p with broadcast ID 0x%06X\n",
	       sink, broadcast_id);

	broadcast_sink = sink;

	k_sem_give(&sem_pa_synced);
}

static void base_recv_cb(struct bt_audio_broadcast_sink *sink,
			 const struct bt_audio_base *base)
{
	uint32_t base_bis_index_bitfield = 0U;

	if (k_sem_count_get(&sem_base_received) != 0U) {
		return;
	}

	printk("Received BASE with %u subgroups from broadcast sink %p\n",
	       base->subgroup_count, sink);

	for (size_t i = 0U; i < base->subgroup_count; i++) {
		for (size_t j = 0U; j < base->subgroups[i].bis_count; j++) {
			const uint8_t index = base->subgroups[i].bis_data[j].index;

			base_bis_index_bitfield |= BIT(index);
		}
	}

	bis_index_bitfield = base_bis_index_bitfield & bis_index_mask;

	k_sem_give(&sem_base_received);
}

static void syncable_cb(struct bt_audio_broadcast_sink *sink, bool encrypted)
{
	if (encrypted) {
		printk("Cannot sync to encrypted broadcast source\n");
		return;
	}

	k_sem_give(&sem_syncable);
}

static void pa_sync_lost_cb(struct bt_audio_broadcast_sink *sink)
{
	if (broadcast_sink == NULL) {
		printk("Unexpected PA sync lost\n");
		return;
	}

	printk("Sink %p disconnected\n", sink);

	broadcast_sink = NULL;

	k_sem_give(&sem_pa_sync_lost);
}

static struct bt_audio_broadcast_sink_cb broadcast_sink_cbs = {
	.scan_recv = scan_recv_cb,
	.scan_term = scan_term_cb,
	.base_recv = base_recv_cb,
	.syncable = syncable_cb,
	.pa_synced = pa_synced_cb,
	.pa_sync_lost = pa_sync_lost_cb
};

static struct bt_audio_capability capabilities = {
	.dir = BT_AUDIO_DIR_SINK,
	.codec = &codec,
};

static int init(void)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth enable failed (err %d)\n", err);
		return err;
	}

	printk("Bluetooth initialized\n");

	err = bt_audio_capability_register(&capabilities);
	if (err) {
		printk("Capability register failed (err %d)\n", err);
		return err;
	}

	bt_audio_broadcast_sink_register_cb(&broadcast_sink_cbs);

	for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
		streams[i].ops = &stream_ops;
	}

	return 0;
}

static void reset(void)
{
	int err;

	bis_index_bitfield = 0U;

	k_sem_reset(&sem_broadcaster_found);
	k_sem_reset(&sem_pa_synced);
	k_sem_reset(&sem_base_received);
	k_sem_reset(&sem_syncable);
	k_sem_reset(&sem_pa_sync_lost);

	if (broadcast_sink != NULL) {
		err = bt_audio_broadcast_sink_delete(broadcast_sink);
		if (err) {
			printk("Deleting broadcast sink failed (err %d)\n", err);
			return;
		}

		broadcast_sink = NULL;
	}
}

void main(void)
{
	struct bt_audio_stream *streams_p[ARRAY_SIZE(streams)];
	int err;

	err = init();
	if (err) {
		printk("Init failed (err %d)\n", err);
		return;
	}

	for (size_t i = 0U; i < ARRAY_SIZE(streams_p); i++) {
		streams_p[i] = &streams[i];
	}

	while (true) {
		reset();

		printk("Scanning for broadcast sources\n");
		err = bt_audio_broadcast_sink_scan_start(BT_LE_SCAN_ACTIVE);
		if (err != 0) {
			printk("Unable to start scan for broadcast sources: %d\n",
			       err);
			return;
		}

		/* TODO: Update K_FOREVER with a sane value, and handle error */
		err = k_sem_take(&sem_broadcaster_found, SEM_TIMEOUT);
		if (err != 0) {
			printk("sem_broadcaster_found timed out, resetting\n");
			continue;
		}
		printk("Broadcast source found, waiting for PA sync\n");

		err = k_sem_take(&sem_pa_synced, SEM_TIMEOUT);
		if (err != 0) {
			printk("sem_pa_synced timed out, resetting\n");
			continue;
		}
		printk("Broadcast source PA synced, waiting for BASE\n");

		err = k_sem_take(&sem_base_received, SEM_TIMEOUT);
		if (err != 0) {
			printk("sem_base_received timed out, resetting\n");
			continue;
		}
		printk("BASE received, waiting for syncable\n");

		err = k_sem_take(&sem_syncable, SEM_TIMEOUT);
		if (err != 0) {
			printk("sem_syncable timed out, resetting\n");
			continue;
		}

		printk("Syncing to broadcast\n");
		err = bt_audio_broadcast_sink_sync(broadcast_sink,
						   bis_index_bitfield,
						   streams_p,
						   NULL);
		if (err != 0) {
			printk("Unable to sync to broadcast source: %d\n", err);
			return;
		}

		printk("Waiting for PA disconnected\n");
		k_sem_take(&sem_pa_sync_lost, K_FOREVER);
	}
}
