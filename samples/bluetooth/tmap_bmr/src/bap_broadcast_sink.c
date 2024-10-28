/*
 * Copyright (c) 2022-2024 Nordic Semiconductor ASA
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/lc3.h>
#include <zephyr/bluetooth/audio/pacs.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <zephyr/bluetooth/audio/tmap.h>
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

#define SEM_TIMEOUT K_SECONDS(10)
#define PA_SYNC_SKIP         5
#define PA_SYNC_INTERVAL_TO_TIMEOUT_RATIO 20 /* Set the timeout relative to interval */

static bool tmap_bms_found;

static K_SEM_DEFINE(sem_pa_synced, 0U, 1U);
static K_SEM_DEFINE(sem_base_received, 0U, 1U);
static K_SEM_DEFINE(sem_syncable, 0U, 1U);
static K_SEM_DEFINE(sem_pa_sync_lost, 0U, 1U);

static void broadcast_scan_recv(const struct bt_le_scan_recv_info *info,
				struct net_buf_simple *ad);
static void broadcast_scan_timeout(void);

static void broadcast_pa_synced(struct bt_le_per_adv_sync *sync,
				struct bt_le_per_adv_sync_synced_info *info);
static void broadcast_pa_recv(struct bt_le_per_adv_sync *sync,
			      const struct bt_le_per_adv_sync_recv_info *info,
			      struct net_buf_simple *buf);
static void broadcast_pa_terminated(struct bt_le_per_adv_sync *sync,
				    const struct bt_le_per_adv_sync_term_info *info);

static struct bt_le_scan_cb broadcast_scan_cb = {
	.recv = broadcast_scan_recv,
	.timeout = broadcast_scan_timeout
};

static struct bt_le_per_adv_sync_cb broadcast_sync_cb = {
	.synced = broadcast_pa_synced,
	.recv = broadcast_pa_recv,
	.term = broadcast_pa_terminated,
};

static struct bt_bap_broadcast_sink *broadcast_sink;
static uint32_t bcast_id;
static struct bt_le_per_adv_sync *bcast_pa_sync;

static struct bt_bap_stream streams[CONFIG_BT_BAP_BROADCAST_SNK_STREAM_COUNT];
struct bt_bap_stream *streams_p[ARRAY_SIZE(streams)];

static const struct bt_audio_codec_cap codec = BT_AUDIO_CODEC_CAP_LC3(
	BT_AUDIO_CODEC_CAP_FREQ_48KHZ, BT_AUDIO_CODEC_CAP_DURATION_10,
	BT_AUDIO_CODEC_CAP_CHAN_COUNT_SUPPORT(1), 40u, 60u, 1u, (BT_AUDIO_CONTEXT_TYPE_MEDIA));

/* Create a mask for the maximum BIS we can sync to using the number of streams
 * we have. We add an additional 1 since the bis indexes start from 1 and not
 * 0.
 */
static const uint32_t bis_index_mask = BIT_MASK(ARRAY_SIZE(streams) + 1U);
static uint32_t bis_index_bitfield;


static void stream_started_cb(struct bt_bap_stream *stream)
{
	printk("Stream %p started\n", stream);
}

static void stream_stopped_cb(struct bt_bap_stream *stream, uint8_t reason)
{
	printk("Stream %p stopped with reason 0x%02X\n", stream, reason);
}

static void stream_recv_cb(struct bt_bap_stream *stream,
			   const struct bt_iso_recv_info *info,
			   struct net_buf *buf)
{
	static uint32_t recv_cnt;

	recv_cnt++;
	if ((recv_cnt % 20U) == 0U) {
		printk("Received %u total ISO packets\n", recv_cnt);
	}
}

static struct bt_bap_stream_ops stream_ops = {
	.started = stream_started_cb,
	.stopped = stream_stopped_cb,
	.recv = stream_recv_cb
};

static struct bt_pacs_cap cap = {
	.codec_cap = &codec,
};

static uint16_t interval_to_sync_timeout(uint16_t interval)
{
	uint32_t interval_ms;
	uint32_t timeout;

	/* Add retries and convert to unit in 10's of ms */
	interval_ms = BT_GAP_PER_ADV_INTERVAL_TO_MS(interval);
	timeout = (interval_ms * PA_SYNC_INTERVAL_TO_TIMEOUT_RATIO) / 10;

	/* Enforce restraints */
	timeout = CLAMP(timeout, BT_GAP_PER_ADV_MIN_TIMEOUT, BT_GAP_PER_ADV_MAX_TIMEOUT);

	return (uint16_t)timeout;
}

static void sync_broadcast_pa(const struct bt_le_scan_recv_info *info,
			      uint32_t broadcast_id)
{
	struct bt_le_per_adv_sync_param param;
	int err;

	/* Unregister the callbacks to prevent broadcast_scan_recv to be called again */
	bt_le_scan_cb_unregister(&broadcast_scan_cb);
	err = bt_le_scan_stop();
	if (err != 0) {
		printk("Could not stop scan: %d", err);
	}

	bt_addr_le_copy(&param.addr, info->addr);
	param.options = 0;
	param.sid = info->sid;
	param.skip = PA_SYNC_SKIP;
	param.timeout = interval_to_sync_timeout(info->interval);
	err = bt_le_per_adv_sync_create(&param, &bcast_pa_sync);
	if (err != 0) {
		printk("Could not sync to PA: %d", err);
	} else {
		bcast_id = broadcast_id;
	}
}

