/*
 * Copyright (c) 2020-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/aics.h>
#include <zephyr/bluetooth/audio/micp.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>

#include "bstests.h"
#include "common.h"

#ifdef CONFIG_BT_MICP_MIC_DEV
extern enum bst_result_t bst_result;

#if defined(CONFIG_BT_AICS)
#define AICS_DESC_SIZE CONFIG_BT_AICS_MAX_INPUT_DESCRIPTION_SIZE
#else
#define AICS_DESC_SIZE 0
#endif /* CONFIG_BT_AICS */

static struct bt_micp_included micp_included;

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

static void micp_mute_cb(uint8_t mute)
{
	g_mute = mute;
	g_cb = true;
}

static struct bt_micp_mic_dev_cb micp_cb = {
	.mute = micp_mute_cb,
};

#if defined(CONFIG_BT_MICP_MIC_DEV_AICS)
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

static struct bt_aics_cb aics_cb = {
	.state = aics_state_cb,
	.gain_setting = aics_gain_setting_cb,
	.type = aics_input_type_cb,
	.status = aics_status_cb,
	.description = aics_description_cb
};
#endif /* CONFIG_BT_MICP_MIC_DEV_AICS */

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
	err = bt_aics_deactivate(micp_included.aics[0]);
	if (err != 0) {
		FAIL("Could not deactivate AICS (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(expected_aics_active == g_aics_active);
	printk("AICS deactivated\n");

	printk("Activating AICS\n");
	expected_aics_active = true;
	err = bt_aics_activate(micp_included.aics[0]);
	if (err != 0) {
		FAIL("Could not activate AICS (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(expected_aics_active == g_aics_active);
	printk("AICS activated\n");

	printk("Getting AICS state\n");
	g_cb = false;
	err = bt_aics_state_get(micp_included.aics[0]);
	if (err != 0) {
		FAIL("Could not get AICS state (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(g_cb);
	printk("AICS state get\n");

	printk("Getting AICS gain setting\n");
	g_cb = false;
	err = bt_aics_gain_setting_get(micp_included.aics[0]);
	if (err != 0) {
		FAIL("Could not get AICS gain setting (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(g_cb);
	printk("AICS gain setting get\n");

	printk("Getting AICS input type\n");
	g_cb = false;
	expected_input_type = BT_AICS_INPUT_TYPE_DIGITAL;
	err = bt_aics_type_get(micp_included.aics[0]);
	if (err != 0) {
		FAIL("Could not get AICS input type (err %d)\n", err);
		return err;
	}
	/* Expect and wait for input_type from init */
	WAIT_FOR_COND(g_cb && expected_input_type == g_aics_input_type);
	printk("AICS input type get\n");

	printk("Getting AICS status\n");
	g_cb = false;
	err = bt_aics_status_get(micp_included.aics[0]);
	if (err != 0) {
		FAIL("Could not get AICS status (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(g_cb);
	printk("AICS status get\n");

	printk("Getting AICS description\n");
	g_cb = false;
	err = bt_aics_description_get(micp_included.aics[0]);
	if (err != 0) {
		FAIL("Could not get AICS description (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(g_cb);
	printk("AICS description get\n");

	printk("Setting AICS mute\n");
	g_cb = false;
	expected_input_mute = BT_AICS_STATE_MUTED;
	err = bt_aics_mute(micp_included.aics[0]);
	if (err != 0) {
		FAIL("Could not set AICS mute (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(g_cb && expected_input_mute == g_aics_input_mute);
	printk("AICS mute set\n");

	printk("Setting AICS unmute\n");
	g_cb = false;
	expected_input_mute = BT_AICS_STATE_UNMUTED;
	err = bt_aics_unmute(micp_included.aics[0]);
	if (err != 0) {
		FAIL("Could not set AICS unmute (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(g_cb && expected_input_mute == g_aics_input_mute);
	printk("AICS unmute set\n");

	printk("Setting AICS auto mode\n");
	g_cb = false;
	expected_mode = BT_AICS_MODE_AUTO;
	err = bt_aics_automatic_gain_set(micp_included.aics[0]);
	if (err != 0) {
		FAIL("Could not set AICS auto mode (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(g_cb && expected_mode == g_aics_mode);
	printk("AICS auto mode set\n");

	printk("Setting AICS manual mode\n");
	g_cb = false;
	expected_mode = BT_AICS_MODE_MANUAL;
	err = bt_aics_manual_gain_set(micp_included.aics[0]);
	if (err != 0) {
		FAIL("Could not set AICS manual mode (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(g_cb && expected_mode == g_aics_mode);
	printk("AICS manual mode set\n");

	printk("Setting AICS gain\n");
	g_cb = false;
	expected_gain = g_aics_gain_max - 1;
	err = bt_aics_gain_set(micp_included.aics[0], expected_gain);
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
	err = bt_aics_description_set(micp_included.aics[0], expected_aics_desc);
	if (err != 0) {
		FAIL("Could not set AICS Description (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(g_cb && !strncmp(expected_aics_desc, g_aics_desc,
				  sizeof(expected_aics_desc)));
	printk("AICS Description set\n");

	return 0;
}

static void test_mic_dev_only(void)
{
	int err;
	struct bt_micp_mic_dev_register_param micp_param;
	uint8_t expected_mute;

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	(void)memset(&micp_param, 0, sizeof(micp_param));

#if defined(CONFIG_BT_MICP_MIC_DEV_AICS)
	char input_desc[CONFIG_BT_MICP_MIC_DEV_AICS_INSTANCE_COUNT][16];

	for (int i = 0; i < ARRAY_SIZE(micp_param.aics_param); i++) {
		micp_param.aics_param[i].desc_writable = true;
		snprintf(input_desc[i], sizeof(input_desc[i]), "Input %d", i + 1);
		micp_param.aics_param[i].description = input_desc[i];
		micp_param.aics_param[i].type = BT_AICS_INPUT_TYPE_DIGITAL;
		micp_param.aics_param[i].status = g_aics_active;
		micp_param.aics_param[i].gain_mode = BT_AICS_MODE_MANUAL;
		micp_param.aics_param[i].units = 1;
		micp_param.aics_param[i].min_gain = 0;
		micp_param.aics_param[i].max_gain = 100;
		micp_param.aics_param[i].cb = &aics_cb;
	}
#endif /* CONFIG_BT_MICP_MIC_DEV_AICS */

	micp_param.cb = &micp_cb;

	err = bt_micp_mic_dev_register(&micp_param);
	if (err != 0) {
		FAIL("MICP init failed (err %d)\n", err);
		return;
	}

	if (IS_ENABLED(CONFIG_BT_MICP_MIC_DEV_AICS)) {
		err = bt_micp_mic_dev_included_get(&micp_included);
		if (err != 0) {
			FAIL("MICP get failed (err %d)\n", err);
			return;
		}
	}

	printk("MICP initialized\n");

	printk("Getting MICP mute\n");
	g_cb = false;
	err = bt_micp_mic_dev_mute_get();
	if (err != 0) {
		FAIL("Could not get MICP mute (err %d)\n", err);
		return;
	}
	WAIT_FOR_COND(g_cb);
	printk("MICP mute get\n");

	printk("Setting MICP mute\n");
	expected_mute = BT_MICP_MUTE_MUTED;
	err = bt_micp_mic_dev_mute();
	if (err != 0) {
		FAIL("MICP mute failed (err %d)\n", err);
		return;
	}
	WAIT_FOR_COND(expected_mute == g_mute);
	printk("MICP mute set\n");

	printk("Setting MICP unmute\n");
	expected_mute = BT_MICP_MUTE_UNMUTED;
	err = bt_micp_mic_dev_unmute();
	if (err != 0) {
		FAIL("MICP unmute failed (err %d)\n", err);
		return;
	}
	WAIT_FOR_COND(expected_mute == g_mute);
	printk("MICP unmute set\n");

	printk("Setting MICP disable\n");
	expected_mute = BT_MICP_MUTE_DISABLED;
	err = bt_micp_mic_dev_mute_disable();
	if (err != 0) {
		FAIL("MICP disable failed (err %d)\n", err);
		return;
	}
	WAIT_FOR_COND(expected_mute == g_mute);
	printk("MICP disable set\n");

	if (CONFIG_BT_MICP_MIC_DEV_AICS_INSTANCE_COUNT > 0) {
		if (test_aics_server_only()) {
			return;
		}
	}

	PASS("MICP mic_dev passed\n");
}

static void test_main(void)
{
	int err;
	struct bt_micp_mic_dev_register_param micp_param;

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	(void)memset(&micp_param, 0, sizeof(micp_param));

#if defined(CONFIG_BT_MICP_MIC_DEV_AICS)
	char input_desc[CONFIG_BT_MICP_MIC_DEV_AICS_INSTANCE_COUNT][16];

	for (int i = 0; i < ARRAY_SIZE(micp_param.aics_param); i++) {
		micp_param.aics_param[i].desc_writable = true;
		snprintf(input_desc[i], sizeof(input_desc[i]),
			 "Input %d", i + 1);
		micp_param.aics_param[i].description = input_desc[i];
		micp_param.aics_param[i].type = BT_AICS_INPUT_TYPE_UNSPECIFIED;
		micp_param.aics_param[i].status = g_aics_active;
		micp_param.aics_param[i].gain_mode = BT_AICS_MODE_MANUAL;
		micp_param.aics_param[i].units = 1;
		micp_param.aics_param[i].min_gain = 0;
		micp_param.aics_param[i].max_gain = 100;
		micp_param.aics_param[i].cb = &aics_cb;
	}
#endif /* CONFIG_BT_MICP_MIC_DEV_AICS */

	micp_param.cb = &micp_cb;

	err = bt_micp_mic_dev_register(&micp_param);
	if (err != 0) {
		FAIL("MICP init failed (err %d)\n", err);
		return;
	}

	if (IS_ENABLED(CONFIG_BT_MICP_MIC_DEV_AICS)) {
		err = bt_micp_mic_dev_included_get(&micp_included);
		if (err != 0) {
			FAIL("MICP get failed (err %d)\n", err);
			return;
		}
	}

	printk("MICP initialized\n");

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, AD_SIZE, NULL, 0);
	if (err != 0) {
		FAIL("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");

	WAIT_FOR_FLAG(flag_connected);

	PASS("MICP mic_dev passed\n");
}

static const struct bst_test_instance test_micp[] = {
	{
		.test_id = "micp_mic_dev_only",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_mic_dev_only
	},
	{
		.test_id = "micp_mic_dev",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_micp_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_micp);
}
#else
struct bst_test_list *test_micp_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_MICP_MIC_DEV */
