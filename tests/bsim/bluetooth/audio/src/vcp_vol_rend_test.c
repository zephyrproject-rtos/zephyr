/*
 * Copyright (c) 2019 Bose Corporation
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/aics.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/vcp.h>
#include <zephyr/bluetooth/audio/vocs.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include "bstests.h"
#include "common.h"

#ifdef CONFIG_BT_VCP_VOL_REND
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

static struct bt_vcp_included vcp_included;

static volatile uint8_t g_volume;
static volatile uint8_t g_mute;
static volatile uint8_t g_flags;
static volatile int16_t g_vocs_offset;
static volatile uint32_t g_vocs_location;
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

static void vcs_state_cb(struct bt_conn *conn, int err, uint8_t volume, uint8_t mute)
{
	if (err != 0) {
		FAIL("VCP state cb err (%d)", err);
		return;
	}

	g_volume = volume;
	g_mute = mute;
	g_cb = true;
}

static void vcs_flags_cb(struct bt_conn *conn, int err, uint8_t flags)
{
	if (err != 0) {
		FAIL("VCP flags cb err (%d)", err);
		return;
	}

	g_flags = flags;
	g_cb = true;
}

static void vocs_state_cb(struct bt_vocs *inst, int err, int16_t offset)
{
	if (err != 0) {
		FAIL("VOCS state cb err (%d)", err);
		return;
	}

	g_vocs_offset = offset;
	g_cb = true;
}

static void vocs_location_cb(struct bt_vocs *inst, int err, uint32_t location)
{
	if (err != 0) {
		FAIL("VOCS location cb err (%d)", err);
		return;
	}

	g_vocs_location = location;
	g_cb = true;
}

static void vocs_description_cb(struct bt_vocs *inst, int err,
				char *description)
{
	if (err != 0) {
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

static struct bt_vcp_vol_rend_cb vcs_cb = {
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

static void test_aics_deactivate(void)
{
	const bool expected_aics_active = false;
	int err;

	/* Invalid behavior */
	err = bt_aics_deactivate(NULL);
	if (err == 0) {
		FAIL("bt_aics_deactivate with NULL inst pointer did not fail");
		return;
	}

	/* Valid behavior */
	printk("Deactivating AICS\n");
	err = bt_aics_deactivate(vcp_included.aics[0]);
	if (err != 0) {
		FAIL("Could not deactivate AICS (err %d)\n", err);
		return;
	}

	WAIT_FOR_COND(expected_aics_active == g_aics_active);
	printk("AICS deactivated\n");
}

static void test_aics_activate(void)
{
	const bool expected_aics_active = true;
	int err;

	/* Invalid behavior */
	err = bt_aics_activate(NULL);
	if (err == 0) {
		FAIL("bt_aics_activate with NULL inst pointer did not fail");
		return;
	}

	/* Valid behavior */
	printk("Activating AICS\n");
	err = bt_aics_activate(vcp_included.aics[0]);
	if (err != 0) {
		FAIL("Could not activate AICS (err %d)\n", err);
		return;
	}

	WAIT_FOR_COND(expected_aics_active == g_aics_active);
	printk("AICS activated\n");
}

static void test_aics_state_get(void)
{
	int err;

	/* Invalid behavior */
	err = bt_aics_state_get(NULL);
	if (err == 0) {
		FAIL("bt_aics_state_get with NULL inst pointer did not fail");
		return;
	}

	/* Valid behavior */
	printk("Getting AICS state\n");
	g_cb = false;

	err = bt_aics_state_get(vcp_included.aics[0]);
	if (err != 0) {
		FAIL("Could not get AICS state (err %d)\n", err);
		return;
	}

	WAIT_FOR_COND(g_cb);
	printk("AICS state get\n");
}

static void aics_gain_setting_get(void)
{
	int err;

	/* Invalid behavior */
	err = bt_aics_gain_setting_get(NULL);
	if (err == 0) {
		FAIL("bt_aics_gain_setting_get with NULL inst pointer did not fail");
		return;
	}

	/* Valid behavior */
	printk("Getting AICS gain setting\n");
	g_cb = false;

	err = bt_aics_gain_setting_get(vcp_included.aics[0]);
	if (err != 0) {
		FAIL("Could not get AICS gain setting (err %d)\n", err);
		return;
	}

	WAIT_FOR_COND(g_cb);
	printk("AICS gain setting get\n");
}

