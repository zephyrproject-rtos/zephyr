/*
 * Copyright (c) 2021-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_AUDIO_BROADCAST_SOURCE)

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/audio.h>
#include "common.h"

extern enum bst_result_t bst_result;

static void test_main(void)
{
	int err;
	struct bt_audio_lc3_preset preset_16_2_1 = BT_AUDIO_LC3_BROADCAST_PRESET_16_2_1;
	struct bt_audio_lc3_preset preset_16_2_2 = BT_AUDIO_LC3_BROADCAST_PRESET_16_2_2;
	struct bt_audio_stream broadcast_source_streams[CONFIG_BT_AUDIO_BROADCAST_SRC_STREAM_COUNT];
	struct bt_audio_stream *streams[ARRAY_SIZE(broadcast_source_streams)];
	struct bt_audio_broadcast_source *source;

	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	(void)memset(broadcast_source_streams, 0,
		     sizeof(broadcast_source_streams));

	for (size_t i = 0; i < ARRAY_SIZE(streams); i++) {
		streams[i] = &broadcast_source_streams[i];
	}

	printk("Creating broadcast source\n");
	err = bt_audio_broadcast_source_create(streams, ARRAY_SIZE(streams),
					       &preset_16_2_1.codec,
					       &preset_16_2_1.qos,
					       &source);
	if (err != 0) {
		FAIL("Unable to create broadcast source: %d", err);
		return;
	}

	printk("Reconfiguring broadcast source\n");
	err = bt_audio_broadcast_source_reconfig(source, &preset_16_2_2.codec,
						 &preset_16_2_2.qos);
	if (err != 0) {
		FAIL("Unable to reconfigure broadcast source: %d", err);
		return;
	}

	k_sleep(K_SECONDS(10));

	printk("Deleting broadcast source\n");
	err = bt_audio_broadcast_source_delete(source);
	if (err != 0) {
		FAIL("Unable to delete broadcast source: %d", err);
		return;
	}
	source = NULL;

	/* Recreate broadcast source to verify that it's possible */
	printk("Recreating broadcast source\n");
	err = bt_audio_broadcast_source_create(streams, ARRAY_SIZE(streams),
					       &preset_16_2_1.codec,
					       &preset_16_2_1.qos,
					       &source);
	if (err != 0) {
		FAIL("Unable to create broadcast source: %d", err);
		return;
	}

	printk("Deleting broadcast source\n");
	err = bt_audio_broadcast_source_delete(source);
	if (err != 0) {
		FAIL("Unable to delete broadcast source: %d", err);
		return;
	}
	source = NULL;

	PASS("Broadcast source passed\n");
}

static const struct bst_test_instance test_broadcast_source[] = {
	{
		.test_id = "broadcast_source",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_broadcast_source_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_broadcast_source);
}

#else /* CONFIG_BT_AUDIO_BROADCAST_SOURCE */

struct bst_test_list *test_broadcast_source_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_AUDIO_BROADCAST_SOURCE */
