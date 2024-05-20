/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_BT_TMAP

#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr/types.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include <zephyr/bluetooth/audio/tmap.h>

#include "common.h"

extern enum bst_result_t bst_result;

static uint8_t tmap_addata[] = {
	BT_UUID_16_ENCODE(BT_UUID_TMAS_VAL), /* TMAS UUID */
	(BT_TMAP_ROLE_UMR | BT_TMAP_ROLE_CT), 0x00, /* TMAP Role  */
};

static const struct bt_data ad_tmas[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE, 0x09, 0x41), /* Appearance - Earbud  */
	BT_DATA(BT_DATA_SVC_DATA16, tmap_addata, ARRAY_SIZE(tmap_addata)),
};

static void test_main(void)
{
	int err;
	struct bt_le_ext_adv *adv;

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");
	/* Initialize TMAP */
	err = bt_tmap_register(BT_TMAP_ROLE_CT | BT_TMAP_ROLE_UMR);
	if (err != 0) {
		return;
	}
	printk("TMAP initialized. Start advertising...\n");
	/* Create a connectable extended advertising set */
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_CONN, NULL, &adv);
	if (err) {
		printk("Failed to create advertising set (err %d)\n", err);
		return;
	}

	err = bt_le_ext_adv_set_data(adv, ad_tmas, ARRAY_SIZE(ad_tmas), NULL, 0);
	if (err) {
		printk("Failed to set advertising data (err %d)\n", err);
		return;
	}

	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err) {
		printk("Failed to start advertising set (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
	WAIT_FOR_FLAG(flag_connected);
	printk("Connected!\n");

	PASS("TMAP test passed\n");
}

static const struct bst_test_instance test_tmas[] = {
	{
		.test_id = "tmap_server",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main,

	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_tmap_server_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_tmas);
}
#else
struct bst_test_list *test_tmap_server_install(struct bst_test_list *tests)
{
	return tests;
}
#endif /* CONFIG_BT_TMAP */