static void aics_type_get(void)
{
	const uint8_t expected_input_type = BT_AICS_INPUT_TYPE_DIGITAL;
	int err;

	/* Invalid behavior */
	err = bt_aics_type_get(NULL);
	if (err == 0) {
		FAIL("bt_aics_type_get with NULL inst pointer did not fail");
		return;
	}

	/* Valid behavior */
	printk("Getting AICS input type\n");

	err = bt_aics_type_get(vcp_included.aics[0]);
	if (err != 0) {
		FAIL("Could not get AICS input type (err %d)\n", err);
		return;
	}

	/* Expect and wait for input_type from init */
	WAIT_FOR_COND(expected_input_type == g_aics_input_type);
	printk("AICS input type get\n");
}

static void aics_status_get(void)
{
	int err;

	/* Invalid behavior */
	err = bt_aics_status_get(NULL);
	if (err == 0) {
		FAIL("bt_aics_status_get with NULL inst pointer did not fail");
		return;
	}

	/* Valid behavior */
	printk("Getting AICS status\n");
	g_cb = false;

	err = bt_aics_status_get(vcp_included.aics[0]);
	if (err != 0) {
		FAIL("Could not get AICS status (err %d)\n", err);
		return;
	}

	WAIT_FOR_COND(g_cb);
	printk("AICS status get\n");
}

static void aics_get_description(void)
{
	int err;

	/* Invalid behavior */
	err = bt_aics_description_get(NULL);
	if (err == 0) {
		FAIL("bt_aics_description_get with NULL inst pointer did not fail");
		return;
	}

	/* Valid behavior */
	printk("Getting AICS description\n");
	g_cb = false;

	err = bt_aics_description_get(vcp_included.aics[0]);
	if (err != 0) {
		FAIL("Could not get AICS description (err %d)\n", err);
		return;
	}

	WAIT_FOR_COND(g_cb);
	printk("AICS description get\n");
}

static void test_aics_mute(void)
{
	const uint8_t expected_input_mute = BT_AICS_STATE_MUTED;
	int err;

	/* Invalid behavior */
	err = bt_aics_mute(NULL);
	if (err == 0) {
		FAIL("bt_aics_mute with NULL inst pointer did not fail");
		return;
	}

	/* Valid behavior */
	printk("Setting AICS mute\n");

	err = bt_aics_mute(vcp_included.aics[0]);
	if (err != 0) {
		FAIL("Could not set AICS mute (err %d)\n", err);
		return;
	}

	WAIT_FOR_COND(expected_input_mute == g_aics_input_mute);
	printk("AICS mute set\n");
}

static void test_aics_unmute(void)
{
	const uint8_t expected_input_mute = BT_AICS_STATE_UNMUTED;
	int err;

	/* Invalid behavior */
	err = bt_aics_unmute(NULL);
	if (err == 0) {
		FAIL("bt_aics_unmute with NULL inst pointer did not fail");
		return;
	}

	/* Valid behavior */
	printk("Setting AICS unmute\n");

	err = bt_aics_unmute(vcp_included.aics[0]);
	if (err != 0) {
		FAIL("Could not set AICS unmute (err %d)\n", err);
		return;
	}

	WAIT_FOR_COND(expected_input_mute == g_aics_input_mute);
	printk("AICS unmute set\n");
}

static void test_aics_automatic_gain_set(void)
{
	const uint8_t expected_mode = BT_AICS_MODE_AUTO;
	int err;

	/* Invalid behavior */
	err = bt_aics_automatic_gain_set(NULL);
	if (err == 0) {
		FAIL("bt_aics_automatic_gain_set with NULL inst pointer did not fail");
		return;
	}

	/* Valid behavior */
	printk("Setting AICS auto mode\n");

	err = bt_aics_automatic_gain_set(vcp_included.aics[0]);
	if (err != 0) {
		FAIL("Could not set AICS auto mode (err %d)\n", err);
		return;
	}

	WAIT_FOR_COND(expected_mode == g_aics_mode);
	printk("AICS auto mode set\n");
}

