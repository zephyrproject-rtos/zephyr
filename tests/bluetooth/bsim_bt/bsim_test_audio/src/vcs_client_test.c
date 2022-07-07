/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_BT_VCS_CLIENT

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/vcs.h>

#include "common.h"


#define VOCS_DESC_SIZE 64
#define AICS_DESC_SIZE 64

extern enum bst_result_t bst_result;

static struct bt_vcs *vcs;
static struct bt_vcs_included vcs_included;
static volatile bool g_bt_init;
static volatile bool g_is_connected;
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
	if (err) {
		FAIL("VOCS write failed (%d)\n", err);
		return;
	}

	g_write_complete = true;
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
	if (err) {
		FAIL("AICS write failed (%d)\n", err);
		return;
	}

	g_write_complete = true;
}

static void vcs_discover_cb(struct bt_vcs *vcs, int err, uint8_t vocs_count,
			    uint8_t aics_count)
{
	if (err) {
		FAIL("VCS could not be discovered (%d)\n", err);
		return;
	}

	g_discovery_complete = true;
}

static void vcs_write_cb(struct bt_vcs *vcs, int err)
{
	if (err) {
		FAIL("VCS write failed (%d)\n", err);
		return;
	}

	g_write_complete = true;
}

static struct bt_vcs_cb vcs_cbs = {
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

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		bt_conn_unref(default_conn);
		default_conn = NULL;
		FAIL("Failed to connect to %s (%u)\n", addr, err);
		return;
	}
	printk("Connected to %s\n", addr);
	g_is_connected = true;
}

