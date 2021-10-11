/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_BAP) && defined(CONFIG_BT_AUDIO_BROADCAST_SOURCE)

#include <bluetooth/bluetooth.h>
#include <bluetooth/audio.h>
#include "../../../../subsys/bluetooth/host/audio/endpoint.h"
#include "common.h"

extern enum bst_result_t bst_result;

struct lc3_preset {
	const char *name;
	struct bt_codec codec;
	struct bt_codec_qos qos;
};

#define LC3_PRESET(_name, _codec, _qos) \
	{ \
		.name = _name, \
		.codec = _codec, \
		.qos = _qos, \
	}

static void test_main(void)
{
	int err;
	struct lc3_preset preset_48_1_2 =
		LC3_PRESET("48_1_2",
			   BT_CODEC_LC3_CONFIG_48_1,
			   BT_CODEC_LC3_QOS_7_5_OUT_UNFRAMED(75u, 23u, 45u,
							     40000u));
	struct lc3_preset preset_48_2_2 =
		LC3_PRESET("48_2_2",
			   BT_CODEC_LC3_CONFIG_48_2,
			   BT_CODEC_LC3_QOS_10_OUT_UNFRAMED(100u, 23u, 60u,
							    40000u));
	struct bt_audio_chan broadcast_source_chans[BROADCAST_STREAM_CNT];
	struct bt_audio_broadcast_source *source;

	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	(void)memset(broadcast_source_chans, 0, sizeof(broadcast_source_chans));

	printk("Creating broadcast source\n");
	err = bt_audio_broadcast_source_create(broadcast_source_chans,
					       ARRAY_SIZE(broadcast_source_chans),
					       &preset_48_1_2.codec,
					       &preset_48_1_2.qos,
					       &source);
	if (err != 0) {
		FAIL("Unable to create broadcast source: %d", err);
		return;
	}

	printk("Reconfiguring broadcast source\n");
	err = bt_audio_broadcast_source_reconfig(source, &preset_48_2_2.codec,
						 &preset_48_2_2.qos);
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
	err = bt_audio_broadcast_source_create(broadcast_source_chans,
					       ARRAY_SIZE(broadcast_source_chans),
					       &preset_48_1_2.codec,
					       &preset_48_1_2.qos,
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

#else /* !(CONFIG_BT_BAP && CONFIG_BT_AUDIO_BROADCAST_SOURCE) */

struct bst_test_list *test_broadcast_source_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_BAP && CONFIG_BT_AUDIO_BROADCAST_SOURCE */
