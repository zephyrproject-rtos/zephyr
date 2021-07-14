/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bluetooth/bluetooth.h>
#include <bluetooth/audio.h>
#include "common.h"

extern enum bst_result_t bst_result;

CREATE_FLAG(broadcaster_found);

static bool scan_recv(const struct bt_le_scan_recv_info *info,
		     uint32_t broadcast_id)
{
	SET_FLAG(broadcaster_found);

	return true;
}

static void scan_term(int err)
{
	if (err != 0) {
		FAIL("Scan terminated with error: %d", err);
	}
}

static struct bt_codec lc3_codec = BT_CODEC_LC3(BT_CODEC_LC3_FREQ_ANY,
						BT_CODEC_LC3_DURATION_ANY,
						0x03, 30, 240, 2,
						(BT_CODEC_META_CONTEXT_VOICE |
						BT_CODEC_META_CONTEXT_MEDIA),
						BT_CODEC_META_CONTEXT_ANY);

static struct bt_audio_capability_ops lc3_ops = {
	.scan_recv = scan_recv,
	.scan_term = scan_term
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
	printk("Scanning for broadcast sources\n");
	err = bt_audio_broadcaster_scan_start(BT_LE_SCAN_ACTIVE);
	if (err != 0) {
		FAIL("Unable to start scan for broadcast sources: %d", err);
		return;
	}
	WAIT_FOR_FLAG(broadcaster_found);

	k_sleep(K_SECONDS(10));

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