static void test_aics_manual_gain_set(void)
{
	const uint8_t expected_mode = BT_AICS_MODE_MANUAL;
	int err;

	/* Invalid behavior */
	err = bt_aics_manual_gain_set(NULL);
	if (err == 0) {
		FAIL("bt_aics_manual_gain_set with NULL inst pointer did not fail");
		return;
	}

	/* Valid behavior */
	printk("Setting AICS manual mode\n");

	err = bt_aics_manual_gain_set(vcp_included.aics[0]);
	if (err != 0) {
		FAIL("Could not set AICS manual mode (err %d)\n", err);
		return;
	}

	WAIT_FOR_COND(expected_mode == g_aics_mode);
	printk("AICS manual mode set\n");
}

static void test_aics_gain_set(void)
{
	const int8_t expected_gain = g_aics_gain_max - 1;
	int err;

	/* Invalid behavior */
	err = bt_aics_gain_set(NULL, expected_gain);
	if (err == 0) {
		FAIL("bt_aics_gain_set with NULL inst pointer did not fail");
		return;
	}

	/* Valid behavior */
	printk("Setting AICS gain\n");

	err = bt_aics_gain_set(vcp_included.aics[0], expected_gain);
	if (err != 0) {
		FAIL("Could not set AICS gain (err %d)\n", err);
		return;
	}

	WAIT_FOR_COND(expected_gain == g_aics_gain);
	printk("AICS gain set\n");
}

static void test_aics_description_set(void)
{
	const char *expected_aics_desc = "New Input Description";
	int err;

	/* Invalid behavior */
	err = bt_aics_description_set(NULL, expected_aics_desc);
	if (err == 0) {
		FAIL("bt_aics_description_set with NULL inst pointer did not fail");
		return;
	}

	err = bt_aics_description_set(vcp_included.aics[0], NULL);
	if (err == 0) {
		FAIL("bt_aics_description_set with NULL description pointer did not fail");
		return;
	}

	/* Valid behavior */
	printk("Setting AICS Description\n");
	g_cb = false;

	err = bt_aics_description_set(vcp_included.aics[0], expected_aics_desc);
	if (err != 0) {
		FAIL("Could not set AICS Description (err %d)\n", err);
		return;
	}

	WAIT_FOR_COND(g_cb &&
		      strncmp(expected_aics_desc, g_aics_desc, strlen(expected_aics_desc)) == 0);
	printk("AICS Description set\n");
}

static void test_aics_standalone(void)
{
	test_aics_deactivate();
	test_aics_activate();
	test_aics_state_get();
	aics_gain_setting_get();
	aics_type_get();
	aics_status_get();
	aics_get_description();
	test_aics_mute();
	test_aics_unmute();
	test_aics_automatic_gain_set();
	test_aics_manual_gain_set();
	test_aics_gain_set();
	test_aics_description_set();
}

static void test_vocs_state_get(void)
{
	int err;

	/* Invalid behavior */
	err = bt_vocs_state_get(NULL);
	if (err == 0) {
		FAIL("bt_vocs_state_get with NULL inst pointer did not fail");
		return;
	}

	/* Valid behavior */
	printk("Getting VOCS state\n");
	g_cb = false;

	err = bt_vocs_state_get(vcp_included.vocs[0]);
	if (err != 0) {
		FAIL("Could not get VOCS state (err %d)\n", err);
		return;
	}

	WAIT_FOR_COND(g_cb);
	printk("VOCS state get\n");
}

static void test_vocs_location_get(void)
{
	int err;

	/* Invalid behavior */
	err = bt_vocs_location_get(NULL);
	if (err == 0) {
		FAIL("bt_vocs_location_get with NULL inst pointer did not fail");
		return;
	}

	/* Valid behavior */
	printk("Getting VOCS location\n");
	g_cb = false;

	err = bt_vocs_location_get(vcp_included.vocs[0]);
	if (err != 0) {
		FAIL("Could not get VOCS location (err %d)\n", err);
		return;
	}

	WAIT_FOR_COND(g_cb);
	printk("VOCS location get\n");
}

static void test_vocs_description_get(void)
{
	int err;

	/* Invalid behavior */
	err = bt_vocs_description_get(NULL);
	if (err == 0) {
		FAIL("bt_vocs_description_get with NULL inst pointer did not fail");
		return;
	}

	/* Valid behavior */
	printk("Getting VOCS description\n");
	g_cb = false;

	err = bt_vocs_description_get(vcp_included.vocs[0]);
	if (err != 0) {
		FAIL("Could not get VOCS description (err %d)\n", err);
		return;
	}

	WAIT_FOR_COND(g_cb);
	printk("VOCS description get\n");
}

