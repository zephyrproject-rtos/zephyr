/*
 * Copyright (c) 2020-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_BT_MICS
#include <zephyr/bluetooth/audio/mics.h>
#include "common.h"

extern enum bst_result_t bst_result;

#if defined(CONFIG_BT_AICS)
#define AICS_DESC_SIZE CONFIG_BT_AICS_MAX_INPUT_DESCRIPTION_SIZE
#else
#define AICS_DESC_SIZE 0
#endif /* CONFIG_BT_AICS */

static struct bt_mics *mics;
static struct bt_mics_included mics_included;

static volatile uint8_t g_mute;
static volatile int8_t g_aics_gain;
static volatile uint8_t g_aics_input_mute;
static volatile uint8_t g_aics_mode;
static volatile uint8_t g_aics_input_type;
static volatile uint8_t g_aics_units;
static volatile uint8_t g_aics_gain_max;
static volatile uint8_t g_aics_gain_min;
static volatile bool g_aics_active = true;
static char g_aics_desc[AICS_DESC_SIZE];
static volatile bool g_cb;
static bool g_is_connected;

static void mics_mute_cb(struct bt_mics *mics, int err, uint8_t mute)
{
	if (err != 0) {
		FAIL("MICS mute cb err (%d)", err);
		return;
	}

	g_mute = mute;
	g_cb = true;
}

static void aics_state_cb(struct bt_aics *inst, int err, int8_t gain,
			  uint8_t mute, uint8_t mode)
{
	if (err != 0) {
		FAIL("AICS state cb err (%d)", err);
		return;
	}

	g_aics_gain = gain;
	g_aics_input_mute = mute;
	g_aics_mode = mode;
	g_cb = true;
}

static void aics_gain_setting_cb(struct bt_aics *inst, int err, uint8_t units,
				 int8_t minimum, int8_t maximum)
{
	if (err != 0) {
		FAIL("AICS gain setting cb err (%d)", err);
		return;
	}

	g_aics_units = units;
	g_aics_gain_min = minimum;
	g_aics_gain_max = maximum;
	g_cb = true;
}

static void aics_input_type_cb(struct bt_aics *inst, int err,
			       uint8_t input_type)
{
	if (err != 0) {
		FAIL("AICS input type cb err (%d)", err);
		return;
	}

	g_aics_input_type = input_type;
	g_cb = true;
}

static void aics_status_cb(struct bt_aics *inst, int err, bool active)
{
	if (err != 0) {
		FAIL("AICS status cb err (%d)", err);
		return;
	}

	g_aics_active = active;
	g_cb = true;
}

static void aics_description_cb(struct bt_aics *inst, int err,
				char *description)
{
	if (err != 0) {
		FAIL("AICS description cb err (%d)", err);
		return;
	}


	strncpy(g_aics_desc, description, sizeof(g_aics_desc) - 1);
	g_aics_desc[sizeof(g_aics_desc) - 1] = '\0';
	g_cb = true;
}

static struct bt_mics_cb mics_cb = {
	.mute = mics_mute_cb,
};

