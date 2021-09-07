/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_BAP) && defined(CONFIG_BT_AUDIO_BROADCAST)

#include <bluetooth/bluetooth.h>
#include <bluetooth/audio.h>
#include "common.h"

extern enum bst_result_t bst_result;

CREATE_FLAG(broadcaster_found);
CREATE_FLAG(base_received);
CREATE_FLAG(pa_synced);
CREATE_FLAG(pa_sync_lost);

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
	printk("PA synced for broadcast sink %p with broadcast ID 0x%06X\n",
	       sink, broadcast_id);

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
	if (TEST_FLAG(pa_sync_lost)) {
		return;
	}

	printk("Sink %p disconnected\n", sink);

	SET_FLAG(pa_sync_lost);
}

static struct bt_codec lc3_codec = BT_CODEC_LC3(BT_CODEC_LC3_FREQ_ANY,
						BT_CODEC_LC3_DURATION_ANY,
						0x03, 30, 240, 2,
						(BT_CODEC_META_CONTEXT_VOICE |
						BT_CODEC_META_CONTEXT_MEDIA),
						BT_CODEC_META_CONTEXT_ANY);

static struct bt_audio_capability_ops lc3_ops = {
	.scan_recv = scan_recv_cb,
	.scan_term = scan_term_cb,
	.base_recv = base_recv_cb,
	.pa_synced = pa_synced_cb,
	.pa_sync_lost = pa_sync_lost_cb
};

static void test_main(void)
{
	int err;
	static struct bt_audio_capability caps = {
		.type = BT_AUDIO_SINK,
		.qos = BT_AUDIO_QOS(0x00, 0x02, 0u, 60u, 20000u, 40000u),
		.codec = &lc3_codec,
		.ops = &lc3_ops,
	};

	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	err = bt_audio_capability_register(&caps);
	if (err != 0) {
		FAIL("Failed to register capabilities: %d", err);
		return;
	}

	UNSET_FLAG(broadcaster_found);
	UNSET_FLAG(base_received);
	UNSET_FLAG(pa_synced);
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

	PASS("Broadcast sink passed\n");
}

static const struct bst_test_instance test_broadcast_sink[] = {
	{
		.test_id = "broadcast_sink",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_broadcast_sink_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_broadcast_sink);
}

#else /* !(CONFIG_BT_BAP && CONFIG_BT_AUDIO_BROADCAST) */

struct bst_test_list *test_broadcast_sink_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_BAP && CONFIG_BT_AUDIO_BROADCAST */
