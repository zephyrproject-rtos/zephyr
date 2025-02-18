/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stddef.h>
#include <stdint.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/tmap.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>

#include "bstests.h"
#include "common.h"

#ifdef CONFIG_BT_TMAP
extern enum bst_result_t bst_result;

static void test_main(void)
{
	int err;
	struct bt_le_ext_adv *ext_adv;

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");
	/* Initialize TMAP */
	err = bt_tmap_register(TMAP_ROLE_SUPPORTED);
	if (err != 0) {
		FAIL("Failed to register TMAP (err %d)\n", err);
		return;
	}
	printk("TMAP initialized. Start advertising...\n");
	setup_connectable_adv(&ext_adv);

	WAIT_FOR_FLAG(flag_connected);
	printk("Connected!\n");

	PASS("TMAP test passed\n");
}

static const struct bst_test_instance test_tmas[] = {
	{
		.test_id = "tmap_server",
		.test_pre_init_f = test_init,
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
