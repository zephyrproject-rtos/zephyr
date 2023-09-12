/*
 * Copyright (c) 2021-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_BT_MICP_MIC_CTLR

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/micp.h>

#include "common.h"

#define AICS_DESC_SIZE 64

extern enum bst_result_t bst_result;

static struct bt_micp_mic_ctlr *mic_ctlr;
static struct bt_micp_included micp_included;
static volatile bool g_bt_init;
static volatile bool g_discovery_complete;
static volatile bool g_write_complete;

static volatile uint8_t g_mute;
static volatile uint8_t g_aics_count;
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

static void micp_mic_ctlr_discover_cb(struct bt_micp_mic_ctlr *mic_ctlr,
				      int err,
				      uint8_t aics_count)
{
	if (err != 0) {
		FAIL("MICS could not be discovered (%d)\n", err);
		return;
	}

	g_aics_count = aics_count;
	g_discovery_complete = true;
}

static void micp_mic_ctlr_mute_written_cb(struct bt_micp_mic_ctlr *mic_ctlr,
					  int err)
{
	if (err != 0) {
		FAIL("mic_ctlr mute write failed (%d)\n", err);
		return;
	}

	g_write_complete = true;
}

static void micp_mic_ctlr_unmute_written_cb(struct bt_micp_mic_ctlr *mic_ctlr,
					    int err)
{
	if (err != 0) {
		FAIL("mic_ctlr unmute write failed (%d)\n", err);
		return;
	}

	g_write_complete = true;
}

static void micp_mic_ctlr_mute_cb(struct bt_micp_mic_ctlr *mic_ctlr, int err,
				  uint8_t mute)
{
	if (err != 0) {
		FAIL("mic_ctlr mute read failed (%d)\n", err);
		return;
	}

	g_mute = mute;
	g_cb = true;
}

static struct bt_micp_mic_ctlr_cb micp_mic_ctlr_cbs = {
	.discover = micp_mic_ctlr_discover_cb,
	.mute = micp_mic_ctlr_mute_cb,
	.mute_written = micp_mic_ctlr_mute_written_cb,
	.unmute_written = micp_mic_ctlr_unmute_written_cb,
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

static void bt_ready(int err)
{
	if (err != 0) {
		FAIL("Bluetooth discover failed (err %d)\n", err);
		return;
	}

	g_bt_init = true;
}

static int test_aics(void)
{
	int err;
	int8_t expected_gain;
	uint8_t expected_input_mute;
	uint8_t expected_mode;
	uint8_t expected_input_type;
	char expected_aics_desc[AICS_DESC_SIZE];
	struct bt_conn *cached_conn;

	printk("Getting AICS client conn\n");
	err = bt_aics_client_conn_get(micp_included.aics[0], &cached_conn);
	if (err != 0) {
		FAIL("Could not get AICS client conn (err %d)\n", err);
		return err;
	}
	if (cached_conn != default_conn) {
		FAIL("Cached conn was not the conn used to discover");
		return -ENOTCONN;
	}

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
	expected_input_type = BT_AICS_INPUT_TYPE_UNSPECIFIED;
	g_cb = false;
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
	expected_input_mute = BT_AICS_STATE_MUTED;
	g_write_complete = g_cb = false;
	err = bt_aics_mute(micp_included.aics[0]);
	if (err != 0) {
		FAIL("Could not set AICS mute (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(g_aics_input_mute == expected_input_mute &&
		 g_cb && g_write_complete);
	printk("AICS mute set\n");

	printk("Setting AICS unmute\n");
	expected_input_mute = BT_AICS_STATE_UNMUTED;
	g_write_complete = g_cb = false;
	err = bt_aics_unmute(micp_included.aics[0]);
	if (err != 0) {
		FAIL("Could not set AICS unmute (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(g_aics_input_mute == expected_input_mute &&
		 g_cb && g_write_complete);
	printk("AICS unmute set\n");

	printk("Setting AICS auto mode\n");
	expected_mode = BT_AICS_MODE_AUTO;
	g_write_complete = g_cb = false;
	err = bt_aics_automatic_gain_set(micp_included.aics[0]);
	if (err != 0) {
		FAIL("Could not set AICS auto mode (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(g_aics_mode == expected_mode && g_cb && g_write_complete);
	printk("AICS auto mode set\n");

	printk("Setting AICS manual mode\n");
	expected_mode = BT_AICS_MODE_MANUAL;
	g_write_complete = g_cb = false;
	err = bt_aics_manual_gain_set(micp_included.aics[0]);
	if (err != 0) {
		FAIL("Could not set AICS manual mode (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(g_aics_mode == expected_mode && g_cb && g_write_complete);
	printk("AICS manual mode set\n");

	printk("Setting AICS gain\n");
	expected_gain = g_aics_gain_max - 1;
	g_write_complete = g_cb = false;
	err = bt_aics_gain_set(micp_included.aics[0], expected_gain);
	if (err != 0) {
		FAIL("Could not set AICS gain (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(g_aics_gain == expected_gain && g_cb && g_write_complete);
	printk("AICS gain set\n");

	printk("Setting AICS Description\n");
	strncpy(expected_aics_desc, "New Input Description",
		sizeof(expected_aics_desc));
	expected_aics_desc[sizeof(expected_aics_desc) - 1] = '\0';
	g_cb = false;
	err = bt_aics_description_set(micp_included.aics[0],
					   expected_aics_desc);
	if (err != 0) {
		FAIL("Could not set AICS Description (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(g_cb &&
		      (strncmp(expected_aics_desc, g_aics_desc,
			       sizeof(expected_aics_desc)) == 0));
	printk("AICS Description set\n");

	printk("AICS passed\n");
	return 0;
}

static void test_main(void)
{
	int err;
	uint8_t expected_mute;
	struct bt_conn *cached_conn;

	err = bt_enable(bt_ready);

	if (err != 0) {
		FAIL("Bluetooth discover failed (err %d)\n", err);
		return;
	}

	bt_le_scan_cb_register(&common_scan_cb);

	bt_micp_mic_ctlr_cb_register(&micp_mic_ctlr_cbs);

	WAIT_FOR_COND(g_bt_init);

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL);
	if (err != 0) {
		FAIL("Scanning failed to start (err %d)\n", err);
		return;
	}
	printk("Scanning successfully started\n");
	WAIT_FOR_FLAG(flag_connected);

	err = bt_micp_mic_ctlr_discover(default_conn, &mic_ctlr);
	if (err != 0) {
		FAIL("Failed to discover MICS %d", err);
	}
	WAIT_FOR_COND(g_discovery_complete);

	err = bt_micp_mic_ctlr_included_get(mic_ctlr, &micp_included);
	if (err != 0) {
		FAIL("Failed to get mic_ctlr context (err %d)\n", err);
		return;
	}

	printk("Getting mic_ctlr conn\n");
	err = bt_micp_mic_ctlr_conn_get(mic_ctlr, &cached_conn);
	if (err != 0) {
		FAIL("Failed to get mic_ctlr conn (err %d)\n", err);
		return;
	}
	if (cached_conn != default_conn) {
		FAIL("Cached conn was not the conn used to discover");
		return;
	}

	printk("Getting mic_ctlr mute state\n");
	g_cb = false;
	err = bt_micp_mic_ctlr_mute_get(mic_ctlr);
	if (err != 0) {
		FAIL("Could not get mic_ctlr mute state (err %d)\n", err);
		return;
	}
	WAIT_FOR_COND(g_cb);
	printk("mic_ctlr mute state received\n");

	printk("Muting mic_ctlr\n");
	expected_mute = 1;
	g_write_complete = g_cb = false;
	err = bt_micp_mic_ctlr_mute(mic_ctlr);
	if (err != 0) {
		FAIL("Could not mute mic_ctlr (err %d)\n", err);
		return;
	}
	WAIT_FOR_COND(g_mute == expected_mute && g_cb && g_write_complete);
	printk("mic_ctlr muted\n");

	printk("Unmuting mic_ctlr\n");
	expected_mute = 0;
	g_write_complete = g_cb = false;
	err = bt_micp_mic_ctlr_unmute(mic_ctlr);
	if (err != 0) {
		FAIL("Could not unmute mic_ctlr (err %d)\n", err);
		return;
	}
	WAIT_FOR_COND(g_mute == expected_mute && g_cb && g_write_complete);
	printk("mic_ctlr unmuted\n");

	if (CONFIG_BT_MICP_MIC_CTLR_MAX_AICS_INST > 0 && g_aics_count > 0) {
		if (test_aics()) {
			return;
		}
	}

	PASS("mic_ctlr Passed\n");
}

static const struct bst_test_instance test_micp[] = {
	{
		.test_id = "micp_mic_ctlr",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_micp_mic_ctlr_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_micp);
}

#else

struct bst_test_list *test_micp_mic_ctlr_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_MICP_MIC_CTLR */