static void test_vocs_location_set(void)
{
	const uint32_t expected_location = g_vocs_location + 1;
	uint32_t invalid_location;
	int err;

	/* Invalid behavior */
	err = bt_vocs_location_set(NULL, expected_location);
	if (err == 0) {
		FAIL("bt_vocs_location_set with NULL inst pointer did not fail");
		return;
	}

	invalid_location = BT_AUDIO_LOCATION_ANY + 1;

	err = bt_vocs_location_set(vcp_included.vocs[0], invalid_location);
	if (err == 0) {
		FAIL("bt_vocs_location_set with location 0x%08X did not fail", invalid_location);
		return;
	}

	/* Valid behavior */
	printk("Setting VOCS location\n");

	err = bt_vocs_location_set(vcp_included.vocs[0], expected_location);
	if (err != 0) {
		FAIL("Could not set VOCS location (err %d)\n", err);
		return;
	}

	WAIT_FOR_COND(expected_location == g_vocs_location);
	printk("VOCS location set\n");
}

static void test_vocs_state_set(void)
{
	const int16_t expected_offset = g_vocs_offset + 1;
	int16_t invalid_offset;
	int err;

	/* Invalid behavior */
	err = bt_vocs_state_set(NULL, expected_offset);
	if (err == 0) {
		FAIL("bt_vocs_state_set with NULL inst pointer did not fail");
		return;
	}

	invalid_offset = BT_VOCS_MIN_OFFSET - 1;

	err = bt_vocs_state_set(vcp_included.vocs[0], invalid_offset);
	if (err == 0) {
		FAIL("bt_vocs_state_set with NULL offset %d did not fail", invalid_offset);
		return;
	}

	invalid_offset = BT_VOCS_MAX_OFFSET + 1;

	err = bt_vocs_state_set(vcp_included.vocs[0], invalid_offset);
	if (err == 0) {
		FAIL("bt_vocs_state_set with NULL offset %d did not fail", invalid_offset);
		return;
	}

	/* Valid behavior */
	printk("Setting VOCS state\n");

	err = bt_vocs_state_set(vcp_included.vocs[0], expected_offset);
	if (err != 0) {
		FAIL("Could not set VOCS state (err %d)\n", err);
		return;
	}

	WAIT_FOR_COND(expected_offset == g_vocs_offset);
	printk("VOCS state set\n");
}

static void test_vocs_description_set(void)
{
	const char *expected_vocs_desc = "New Output Description";
	int err;

	/* Invalid behavior */
	err = bt_vocs_description_set(NULL, expected_vocs_desc);
	if (err == 0) {
		FAIL("bt_vocs_description_set with NULL inst pointer did not fail");
		return;
	}

	err = bt_vocs_description_set(vcp_included.vocs[0], NULL);
	if (err == 0) {
		FAIL("bt_vocs_description_set with NULL description pointer did not fail");
		return;
	}

	/* Valid behavior */
	printk("Setting VOCS description\n");
	g_cb = false;

	err = bt_vocs_description_set(vcp_included.vocs[0], expected_vocs_desc);
	if (err != 0) {
		FAIL("Could not set VOCS description (err %d)\n", err);
		return;
	}

	WAIT_FOR_COND(g_cb &&
		      strncmp(expected_vocs_desc, g_vocs_desc, strlen(expected_vocs_desc)) == 0);
	printk("VOCS description set\n");
}

static void test_vocs_standalone(void)
{
	test_vocs_state_get();
	test_vocs_location_get();
	test_vocs_description_get();
	test_vocs_location_set();
	test_vocs_state_set();
	test_vocs_description_set();
}

