/*
 * Copyright (c) 2021-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_AUDIO_BROADCAST_SINK)

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/capabilities.h>
#include "common.h"

extern enum bst_result_t bst_result;

CREATE_FLAG(broadcaster_found);
CREATE_FLAG(base_received);
CREATE_FLAG(pa_synced);
CREATE_FLAG(flag_syncable);
CREATE_FLAG(pa_sync_lost);
CREATE_FLAG(flag_received);

static struct bt_audio_broadcast_sink *g_sink;
static struct bt_audio_stream broadcast_sink_streams[CONFIG_BT_AUDIO_BROADCAST_SNK_STREAM_COUNT];
static struct bt_audio_stream *streams[ARRAY_SIZE(broadcast_sink_streams)];
static struct bt_audio_lc3_preset preset_16_2_1 =
	BT_AUDIO_LC3_BROADCAST_PRESET_16_2_1(BT_AUDIO_LOCATION_FRONT_LEFT,
					     BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);

static K_SEM_DEFINE(sem_started, 0U, ARRAY_SIZE(streams));
static K_SEM_DEFINE(sem_stopped, 0U, ARRAY_SIZE(streams));

/* Create a mask for the maximum BIS we can sync to using the number of streams
 * we have. We add an additional 1 since the bis indexes start from 1 and not
 * 0.
 */
static const uint32_t bis_index_mask = BIT_MASK(ARRAY_SIZE(streams) + 1U);
static uint32_t bis_index_bitfield;

static bool scan_recv_cb(const struct bt_le_scan_recv_info *info,
			 uint32_t broadcast_id)
{
	SET_FLAG(broadcaster_found);

	return true;
}

static void scan_term_cb(int err)
{
	if (err != 0) {
		FAIL("Scan terminated with error: %d", err);
	}
}

static void pa_synced_cb(struct bt_audio_broadcast_sink *sink,
			 struct bt_le_per_adv_sync *sync,
			 uint32_t broadcast_id)
{
	if (g_sink != NULL) {
		FAIL("Unexpected PA sync");
		return;
	}

	printk("PA synced for broadcast sink %p with broadcast ID 0x%06X\n",
	       sink, broadcast_id);

	g_sink = sink;

	SET_FLAG(pa_synced);
}