static void bt_ready(int err)
{
	if (err) {
		FAIL("Bluetooth discover failed (err %d)\n", err);
		return;
	}

	g_bt_init = true;
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

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
	err = bt_aics_client_conn_get(vcs_included.aics[0], &cached_conn);
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
	g_cb = false;
	err = bt_vcs_aics_type_get(vcs, vcs_included.aics[0]);
	if (err) {
		FAIL("Could not get AICS input type (err %d)\n", err);
		return err;
	}
	/* Expect and wait for input_type from init */
	WAIT_FOR_COND(g_cb && expected_input_type == g_aics_input_type);
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
	g_write_complete = g_cb = false;
	err = bt_vcs_aics_mute(vcs, vcs_included.aics[0]);
	if (err) {
		FAIL("Could not set AICS mute (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(g_aics_input_mute == expected_input_mute &&
		 g_cb && g_write_complete);
	printk("AICS mute set\n");

	printk("Setting AICS unmute\n");
	expected_input_mute = BT_AICS_STATE_UNMUTED;
	g_write_complete = g_cb = false;
	err = bt_vcs_aics_unmute(vcs, vcs_included.aics[0]);
	if (err) {
		FAIL("Could not set AICS unmute (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(g_aics_input_mute == expected_input_mute &&
		 g_cb && g_write_complete);
	printk("AICS unmute set\n");

	printk("Setting AICS auto mode\n");
	expected_mode = BT_AICS_MODE_AUTO;
	g_write_complete = g_cb = false;
	err = bt_vcs_aics_automatic_gain_set(vcs, vcs_included.aics[0]);
	if (err) {
		FAIL("Could not set AICS auto mode (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(g_aics_mode == expected_mode && g_cb && g_write_complete);
	printk("AICS auto mode set\n");

	printk("Setting AICS manual mode\n");
	expected_mode = BT_AICS_MODE_MANUAL;
	g_write_complete = g_cb = false;
	err = bt_vcs_aics_manual_gain_set(vcs, vcs_included.aics[0]);
	if (err) {
		FAIL("Could not set AICS manual mode (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(g_aics_mode == expected_mode && g_cb && g_write_complete);
	printk("AICS manual mode set\n");

	printk("Setting AICS gain\n");
	expected_gain = g_aics_gain_max - 1;
	g_write_complete = g_cb = false;
	err = bt_vcs_aics_gain_set(vcs, vcs_included.aics[0], expected_gain);
	if (err) {
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
	err = bt_vcs_aics_description_set(vcs, vcs_included.aics[0],
					  expected_aics_desc);
	if (err) {
		FAIL("Could not set AICS Description (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(!strncmp(expected_aics_desc, g_aics_desc,
			  sizeof(expected_aics_desc)) &&
		g_cb);
	printk("AICS Description set\n");

	printk("AICS passed\n");
	return 0;
}

static int test_vocs(void)
{
	int err;
	uint32_t expected_location;
	int16_t expected_offset;
	char expected_description[VOCS_DESC_SIZE];
	struct bt_conn *cached_conn;

	printk("Getting VOCS client conn\n");
	err = bt_vocs_client_conn_get(vcs_included.vocs[0], &cached_conn);
	if (err != 0) {
		FAIL("Could not get VOCS client conn (err %d)\n", err);
		return err;
	}
	if (cached_conn != default_conn) {
		FAIL("Cached conn was not the conn used to discover");
		return -ENOTCONN;
	}

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
	g_cb = false;
	err = bt_vcs_vocs_location_set(vcs, vcs_included.vocs[0],
				       expected_location);
	if (err) {
		FAIL("Could not set VOCS location (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(g_vocs_location == expected_location && g_cb);
	printk("VOCS location set\n");

	printk("Setting VOCS state\n");
	expected_offset = g_vocs_offset + 1;
	g_write_complete = g_cb = false;
	err = bt_vcs_vocs_state_set(vcs, vcs_included.vocs[0], expected_offset);
	if (err) {
		FAIL("Could not set VOCS state (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(g_vocs_offset == expected_offset && g_cb && g_write_complete);
	printk("VOCS state set\n");

	printk("Setting VOCS description\n");
	strncpy(expected_description, "New Output Description",
		sizeof(expected_description));
	expected_description[sizeof(expected_description) - 1] = '\0';
	g_cb = false;
	err = bt_vcs_vocs_description_set(vcs, vcs_included.vocs[0],
					  expected_description);
	if (err) {
		FAIL("Could not set VOCS description (err %d)\n", err);
		return err;
	}
	WAIT_FOR_COND(!strncmp(expected_description, g_vocs_desc,
			  sizeof(expected_description)) &&
		 g_cb);
	printk("VOCS description set\n");

	printk("VOCS passed\n");
	return 0;
}

static void test_main(void)
{
	int err;
	uint8_t expected_volume;
	uint8_t previous_volume;
	uint8_t expected_mute;
	struct bt_conn *cached_conn;

	err = bt_enable(bt_ready);

	if (err) {
		FAIL("Bluetooth discover failed (err %d)\n", err);
		return;
	}

	err = bt_vcs_client_cb_register(&vcs_cbs);
	if (err) {
		FAIL("CB register failed (err %d)\n", err);
		return;
	}

	WAIT_FOR_COND(g_bt_init);

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err) {
		FAIL("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning successfully started\n");

	WAIT_FOR_COND(g_is_connected);

	err = bt_vcs_discover(default_conn, &vcs);
	if (err) {
		FAIL("Failed to discover VCS %d", err);
	}

	WAIT_FOR_COND(g_discovery_complete);

	err = bt_vcs_included_get(vcs, &vcs_included);
	if (err) {
		FAIL("Failed to get VCS included services (err %d)\n", err);
		return;
	}

	printk("Getting VCS client conn\n");
	err = bt_vcs_client_conn_get(vcs, &cached_conn);
	if (err != 0) {
		FAIL("Could not get VCS client conn (err %d)\n", err);
		return;
	}
	if (cached_conn != default_conn) {
		FAIL("Cached conn was not the conn used to discover");
		return;
	}

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

	expected_volume = g_volume != 100 ? 100 : 101; /* ensure change */
	g_write_complete = g_cb = false;
	err = bt_vcs_vol_set(vcs, expected_volume);
	if (err) {
		FAIL("Could not set VCS volume (err %d)\n", err);
		return;
	}
	WAIT_FOR_COND(g_volume == expected_volume && g_cb && g_write_complete);
	printk("VCS volume set\n");

	printk("Downing VCS volume\n");
	previous_volume = g_volume;
	g_write_complete = g_cb = false;
	err = bt_vcs_vol_down(vcs);
	if (err) {
		FAIL("Could not get down VCS volume (err %d)\n", err);
		return;
	}
	WAIT_FOR_COND(g_volume < previous_volume && g_cb && g_write_complete);
	printk("VCS volume downed\n");

	printk("Upping VCS volume\n");
	previous_volume = g_volume;
	g_write_complete = g_cb = false;
	err = bt_vcs_vol_up(vcs);
	if (err) {
		FAIL("Could not up VCS volume (err %d)\n", err);
		return;
	}
	WAIT_FOR_COND(g_volume > previous_volume && g_cb && g_write_complete);
	printk("VCS volume upped\n");

	printk("Muting VCS\n");
	expected_mute = 1;
	g_write_complete = g_cb = false;
	err = bt_vcs_mute(vcs);
	if (err) {
		FAIL("Could not mute VCS (err %d)\n", err);
		return;
	}
	WAIT_FOR_COND(g_mute == expected_mute && g_cb && g_write_complete);
	printk("VCS muted\n");

	printk("Downing and unmuting VCS\n");
	previous_volume = g_volume;
	expected_mute = 0;
	g_write_complete = g_cb = false;
	err = bt_vcs_unmute_vol_down(vcs);
	if (err) {
		FAIL("Could not down and unmute VCS (err %d)\n", err);
		return;
	}
	WAIT_FOR_COND(g_volume < previous_volume && expected_mute == g_mute &&
		 g_cb && g_write_complete);
	printk("VCS volume downed and unmuted\n");

	printk("Muting VCS\n");
	expected_mute = 1;
	g_write_complete = g_cb = false;
	err = bt_vcs_mute(vcs);
	if (err) {
		FAIL("Could not mute VCS (err %d)\n", err);
		return;
	}
	WAIT_FOR_COND(g_mute == expected_mute && g_cb && g_write_complete);
	printk("VCS muted\n");

	printk("Upping and unmuting VCS\n");
	previous_volume = g_volume;
	expected_mute = 0;
	g_write_complete = g_cb = false;
	err = bt_vcs_unmute_vol_up(vcs);
	if (err) {
		FAIL("Could not up and unmute VCS (err %d)\n", err);
		return;
	}
	WAIT_FOR_COND(g_volume > previous_volume && g_mute == expected_mute &&
		 g_cb && g_write_complete);
	printk("VCS volume upped and unmuted\n");

	printk("Muting VCS\n");
	expected_mute = 1;
	g_write_complete = g_cb = false;
	err = bt_vcs_mute(vcs);
	if (err) {
		FAIL("Could not mute VCS (err %d)\n", err);
		return;
	}
	WAIT_FOR_COND(g_mute == expected_mute && g_cb && g_write_complete);
	printk("VCS muted\n");

	printk("Unmuting VCS\n");
	expected_mute = 0;
	g_write_complete = g_cb = false;
	err = bt_vcs_unmute(vcs);
	if (err) {
		FAIL("Could not unmute VCS (err %d)\n", err);
		return;
	}
	WAIT_FOR_COND(g_mute == expected_mute && g_cb && g_write_complete);
	printk("VCS volume unmuted\n");

	if (CONFIG_BT_VCS_CLIENT_VOCS > 0) {
		if (test_vocs()) {
			return;
		}
	}

	if (CONFIG_BT_VCS_CLIENT_MAX_AICS_INST > 0) {
		if (test_aics()) {
			return;
		}
	}

	PASS("VCS client Passed\n");
}

static const struct bst_test_instance test_vcs[] = {
	{
		.test_id = "vcs_client",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_vcs_client_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_vcs);
}

#else

struct bst_test_list *test_vcs_client_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_VCS_CLIENT */
