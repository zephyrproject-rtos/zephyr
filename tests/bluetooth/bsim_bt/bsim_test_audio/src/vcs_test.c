/*
 * Copyright (c) 2019 Bose Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_BT_VCS
#include <zephyr/bluetooth/audio/vcs.h>
#include "common.h"

extern enum bst_result_t bst_result;

#if defined(CONFIG_BT_VOCS)
#define VOCS_DESC_SIZE CONFIG_BT_VOCS_MAX_OUTPUT_DESCRIPTION_SIZE
#else
#define VOCS_DESC_SIZE 0
#endif /* CONFIG_BT_VOCS */

#if defined(CONFIG_BT_AICS)
#define AICS_DESC_SIZE CONFIG_BT_AICS_MAX_INPUT_DESCRIPTION_SIZE
#else
#define AICS_DESC_SIZE 0
#endif /* CONFIG_BT_AICS */

static struct bt_vcs *vcs;
static struct bt_vcs_included vcs_included;

static volatile uint8_t g_volume;
static volatile uint8_t g_mute;
static volatile uint8_t g_flags;
static volatile int16_t g_vocs_offset;
static volatile uint8_t g_vocs_location;
static char g_vocs_desc[VOCS_DESC_SIZE];
static volatile int8_t g_aics_gain;
static volatile uint8_t g_aics_input_mute;
static volatile uint8_t g_aics_mode;
static volatile uint8_t g_aics_input_type;
static volatile uint8_t g_aics_units;
static volatile uint8_t g_aics_gain_max;
static volatile uint8_t g_aics_gain_min;
static volatile bool g_aics_active = 1;
static char g_aics_desc[AICS_DESC_SIZE];
static volatile bool g_cb;
static bool g_is_connected;

static void vcs_state_cb(struct bt_vcs *vcs, int err, uint8_t volume,
			 uint8_t mute)
{
	if (err) {
		FAIL("VCS state cb err (%d)", err);
		return;
	}

	g_volume = volume;
	g_mute = mute;
	g_cb = true;
}

static void vcs_flags_cb(struct bt_vcs *vcs, int err, uint8_t flags)
{
	if (err) {
		FAIL("VCS flags cb err (%d)", err);
		return;
	}

	g_flags = flags;
	g_cb = true;
}

static void vocs_state_cb(struct bt_vocs *inst, int err, int16_t offset)
{
	if (err) {
		FAIL("VOCS state cb err (%d)", err);
		return;
	}

	g_vocs_offset = offset;
	g_cb = true;
}

static void vocs_location_cb(struct bt_vocs *inst, int err, uint32_t location)
{
	if (err) {
		FAIL("VOCS location cb err (%d)", err);
		return;
	}

	g_vocs_location = location;
	g_cb = true;
}

static void vocs_description_cb(struct bt_vocs *inst, int err,
				char *description)
{
	if (err) {
		FAIL("VOCS description cb err (%d)", err);
		return;
	}

	strncpy(g_vocs_desc, description, sizeof(g_vocs_desc) - 1);
	g_vocs_desc[sizeof(g_vocs_desc) - 1] = '\0';
	g_cb = true;
}