static bool scan_check_and_sync_broadcast(struct bt_data *data, void *user_data)
{
	uint32_t *broadcast_id = user_data;
	struct bt_uuid_16 adv_uuid;

	if (data->type != BT_DATA_SVC_DATA16) {
		return true;
	}

	if (!bt_uuid_create(&adv_uuid.uuid, data->data, BT_UUID_SIZE_16)) {
		return true;
	}

	if (!bt_uuid_cmp(&adv_uuid.uuid, BT_UUID_BROADCAST_AUDIO)) {
		*broadcast_id = sys_get_le24(data->data + BT_UUID_SIZE_16);
		return true;
	}

	if (!bt_uuid_cmp(&adv_uuid.uuid, BT_UUID_TMAS)) {
		struct net_buf_simple tmas_svc_data;
		uint16_t uuid_val;
		uint16_t peer_tmap_role = 0;

		net_buf_simple_init_with_data(&tmas_svc_data,
						(void *)data->data,
						data->data_len);
		uuid_val = net_buf_simple_pull_le16(&tmas_svc_data);
		if (tmas_svc_data.len < sizeof(peer_tmap_role)) {
			return false;
		}

		peer_tmap_role = net_buf_simple_pull_le16(&tmas_svc_data);
		if ((peer_tmap_role & BT_TMAP_ROLE_BMS)) {
			printk("Found TMAP BMS\n");
			tmap_bms_found = true;
		}

		return true;
	}

	return true;
}

static void broadcast_scan_recv(const struct bt_le_scan_recv_info *info,
				struct net_buf_simple *ad)
{
	uint32_t broadcast_id;

	tmap_bms_found = false;

	/* We are only interested in non-connectable periodic advertisers */
	if ((info->adv_props & BT_GAP_ADV_PROP_CONNECTABLE) ||
	     info->interval == 0) {
		return;
	}

	broadcast_id = BT_BAP_INVALID_BROADCAST_ID;
	bt_data_parse(ad, scan_check_and_sync_broadcast, (void *)&broadcast_id);

	if ((broadcast_id != BT_BAP_INVALID_BROADCAST_ID) && tmap_bms_found) {
		sync_broadcast_pa(info, broadcast_id);
	}
}

static void broadcast_scan_timeout(void)
{
	printk("Broadcast scan timed out\n");
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

static void syncable_cb(struct bt_bap_broadcast_sink *sink, const struct bt_iso_biginfo *biginfo)
{
	k_sem_give(&sem_syncable);
}

static void base_recv_cb(struct bt_bap_broadcast_sink *sink, const struct bt_bap_base *base,
			 size_t base_size)
{
	k_sem_give(&sem_base_received);
}

static struct bt_bap_broadcast_sink_cb broadcast_sink_cbs = {
	.syncable = syncable_cb,
	.base_recv = base_recv_cb,
};

static void broadcast_pa_synced(struct bt_le_per_adv_sync *sync,
				struct bt_le_per_adv_sync_synced_info *info)
{
	if (sync == bcast_pa_sync) {
		printk("PA sync %p synced for broadcast sink with broadcast ID 0x%06X\n", sync,
		       bcast_id);

		k_sem_give(&sem_pa_synced);
	}
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

static int reset(void)
{
	if (broadcast_sink != NULL) {
		int err = bt_bap_broadcast_sink_delete(broadcast_sink);

		if (err) {
			printk("Deleting broadcast sink failed (err %d)\n", err);

			return err;
		}

		broadcast_sink = NULL;
	}

	k_sem_reset(&sem_pa_synced);
	k_sem_reset(&sem_base_received);
	k_sem_reset(&sem_syncable);
	k_sem_reset(&sem_pa_sync_lost);

	return 0;
}

int bap_broadcast_sink_init(void)
{
	int err;

	bt_bap_broadcast_sink_register_cb(&broadcast_sink_cbs);
	bt_le_per_adv_sync_cb_register(&broadcast_sync_cb);

	err = bt_pacs_cap_register(BT_AUDIO_DIR_SINK, &cap);
	if (err) {
		printk("Capability register failed (err %d)\n", err);
		return err;
	}

	for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
		streams[i].ops = &stream_ops;
	}

	for (size_t i = 0U; i < ARRAY_SIZE(streams_p); i++) {
		streams_p[i] = &streams[i];
	}

	return 0;
}

int bap_broadcast_sink_run(void)
{
	while (true) {
		int err = reset();

		if (err != 0) {
			printk("Resetting failed: %d - Aborting\n", err);
			return err;
		}

		bt_le_scan_cb_register(&broadcast_scan_cb);
		/* Start scanning */
		err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, NULL);
		if (err) {
			printk("Scan start failed (err %d)\n", err);
			return err;
		}

		/* Wait for PA sync */
		err = k_sem_take(&sem_pa_synced, SEM_TIMEOUT);
		if (err != 0) {
			printk("sem_pa_synced timed out\n");
			return err;
		}
		printk("Broadcast source PA synced, waiting for BASE\n");

		/* Wait for BASE decode */
		err = k_sem_take(&sem_base_received, SEM_TIMEOUT);
		if (err != 0) {
			printk("sem_base_received timed out\n");
			return err;
		}

		/* Create broadcast sink */
		printk("BASE received, creating broadcast sink\n");
		err = bt_bap_broadcast_sink_create(bcast_pa_sync, bcast_id, &broadcast_sink);
		if (err != 0) {
			printk("bt_bap_broadcast_sink_create failed: %d\n", err);
			return err;
		}

		k_sem_take(&sem_syncable, SEM_TIMEOUT);
		if (err != 0) {
			printk("sem_syncable timed out\n");
			return err;
		}

		/* Sync to broadcast source */
		printk("Syncing to broadcast\n");
		err = bt_bap_broadcast_sink_sync(broadcast_sink, bis_index_bitfield,
						streams_p, NULL);
		if (err != 0) {
			printk("Unable to sync to broadcast source: %d\n", err);
			return err;
		}

		k_sem_take(&sem_pa_sync_lost, K_FOREVER);
	}

	return 0;
}