static void base_recv_cb(struct bt_audio_broadcast_sink *sink,
			 const struct bt_audio_base *base)
{
	uint32_t base_bis_index_bitfield = 0U;

	if (TEST_FLAG(base_received)) {
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

static void syncable_cb(struct bt_audio_broadcast_sink *sink, bool encrypted)
{
	printk("Broadcast sink %p syncable with%s encryption\n",
	       sink, encrypted ? "" : "out");
	SET_FLAG(flag_syncable);
}

static void pa_sync_lost_cb(struct bt_audio_broadcast_sink *sink)
{
	if (g_sink == NULL) {
		FAIL("Unexpected PA sync lost");
		return;
	}

	if (TEST_FLAG(pa_sync_lost)) {
		return;
	}

	printk("Sink %p disconnected\n", sink);

	g_sink = NULL;

	SET_FLAG(pa_sync_lost);
}

static struct bt_audio_broadcast_sink_cb broadcast_sink_cbs = {
	.scan_recv = scan_recv_cb,
	.scan_term = scan_term_cb,
	.base_recv = base_recv_cb,
	.pa_synced = pa_synced_cb,
	.syncable = syncable_cb,
	.pa_sync_lost = pa_sync_lost_cb
};

static struct bt_audio_capability capabilities = {
	.dir = BT_AUDIO_DIR_SINK,
	.codec = &preset_16_2_1.codec,
};

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

static void recv_cb(struct bt_audio_stream *stream,
		    const struct bt_iso_recv_info *info,
		    struct net_buf *buf)
{
	SET_FLAG(flag_received);
}

static struct bt_audio_stream_ops stream_ops = {
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

	err = bt_audio_capability_register(&capabilities);
	if (err) {
		FAIL("Capability register failed (err %d)\n", err);
		return err;
	}

	bt_audio_broadcast_sink_register_cb(&broadcast_sink_cbs);

	UNSET_FLAG(broadcaster_found);
	UNSET_FLAG(base_received);
	UNSET_FLAG(pa_synced);

	for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
		streams[i] = &broadcast_sink_streams[i];
		bt_audio_stream_cb_register(streams[i], &stream_ops);
	}

	return 0;
}

static void test_main(void)
{
	int err;

	err = init();
	if (err) {
		FAIL("Init failed (err %d)\n", err);
		return;
	}

	printk("Scanning for broadcast sources\n");
	err = bt_audio_broadcast_sink_scan_start(BT_LE_SCAN_ACTIVE);
	if (err != 0) {
		FAIL("Unable to start scan for broadcast sources: %d", err);
		return;
	}

	WAIT_FOR_FLAG(broadcaster_found);
	printk("Broadcast source found, waiting for PA sync\n");
	WAIT_FOR_FLAG(pa_synced);
	printk("Broadcast source PA synced, waiting for BASE\n");
	WAIT_FOR_FLAG(base_received);
	printk("BASE received\n");

	printk("Waiting for BIG syncable\n");
	WAIT_FOR_FLAG(flag_syncable);

	printk("Syncing the sink\n");
	err = bt_audio_broadcast_sink_sync(g_sink, bis_index_bitfield, streams,
					   NULL);
	if (err != 0) {
		FAIL("Unable to sync the sink: %d\n", err);
		return;
	}

	/* Wait for all to be started */
	printk("Waiting for streams to be started\n");
	for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
		k_sem_take(&sem_started, K_FOREVER);
	}

	printk("Waiting for data\n");
	WAIT_FOR_FLAG(flag_received);

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
	int err;

	err = init();
	if (err) {
		FAIL("Init failed (err %d)\n", err);
		return;
	}

	printk("Scanning for broadcast sources\n");
	err = bt_audio_broadcast_sink_scan_start(BT_LE_SCAN_ACTIVE);
	if (err != 0) {
		FAIL("Unable to start scan for broadcast sources: %d", err);
		return;
	}
	WAIT_FOR_FLAG(broadcaster_found);
	printk("Broadcast source found, waiting for PA sync\n");
	WAIT_FOR_FLAG(pa_synced);
	printk("Broadcast source PA synced, waiting for BASE\n");
	WAIT_FOR_FLAG(base_received);
	printk("BASE received\n");

	printk("Waiting for BIG syncable\n");
	WAIT_FOR_FLAG(flag_syncable);

	printk("Syncing the sink\n");
	/* TODO: Sync to max streams instead of just BIT(1) */
	err = bt_audio_broadcast_sink_sync(g_sink, BIT(1), streams, NULL);
	if (err != 0) {
		FAIL("Unable to sync the sink: %d\n", err);
		return;
	}

	/* Wait for all to be started */
	printk("Waiting for streams to be started\n");
	for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
		k_sem_take(&sem_started, K_FOREVER);
	}

	printk("Waiting for data\n");
	WAIT_FOR_FLAG(flag_received);

	err = bt_audio_broadcast_sink_stop(g_sink);
	if (err != 0) {
		FAIL("Unable to stop sink: %d", err);
		return;
	}

	printk("Waiting for streams to be stopped\n");
	for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
		k_sem_take(&sem_stopped, K_FOREVER);
	}

	err = bt_audio_broadcast_sink_delete(g_sink);
	if (err != 0) {
		FAIL("Unable to delete sink: %d", err);
		return;
	}
	/* No "sync lost" event is generated when we initialized the disconnect */
	g_sink = NULL;

	PASS("Broadcast sink passed\n");
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

#else /* !CONFIG_BT_AUDIO_BROADCAST_SINK */

struct bst_test_list *test_broadcast_sink_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_AUDIO_BROADCAST_SINK */