static void aics_state_cb(struct bt_aics *inst, int err, int8_t gain,
			  uint8_t mute, uint8_t mode)
{
	if (err) {
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
	if (err) {
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
	if (err) {
		FAIL("AICS input type cb err (%d)", err);
		return;
	}

	g_aics_input_type = input_type;
	g_cb = true;
}

static void aics_status_cb(struct bt_aics *inst, int err, bool active)
{
	if (err) {
		FAIL("AICS status cb err (%d)", err);
		return;
	}

	g_aics_active = active;
	g_cb = true;
}

static void aics_description_cb(struct bt_aics *inst, int err,
				char *description)
{
	if (err) {
		FAIL("AICS description cb err (%d)", err);
		return;
	}

	strncpy(g_aics_desc, description, sizeof(g_aics_desc) - 1);
	g_aics_desc[sizeof(g_aics_desc) - 1] = '\0';
	g_cb = true;
}

static struct bt_vcs_cb vcs_cb = {
	.state = vcs_state_cb,
	.flags = vcs_flags_cb,
};

static struct bt_vocs_cb vocs_cb = {
	.state = vocs_state_cb,
	.location = vocs_location_cb,
	.description = vocs_description_cb
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

	if (err) {
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

static int test_aics_standalone(void)
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
	err = bt_vcs_aics_deactivate(vcs, vcs_included.aics[0]);
	if (err) {
		FAIL("Could not deactivate AICS (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(expected_aics_active == g_aics_active);
	printk("AICS deactivated\n");

	printk("Activating AICS\n");
	expected_aics_active = true;
	err = bt_vcs_aics_activate(vcs, vcs_included.aics[0]);
	if (err) {
		FAIL("Could not activate AICS (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(expected_aics_active == g_aics_active);
	printk("AICS activated\n");

	printk("Getting AICS state\n");
	g_cb = false;
	err = bt_vcs_aics_state_get(vcs, vcs_included.aics[0]);
	if (err) {
		FAIL("Could not get AICS state (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(g_cb);
	printk("AICS state get\n");

	printk("Getting AICS gain setting\n");
	g_cb = false;
	err = bt_vcs_aics_gain_setting_get(vcs, vcs_included.aics[0]);
	if (err) {
		FAIL("Could not get AICS gain setting (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(g_cb);
	printk("AICS gain setting get\n");

	printk("Getting AICS input type\n");
	expected_input_type = BT_AICS_INPUT_TYPE_DIGITAL;
	err = bt_vcs_aics_type_get(vcs, vcs_included.aics[0]);
	if (err) {
		FAIL("Could not get AICS input type (err %d)\n", err);
		return err;
	}
	/* Expect and wait for input_type from init */
	WAIT_FOR_COND(expected_input_type == g_aics_input_type);
	printk("AICS input type get\n");

	printk("Getting AICS status\n");
	g_cb = false;
	err = bt_vcs_aics_status_get(vcs, vcs_included.aics[0]);
	if (err) {
		FAIL("Could not get AICS status (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(g_cb);
	printk("AICS status get\n");

	printk("Getting AICS description\n");
	g_cb = false;
	err = bt_vcs_aics_description_get(vcs, vcs_included.aics[0]);
	if (err) {
		FAIL("Could not get AICS description (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(g_cb);
	printk("AICS description get\n");

	printk("Setting AICS mute\n");
	expected_input_mute = BT_AICS_STATE_MUTED;
	err = bt_vcs_aics_mute(vcs, vcs_included.aics[0]);
	if (err) {
		FAIL("Could not set AICS mute (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(expected_input_mute == g_aics_input_mute);
	printk("AICS mute set\n");

	printk("Setting AICS unmute\n");
	expected_input_mute = BT_AICS_STATE_UNMUTED;
	err = bt_vcs_aics_unmute(vcs, vcs_included.aics[0]);
	if (err) {
		FAIL("Could not set AICS unmute (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(expected_input_mute == g_aics_input_mute);
	printk("AICS unmute set\n");

	printk("Setting AICS auto mode\n");
	expected_mode = BT_AICS_MODE_AUTO;
	err = bt_vcs_aics_automatic_gain_set(vcs, vcs_included.aics[0]);
	if (err) {
		FAIL("Could not set AICS auto mode (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(expected_mode == g_aics_mode);
	printk("AICS auto mode set\n");

	printk("Setting AICS manual mode\n");
	expected_mode = BT_AICS_MODE_MANUAL;
	err = bt_vcs_aics_manual_gain_set(vcs, vcs_included.aics[0]);
	if (err) {
		FAIL("Could not set AICS manual mode (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(expected_mode == g_aics_mode);
	printk("AICS manual mode set\n");

	printk("Setting AICS gain\n");
	expected_gain = g_aics_gain_max - 1;
	err = bt_vcs_aics_gain_set(vcs, vcs_included.aics[0], expected_gain);
	if (err) {
		FAIL("Could not set AICS gain (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(expected_gain == g_aics_gain);
	printk("AICS gain set\n");

	printk("Setting AICS Description\n");
	strncpy(expected_aics_desc, "New Input Description",
		sizeof(expected_aics_desc));
	expected_aics_desc[sizeof(expected_aics_desc) - 1] = '\0';
	g_cb = false;
	err = bt_vcs_aics_description_set(vcs, vcs_included.aics[0],
					  expected_aics_desc);
	if (err) {
		FAIL("Could not set AICS Description (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(g_cb && !strncmp(expected_aics_desc, g_aics_desc,
				  sizeof(expected_aics_desc)));
	printk("AICS Description set\n");

	return 0;
}

static int test_vocs_standalone(void)
{
	int err;
	uint8_t expected_location;
	int16_t expected_offset;
	char expected_description[VOCS_DESC_SIZE];

	printk("Getting VOCS state\n");
	g_cb = false;
	err = bt_vcs_vocs_state_get(vcs, vcs_included.vocs[0]);
	if (err) {
		FAIL("Could not get VOCS state (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(g_cb);
	printk("VOCS state get\n");

	printk("Getting VOCS location\n");
	g_cb = false;
	err = bt_vcs_vocs_location_get(vcs, vcs_included.vocs[0]);
	if (err) {
		FAIL("Could not get VOCS location (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(g_cb);
	printk("VOCS location get\n");

	printk("Getting VOCS description\n");
	g_cb = false;
	err = bt_vcs_vocs_description_get(vcs, vcs_included.vocs[0]);
	if (err) {
		FAIL("Could not get VOCS description (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(g_cb);
	printk("VOCS description get\n");

	printk("Setting VOCS location\n");
	expected_location = g_vocs_location + 1;
	err = bt_vcs_vocs_location_set(vcs, vcs_included.vocs[0],
				       expected_location);
	if (err) {
		FAIL("Could not set VOCS location (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(expected_location == g_vocs_location);
	printk("VOCS location set\n");

	printk("Setting VOCS state\n");
	expected_offset = g_vocs_offset + 1;
	err = bt_vcs_vocs_state_set(vcs, vcs_included.vocs[0], expected_offset);
	if (err) {
		FAIL("Could not set VOCS state (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(expected_offset == g_vocs_offset);
	printk("VOCS state set\n");

	printk("Setting VOCS description\n");
	strncpy(expected_description, "New Output Description",
		sizeof(expected_description) - 1);
	expected_description[sizeof(expected_description) - 1] = '\0';
	g_cb = false;
	err = bt_vcs_vocs_description_set(vcs, vcs_included.vocs[0],
					  expected_description);
	if (err) {
		FAIL("Could not set VOCS description (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(g_cb && !strncmp(expected_description, g_vocs_desc,
				  sizeof(expected_description)));
	printk("VOCS description set\n");

	return err;
}

static void test_standalone(void)
{
	int err;
	struct bt_vcs_register_param vcs_param;
	char input_desc[CONFIG_BT_VCS_AICS_INSTANCE_COUNT][16];
	char output_desc[CONFIG_BT_VCS_VOCS_INSTANCE_COUNT][16];
	const uint8_t volume_step = 5;
	uint8_t expected_volume;
	uint8_t expected_mute;

	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	memset(&vcs_param, 0, sizeof(vcs_param));

	for (int i = 0; i < ARRAY_SIZE(vcs_param.vocs_param); i++) {
		vcs_param.vocs_param[i].location_writable = true;
		vcs_param.vocs_param[i].desc_writable = true;
		snprintf(output_desc[i], sizeof(output_desc[i]),
			 "Output %d", i + 1);
		vcs_param.vocs_param[i].output_desc = output_desc[i];
		vcs_param.vocs_param[i].cb = &vocs_cb;
	}

	for (int i = 0; i < ARRAY_SIZE(vcs_param.aics_param); i++) {
		vcs_param.aics_param[i].desc_writable = true;
		snprintf(input_desc[i], sizeof(input_desc[i]),
			 "Input %d", i + 1);
		vcs_param.aics_param[i].description = input_desc[i];
		vcs_param.aics_param[i].type = BT_AICS_INPUT_TYPE_DIGITAL;
		vcs_param.aics_param[i].status = g_aics_active;
		vcs_param.aics_param[i].gain_mode = BT_AICS_MODE_MANUAL;
		vcs_param.aics_param[i].units = 1;
		vcs_param.aics_param[i].min_gain = 0;
		vcs_param.aics_param[i].max_gain = 100;
		vcs_param.aics_param[i].cb = &aics_cb;
	}

	vcs_param.step = 1;
	vcs_param.mute = BT_VCS_STATE_UNMUTED;
	vcs_param.volume = 100;
	vcs_param.cb = &vcs_cb;

	err = bt_vcs_register(&vcs_param, &vcs);
	if (err) {
		FAIL("VCS register failed (err %d)\n", err);
		return;
	}

	err = bt_vcs_included_get(vcs, &vcs_included);
	if (err) {
		FAIL("VCS included get failed (err %d)\n", err);
		return;
	}

	printk("VCS initialized\n");


	printk("Setting VCS step\n");
	err = bt_vcs_vol_step_set(volume_step);
	if (err) {
		FAIL("VCS step set failed (err %d)\n", err);
		return;
	}
	printk("VCS step set\n");

	printk("Getting VCS volume state\n");
	g_cb = false;
	err = bt_vcs_vol_get(vcs);
	if (err) {
		FAIL("Could not get VCS volume (err %d)\n", err);
		return;
	}
	WAIT_FOR_COND(g_cb);
	printk("VCS volume get\n");

	printk("Getting VCS flags\n");
	g_cb = false;
	err = bt_vcs_flags_get(vcs);
	if (err) {
		FAIL("Could not get VCS flags (err %d)\n", err);
		return;
	}
	WAIT_FOR_COND(g_cb);
	printk("VCS flags get\n");

	printk("Downing VCS volume\n");
	expected_volume = g_volume - volume_step;
	err = bt_vcs_vol_down(vcs);
	if (err) {
		FAIL("Could not get down VCS volume (err %d)\n", err);
		return;
	}
	WAIT_FOR_COND(expected_volume == g_volume || g_volume == 0);
	printk("VCS volume downed\n");

	printk("Upping VCS volume\n");
	expected_volume = g_volume + volume_step;
	err = bt_vcs_vol_up(vcs);
	if (err) {
		FAIL("Could not up VCS volume (err %d)\n", err);
		return;
	}
	WAIT_FOR_COND(expected_volume == g_volume || g_volume == UINT8_MAX);
	printk("VCS volume upped\n");

	printk("Muting VCS\n");
	expected_mute = 1;
	err = bt_vcs_mute(vcs);
	if (err) {
		FAIL("Could not mute VCS (err %d)\n", err);
		return;
	}
	WAIT_FOR_COND(expected_mute == g_mute);
	printk("VCS muted\n");

	printk("Downing and unmuting VCS\n");
	expected_volume = g_volume - volume_step;
	expected_mute = 0;
	err = bt_vcs_unmute_vol_down(vcs);
	if (err) {
		FAIL("Could not down and unmute VCS (err %d)\n", err);
		return;
	}
	WAIT_FOR_COND((expected_volume == g_volume || g_volume == 0) &&
		 expected_mute == g_mute);
	printk("VCS volume downed and unmuted\n");

	printk("Muting VCS\n");
	expected_mute = 1;
	err = bt_vcs_mute(vcs);
	if (err) {
		FAIL("Could not mute VCS (err %d)\n", err);
		return;
	}
	WAIT_FOR_COND(expected_mute == g_mute);
	printk("VCS muted\n");

	printk("Upping and unmuting VCS\n");
	expected_volume = g_volume + volume_step;
	expected_mute = 0;
	err = bt_vcs_unmute_vol_up(vcs);
	if (err) {
		FAIL("Could not up and unmute VCS (err %d)\n", err);
		return;
	}
	WAIT_FOR_COND((expected_volume == g_volume  || g_volume == UINT8_MAX) &&
		 expected_mute == g_mute);
	printk("VCS volume upped and unmuted\n");

	printk("Muting VCS\n");
	expected_mute = 1;
	err = bt_vcs_mute(vcs);
	if (err) {
		FAIL("Could not mute VCS (err %d)\n", err);
		return;
	}
	WAIT_FOR_COND(expected_mute == g_mute);
	printk("VCS volume upped\n");

	printk("Unmuting VCS\n");
	expected_mute = 0;
	err = bt_vcs_unmute(vcs);
	if (err) {
		FAIL("Could not unmute VCS (err %d)\n", err);
		return;
	}
	WAIT_FOR_COND(expected_mute == g_mute);
	printk("VCS volume unmuted\n");

	expected_volume = g_volume - 5;
	err = bt_vcs_vol_set(vcs, expected_volume);
	if (err) {
		FAIL("Could not set VCS volume (err %d)\n", err);
		return;
	}
	WAIT_FOR_COND(expected_volume == g_volume);
	printk("VCS volume set\n");

	if (CONFIG_BT_VCS_VOCS_INSTANCE_COUNT > 0) {
		if (test_vocs_standalone()) {
			return;
		}
	}

	if (CONFIG_BT_VCS_AICS_INSTANCE_COUNT > 0) {
		if (test_aics_standalone()) {
			return;
		}
	}

	PASS("VCS passed\n");
}

static void test_main(void)
{
	int err;
	struct bt_vcs_register_param vcs_param;
	char input_desc[CONFIG_BT_VCS_AICS_INSTANCE_COUNT][16];
	char output_desc[CONFIG_BT_VCS_VOCS_INSTANCE_COUNT][16];

	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	memset(&vcs_param, 0, sizeof(vcs_param));

	for (int i = 0; i < ARRAY_SIZE(vcs_param.vocs_param); i++) {
		vcs_param.vocs_param[i].location_writable = true;
		vcs_param.vocs_param[i].desc_writable = true;
		snprintf(output_desc[i], sizeof(output_desc[i]),
			 "Output %d", i + 1);
		vcs_param.vocs_param[i].output_desc = output_desc[i];
		vcs_param.vocs_param[i].cb = &vocs_cb;
	}

	for (int i = 0; i < ARRAY_SIZE(vcs_param.aics_param); i++) {
		vcs_param.aics_param[i].desc_writable = true;
		snprintf(input_desc[i], sizeof(input_desc[i]),
			 "Input %d", i + 1);
		vcs_param.aics_param[i].description = input_desc[i];
		vcs_param.aics_param[i].type = BT_AICS_INPUT_TYPE_DIGITAL;
		vcs_param.aics_param[i].status = g_aics_active;
		vcs_param.aics_param[i].gain_mode = BT_AICS_MODE_MANUAL;
		vcs_param.aics_param[i].units = 1;
		vcs_param.aics_param[i].min_gain = 0;
		vcs_param.aics_param[i].max_gain = 100;
		vcs_param.aics_param[i].cb = &aics_cb;
	}

	vcs_param.step = 1;
	vcs_param.mute = BT_VCS_STATE_UNMUTED;
	vcs_param.volume = 100;
	vcs_param.cb = &vcs_cb;

	err = bt_vcs_register(&vcs_param, &vcs);
	if (err) {
		FAIL("VCS register failed (err %d)\n", err);
		return;
	}

	err = bt_vcs_included_get(vcs, &vcs_included);
	if (err) {
		FAIL("VCS included get failed (err %d)\n", err);
		return;
	}

	printk("VCS initialized\n");

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, AD_SIZE, NULL, 0);
	if (err) {
		FAIL("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");

	WAIT_FOR_COND(g_is_connected);

	PASS("VCS passed\n");
}

static const struct bst_test_instance test_vcs[] = {
	{
		.test_id = "vcs_standalone",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_standalone
	},
	{
		.test_id = "vcs",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_vcs_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_vcs);
}
#else
struct bst_test_list *test_vcs_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_VCS */
