/*
 * Copyright (c) 2021-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_BAP_BROADCAST_SINK)

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <zephyr/bluetooth/audio/pacs.h>
#include <zephyr/sys/byteorder.h>
#include "common.h"

extern enum bst_result_t bst_result;

CREATE_FLAG(broadcaster_found);
CREATE_FLAG(base_received);
CREATE_FLAG(flag_base_metadata_updated);
CREATE_FLAG(pa_synced);
CREATE_FLAG(flag_syncable);
CREATE_FLAG(pa_sync_lost);
CREATE_FLAG(flag_received);

static struct bt_bap_broadcast_sink *g_sink;
static struct bt_le_scan_recv_info broadcaster_info;
static bt_addr_le_t broadcaster_addr;
static struct bt_le_per_adv_sync *pa_sync;
static uint32_t broadcaster_broadcast_id;
static struct bt_bap_stream broadcast_sink_streams[CONFIG_BT_BAP_BROADCAST_SNK_STREAM_COUNT];
static struct bt_bap_stream *streams[ARRAY_SIZE(broadcast_sink_streams)];

static const struct bt_audio_codec_cap codec_cap = BT_AUDIO_CODEC_CAP_LC3(
	BT_AUDIO_CODEC_LC3_FREQ_ANY, BT_AUDIO_CODEC_LC3_DURATION_ANY,
	BT_AUDIO_CODEC_LC3_CHAN_COUNT_SUPPORT(1, 2), 30, 240, 2,
	(BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BT_AUDIO_CONTEXT_TYPE_MEDIA));

static K_SEM_DEFINE(sem_started, 0U, ARRAY_SIZE(streams));
static K_SEM_DEFINE(sem_stopped, 0U, ARRAY_SIZE(streams));

static uint8_t metadata[CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE];

/* Create a mask for the maximum BIS we can sync to using the number of streams
 * we have. We add an additional 1 since the bis indexes start from 1 and not
 * 0.
 */
static const uint32_t bis_index_mask = BIT_MASK(ARRAY_SIZE(streams) + 1U);
static uint32_t bis_index_bitfield;

