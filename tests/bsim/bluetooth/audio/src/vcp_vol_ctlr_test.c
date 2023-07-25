/*
 * Copyright (c) 2021-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_BT_VCP_VOL_CTLR

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/vcp.h>

#include "common.h"


#define VOCS_DESC_SIZE 64
#define AICS_DESC_SIZE 64

extern enum bst_result_t bst_result;

static struct bt_vcp_vol_ctlr *vol_ctlr;
static struct bt_vcp_included vcp_included;
static volatile bool g_discovery_complete;
static volatile bool g_write_complete;

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

static void vcs_state_cb(struct bt_vcp_vol_ctlr *vol_ctlr, int err,
			 uint8_t volume, uint8_t mute)
{
	if (err != 0) {
		FAIL("VCP state cb err (%d)", err);
		return;
	}

	g_volume = volume;
	g_mute = mute;

	g_cb = true;
}

static void vcs_flags_cb(struct bt_vcp_vol_ctlr *vol_ctlr, int err,
			 uint8_t flags)
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

	if (strlen(description) > sizeof(g_vocs_desc) - 1) {
		printk("Warning: VOCS description (%zu) is larger than buffer (%zu)\n",
		       strlen(description), sizeof(g_vocs_desc) - 1);
	}

	strncpy(g_vocs_desc, description, sizeof(g_vocs_desc) - 1);
	g_vocs_desc[sizeof(g_vocs_desc) - 1] = '\0';

	g_cb = true;
}

static void vocs_write_cb(struct bt_vocs *inst, int err)
{
	if (err != 0) {
		FAIL("VOCS write failed (%d)\n", err);
		return;
	}

	g_write_complete = true;
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

	if (strlen(description) > sizeof(g_aics_desc) - 1) {
		printk("Warning: AICS description (%zu) is larger than buffer (%zu)\n",
		       strlen(description), sizeof(g_aics_desc) - 1);
	}

	strncpy(g_aics_desc, description, sizeof(g_aics_desc) - 1);
	g_aics_desc[sizeof(g_aics_desc) - 1] = '\0';

	g_cb = true;
}

static void aics_write_cb(struct bt_aics *inst, int err)
{
	if (err != 0) {
		FAIL("AICS write failed (%d)\n", err);
		return;
	}

	g_write_complete = true;
}

static void vcs_discover_cb(struct bt_vcp_vol_ctlr *vol_ctlr, int err,
			    uint8_t vocs_count, uint8_t aics_count)
{
	if (err != 0) {
		FAIL("VCP could not be discovered (%d)\n", err);
		return;
	}

	g_discovery_complete = true;
}

static void vcs_write_cb(struct bt_vcp_vol_ctlr *vol_ctlr, int err)
{
	if (err != 0) {
		FAIL("VCP write failed (%d)\n", err);
		return;
	}

	g_write_complete = true;
}

static void test_aics_deactivate(void)
{
	int err;

	/* Invalid behavior */
	err = bt_aics_deactivate(NULL);
	if (err == 0) {
		FAIL("bt_aics_deactivate with NULL inst pointer did not fail");
		return;
	}

	/* Valid behavior */
	printk("Attempting to deactivate AICS\n");
	err = bt_aics_deactivate(vcp_included.aics[0]);
	if (err == 0) {
		FAIL("bt_aics_deactivate as client instance did not fail");
		return;
	}
}

