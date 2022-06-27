/*
 * Copyright (c) 2021-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_AUDIO_BROADCAST_SINK)

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/audio.h>
#include "common.h"

extern enum bst_result_t bst_result;

CREATE_FLAG(broadcaster_found);
CREATE_FLAG(base_received);
CREATE_FLAG(pa_synced);
CREATE_FLAG(pa_sync_lost);

static struct bt_audio_broadcast_sink *g_sink;

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
	if (TEST_FLAG(base_received)) {
		return;
	}

	printk("Received BASE with %u subgroups from broadcast sink %p\n",
	       base->subgroup_count, sink);

	SET_FLAG(base_received);
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
	.pa_sync_lost = pa_sync_lost_cb
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

	bt_audio_broadcast_sink_register_cb(&broadcast_sink_cbs);

	UNSET_FLAG(broadcaster_found);
	UNSET_FLAG(base_received);
	UNSET_FLAG(pa_synced);

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

	printk("Waiting for PA disconnected\n");
	WAIT_FOR_FLAG(pa_sync_lost);

	/* TODO: Handle Audio when ISO is supported in BSIM */

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
