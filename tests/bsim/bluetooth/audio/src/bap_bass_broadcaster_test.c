/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/byteorder.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include "bstests.h"
#include "common.h"

extern enum bst_result_t bst_result;

/* TODO: Deprecate in favor of broadcast_source_test */

static void test_main(void)
{
	int err;
	uint32_t broadcast_id = 1234;
	struct bt_le_ext_adv *adv;
	struct bt_data ad[2] = {
		BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR),
		BT_DATA_BYTES(BT_DATA_SVC_DATA16, BT_UUID_16_ENCODE(BT_UUID_BROADCAST_AUDIO_VAL),
			      BT_BYTES_LIST_LE24(broadcast_id)),
	};

	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	setup_broadcast_adv(&adv);

	/* Set adv data */
	err = bt_le_ext_adv_set_data(adv, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		FAIL("Failed to set advertising data (err %d)\n", err);
		return;
	}

	/* Enable Periodic Advertising */
	err = bt_le_per_adv_start(adv);
	if (err) {
		FAIL("Failed to enable periodic advertising (err %d)\n", err);
		return;
	}

	/* Start extended advertising */
	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err) {
		FAIL("Failed to start extended advertising (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");

	k_sleep(K_SECONDS(10));

	PASS("BASS broadcaster passed\n");
}

static const struct bst_test_instance test_bass_broadcaster[] = {
	{
		.test_id = "bass_broadcaster",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_bass_broadcaster_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_bass_broadcaster);
}