static void base_recv_cb(struct bt_bap_broadcast_sink *sink, const struct bt_bap_base *base)
{
	uint32_t base_bis_index_bitfield = 0U;

	if (TEST_FLAG(base_received)) {

		if (base->subgroup_count > 0 &&
		    memcmp(metadata, base->subgroups[0].codec_cfg.meta,
			   sizeof(base->subgroups[0].codec_cfg.meta)) != 0) {

			(void)memcpy(metadata, base->subgroups[0].codec_cfg.meta,
				     sizeof(base->subgroups[0].codec_cfg.meta));

			SET_FLAG(flag_base_metadata_updated);
		}

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

	SET_FLAG(base_received);
}

static void syncable_cb(struct bt_bap_broadcast_sink *sink, bool encrypted)
{
	printk("Broadcast sink %p syncable with%s encryption\n",
	       sink, encrypted ? "" : "out");
	SET_FLAG(flag_syncable);
}

static struct bt_bap_broadcast_sink_cb broadcast_sink_cbs = {
	.base_recv = base_recv_cb,
	.syncable = syncable_cb,
};

static bool scan_check_and_sync_broadcast(struct bt_data *data, void *user_data)
{
	const struct bt_le_scan_recv_info *info = user_data;
	char le_addr[BT_ADDR_LE_STR_LEN];
	struct bt_uuid_16 adv_uuid;
	uint32_t broadcast_id;

	if (TEST_FLAG(broadcaster_found)) {
		/* no-op*/
		return false;
	}

	if (data->type != BT_DATA_SVC_DATA16) {
		return true;
	}

	if (data->data_len < BT_UUID_SIZE_16 + BT_AUDIO_BROADCAST_ID_SIZE) {
		return true;
	}

	if (!bt_uuid_create(&adv_uuid.uuid, data->data, BT_UUID_SIZE_16)) {
		return true;
	}

	if (bt_uuid_cmp(&adv_uuid.uuid, BT_UUID_BROADCAST_AUDIO)) {
		return true;
	}

	broadcast_id = sys_get_le24(data->data + BT_UUID_SIZE_16);

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	printk("Found broadcaster with ID 0x%06X and addr %s and sid 0x%02X\n", broadcast_id,
	       le_addr, info->sid);

	SET_FLAG(broadcaster_found);

	/* Store info for PA sync parameters */
	memcpy(&broadcaster_info, info, sizeof(broadcaster_info));
	bt_addr_le_copy(&broadcaster_addr, info->addr);
	broadcaster_broadcast_id = broadcast_id;

	/* Stop parsing */
	return false;
}

static void broadcast_scan_recv(const struct bt_le_scan_recv_info *info, struct net_buf_simple *ad)
{
	if (info->interval != 0U) {
		bt_data_parse(ad, scan_check_and_sync_broadcast, (void *)info);
	}
}

static struct bt_le_scan_cb bap_scan_cb = {
	.recv = broadcast_scan_recv,
};

static void bap_pa_sync_synced_cb(struct bt_le_per_adv_sync *sync,
				  struct bt_le_per_adv_sync_synced_info *info)
{
	if (sync == pa_sync) {
		printk("PA sync %p synced for broadcast sink with broadcast ID 0x%06X\n", sync,
		       broadcaster_broadcast_id);

		SET_FLAG(pa_synced);
	}
}

static void bap_pa_sync_terminated_cb(struct bt_le_per_adv_sync *sync,
				      const struct bt_le_per_adv_sync_term_info *info)
{
	if (sync == pa_sync) {
		printk("PA sync %p lost with reason %u\n", sync, info->reason);
		pa_sync = NULL;

		SET_FLAG(pa_sync_lost);
	}
}

static struct bt_le_per_adv_sync_cb bap_pa_sync_cb = {
	.synced = bap_pa_sync_synced_cb,
	.term = bap_pa_sync_terminated_cb,
};

static struct bt_pacs_cap cap = {
	.codec_cap = &codec_cap,
};

static void started_cb(struct bt_bap_stream *stream)
{
	printk("Stream %p started\n", stream);
	k_sem_give(&sem_started);
}

static void stopped_cb(struct bt_bap_stream *stream, uint8_t reason)
{
	printk("Stream %p stopped with reason 0x%02X\n", stream, reason);
	k_sem_give(&sem_stopped);
}

static void recv_cb(struct bt_bap_stream *stream,
		    const struct bt_iso_recv_info *info,
		    struct net_buf *buf)
{
	SET_FLAG(flag_received);
}

static struct bt_bap_stream_ops stream_ops = {
	.started = started_cb,
	.stopped = stopped_cb,
	.recv = recv_cb
};

static int init(void)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth enable failed (err %d)\n", err);
		return err;
	}

	printk("Bluetooth initialized\n");

	err = bt_pacs_cap_register(BT_AUDIO_DIR_SINK, &cap);
	if (err) {
		FAIL("Capability register failed (err %d)\n", err);
		return err;
	}

	/* Test invalid input */
	err = bt_bap_broadcast_sink_register_cb(NULL);
	if (err == 0) {
		FAIL("bt_bap_broadcast_sink_register_cb did not fail with NULL cb\n");
		return err;
	}

	err = bt_bap_broadcast_sink_register_cb(&broadcast_sink_cbs);
	if (err != 0) {
		FAIL("Sink callback register failed (err %d)\n", err);
		return err;
	}

	bt_le_per_adv_sync_cb_register(&bap_pa_sync_cb);
	bt_le_scan_cb_register(&bap_scan_cb);

	UNSET_FLAG(broadcaster_found);
	UNSET_FLAG(base_received);
	UNSET_FLAG(pa_synced);

	for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
		streams[i] = &broadcast_sink_streams[i];
		bt_bap_stream_cb_register(streams[i], &stream_ops);
	}

	return 0;
}

static uint16_t interval_to_sync_timeout(uint16_t pa_interval)
{
	uint16_t pa_timeout;

	if (pa_interval == BT_BAP_PA_INTERVAL_UNKNOWN) {
		/* Use maximum value to maximize chance of success */
		pa_timeout = BT_GAP_PER_ADV_MAX_TIMEOUT;
	} else {
		/* Ensure that the following calculation does not overflow silently */
		__ASSERT(SYNC_RETRY_COUNT < 10, "SYNC_RETRY_COUNT shall be less than 10");

		/* Add retries and convert to unit in 10's of ms */
		pa_timeout = ((uint32_t)pa_interval * SYNC_RETRY_COUNT) / 10;

		/* Enforce restraints */
		pa_timeout =
			CLAMP(pa_timeout, BT_GAP_PER_ADV_MIN_TIMEOUT, BT_GAP_PER_ADV_MAX_TIMEOUT);
	}

	return pa_timeout;
}