static void test_register(void)
{
	char output_desc[CONFIG_BT_VCP_VOL_REND_VOCS_INSTANCE_COUNT][16];
	char input_desc[CONFIG_BT_VCP_VOL_REND_AICS_INSTANCE_COUNT][16];
	struct bt_vcp_vol_rend_register_param vcp_register_param;
	int err;

	memset(&vcp_register_param, 0, sizeof(vcp_register_param));

	for (int i = 0; i < ARRAY_SIZE(vcp_register_param.vocs_param); i++) {
		vcp_register_param.vocs_param[i].location_writable = true;
		vcp_register_param.vocs_param[i].desc_writable = true;
		snprintf(output_desc[i], sizeof(output_desc[i]),
			 "Output %d", i + 1);
		vcp_register_param.vocs_param[i].output_desc = output_desc[i];
		vcp_register_param.vocs_param[i].cb = &vocs_cb;
	}

	for (int i = 0; i < ARRAY_SIZE(vcp_register_param.aics_param); i++) {
		vcp_register_param.aics_param[i].desc_writable = true;
		snprintf(input_desc[i], sizeof(input_desc[i]),
			 "Input %d", i + 1);
		vcp_register_param.aics_param[i].description = input_desc[i];
		vcp_register_param.aics_param[i].type = BT_AICS_INPUT_TYPE_DIGITAL;
		vcp_register_param.aics_param[i].status = g_aics_active;
		vcp_register_param.aics_param[i].gain_mode = BT_AICS_MODE_MANUAL;
		vcp_register_param.aics_param[i].units = 1;
		vcp_register_param.aics_param[i].min_gain = 0;
		vcp_register_param.aics_param[i].max_gain = 100;
		vcp_register_param.aics_param[i].cb = &aics_cb;
	}

	vcp_register_param.step = 1;
	vcp_register_param.mute = BT_VCP_STATE_UNMUTED;
	vcp_register_param.volume = 100;
	vcp_register_param.cb = &vcs_cb;

	/* Invalid behavior */
	err = bt_vcp_vol_rend_register(NULL);
	if (err == 0) {
		FAIL("bt_vcp_vol_rend_register with NULL pointer did not fail");
		return;
	}

	/* Valid behavior */
	err = bt_vcp_vol_rend_register(&vcp_register_param);
	if (err != 0) {
		FAIL("VCP register failed (err %d)\n", err);
		return;
	}
}

static void test_included_get(void)
{
	int err;

	/* Invalid behavior */
	err = bt_vcp_vol_rend_included_get(NULL);
	if (err == 0) {
		FAIL("bt_vcp_vol_rend_included_get with NULL pointer did not fail");
		return;
	}

	/* Valid behavior */
	err = bt_vcp_vol_rend_included_get(&vcp_included);
	if (err != 0) {
		FAIL("VCP included get failed (err %d)\n", err);
		return;
	}
}

static void test_set_step(uint8_t volume_step)
{
	int err;

	/* Invalid behavior */
	err = bt_vcp_vol_rend_set_step(0);
	if (err == 0) {
		FAIL("bt_vcp_vol_rend_set_step with step size 0 did not fail");
		return;
	}

	/* Valid behavior */
	printk("Setting VCP step\n");

	err = bt_vcp_vol_rend_set_step(volume_step);
	if (err != 0) {
		FAIL("VCP step set failed (err %d)\n", err);
		return;
	}

	printk("VCP step set\n");
}

static void test_get_state(void)
{
	int err;

	printk("Getting VCP volume state\n");
	g_cb = false;

	err = bt_vcp_vol_rend_get_state();
	if (err != 0) {
		FAIL("Could not get VCP volume (err %d)\n", err);
		return;
	}

	WAIT_FOR_COND(g_cb);
	printk("VCP volume get\n");
}

static void test_get_flags(void)
{
	int err;

	printk("Getting VCP flags\n");
	g_cb = false;

	err = bt_vcp_vol_rend_get_flags();
	if (err != 0) {
		FAIL("Could not get VCP flags (err %d)\n", err);
		return;
	}

	WAIT_FOR_COND(g_cb);
	printk("VCP flags get\n");
}

static void test_vol_down(uint8_t volume_step)
{
	const uint8_t expected_volume = g_volume > volume_step ? g_volume - volume_step : 0;
	int err;

	printk("Downing VCP volume\n");

	err = bt_vcp_vol_rend_vol_down();
	if (err != 0) {
		FAIL("Could not get down VCP volume (err %d)\n", err);
		return;
	}

	WAIT_FOR_COND(expected_volume == g_volume);
	printk("VCP volume downed\n");
}