static struct bt_aics_cb aics_cb = {
	.state = aics_state_cb,
	.gain_setting = aics_gain_setting_cb,
	.type = aics_input_type_cb,
	.status = aics_status_cb,
	.description = aics_description_cb
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err != 0) {
		FAIL("Failed to connect to %s (%u)\n", addr, err);
		return;
	}

	printk("Connected to %s\n", addr);
	default_conn = bt_conn_ref(conn);
	g_is_connected = true;
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static int test_aics_server_only(void)
{
	int err;
	int8_t expected_gain;
	uint8_t expected_input_mute;
	uint8_t expected_mode;
	uint8_t expected_input_type;
	bool expected_aics_active;
	char expected_aics_desc[AICS_DESC_SIZE];

	printk("Deactivating AICS\n");
	expected_aics_active = false;
	err = bt_mics_aics_deactivate(mics, mics_included.aics[0]);
	if (err != 0) {
		FAIL("Could not deactivate AICS (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(expected_aics_active == g_aics_active);
	printk("AICS deactivated\n");

	printk("Activating AICS\n");
	expected_aics_active = true;
	err = bt_mics_aics_activate(mics, mics_included.aics[0]);
	if (err != 0) {
		FAIL("Could not activate AICS (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(expected_aics_active == g_aics_active);
	printk("AICS activated\n");

	printk("Getting AICS state\n");
	g_cb = false;
	err = bt_mics_aics_state_get(mics, mics_included.aics[0]);
	if (err != 0) {
		FAIL("Could not get AICS state (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(g_cb);
	printk("AICS state get\n");

	printk("Getting AICS gain setting\n");
	g_cb = false;
	err = bt_mics_aics_gain_setting_get(mics, mics_included.aics[0]);
	if (err != 0) {
		FAIL("Could not get AICS gain setting (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(g_cb);
	printk("AICS gain setting get\n");

	printk("Getting AICS input type\n");
	g_cb = false;
	expected_input_type = BT_AICS_INPUT_TYPE_DIGITAL;
	err = bt_mics_aics_type_get(mics, mics_included.aics[0]);
	if (err != 0) {
		FAIL("Could not get AICS input type (err %d)\n", err);
		return err;
	}
	/* Expect and wait for input_type from init */
	WAIT_FOR_COND(g_cb && expected_input_type == g_aics_input_type);
	printk("AICS input type get\n");

	printk("Getting AICS status\n");
	g_cb = false;
	err = bt_mics_aics_status_get(mics, mics_included.aics[0]);
	if (err != 0) {
		FAIL("Could not get AICS status (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(g_cb);
	printk("AICS status get\n");

	printk("Getting AICS description\n");
	g_cb = false;
	err = bt_mics_aics_description_get(mics, mics_included.aics[0]);
	if (err != 0) {
		FAIL("Could not get AICS description (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(g_cb);
	printk("AICS description get\n");

	printk("Setting AICS mute\n");
	g_cb = false;
	expected_input_mute = BT_AICS_STATE_MUTED;
	err = bt_mics_aics_mute(mics, mics_included.aics[0]);
	if (err != 0) {
		FAIL("Could not set AICS mute (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(g_cb && expected_input_mute == g_aics_input_mute);
	printk("AICS mute set\n");

	printk("Setting AICS unmute\n");
	g_cb = false;
	expected_input_mute = BT_AICS_STATE_UNMUTED;
	err = bt_mics_aics_unmute(mics, mics_included.aics[0]);
	if (err != 0) {
		FAIL("Could not set AICS unmute (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(g_cb && expected_input_mute == g_aics_input_mute);
	printk("AICS unmute set\n");

	printk("Setting AICS auto mode\n");
	g_cb = false;
	expected_mode = BT_AICS_MODE_AUTO;
	err = bt_mics_aics_automatic_gain_set(mics, mics_included.aics[0]);
	if (err != 0) {
		FAIL("Could not set AICS auto mode (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(g_cb && expected_mode == g_aics_mode);
	printk("AICS auto mode set\n");

	printk("Setting AICS manual mode\n");
	g_cb = false;
	expected_mode = BT_AICS_MODE_MANUAL;
	err = bt_mics_aics_manual_gain_set(mics, mics_included.aics[0]);
	if (err != 0) {
		FAIL("Could not set AICS manual mode (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(g_cb && expected_mode == g_aics_mode);
	printk("AICS manual mode set\n");

	printk("Setting AICS gain\n");
	g_cb = false;
	expected_gain = g_aics_gain_max - 1;
	err = bt_mics_aics_gain_set(mics, mics_included.aics[0], expected_gain);
	if (err != 0) {
		FAIL("Could not set AICS gain (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(g_cb && expected_gain == g_aics_gain);
	printk("AICS gain set\n");

	printk("Setting AICS Description\n");
	g_cb = false;
	strncpy(expected_aics_desc, "New Input Description",
		sizeof(expected_aics_desc));
	expected_aics_desc[sizeof(expected_aics_desc) - 1] = '\0';
	err = bt_mics_aics_description_set(mics, mics_included.aics[0], expected_aics_desc);
	if (err != 0) {
		FAIL("Could not set AICS Description (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(g_cb && !strncmp(expected_aics_desc, g_aics_desc,
				  sizeof(expected_aics_desc)));
	printk("AICS Description set\n");

	return 0;
}

static void test_server_only(void)
{
	int err;
	struct bt_mics_register_param mics_param;
	char input_desc[CONFIG_BT_MICS_AICS_INSTANCE_COUNT][16];
	uint8_t expected_mute;

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	(void)memset(&mics_param, 0, sizeof(mics_param));

	for (int i = 0; i < ARRAY_SIZE(mics_param.aics_param); i++) {
		mics_param.aics_param[i].desc_writable = true;
		snprintf(input_desc[i], sizeof(input_desc[i]), "Input %d", i + 1);
		mics_param.aics_param[i].description = input_desc[i];
		mics_param.aics_param[i].type = BT_AICS_INPUT_TYPE_DIGITAL;
		mics_param.aics_param[i].status = g_aics_active;
		mics_param.aics_param[i].gain_mode = BT_AICS_MODE_MANUAL;
		mics_param.aics_param[i].units = 1;
		mics_param.aics_param[i].min_gain = 0;
		mics_param.aics_param[i].max_gain = 100;
		mics_param.aics_param[i].cb = &aics_cb;
	}
	mics_param.cb = &mics_cb;

	err = bt_mics_register(&mics_param, &mics);
	if (err != 0) {
		FAIL("MICS init failed (err %d)\n", err);
		return;
	}

	err = bt_mics_included_get(mics, &mics_included);
	if (err != 0) {
		FAIL("MICS get failed (err %d)\n", err);
		return;
	}

	printk("MICS initialized\n");

	printk("Getting MICS mute\n");
	g_cb = false;
	err = bt_mics_mute_get(mics);
	if (err != 0) {
		FAIL("Could not get MICS mute (err %d)\n", err);
		return;
	}
	WAIT_FOR_COND(g_cb);
	printk("MICS mute get\n");

	printk("Setting MICS mute\n");
	expected_mute = BT_MICS_MUTE_MUTED;
	err = bt_mics_mute(mics);
	if (err != 0) {
		FAIL("MICS mute failed (err %d)\n", err);
		return;
	}
	WAIT_FOR_COND(expected_mute == g_mute);
	printk("MICS mute set\n");

	printk("Setting MICS unmute\n");
	expected_mute = BT_MICS_MUTE_UNMUTED;
	err = bt_mics_unmute(mics);
	if (err != 0) {
		FAIL("MICS unmute failed (err %d)\n", err);
		return;
	}
	WAIT_FOR_COND(expected_mute == g_mute);
	printk("MICS unmute set\n");

	printk("Setting MICS disable\n");
	expected_mute = BT_MICS_MUTE_DISABLED;
	err = bt_mics_mute_disable(mics);
	if (err != 0) {
		FAIL("MICS disable failed (err %d)\n", err);
		return;
	}
	WAIT_FOR_COND(expected_mute == g_mute);
	printk("MICS disable set\n");

	if (CONFIG_BT_MICS_AICS_INSTANCE_COUNT > 0) {
		if (test_aics_server_only()) {
			return;
		}
	}

	PASS("MICS passed\n");
}

static void test_main(void)
{
	int err;
	struct bt_mics_register_param mics_param;
	char input_desc[CONFIG_BT_MICS_AICS_INSTANCE_COUNT][16];

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	(void)memset(&mics_param, 0, sizeof(mics_param));

	for (int i = 0; i < ARRAY_SIZE(mics_param.aics_param); i++) {
		mics_param.aics_param[i].desc_writable = true;
		snprintf(input_desc[i], sizeof(input_desc[i]),
			 "Input %d", i + 1);
		mics_param.aics_param[i].description = input_desc[i];
		mics_param.aics_param[i].type = BT_AICS_INPUT_TYPE_UNSPECIFIED;
		mics_param.aics_param[i].status = g_aics_active;
		mics_param.aics_param[i].gain_mode = BT_AICS_MODE_MANUAL;
		mics_param.aics_param[i].units = 1;
		mics_param.aics_param[i].min_gain = 0;
		mics_param.aics_param[i].max_gain = 100;
		mics_param.aics_param[i].cb = &aics_cb;
	}
	mics_param.cb = &mics_cb;

	err = bt_mics_register(&mics_param, &mics);
	if (err != 0) {
		FAIL("MICS init failed (err %d)\n", err);
		return;
	}

	err = bt_mics_included_get(mics, &mics_included);
	if (err != 0) {
		FAIL("MICS get failed (err %d)\n", err);
		return;
	}

	printk("MICS initialized\n");

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, AD_SIZE, NULL, 0);
	if (err != 0) {
		FAIL("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");

	WAIT_FOR_COND(g_is_connected);

	PASS("MICS passed\n");
}

static const struct bst_test_instance test_mics[] = {
	{
		.test_id = "mics_server_only",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_server_only
	},
	{
		.test_id = "mics",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_mics_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_mics);
}
#else
struct bst_test_list *test_mics_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_MICS */