static int pa_sync_create(void)
{
	struct bt_le_per_adv_sync_param create_params = {0};

	bt_addr_le_copy(&create_params.addr, &broadcaster_addr);
	create_params.options = BT_LE_PER_ADV_SYNC_OPT_FILTER_DUPLICATE;
	create_params.sid = broadcaster_info.sid;
	create_params.skip = PA_SYNC_SKIP;
	create_params.timeout = interval_to_sync_timeout(broadcaster_info.interval);

	return bt_le_per_adv_sync_create(&create_params, &pa_sync);
}

static void test_scan_and_pa_sync(void)
{
	int err;

	printk("Scanning for broadcast sources\n");
	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, NULL);
	if (err != 0) {
		FAIL("Unable to start scan for broadcast sources: %d", err);
		return;
	}

	WAIT_FOR_FLAG(broadcaster_found);

	printk("Broadcast source found, stopping scan\n");
	err = bt_le_scan_stop();
	if (err != 0) {
		FAIL("bt_le_scan_stop failed with %d\n", err);
		return;
	}

	printk("Scan stopped, attempting to PA sync to the broadcaster with id 0x%06X\n",
	       broadcaster_broadcast_id);
	err = pa_sync_create();
	if (err != 0) {
		FAIL("Could not create Broadcast PA sync: %d\n", err);
		return;
	}

	printk("Waiting for PA sync\n");
	WAIT_FOR_FLAG(pa_synced);
}

static void test_broadcast_sink_create(void)
{
	int err;

	printk("Creating the broadcast sink\n");
	err = bt_bap_broadcast_sink_create(pa_sync, broadcaster_broadcast_id, &g_sink);
	if (err != 0) {
		FAIL("Unable to create the sink: %d\n", err);
		return;
	}
}

static void test_broadcast_sink_create_inval(void)
{
	int err;

	err = bt_bap_broadcast_sink_create(NULL, broadcaster_broadcast_id, &g_sink);
	if (err == 0) {
		FAIL("bt_bap_broadcast_sink_create did not fail with NULL sink\n");
		return;
	}

	err = bt_bap_broadcast_sink_create(pa_sync, INVALID_BROADCAST_ID, &g_sink);
	if (err == 0) {
		FAIL("bt_bap_broadcast_sink_create did not fail with invalid broadcast ID\n");
		return;
	}

	err = bt_bap_broadcast_sink_create(pa_sync, broadcaster_broadcast_id, NULL);
	if (err == 0) {
		FAIL("bt_bap_broadcast_sink_create did not fail with NULL sink\n");
		return;
	}
}

static void test_broadcast_sync(void)
{
	int err;

	printk("Syncing the sink\n");
	err = bt_bap_broadcast_sink_sync(g_sink, bis_index_bitfield, streams, NULL);
	if (err != 0) {
		FAIL("Unable to sync the sink: %d\n", err);
		return;
	}

	/* Wait for all to be started */
	printk("Waiting for streams to be started\n");
	for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
		k_sem_take(&sem_started, K_FOREVER);
	}
}

static void test_broadcast_sync_inval(void)
{
	struct bt_bap_stream *tmp_streams[ARRAY_SIZE(streams) + 1] = {0};
	uint32_t bis_index;
	int err;

	err = bt_bap_broadcast_sink_sync(NULL, bis_index_bitfield, streams, NULL);
	if (err == 0) {
		FAIL("bt_bap_broadcast_sink_sync did not fail with NULL sink\n");
		return;
	}

	bis_index = 0;
	err = bt_bap_broadcast_sink_sync(g_sink, bis_index, streams, NULL);
	if (err == 0) {
		FAIL("bt_bap_broadcast_sink_sync did not fail with invalid BIS indexes: 0x%08X\n",
		     bis_index);
		return;
	}

	bis_index = BIT(0);
	err = bt_bap_broadcast_sink_sync(g_sink, bis_index, streams, NULL);
	if (err == 0) {
		FAIL("bt_bap_broadcast_sink_sync did not fail with invalid BIS indexes: 0x%08X\n",
		     bis_index);
		return;
	}

	err = bt_bap_broadcast_sink_sync(g_sink, bis_index, NULL, NULL);
	if (err == 0) {
		FAIL("bt_bap_broadcast_sink_sync did not fail with NULL streams\n");
		return;
	}

	memcpy(tmp_streams, streams, sizeof(streams));
	bis_index = 0U;
	for (size_t i = 0U; i < ARRAY_SIZE(tmp_streams); i++) {
		bis_index |= BIT(i + BT_ISO_BIS_INDEX_MIN);
	}

	err = bt_bap_broadcast_sink_sync(g_sink, bis_index, tmp_streams, NULL);
	if (err == 0) {
		FAIL("bt_bap_broadcast_sink_sync did not fail with NULL streams[%zu]\n",
		     ARRAY_SIZE(tmp_streams) - 1);
		return;
	}

	bis_index = 0U;
	for (size_t i = 0U; i < CONFIG_BT_BAP_BROADCAST_SNK_STREAM_COUNT + 1; i++) {
		bis_index |= BIT(i + BT_ISO_BIS_INDEX_MIN);
	}

	err = bt_bap_broadcast_sink_sync(g_sink, bis_index, tmp_streams, NULL);
	if (err == 0) {
		FAIL("bt_bap_broadcast_sink_sync did not fail with invalid BIS indexes: 0x%08X\n",
		     bis_index);
		return;
	}
}