static void test_vol_up(uint8_t volume_step)
{
	const uint8_t expected_volume = MIN((uint16_t)g_volume + volume_step, UINT8_MAX);
	int err;

	printk("Upping VCP volume\n");

	err = bt_vcp_vol_rend_vol_up();
	if (err != 0) {
		FAIL("Could not up VCP volume (err %d)\n", err);
		return;
	}

	WAIT_FOR_COND(expected_volume == g_volume);
	printk("VCP volume upped\n");
}

static void test_mute(void)
{
	const uint8_t expected_mute = BT_VCP_STATE_MUTED;
	int err;

	printk("Muting VCP\n");

	err = bt_vcp_vol_rend_mute();
	if (err != 0) {
		FAIL("Could not mute VCP (err %d)\n", err);
		return;
	}

	WAIT_FOR_COND(expected_mute == g_mute);
	printk("VCP muted\n");
}

static void test_unmute_vol_down(uint8_t volume_step)
{
	const uint8_t expected_volume = g_volume > volume_step ? g_volume - volume_step : 0;
	const uint8_t expected_mute = BT_VCP_STATE_UNMUTED;
	int err;

	printk("Downing and unmuting VCP\n");

	err = bt_vcp_vol_rend_unmute_vol_down();
	if (err != 0) {
		FAIL("Could not down and unmute VCP (err %d)\n", err);
		return;
	}

	WAIT_FOR_COND(expected_volume == g_volume && expected_mute == g_mute);
	printk("VCP volume downed and unmuted\n");
}

static void test_unmute_vol_up(uint8_t volume_step)
{
	const uint8_t expected_volume = MIN((uint16_t)g_volume + volume_step, UINT8_MAX);
	const uint8_t expected_mute = BT_VCP_STATE_UNMUTED;
	int err;

	printk("Upping and unmuting VCP\n");

	err = bt_vcp_vol_rend_unmute_vol_up();
	if (err != 0) {
		FAIL("Could not up and unmute VCP (err %d)\n", err);
		return;
	}

	WAIT_FOR_COND(expected_volume == g_volume && expected_mute == g_mute);
	printk("VCP volume upped and unmuted\n");
}

static void test_unmute(void)
{
	const uint8_t expected_mute = BT_VCP_STATE_UNMUTED;
	int err;

	printk("Unmuting VCP\n");

	err = bt_vcp_vol_rend_unmute();
	if (err != 0) {
		FAIL("Could not unmute VCP (err %d)\n", err);
		return;
	}

	WAIT_FOR_COND(expected_mute == g_mute);
	printk("VCP volume unmuted\n");
}

static void test_set_vol(void)
{
	const uint8_t expected_volume = g_volume - 5; /* any underflow is fine too */
	int err;

	err = bt_vcp_vol_rend_set_vol(expected_volume);
	if (err != 0) {
		FAIL("Could not set VCP volume (err %d)\n", err);
		return;
	}

	WAIT_FOR_COND(expected_volume == g_volume);
	printk("VCP volume set\n");
}

static void test_standalone(void)
{
	const uint8_t volume_step = 5;
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	test_register();
	test_included_get();

	printk("VCP initialized\n");
	test_set_step(volume_step);
	test_get_state();
	test_get_flags();
	test_vol_down(volume_step);
	test_vol_up(volume_step);
	test_mute();
	test_unmute_vol_down(volume_step);
	test_mute();
	test_unmute_vol_up(volume_step);
	test_mute();
	test_unmute();
	test_set_vol();

	if (CONFIG_BT_VCP_VOL_REND_VOCS_INSTANCE_COUNT > 0) {
		test_vocs_standalone();
	}

	if (CONFIG_BT_VCP_VOL_REND_AICS_INSTANCE_COUNT > 0) {
		test_aics_standalone();
	}

	PASS("VCP passed\n");
}

static void test_main(void)
{
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	test_register();
	test_included_get();

	printk("VCP initialized\n");

	err = bt_le_adv_start(BT_LE_ADV_CONN_ONE_TIME, ad, AD_SIZE, NULL, 0);
	if (err != 0) {
		FAIL("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");

	WAIT_FOR_FLAG(flag_connected);

	PASS("VCP volume renderer passed\n");
}

static const struct bst_test_instance test_vcs[] = {
	{
		.test_id = "vcp_vol_rend_standalone",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_standalone
	},
	{
		.test_id = "vcp_vol_rend",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_vcp_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_vcs);
}
#else
struct bst_test_list *test_vcp_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_VCP_VOL_REND */