static void test_aics_activate(void)
{
	int err;

	/* Invalid behavior */
	err = bt_aics_activate(NULL);
	if (err == 0) {
		FAIL("bt_aics_activate with NULL inst pointer did not fail");
		return;
	}

	/* Valid behavior */
	printk("Attempting to activate AICS\n");
	err = bt_aics_activate(vcp_included.aics[0]);
	if (err == 0) {
		FAIL("bt_aics_activate as client instance did not fail");
		return;
	}
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
	g_write_complete = false;

	err = bt_aics_mute(vcp_included.aics[0]);
	if (err != 0) {
		FAIL("Could not set AICS mute (err %d)\n", err);
		return;
	}

	WAIT_FOR_COND(g_write_complete && expected_input_mute == g_aics_input_mute);
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
	g_write_complete = false;

	err = bt_aics_unmute(vcp_included.aics[0]);
	if (err != 0) {
		FAIL("Could not set AICS unmute (err %d)\n", err);
		return;
	}

	WAIT_FOR_COND(g_write_complete && expected_input_mute == g_aics_input_mute);
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
	g_write_complete = false;

	err = bt_aics_automatic_gain_set(vcp_included.aics[0]);
	if (err != 0) {
		FAIL("Could not set AICS auto mode (err %d)\n", err);
		return;
	}

	WAIT_FOR_COND(g_write_complete && expected_mode == g_aics_mode);
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
	g_write_complete = false;

	err = bt_aics_manual_gain_set(vcp_included.aics[0]);
	if (err != 0) {
		FAIL("Could not set AICS manual mode (err %d)\n", err);
		return;
	}

	WAIT_FOR_COND(g_write_complete && expected_mode == g_aics_mode);
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
	g_write_complete = false;

	err = bt_aics_gain_set(vcp_included.aics[0], expected_gain);
	if (err != 0) {
		FAIL("Could not set AICS gain (err %d)\n", err);
		return;
	}

	WAIT_FOR_COND(g_write_complete && expected_gain == g_aics_gain);
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

static void test_aics(void)
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

	invalid_location = BT_AUDIO_LOCATION_PROHIBITED;

	err = bt_vocs_location_set(vcp_included.vocs[0], invalid_location);
	if (err == 0) {
		FAIL("bt_vocs_location_set with location 0x%08X did not fail", invalid_location);
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
	g_write_complete = false;

	err = bt_vocs_state_set(vcp_included.vocs[0], expected_offset);
	if (err != 0) {
		FAIL("Could not set VOCS state (err %d)\n", err);
		return;
	}

	WAIT_FOR_COND(g_write_complete && expected_offset == g_vocs_offset);
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

static void test_vocs(void)
{
	test_vocs_state_get();
	test_vocs_location_get();
	test_vocs_description_get();
	test_vocs_location_set();
	test_vocs_state_set();
	test_vocs_description_set();
}

static void test_cb_register(void)
{
	static struct bt_vcp_vol_ctlr_cb vcp_cbs = {
		.discover = vcs_discover_cb,
		.vol_down = vcs_write_cb,
		.vol_up = vcs_write_cb,
		.mute = vcs_write_cb,
		.unmute = vcs_write_cb,
		.vol_down_unmute = vcs_write_cb,
		.vol_up_unmute = vcs_write_cb,
		.vol_set = vcs_write_cb,
		.state = vcs_state_cb,
		.flags = vcs_flags_cb,
		.vocs_cb = {
			.state = vocs_state_cb,
			.location = vocs_location_cb,
			.description = vocs_description_cb,
			.set_offset = vocs_write_cb,
		},
		.aics_cb  = {
			.state = aics_state_cb,
			.gain_setting = aics_gain_setting_cb,
			.type = aics_input_type_cb,
			.status = aics_status_cb,
			.description = aics_description_cb,
			.set_gain = aics_write_cb,
			.unmute = aics_write_cb,
			.mute = aics_write_cb,
			.set_manual_mode = aics_write_cb,
			.set_auto_mode = aics_write_cb,
		}
	};
	int err;

	err = bt_vcp_vol_ctlr_cb_register(&vcp_cbs);
	if (err != 0) {
		FAIL("CB register failed (err %d)\n", err);
		return;
	}
}

static void test_discover(void)
{
	int err;

	/* Invalid behavior */
	err = bt_vcp_vol_ctlr_discover(NULL, &vol_ctlr);
	if (err == 0) {
		FAIL("bt_vcp_vol_ctlr_discover with NULL conn pointer did not fail");
		return;
	}

	err = bt_vcp_vol_ctlr_discover(default_conn, NULL);
	if (err == 0) {
		FAIL("bt_vcp_vol_ctlr_discover with NULL inst pointer did not fail");
		return;
	}

	/* Valid behavior */
	err = bt_vcp_vol_ctlr_discover(default_conn, &vol_ctlr);
	if (err != 0) {
		FAIL("Failed to discover VCP %d", err);
		return;
	}

	WAIT_FOR_COND(g_discovery_complete);
}

static void test_included_get(void)
{
	int err;

	/* Invalid behavior */
	err = bt_vcp_vol_ctlr_included_get(NULL, &vcp_included);
	if (err == 0) {
		FAIL("bt_vcp_vol_ctlr_included_get with NULL inst pointer did not fail");
		return;
	}

	err = bt_vcp_vol_ctlr_included_get(vol_ctlr, NULL);
	if (err == 0) {
		FAIL("bt_vcp_vol_ctlr_included_get with NULL include pointer did not fail");
		return;
	}

	/* Valid behavior */
	err = bt_vcp_vol_ctlr_included_get(vol_ctlr, &vcp_included);
	if (err != 0) {
		FAIL("Failed to get VCP included services (err %d)\n", err);
		return;
	}
}

static void test_conn_get(void)
{
	struct bt_conn *cached_conn;
	int err;

	/* Invalid behavior */
	err = bt_vcp_vol_ctlr_conn_get(NULL, &cached_conn);
	if (err == 0) {
		FAIL("bt_vcp_vol_ctlr_conn_get with NULL inst pointer did not fail");
		return;
	}

	err = bt_vcp_vol_ctlr_conn_get(vol_ctlr, NULL);
	if (err == 0) {
		FAIL("bt_vcp_vol_ctlr_conn_get with NULL cached_conn pointer did not fail");
		return;
	}

	/* Valid behavior */
	printk("Getting VCP volume controller conn\n");

	err = bt_vcp_vol_ctlr_conn_get(vol_ctlr, &cached_conn);
	if (err != 0) {
		FAIL("Could not get VCP volume controller conn (err %d)\n", err);
		return;
	}

	if (cached_conn != default_conn) {
		FAIL("Cached conn was not the conn used to discover");
		return;
	}

	printk("Got VCP volume controller conn\n");
}

static void test_read_state(void)
{
	int err;

	/* Invalid behavior */
	err = bt_vcp_vol_ctlr_read_state(NULL);
	if (err == 0) {
		FAIL("bt_vcp_vol_ctlr_read_state with NULL inst pointer did not fail");
		return;
	}

	/* Valid behavior */
	printk("Getting VCP volume state\n");
	g_cb = false;

	err = bt_vcp_vol_ctlr_read_state(vol_ctlr);
	if (err != 0) {
		FAIL("Could not get VCP volume (err %d)\n", err);
		return;
	}

	WAIT_FOR_COND(g_cb);
	printk("VCP volume get\n");
}

static void test_read_flags(void)
{
	int err;

	/* Invalid behavior */
	err = bt_vcp_vol_ctlr_read_flags(NULL);
	if (err == 0) {
		FAIL("bt_vcp_vol_ctlr_read_flags with NULL inst pointer did not fail");
		return;
	}

	/* Valid behavior */
	printk("Getting VCP flags\n");
	g_cb = false;

	err = bt_vcp_vol_ctlr_read_flags(vol_ctlr);
	if (err != 0) {
		FAIL("Could not get VCP flags (err %d)\n", err);
		return;
	}

	WAIT_FOR_COND(g_cb);
	printk("VCP flags get\n");
}

static void test_set_vol(void)
{
	const uint8_t expected_volume = g_volume + 5; /* Overflow is OK */
	int err;

	g_write_complete = g_cb = false;

	/* Invalid behavior - No invalid volume values to attempt to set */
	err = bt_vcp_vol_ctlr_set_vol(NULL, expected_volume);
	if (err == 0) {
		FAIL("bt_vcp_vol_ctlr_set_vol with NULL inst pointer did not fail");
		return;
	}

	/* Valid behavior */
	err = bt_vcp_vol_ctlr_set_vol(vol_ctlr, expected_volume);
	if (err != 0) {
		FAIL("Could not set VCP volume (err %d)\n", err);
		return;
	}

	WAIT_FOR_COND(g_volume == expected_volume && g_cb && g_write_complete);
	printk("VCP volume set\n");
}

static void test_vol_down(void)
{
	const uint8_t previous_volume = g_volume;
	int err;

	/* Invalid behavior */
	err = bt_vcp_vol_ctlr_vol_down(NULL);
	if (err == 0) {
		FAIL("bt_vcp_vol_ctlr_vol_down with NULL inst pointer did not fail");
		return;
	}

	/* Valid behavior */
	printk("Downing VCP volume\n");
	g_write_complete = g_cb = false;

	err = bt_vcp_vol_ctlr_vol_down(vol_ctlr);
	if (err != 0) {
		FAIL("Could not get down VCP volume (err %d)\n", err);
		return;
	}

	WAIT_FOR_COND(previous_volume == 0 ||
		      (g_volume < previous_volume && g_cb && g_write_complete));
	printk("VCP volume downed\n");
}

static void test_vol_up(void)
{
	const uint8_t previous_volume = g_volume;
	int err;

	/* Invalid behavior */
	err = bt_vcp_vol_ctlr_vol_up(NULL);
	if (err == 0) {
		FAIL("bt_vcp_vol_ctlr_vol_up with NULL inst pointer did not fail");
		return;
	}

	/* Valid behavior */
	printk("Upping VCP volume\n");
	g_write_complete = g_cb = false;

	err = bt_vcp_vol_ctlr_vol_up(vol_ctlr);
	if (err != 0) {
		FAIL("Could not up VCP volume (err %d)\n", err);
		return;
	}

	WAIT_FOR_COND(previous_volume == UINT8_MAX ||
		      (g_volume > previous_volume && g_cb && g_write_complete));
	printk("VCP volume upped\n");
}

static void test_mute(void)
{
	const uint8_t expected_mute = BT_VCP_STATE_MUTED;
	int err;

	/* Invalid behavior */
	err = bt_vcp_vol_ctlr_mute(NULL);
	if (err == 0) {
		FAIL("bt_vcp_vol_ctlr_mute with NULL inst pointer did not fail");
		return;
	}

	/* Valid behavior */
	printk("Muting VCP\n");
	g_write_complete = g_cb = false;

	err = bt_vcp_vol_ctlr_mute(vol_ctlr);
	if (err != 0) {
		FAIL("Could not mute VCP (err %d)\n", err);
		return;
	}

	WAIT_FOR_COND(g_mute == expected_mute && g_cb && g_write_complete);
	printk("VCP muted\n");
}

static void test_unmute_vol_down(void)
{
	const uint8_t expected_mute = BT_VCP_STATE_UNMUTED;
	const uint8_t previous_volume = g_volume;
	int err;

	/* Invalid behavior */
	err = bt_vcp_vol_ctlr_unmute_vol_down(NULL);
	if (err == 0) {
		FAIL("bt_vcp_vol_ctlr_unmute_vol_down with NULL inst pointer did not fail");
		return;
	}

	/* Valid behavior */
	printk("Downing and unmuting VCP\n");
	g_write_complete = g_cb = false;

	err = bt_vcp_vol_ctlr_unmute_vol_down(vol_ctlr);
	if (err != 0) {
		FAIL("Could not down and unmute VCP (err %d)\n", err);
		return;
	}

	WAIT_FOR_COND((previous_volume == 0 || g_volume < previous_volume) &&
		      expected_mute == g_mute &&
		      g_cb &&
		      g_write_complete);
	printk("VCP volume downed and unmuted\n");
}

static void test_unmute_vol_up(void)
{
	const uint8_t expected_mute = BT_VCP_STATE_UNMUTED;
	const uint8_t previous_volume = g_volume;
	int err;

	/* Invalid behavior */
	err = bt_vcp_vol_ctlr_unmute_vol_up(NULL);
	if (err == 0) {
		FAIL("bt_vcp_vol_ctlr_unmute_vol_up with NULL inst pointer did not fail");
		return;
	}

	/* Valid behavior */
	printk("Upping and unmuting VCP\n");
	g_write_complete = g_cb = false;

	err = bt_vcp_vol_ctlr_unmute_vol_up(vol_ctlr);
	if (err != 0) {
		FAIL("Could not up and unmute VCP (err %d)\n", err);
		return;
	}

	WAIT_FOR_COND((previous_volume == UINT8_MAX || g_volume > previous_volume) &&
		      g_mute == expected_mute &&
		      g_cb &&
		      g_write_complete);
	printk("VCP volume upped and unmuted\n");
}

static void test_unmute(void)
{
	const uint8_t expected_mute = BT_VCP_STATE_UNMUTED;
	int err;

	/* Invalid behavior */
	err = bt_vcp_vol_ctlr_unmute(NULL);
	if (err == 0) {
		FAIL("bt_vcp_vol_ctlr_unmute with NULL inst pointer did not fail");
		return;
	}

	/* Valid behavior */
	printk("Unmuting VCP\n");
	g_write_complete = g_cb = false;

	err = bt_vcp_vol_ctlr_unmute(vol_ctlr);
	if (err != 0) {
		FAIL("Could not unmute VCP (err %d)\n", err);
		return;
	}

	WAIT_FOR_COND(g_mute == expected_mute && g_cb && g_write_complete);
	printk("VCP volume unmuted\n");
}

static void test_main(void)
{
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth discover failed (err %d)\n", err);
		return;
	}

	test_cb_register();

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err != 0) {
		FAIL("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning successfully started\n");

	WAIT_FOR_FLAG(flag_connected);

	test_discover();
	test_included_get();
	test_conn_get();
	test_read_state();
	test_read_flags();
	test_set_vol();
	test_vol_down();
	test_vol_up();
	test_mute();
	test_unmute_vol_down();
	test_mute();
	test_unmute_vol_up();
	test_mute();
	test_unmute();

	if (CONFIG_BT_VCP_VOL_CTLR_VOCS > 0) {
		test_vocs();
	}

	if (CONFIG_BT_VCP_VOL_CTLR_MAX_AICS_INST > 0) {
		test_aics();
	}

	PASS("VCP volume controller Passed\n");
}

static const struct bst_test_instance test_vcs[] = {
	{
		.test_id = "vcp_vol_ctlr",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_vcp_vol_ctlr_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_vcs);
}

#else

struct bst_test_list *test_vcp_vol_ctlr_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_VCP_VOL_CTLR */