static void test_broadcast_stop(void)
{
	int err;

	err = bt_bap_broadcast_sink_stop(g_sink);
	if (err != 0) {
		FAIL("Unable to stop sink: %d", err);
		return;
	}

	printk("Waiting for streams to be stopped\n");
	for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
		k_sem_take(&sem_stopped, K_FOREVER);
	}
}

static void test_broadcast_stop_inval(void)
{
	int err;

	err = bt_bap_broadcast_sink_stop(NULL);
	if (err == 0) {
		FAIL("bt_bap_broadcast_sink_stop did not fail with NULL sink\n");
		return;
	}
}

static void test_broadcast_delete(void)
{
	int err;

	err = bt_bap_broadcast_sink_delete(g_sink);
	if (err != 0) {
		FAIL("Unable to stop sink: %d", err);
		return;
	}

	/* No "sync lost" event is generated when we initialized the disconnect */
}

static void test_broadcast_delete_inval(void)
{
	int err;

	err = bt_bap_broadcast_sink_delete(NULL);
	if (err == 0) {
		FAIL("bt_bap_broadcast_sink_delete did not fail with NULL sink\n");
		return;
	}
}

static void test_common(void)
{
	int err;

	err = init();
	if (err) {
		FAIL("Init failed (err %d)\n", err);
		return;
	}

	test_scan_and_pa_sync();

	test_broadcast_sink_create_inval();
	test_broadcast_sink_create();

	printk("Broadcast source PA synced, waiting for BASE\n");
	WAIT_FOR_FLAG(base_received);
	printk("BASE received\n");

	printk("Waiting for BIG syncable\n");
	WAIT_FOR_FLAG(flag_syncable);

	test_broadcast_sync_inval();
	test_broadcast_sync();

	printk("Waiting for data\n");
	WAIT_FOR_FLAG(flag_received);

	/* Ensure that we also see the metadata update */
	printk("Waiting for metadata update\n");
	WAIT_FOR_FLAG(flag_base_metadata_updated)
}

static void test_main(void)
{
	test_common();

	/* The order of PA sync lost and BIG Sync lost is irrelevant
	 * and depend on timeout parameters. We just wait for PA first, but
	 * either way will work.
	 */
	printk("Waiting for PA disconnected\n");
	WAIT_FOR_FLAG(pa_sync_lost);

	printk("Waiting for streams to be stopped\n");
	for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
		k_sem_take(&sem_stopped, K_FOREVER);
	}

	PASS("Broadcast sink passed\n");
}

static void test_sink_disconnect(void)
{
	test_common();

	test_broadcast_stop_inval();
	test_broadcast_stop();

	/* Retry sync*/
	test_broadcast_sync();
	test_broadcast_stop();

	test_broadcast_delete_inval();
	test_broadcast_delete();
	g_sink = NULL;

	PASS("Broadcast sink disconnect passed\n");
}

static const struct bst_test_instance test_broadcast_sink[] = {
	{
		.test_id = "broadcast_sink",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main
	},
	{
		.test_id = "broadcast_sink_disconnect",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_sink_disconnect
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_broadcast_sink_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_broadcast_sink);
}

#else /* !CONFIG_BT_BAP_BROADCAST_SINK */

struct bst_test_list *test_broadcast_sink_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_BAP_BROADCAST_SINK */
