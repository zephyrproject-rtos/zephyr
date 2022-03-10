/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_BT_HAS
#include "bluetooth/audio/has.h"
#include "common.h"

extern enum bst_result_t bst_result;

static volatile bool g_is_connected;

static struct bt_conn *g_conn;
static struct bt_has *g_has;

static const uint8_t g_universal_idx = 1;
static const uint8_t g_outdoor_idx = 5;
static const uint8_t g_noisy_idx = 8;
static const uint8_t g_office_idx = 22;

static int set_active_preset(struct bt_has *has, uint8_t index, bool sync)
{
	int err;

	printk("Set active preset index 0x%02x sync %d\n", index, sync);

	/* Dummy set the active preset */
	err = bt_has_preset_active_set(has, index);
	if (err < 0) {
		printk("Set active failed (err %d)", err);
	}

	return err;
}

struct bt_has_ops preset_ops = {
	.active_set = set_active_preset,
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		FAIL("Failed to connect to %s (err %u)\n", addr, err);
		return;
	}
	printk("Connected to %s\n", addr);

	g_conn = conn;
	g_is_connected = true;
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static void test_standalone(void)
{
	int err;
	struct bt_has_register_param register_param = {
		.preset_param = {
			{
				.index = g_universal_idx,
				.properties = BT_HAS_PROP_WRITABLE | BT_HAS_PROP_AVAILABLE,
				.name = "Universal",
			},
		},
		.ops = &preset_ops,
	};

	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	err = bt_has_register(&register_param, &g_has);
	if (err) {
		FAIL("HAS register failed (err %d)\n", err);
		return;
	}

	PASS("HAS standalone passed\n");
}

static void test_main(void)
{
	int err;
	struct bt_has_register_param register_param = {
		.preset_param = {
			{
				.index = g_universal_idx,
				.properties = BT_HAS_PROP_WRITABLE | BT_HAS_PROP_AVAILABLE,
				.name = "Universal",
			},
			{
				.index = g_outdoor_idx,
				.properties = BT_HAS_PROP_WRITABLE | BT_HAS_PROP_AVAILABLE,
				.name = "Outdoor",
			},
			{
				.index = g_noisy_idx,
				.properties = BT_HAS_PROP_WRITABLE | BT_HAS_PROP_AVAILABLE,
				.name = "Noisy environment",
			},
			{
				.index = g_office_idx,
				.properties = BT_HAS_PROP_WRITABLE | BT_HAS_PROP_AVAILABLE,
				.name = "Office",
			},
		},
		.ops = &preset_ops,
	};

	err = bt_enable(NULL);

	if (err) {
		FAIL("Bluetooth discover failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	err = bt_has_register(&register_param, &g_has);
	if (err) {
		FAIL("HAS register failed (err %d)\n", err);
		return;
	}

	printk("HAS initialized\n");

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, AD_SIZE, NULL, 0);
	if (err) {
		FAIL("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");

	PASS("HAS passed\n");
}

static const struct bst_test_instance test_has[] = {
	{
		.test_id = "has_standalone",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_standalone,
	},
	{
		.test_id = "has",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main,
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_has_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_has);
}
#else
struct bst_test_list *test_has_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_HAS */
