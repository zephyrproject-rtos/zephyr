/*
 * Copyright (c) 2019 - 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_BT_MCC

#include <zephyr/bluetooth/audio/mcc.h>
#include <zephyr/bluetooth/audio/media_proxy.h>
#include <zephyr/bluetooth/services/ots.h>

#include "common.h"

extern enum bst_result_t bst_result;

static struct bt_mcc_cb mcc_cb;

static uint64_t g_icon_object_id;
static uint64_t g_track_segments_object_id;
static uint64_t g_current_track_object_id;
static uint64_t g_next_track_object_id;
static uint64_t g_parent_group_object_id;
static uint64_t g_current_group_object_id;
static uint64_t g_search_results_object_id;

static int32_t g_pos;
static int8_t  g_pb_speed;
static uint8_t g_playing_order;
static uint8_t g_state;
static uint8_t g_command_result;
static uint8_t g_search_result;
static uint32_t g_supported_opcodes;

CREATE_FLAG(ble_is_initialized);
CREATE_FLAG(discovery_done);
CREATE_FLAG(player_name_read);
CREATE_FLAG(icon_object_id_read);
CREATE_FLAG(icon_url_read);
CREATE_FLAG(track_change_notified);
CREATE_FLAG(track_title_read);
CREATE_FLAG(track_duration_read);
CREATE_FLAG(track_position_read);
CREATE_FLAG(track_position_set);
CREATE_FLAG(playback_speed_read);
CREATE_FLAG(playback_speed_set);
CREATE_FLAG(seeking_speed_read);
CREATE_FLAG(supported_opcodes_read);
CREATE_FLAG(track_segments_object_id_read);
CREATE_FLAG(current_track_object_id_read);
CREATE_FLAG(current_track_object_id_set);
CREATE_FLAG(next_track_object_id_read);
CREATE_FLAG(next_track_object_id_set);
CREATE_FLAG(parent_group_object_id_read);
CREATE_FLAG(current_group_object_id_read);
CREATE_FLAG(current_group_object_id_set);
CREATE_FLAG(search_results_object_id_read);
CREATE_FLAG(playing_order_read);
CREATE_FLAG(playing_order_set);
CREATE_FLAG(playing_orders_supported_read);
CREATE_FLAG(ccid_read);
CREATE_FLAG(media_state_read);
CREATE_FLAG(command_sent);
CREATE_FLAG(command_notified);
CREATE_FLAG(search_sent);
CREATE_FLAG(search_notified);
CREATE_FLAG(object_selected);
CREATE_FLAG(metadata_read);
CREATE_FLAG(object_read);


static void mcc_discover_mcs_cb(struct bt_conn *conn, int err)
{
	if (err) {
		FAIL("Discovery of MCS failed (%d)\n", err);
		return;
	}

	SET_FLAG(discovery_done);
}

static void mcc_read_player_name_cb(struct bt_conn *conn, int err, const char *name)
{
	if (err) {
		FAIL("Player Name read failed (%d)\n", err);
		return;
	}

	SET_FLAG(player_name_read);
}

static void mcc_read_icon_obj_id_cb(struct bt_conn *conn, int err, uint64_t id)
{
	if (err) {
		FAIL("Icon Object ID read failed (%d)", err);
		return;
	}

	g_icon_object_id = id;
	SET_FLAG(icon_object_id_read);
}

static void mcc_read_icon_url_cb(struct bt_conn *conn, int err, const char *url)
{
	if (err) {
		FAIL("Icon URL read failed (%d)", err);
		return;
	}

	SET_FLAG(icon_url_read);
}

static void mcc_track_changed_ntf_cb(struct bt_conn *conn, int err)
{
	if (err) {
		FAIL("Track change notification failed (%d)", err);
		return;
	}

	SET_FLAG(track_change_notified);
}

static void mcc_read_track_title_cb(struct bt_conn *conn, int err, const char *title)
{
	if (err) {
		FAIL("Track title read failed (%d)", err);
		return;
	}

	SET_FLAG(track_title_read);
}

static void mcc_read_track_duration_cb(struct bt_conn *conn, int err, int32_t dur)
{
	if (err) {
		FAIL("Track duration read failed (%d)", err);
		return;
	}

	SET_FLAG(track_duration_read);
}

static void mcc_read_track_position_cb(struct bt_conn *conn, int err, int32_t pos)
{
	if (err) {
		FAIL("Track position read failed (%d)", err);
		return;
	}

	g_pos = pos;
	SET_FLAG(track_position_read);
}

static void mcc_set_track_position_cb(struct bt_conn *conn, int err, int32_t pos)
{
	if (err) {
		FAIL("Track Position set failed (%d)", err);
		return;
	}

	g_pos = pos;
	SET_FLAG(track_position_set);
}

static void mcc_read_playback_speed_cb(struct bt_conn *conn, int err,
				       int8_t speed)
{
	if (err) {
		FAIL("Playback speed read failed (%d)", err);
		return;
	}

	g_pb_speed = speed;
	SET_FLAG(playback_speed_read);
}

static void mcc_set_playback_speed_cb(struct bt_conn *conn, int err, int8_t speed)
{
	if (err) {
		FAIL("Playback speed set failed (%d)", err);
		return;
	}

	g_pb_speed = speed;
	SET_FLAG(playback_speed_set);
}

static void mcc_read_seeking_speed_cb(struct bt_conn *conn, int err,
				      int8_t speed)
{
	if (err) {
		FAIL("Seeking speed read failed (%d)", err);
		return;
	}

	SET_FLAG(seeking_speed_read);
}

static void mcc_read_segments_obj_id_cb(struct bt_conn *conn, int err,
					uint64_t id)
{
	if (err) {
		FAIL("Track Segments ID read failed (%d)\n", err);
		return;
	}

	g_track_segments_object_id = id;
	SET_FLAG(track_segments_object_id_read);
}

static void mcc_read_current_track_obj_id_cb(struct bt_conn *conn, int err,
					     uint64_t id)
{
	if (err) {
		FAIL("Current Track Object ID read failed (%d)\n", err);
		return;
	}

	g_current_track_object_id = id;
	SET_FLAG(current_track_object_id_read);
}

static void mcc_set_current_track_obj_id_cb(struct bt_conn *conn, int err,
					    uint64_t id)
{
	if (err) {
		FAIL("Current Track Object ID set failed (%d)\n", err);
		return;
	}

	g_current_track_object_id = id;
	SET_FLAG(current_track_object_id_set);
}

static void mcc_read_next_track_obj_id_cb(struct bt_conn *conn, int err,
					     uint64_t id)
{
	if (err) {
		FAIL("Next Track Object ID read failed (%d)\n", err);
		return;
	}

	g_next_track_object_id = id;
	SET_FLAG(next_track_object_id_read);
}

static void mcc_set_next_track_obj_id_cb(struct bt_conn *conn, int err,
					    uint64_t id)
{
	if (err) {
		FAIL("Next Track Object ID set failed (%d)\n", err);
		return;
	}

	g_next_track_object_id = id;
	SET_FLAG(next_track_object_id_set);
}

static void mcc_read_current_group_obj_id_cb(struct bt_conn *conn, int err,
					     uint64_t id)
{
	if (err) {
		FAIL("Current Group Object ID read failed (%d)\n", err);
		return;
	}

	g_current_group_object_id = id;
	SET_FLAG(current_group_object_id_read);
}

static void mcc_set_current_group_obj_id_cb(struct bt_conn *conn, int err,
					    uint64_t id)
{
	if (err) {
		FAIL("Current Group Object ID set failed (%d)\n", err);
		return;
	}

	g_current_group_object_id = id;
	SET_FLAG(current_group_object_id_set);
}

static void mcc_read_parent_group_obj_id_cb(struct bt_conn *conn, int err,
					    uint64_t id)
{
	if (err) {
		FAIL("Parent Group Object ID read failed (%d)\n", err);
		return;
	}

	g_parent_group_object_id = id;
	SET_FLAG(parent_group_object_id_read);
}

static void mcc_read_playing_order_cb(struct bt_conn *conn, int err, uint8_t order)
{
	if (err) {
		FAIL("Playing order read failed (%d)", err);
		return;
	}

	g_playing_order = order;
	SET_FLAG(playing_order_read);
}

static void mcc_set_playing_order_cb(struct bt_conn *conn, int err, uint8_t order)
{
	if (err) {
		FAIL("Playing order set failed (%d)", err);
		return;
	}

	g_playing_order = order;
	SET_FLAG(playing_order_set);
}

static void mcc_read_playing_orders_supported_cb(struct bt_conn *conn, int err,
						 uint16_t orders)
{
	if (err) {
		FAIL("Playing orders supported read failed (%d)", err);
		return;
	}

	SET_FLAG(playing_orders_supported_read);
}

static void mcc_read_media_state_cb(struct bt_conn *conn, int err, uint8_t state)
{
	if (err) {
		FAIL("Media State read failed (%d)", err);
		return;
	}

	g_state = state;
	SET_FLAG(media_state_read);
}

static void mcc_send_command_cb(struct bt_conn *conn, int err, const struct mpl_cmd *cmd)
{
	if (err) {
		FAIL("Command send failed (%d) - opcode: %u, param: %d",
		     err, cmd->opcode, cmd->param);
		return;
	}

	SET_FLAG(command_sent);
}

static void mcc_cmd_ntf_cb(struct bt_conn *conn, int err, const struct mpl_cmd_ntf *ntf)
{
	if (err) {
		FAIL("Command notification error (%d) - opcode: %u, result: %u",
		     err, ntf->requested_opcode, ntf->result_code);
		return;
	}

	g_command_result = ntf->result_code;
	SET_FLAG(command_notified);
}

static void mcc_read_opcodes_supported_cb(struct bt_conn *conn, int err,
					  uint32_t opcodes)
{
	if (err != 0) {
		FAIL("Media State read failed (%d)", err);
		return;
	}

	g_supported_opcodes = opcodes;
	SET_FLAG(supported_opcodes_read);
}

static void mcc_send_search_cb(struct bt_conn *conn, int err,
			       const struct mpl_search *search)
{
	if (err) {
		FAIL("Search send failed (%d)", err);
		return;
	}

	SET_FLAG(search_sent);
}

static void mcc_search_ntf_cb(struct bt_conn *conn, int err, uint8_t result_code)
{
	if (err) {
		FAIL("Search notification error (%d), result code: %u",
		     err, result_code);
		return;
	}

	g_search_result = result_code;
	SET_FLAG(search_notified);
}

static void mcc_read_search_results_obj_id_cb(struct bt_conn *conn, int err,
					      uint64_t id)
{
	if (err) {
		FAIL("Search Results Object ID read failed (%d)", err);
		return;
	}

	g_search_results_object_id = id;
	SET_FLAG(search_results_object_id_read);
}

static void mcc_read_content_control_id_cb(struct bt_conn *conn, int err, uint8_t ccid)
{
	if (err) {
		FAIL("Content control ID read failed (%d)", err);
		return;
	}

	SET_FLAG(ccid_read);
}

static void mcc_otc_obj_selected_cb(struct bt_conn *conn, int err)
{
	if (err) {
		FAIL("Selecting object failed (%d)\n", err);
		return;
	}

	SET_FLAG(object_selected);
}

static void mcc_otc_obj_metadata_cb(struct bt_conn *conn, int err)
{
	if (err) {
		FAIL("Reading object metadata failed (%d)\n", err);
		return;
	}

	SET_FLAG(metadata_read);
}

static void mcc_icon_object_read_cb(struct bt_conn *conn, int err,
				    struct net_buf_simple *buf)
{
	if (err) {
		FAIL("Reading Icon Object failed (%d)", err);
		return;
	}

	SET_FLAG(object_read);
}

static void mcc_track_segments_object_read_cb(struct bt_conn *conn, int err,
					      struct net_buf_simple *buf)
{
	if (err) {
		FAIL("Reading Track Segments Object failed (%d)", err);
		return;
	}

	SET_FLAG(object_read);
}

static void mcc_otc_read_current_track_object_cb(struct bt_conn *conn, int err,
						 struct net_buf_simple *buf)
{
	if (err) {
		FAIL("Current Track Object read failed (%d)", err);
		return;
	}

	SET_FLAG(object_read);
}

static void mcc_otc_read_next_track_object_cb(struct bt_conn *conn, int err,
						 struct net_buf_simple *buf)
{
	if (err) {
		FAIL("Next Track Object read failed (%d)", err);
		return;
	}

	SET_FLAG(object_read);
}

static void mcc_otc_read_parent_group_object_cb(struct bt_conn *conn, int err,
						struct net_buf_simple *buf)
{
	if (err) {
		FAIL("Parent Group Object read failed (%d)", err);
		return;
	}

	SET_FLAG(object_read);
}

static void mcc_otc_read_current_group_object_cb(struct bt_conn *conn, int err,
						 struct net_buf_simple *buf)
{
	if (err) {
		FAIL("Current Group Object read failed (%d)", err);
		return;
	}

	SET_FLAG(object_read);
}

int do_mcc_init(void)
{
	/* Set up the callbacks */
	mcc_cb.discover_mcs                  = mcc_discover_mcs_cb;
	mcc_cb.read_player_name              = mcc_read_player_name_cb;
	mcc_cb.read_icon_obj_id              = mcc_read_icon_obj_id_cb;
	mcc_cb.read_icon_url                 = mcc_read_icon_url_cb;
	mcc_cb.track_changed_ntf             = mcc_track_changed_ntf_cb;
	mcc_cb.read_track_title              = mcc_read_track_title_cb;
	mcc_cb.read_track_duration           = mcc_read_track_duration_cb;
	mcc_cb.read_track_position           = mcc_read_track_position_cb;
	mcc_cb.set_track_position            = mcc_set_track_position_cb;
	mcc_cb.read_playback_speed           = mcc_read_playback_speed_cb;
	mcc_cb.set_playback_speed            = mcc_set_playback_speed_cb;
	mcc_cb.read_seeking_speed            = mcc_read_seeking_speed_cb;
	mcc_cb.read_segments_obj_id          = mcc_read_segments_obj_id_cb;
	mcc_cb.read_current_track_obj_id     = mcc_read_current_track_obj_id_cb;
	mcc_cb.set_current_track_obj_id      = mcc_set_current_track_obj_id_cb;
	mcc_cb.read_next_track_obj_id        = mcc_read_next_track_obj_id_cb;
	mcc_cb.set_next_track_obj_id         = mcc_set_next_track_obj_id_cb;
	mcc_cb.read_current_group_obj_id     = mcc_read_current_group_obj_id_cb;
	mcc_cb.set_current_group_obj_id      = mcc_set_current_group_obj_id_cb;
	mcc_cb.read_parent_group_obj_id      = mcc_read_parent_group_obj_id_cb;
	mcc_cb.read_playing_order            = mcc_read_playing_order_cb;
	mcc_cb.set_playing_order             = mcc_set_playing_order_cb;
	mcc_cb.read_playing_orders_supported = mcc_read_playing_orders_supported_cb;
	mcc_cb.read_media_state              = mcc_read_media_state_cb;
	mcc_cb.send_cmd                      = mcc_send_command_cb;
	mcc_cb.cmd_ntf                       = mcc_cmd_ntf_cb;
	mcc_cb.read_opcodes_supported        = mcc_read_opcodes_supported_cb;
	mcc_cb.send_search                   = mcc_send_search_cb;
	mcc_cb.search_ntf                    = mcc_search_ntf_cb;
	mcc_cb.read_search_results_obj_id    = mcc_read_search_results_obj_id_cb;
	mcc_cb.read_content_control_id       = mcc_read_content_control_id_cb;
	mcc_cb.otc_obj_selected              = mcc_otc_obj_selected_cb;
	mcc_cb.otc_obj_metadata              = mcc_otc_obj_metadata_cb;
	mcc_cb.otc_icon_object               = mcc_icon_object_read_cb;
	mcc_cb.otc_track_segments_object     = mcc_track_segments_object_read_cb;
	mcc_cb.otc_current_track_object      = mcc_otc_read_current_track_object_cb;
	mcc_cb.otc_next_track_object         = mcc_otc_read_next_track_object_cb;
	mcc_cb.otc_current_group_object      = mcc_otc_read_current_group_object_cb;
	mcc_cb.otc_parent_group_object       = mcc_otc_read_parent_group_object_cb;

	/* Initialize the module */
	return bt_mcc_init(&mcc_cb);
}

/* Callback after Bluetoot initialization attempt */
static void bt_ready(int err)
{
	if (err) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	SET_FLAG(ble_is_initialized);
}

/* Helper function - select object and read the object metadata
 *
 * Will FAIL the test on errors calling select and read metadata.
 * Will WAIT (hang) until callbacks are received.
 * If callbacks are not received, the test fill FAIL due to timeout.
 *
 * @param object_id    ID of the object to select and read metadata for
 */
static void test_select_obj_id(uint64_t id)
{
	uint64_t invalid_id;
	int err;

	/* Invalid behavior */
	err = bt_ots_client_select_id(NULL, default_conn, id);
	if (err == 0) {
		FAIL("bt_ots_client_select_id did not fail with NULL OTS instance");
		return;
	}

	err = bt_ots_client_select_id(bt_mcc_otc_inst(default_conn), NULL, id);
	if (err == 0) {
		FAIL("bt_ots_client_select_id did not fail with NULL conn");
		return;
	}

	invalid_id = BT_OTS_OBJ_ID_MIN - 1;

	err = bt_ots_client_select_id(bt_mcc_otc_inst(default_conn), default_conn, invalid_id);
	if (err == 0) {
		FAIL("bt_ots_client_select_id did not fail with invalid ID 0x%016llx",
		     invalid_id);
		return;
	}

	invalid_id = BT_OTS_OBJ_ID_MAX + 1;

	err = bt_ots_client_select_id(bt_mcc_otc_inst(default_conn), default_conn, invalid_id);
	if (err == 0) {
		FAIL("bt_ots_client_select_id did not fail with invalid ID 0x%016llx",
		     invalid_id);
		return;
	}

	/* Valid behavior */
	UNSET_FLAG(object_selected);
	err = bt_ots_client_select_id(bt_mcc_otc_inst(default_conn),
				      default_conn, id);
	if (err) {
		FAIL("Failed to select object\n");
		return;
	}

	WAIT_FOR_FLAG(object_selected);
	printk("Selecting object succeeded\n");
}

static void test_read_object_meta(void)
{
	int err;

	/* Invalid behavior */
	err = bt_mcc_otc_read_object_metadata(NULL);
	if (err == 0) {
		FAIL("bt_mcc_otc_read_object_metadata did not fail with NULL conn");
		return;
	}

	/* Valid behavior */

	UNSET_FLAG(metadata_read);
	err = bt_mcc_otc_read_object_metadata(default_conn);
	if (err) {
		FAIL("Failed to read object metadata\n");
		return;
	}

	WAIT_FOR_FLAG(metadata_read);
	printk("Reading object metadata succeeded\n");
}

/* Helper function to read the media state and verify that it is as expected
 * Will FAIL on error reading the media state
 * Will FAIL if the state is not as expected
 *
 * Returns true if the state is as expected
 * Returns false in case of errors, or if the state is not as expected
 */
static bool test_verify_media_state_wait_flags(uint8_t expected_state)
{
	int err;

	UNSET_FLAG(media_state_read);
	err = bt_mcc_read_media_state(default_conn);
	if (err) {
		FAIL("Failed to read media state: %d", err);
		return false;
	}

	WAIT_FOR_FLAG(media_state_read);
	if (g_state != expected_state) {
		FAIL("Server is not in expected state: %d, expected: %d\n",
		     g_state, expected_state);
		return false;
	}

	return true;
}

static void test_read_supported_opcodes(void)
{
	int err;

	/* Invalid behavior */
	err = bt_mcc_read_opcodes_supported(NULL);
	if (err == 0) {
		FAIL("bt_mcc_read_opcodes_supported did not fail with NULL conn");
		return;
	}

	/* Valid behavior */
	UNSET_FLAG(supported_opcodes_read);
	err = bt_mcc_read_opcodes_supported(default_conn);
	if (err != 0) {
		FAIL("Failed to read supported opcodes: %d", err);
		return;
	}

	WAIT_FOR_FLAG(supported_opcodes_read);
	printk("Supported opcodes read succeeded\n");
}

/* This will only test invalid behavior for send_cmd as valid behavior is
 * tested by test_send_cmd_wait_flags
 */
static void test_invalid_send_cmd(void)
{
	struct mpl_cmd cmd = { 0 };
	int err;

	err = bt_mcc_send_cmd(NULL, &cmd);
	if (err == 0) {
		FAIL("bt_mcc_send_cmd did not fail with NULL conn");
		return;
	}

	err = bt_mcc_send_cmd(default_conn, NULL);
	if (err == 0) {
		FAIL("bt_mcc_send_cmd did not fail with NULL cmd");
		return;
	}

	cmd.opcode = 0; /* Invalid opcode */

	err = bt_mcc_send_cmd(default_conn, &cmd);
	if (err == 0) {
		FAIL("bt_mcc_send_cmd did not fail with invalid opcode %u", cmd.opcode);
		return;
	}

	cmd.opcode = 0x80; /* Invalid opcode */

	err = bt_mcc_send_cmd(default_conn, &cmd);
	if (err == 0) {
		FAIL("bt_mcc_send_cmd did not fail with invalid opcode %u", cmd.opcode);
		return;
	}
}

/* Helper function to write commands to to the control point, including the
 * flag handling.
 * Will FAIL on error to send the command.
 * Will WAIT for the required flags before returning.
 */
static void test_send_cmd_wait_flags(struct mpl_cmd *cmd)
{
	int err;

	/* Need both flags, even if the notification result is what we care
	 * about.  The notification may come before the write callback, and if
	 * the write callback has not yet arrived, we will get EBUSY at the
	 * next call.
	 */
	UNSET_FLAG(command_sent);
	UNSET_FLAG(command_notified);
	err = bt_mcc_send_cmd(default_conn, cmd);
	if (err) {
		FAIL("Failed to send command: %d, opcode: %u",
		     err, cmd->opcode);
		return;
	}

	WAIT_FOR_FLAG(command_sent);
	WAIT_FOR_FLAG(command_notified);
}

static void test_cp_play(void)
{
	struct mpl_cmd cmd;

	cmd.opcode = BT_MCS_OPC_PLAY;
	cmd.use_param = false;

	test_send_cmd_wait_flags(&cmd);

	if (g_command_result != BT_MCS_OPC_NTF_SUCCESS) {
		FAIL("PLAY command failed\n");
		return;
	}

	if (test_verify_media_state_wait_flags(BT_MCS_MEDIA_STATE_PLAYING)) {
		printk("PLAY command succeeded\n");
	}
}

static void test_cp_pause(void)
{
	struct mpl_cmd cmd;

	cmd.opcode = BT_MCS_OPC_PAUSE;
	cmd.use_param = false;

	test_send_cmd_wait_flags(&cmd);

	if (g_command_result != BT_MCS_OPC_NTF_SUCCESS) {
		FAIL("PAUSE command failed\n");
		return;
	}

	if (test_verify_media_state_wait_flags(BT_MCS_MEDIA_STATE_PAUSED)) {
		printk("PAUSE command succeeded\n");
	}
}

static void test_cp_fast_rewind(void)
{
	struct mpl_cmd cmd;

	cmd.opcode = BT_MCS_OPC_FAST_REWIND;
	cmd.use_param = false;

	test_send_cmd_wait_flags(&cmd);

	if (g_command_result != BT_MCS_OPC_NTF_SUCCESS) {
		FAIL("FAST REWIND command failed\n");
		return;
	}

	if (test_verify_media_state_wait_flags(BT_MCS_MEDIA_STATE_SEEKING)) {
		printk("FAST REWIND command succeeded\n");
	}
}

static void test_cp_fast_forward(void)
{
	struct mpl_cmd cmd;

	cmd.opcode = BT_MCS_OPC_FAST_FORWARD;
	cmd.use_param = false;

	test_send_cmd_wait_flags(&cmd);

	if (g_command_result != BT_MCS_OPC_NTF_SUCCESS) {
		FAIL("FAST FORWARD command failed\n");
		return;
	}

	if (test_verify_media_state_wait_flags(BT_MCS_MEDIA_STATE_SEEKING)) {
		printk("FAST FORWARD command succeeded\n");
	}
}

static void test_cp_stop(void)
{
	struct mpl_cmd cmd;

	cmd.opcode = BT_MCS_OPC_STOP;
	cmd.use_param = false;

	test_send_cmd_wait_flags(&cmd);

	if (g_command_result != BT_MCS_OPC_NTF_SUCCESS) {
		FAIL("STOP command failed\n");
		return;
	}

	/* There is no "STOPPED" state in the spec - STOP goes to PAUSED */
	if (test_verify_media_state_wait_flags(BT_MCS_MEDIA_STATE_PAUSED)) {
		printk("STOP command succeeded\n");
	}
}

static void test_cp_move_relative(void)
{
	int err;
	struct mpl_cmd cmd;

	/* Assumes that the server is in a state where it is  able to change
	 * the current track position
	 * Also assumes position will not change by itself, which is wrong if
	 * if the player is playing.
	 */
	UNSET_FLAG(track_position_read);
	err = bt_mcc_read_track_position(default_conn);
	if (err) {
		FAIL("Failed to read track position: %d\n", err);
		return;
	}

	WAIT_FOR_FLAG(track_position_read);
	uint32_t tmp_pos = g_pos;

	cmd.opcode = BT_MCS_OPC_MOVE_RELATIVE;
	cmd.use_param = true;
	cmd.param = 1000;  /* Position change, measured in 1/100 of a second */

	test_send_cmd_wait_flags(&cmd);

	if (g_command_result != BT_MCS_OPC_NTF_SUCCESS) {
		FAIL("MOVE RELATIVE command failed\n");
		return;
	}

	UNSET_FLAG(track_position_read);
	err = bt_mcc_read_track_position(default_conn);
	if (err) {
		FAIL("Failed to read track position: %d\n", err);
		return;
	}

	WAIT_FOR_FLAG(track_position_read);
	if (g_pos == tmp_pos) {
		/* Position did not change */
		FAIL("Server did not move track position\n");
		return;
	}

	printk("MOVE RELATIVE command succeeded\n");
}

static void test_cp_prev_segment(void)
{
	struct mpl_cmd cmd;

	/* Assumes that the server is in a state where there is a current
	 * track that has segments, and where the server may switch between
	 * these
	 */

	/* To properly verify track segment changes, the track segments
	 * object must be downloaded and parsed.  That is somewhat complex,
	 * and is getting close to what the qualification tests do.
	 * Alternatively, the track position may be checked, but the server
	 * implementation does not set that for segment changes yet.
	 * For now, we will settle for seeing that the opcodes are accepted.
	 */

	cmd.opcode = BT_MCS_OPC_PREV_SEGMENT;
	cmd.use_param = false;

	test_send_cmd_wait_flags(&cmd);

	if (g_command_result != BT_MCS_OPC_NTF_SUCCESS) {
		FAIL("PREV SEGMENT command failed\n");
		return;
	}

	printk("PREV SEGMENT command succeeded\n");
}

static void test_cp_next_segment(void)
{
	struct mpl_cmd cmd;

	cmd.opcode = BT_MCS_OPC_NEXT_SEGMENT;
	cmd.use_param = false;

	test_send_cmd_wait_flags(&cmd);

	if (g_command_result != BT_MCS_OPC_NTF_SUCCESS) {
		FAIL("NEXT SEGMENT command failed\n");
		return;
	}

	printk("NEXT SEGMENT command succeeded\n");
}

static void test_cp_first_segment(void)
{
	struct mpl_cmd cmd;

	cmd.opcode = BT_MCS_OPC_FIRST_SEGMENT;
	cmd.use_param = false;

	test_send_cmd_wait_flags(&cmd);

	if (g_command_result != BT_MCS_OPC_NTF_SUCCESS) {
		FAIL("FIRST SEGMENT command failed\n");
		return;
	}

	printk("FIRST SEGMENT command succeeded\n");
}

static void test_cp_last_segment(void)
{
	struct mpl_cmd cmd;

	cmd.opcode = BT_MCS_OPC_LAST_SEGMENT;
	cmd.use_param = false;

	test_send_cmd_wait_flags(&cmd);

	if (g_command_result != BT_MCS_OPC_NTF_SUCCESS) {
		FAIL("LAST SEGMENT command failed\n");
		return;
	}

	printk("LAST SEGMENT command succeeded\n");
}

static void test_cp_goto_segment(void)
{
	struct mpl_cmd cmd;

	cmd.opcode = BT_MCS_OPC_GOTO_SEGMENT;
	cmd.use_param = true;
	cmd.param = 2;    /* Second segment - not the first, maybe not last */

	test_send_cmd_wait_flags(&cmd);

	if (g_command_result != BT_MCS_OPC_NTF_SUCCESS) {
		FAIL("GOTO SEGMENT command failed\n");
		return;
	}

	printk("GOTO SEGMENT command succeeded\n");
}

/* Helper function to read the current track object ID, including flag handling
 * Will FAIL on error reading object ID
 * Will WAIT until the read is completed (object ID flag read flag is set)
 */
static void test_read_current_track_object_id_wait_flags(void)
{
	int err;

	UNSET_FLAG(current_track_object_id_read);
	err = bt_mcc_read_current_track_obj_id(default_conn);
	if (err) {
		FAIL("Failed to read current track object ID: %d", err);
		return;
	}

	WAIT_FOR_FLAG(current_track_object_id_read);
}

static void test_cp_prev_track(void)
{
	uint64_t object_id;
	struct mpl_cmd cmd;

	/* Assumes that the server is in a state where it has multiple tracks
	 * and can change between them.
	 */

	/* To verify that a track change has happeded, the test checks that the
	 * current track object ID has changed.
	 */

	cmd.opcode = BT_MCS_OPC_PREV_TRACK;
	cmd.use_param = false;

	test_read_current_track_object_id_wait_flags();
	object_id = g_current_track_object_id;

	test_send_cmd_wait_flags(&cmd);

	if (g_command_result != BT_MCS_OPC_NTF_SUCCESS) {
		FAIL("PREV TRACK command failed\n");
		return;
	}

	test_read_current_track_object_id_wait_flags();

	if (g_current_track_object_id == object_id) {
		/* Track did not change */
		FAIL("Server did not change track\n");
		return;
	}

	printk("PREV TRACK command succeeded\n");
}

static void test_cp_next_track_and_track_changed(void)
{
	uint64_t object_id;
	struct mpl_cmd cmd;

	/* This test is also used to test the track changed notification */
	UNSET_FLAG(track_change_notified);

	cmd.opcode = BT_MCS_OPC_NEXT_TRACK;
	cmd.use_param = false;

	test_read_current_track_object_id_wait_flags();
	object_id = g_current_track_object_id;

	test_send_cmd_wait_flags(&cmd);

	if (g_command_result != BT_MCS_OPC_NTF_SUCCESS) {
		FAIL("NEXT TRACK command failed\n");
		return;
	}

	WAIT_FOR_FLAG(track_change_notified);
	printk("Track change notified\n");

	test_read_current_track_object_id_wait_flags();

	if (g_current_track_object_id == object_id) {
		FAIL("Server did not change track\n");
		return;
	}

	printk("NEXT TRACK command succeeded\n");
}

static void test_cp_first_track(void)
{
	uint64_t object_id;
	struct mpl_cmd cmd;

	cmd.opcode = BT_MCS_OPC_FIRST_TRACK;
	cmd.use_param = false;

	test_read_current_track_object_id_wait_flags();
	object_id = g_current_track_object_id;

	test_send_cmd_wait_flags(&cmd);

	if (g_command_result != BT_MCS_OPC_NTF_SUCCESS) {
		FAIL("FIRST TRACK command failed\n");
		return;
	}

	test_read_current_track_object_id_wait_flags();

	if (g_current_track_object_id == object_id) {
		FAIL("Server did not change track\n");
		return;
	}

	printk("FIRST TRACK command succeeded\n");
}

static void test_cp_last_track(void)
{
	uint64_t object_id;
	struct mpl_cmd cmd;

	cmd.opcode = BT_MCS_OPC_LAST_TRACK;
	cmd.use_param = false;

	test_read_current_track_object_id_wait_flags();
	object_id = g_current_track_object_id;

	test_send_cmd_wait_flags(&cmd);

	if (g_command_result != BT_MCS_OPC_NTF_SUCCESS) {
		FAIL("LAST TRACK command failed\n");
		return;
	}

	test_read_current_track_object_id_wait_flags();

	if (g_current_track_object_id == object_id) {
		FAIL("Server did not change track\n");
		return;
	}

	printk("LAST TRACK command succeeded\n");
}

static void test_cp_goto_track(void)
{
	uint64_t object_id;
	struct mpl_cmd cmd;

	cmd.opcode = BT_MCS_OPC_GOTO_TRACK;
	cmd.use_param = true;
	cmd.param = 2; /* Second track, not the first, maybe not the last */

	test_read_current_track_object_id_wait_flags();
	object_id = g_current_track_object_id;

	test_send_cmd_wait_flags(&cmd);

	if (g_command_result != BT_MCS_OPC_NTF_SUCCESS) {
		FAIL("GOTO TRACK command failed\n");
		return;
	}

	test_read_current_track_object_id_wait_flags();

	if (g_current_track_object_id == object_id) {
		FAIL("Server did not change track\n");
		return;
	}

	printk("GOTO TRACK command succeeded\n");
}

/* Helper function to read the current group object ID, including flag handling
 * Will FAIL on error reading object ID
 * Will WAIT until the read is completed (object ID flag read flag is set)
 */
static void test_read_current_group_object_id_wait_flags(void)
{
	int err;

	UNSET_FLAG(current_group_object_id_read);
	err = bt_mcc_read_current_group_obj_id(default_conn);
	if (err) {
		FAIL("Failed to read current group object ID: %d", err);
		return;
	}

	WAIT_FOR_FLAG(current_group_object_id_read);
}

static void test_cp_prev_group(void)
{
	uint64_t object_id;
	struct mpl_cmd cmd;

	/* Assumes that the server is in a state where it has multiple groups
	 * and can change between them.
	 */

	/* To verify that a group change has happeded, the test checks that the
	 * current group object ID has changed.
	 */

	cmd.opcode = BT_MCS_OPC_PREV_GROUP;
	cmd.use_param = false;

	test_read_current_group_object_id_wait_flags();
	object_id = g_current_group_object_id;

	test_send_cmd_wait_flags(&cmd);

	if (g_command_result != BT_MCS_OPC_NTF_SUCCESS) {
		FAIL("PREV GROUP command failed\n");
		return;
	}

	test_read_current_group_object_id_wait_flags();

	if (g_current_group_object_id == object_id) {
		/* Group did not change */
		FAIL("Server did not change group\n");
		return;
	}

	printk("PREV GROUP command succeeded\n");
}

static void test_cp_next_group(void)
{
	uint64_t object_id;
	struct mpl_cmd cmd;

	cmd.opcode = BT_MCS_OPC_NEXT_GROUP;
	cmd.use_param = false;

	test_read_current_group_object_id_wait_flags();
	object_id = g_current_group_object_id;

	test_send_cmd_wait_flags(&cmd);

	if (g_command_result != BT_MCS_OPC_NTF_SUCCESS) {
		FAIL("NEXT GROUP command failed\n");
		return;
	}

	test_read_current_group_object_id_wait_flags();

	if (g_current_group_object_id == object_id) {
		FAIL("Server did not change group\n");
		return;
	}

	printk("NEXT GROUP command succeeded\n");
}

static void test_cp_first_group(void)
{
	uint64_t object_id;
	struct mpl_cmd cmd;

	cmd.opcode = BT_MCS_OPC_FIRST_GROUP;
	cmd.use_param = false;

	test_read_current_group_object_id_wait_flags();
	object_id = g_current_group_object_id;

	test_send_cmd_wait_flags(&cmd);

	if (g_command_result != BT_MCS_OPC_NTF_SUCCESS) {
		FAIL("FIRST GROUP command failed\n");
		return;
	}

	test_read_current_group_object_id_wait_flags();

	if (g_current_group_object_id == object_id) {
		FAIL("Server did not change group\n");
		return;
	}

	printk("FIRST GROUP command succeeded\n");
}

static void test_cp_last_group(void)
{
	uint64_t object_id;
	struct mpl_cmd cmd;

	cmd.opcode = BT_MCS_OPC_LAST_GROUP;
	cmd.use_param = false;

	test_read_current_group_object_id_wait_flags();
	object_id = g_current_group_object_id;

	test_send_cmd_wait_flags(&cmd);

	if (g_command_result != BT_MCS_OPC_NTF_SUCCESS) {
		FAIL("LAST GROUP command failed\n");
		return;
	}

	test_read_current_group_object_id_wait_flags();

	if (g_current_group_object_id == object_id) {
		FAIL("Server did not change group\n");
		return;
	}

	printk("LAST GROUP command succeeded\n");
}

static void test_cp_goto_group(void)
{
	uint64_t object_id;
	struct mpl_cmd cmd;

	cmd.opcode = BT_MCS_OPC_GOTO_GROUP;
	cmd.use_param = true;
	cmd.param = 2; /* Second group, not the first, maybe not the last */

	test_read_current_group_object_id_wait_flags();
	object_id = g_current_group_object_id;

	test_send_cmd_wait_flags(&cmd);

	if (g_command_result != BT_MCS_OPC_NTF_SUCCESS) {
		FAIL("GOTO GROUP command failed\n");
		return;
	}

	test_read_current_group_object_id_wait_flags();

	if (g_current_group_object_id == object_id) {
		FAIL("Server did not change group\n");
		return;
	}

	printk("GOTO GROUP command succeeded\n");
}

static void test_search(void)
{
	struct mpl_search search = { 0 };
	struct mpl_sci sci = {0};
	int err;

	/* Invalid behavior */
	err = bt_mcc_read_search_results_obj_id(NULL);
	if (err == 0) {
		FAIL("bt_mcc_read_search_results_obj_id did not fail with NULL conn");
		return;
	}

	err = bt_mcc_send_search(NULL, &search);
	if (err == 0) {
		FAIL("bt_mcc_send_search did not fail with NULL conn");
		return;
	}

	err = bt_mcc_send_search(default_conn, NULL);
	if (err == 0) {
		FAIL("bt_mcc_send_search did not fail with NULL search");
		return;
	}

	search.len = SEARCH_LEN_MAX + 1;

	err = bt_mcc_send_search(default_conn, &search);
	if (err == 0) {
		FAIL("bt_mcc_send_search did not fail with search len above max");
		return;
	}

	search.len = SEARCH_LEN_MIN - 1;

	err = bt_mcc_send_search(default_conn, &search);
	if (err == 0) {
		FAIL("bt_mcc_send_search did not fail with search len below min");
		return;
	}

	/* Valid behavior */
	/* Test outline:
	 * - verify that the search results object ID is zero before search
	 * - write a search (one search control item) to the search control point,
	 *   get write callback and notification
	 * - verify that the search results object ID is non-zero
	 */

	UNSET_FLAG(search_results_object_id_read);
	err = bt_mcc_read_search_results_obj_id(default_conn);

	if (err) {
		FAIL("Failed to read search results object ID: %d", err);
		return;
	}

	WAIT_FOR_FLAG(search_results_object_id_read);

	if (g_search_results_object_id != 0) {
		FAIL("Search results object ID not zero before search\n");
		return;
	}

	/* Set up the search control item, then the search
	 *  Note: As of now, the server implementation only fakes the search,
	 * so it makes no difference what we search for.  The result is the
	 * same anyway.
	 */
	sci.type = BT_MCS_SEARCH_TYPE_TRACK_NAME;
	strcpy(sci.param, "Some track name");
	/* Length is length of type, plus length of param w/o termination */
	sci.len = sizeof(sci.type) + strlen(sci.param);

	search.len = 0;
	memcpy(&search.search[search.len], &sci.len, sizeof(sci.len));
	search.len += sizeof(sci.len);

	memcpy(&search.search[search.len], &sci.type, sizeof(sci.type));
	search.len += sizeof(sci.type);

	memcpy(&search.search[search.len], &sci.param, strlen(sci.param));
	search.len += strlen(sci.param);

	UNSET_FLAG(search_sent);
	UNSET_FLAG(search_notified);
	UNSET_FLAG(search_results_object_id_read);

	err = bt_mcc_send_search(default_conn, &search);
	if (err) {
		FAIL("Failed to write to search control point\n");
		return;
	}

	WAIT_FOR_FLAG(search_sent);
	WAIT_FOR_FLAG(search_notified);

	if (g_search_result != BT_MCS_SCP_NTF_SUCCESS) {
		FAIL("SEARCH operation failed\n");
		return;
	}

	/* A search results object will have been created and the search
	 * results object ID will have been notified if the search gave results
	 */
	WAIT_FOR_FLAG(search_results_object_id_read);
	if (g_search_results_object_id == 0) {
		FAIL("No search results\n");
		return;
	}

	printk("SEARCH operation succeeded\n");
}

static void test_discover(void)
{
	int err;

	/* Invalid behavior */
	err = bt_mcc_discover_mcs(NULL, true);
	if (err == 0) {
		FAIL("bt_mcc_discover_mcs did not fail with NULL conn");
		return;
	}

	/* Valid behavior */
	UNSET_FLAG(discovery_done);

	err = bt_mcc_discover_mcs(default_conn, true);
	if (err != 0) {
		FAIL("Failed to start discovery of MCS: %d\n", err);
	}

	WAIT_FOR_FLAG(discovery_done);
	printk("Discovery of MCS succeeded\n");
}

static void test_read_player_name(void)
{
	int err;

	/* Invalid behavior */
	err = bt_mcc_read_player_name(NULL);
	if (err == 0) {
		FAIL("bt_mcc_read_player_name did not fail with NULL conn");
		return;
	}

	/* Valid behavior */
	UNSET_FLAG(player_name_read);

	err = bt_mcc_read_player_name(default_conn);
	if (err != 0) {
		FAIL("Failed to read media player name ID: %d", err);
		return;
	}

	WAIT_FOR_FLAG(player_name_read);
	printk("Player Name read succeeded\n");
}

static void test_read_icon_obj_id(void)
{
	int err;

	/* Invalid behavior */
	err = bt_mcc_read_icon_obj_id(NULL);
	if (err == 0) {
		FAIL("bt_mcc_read_icon_obj_id did not fail with NULL conn");
		return;
	}

	/* Valid behavior */
	UNSET_FLAG(icon_object_id_read);

	err = bt_mcc_read_icon_obj_id(default_conn);
	if (err != 0) {
		FAIL("Failed to read icon object ID: %d", err);
		return;
	}

	WAIT_FOR_FLAG(icon_object_id_read);
	printk("Icon Object ID read succeeded\n");
}

static void test_read_icon_obj(void)
{
	int err;

	/* Invalid behavior */
	err = bt_mcc_otc_read_icon_object(NULL);
	if (err == 0) {
		FAIL("bt_mcc_otc_read_icon_object did not fail with NULL conn");
		return;
	}

	/* Valid behavior */
	UNSET_FLAG(object_read);

	err = bt_mcc_otc_read_icon_object(default_conn);

	if (err != 0) {
		FAIL("Failed to read icon object\n");
		return;
	}

	WAIT_FOR_FLAG(object_read);
	printk("Reading Icon Object succeeded\n");
}

static void test_read_icon_url(void)
{
	int err;

	/* Invalid behavior */
	err = bt_mcc_read_icon_url(NULL);
	if (err == 0) {
		FAIL("bt_mcc_read_icon_url did not fail with NULL conn");
		return;
	}

	/* Valid behavior */
	UNSET_FLAG(icon_url_read);

	err = bt_mcc_read_icon_url(default_conn);
	if (err != 0) {
		FAIL("Failed to read icon url: %d", err);
		return;
	}

	WAIT_FOR_FLAG(icon_url_read);
	printk("Icon URL read succeeded\n");
}

static void test_read_track_title(void)
{
	int err;

	/* Invalid behavior */
	err = bt_mcc_read_track_title(NULL);
	if (err == 0) {
		FAIL("bt_mcc_read_track_title did not fail with NULL conn");
		return;
	}

	/* Valid behavior */
	UNSET_FLAG(track_title_read);

	err = bt_mcc_read_track_title(default_conn);
	if (err != 0) {
		FAIL("Failed to read track_title: %d", err);
		return;
	}

	WAIT_FOR_FLAG(track_title_read);
	printk("Track title read succeeded\n");
}

static void test_read_track_duration(void)
{
	int err;

	/* Invalid behavior */
	err = bt_mcc_read_track_duration(NULL);
	if (err == 0) {
		FAIL("bt_mcc_read_track_duration did not fail with NULL conn");
		return;
	}

	/* Valid behavior */
	UNSET_FLAG(track_duration_read);

	err = bt_mcc_read_track_duration(default_conn);
	if (err != 0) {
		FAIL("Failed to read track_duration: %d", err);
		return;
	}

	WAIT_FOR_FLAG(track_duration_read);
	printk("Track duration read succeeded\n");
}

static void test_read_track_position(void)
{
	int err;

	/* Invalid behavior */
	err = bt_mcc_read_track_position(NULL);
	if (err == 0) {
		FAIL("bt_mcc_read_track_position did not fail with NULL conn");
		return;
	}

	/* Valid behavior */
	UNSET_FLAG(track_position_read);

	err = bt_mcc_read_track_position(default_conn);
	if (err != 0) {
		FAIL("Failed to read track position: %d", err);
		return;
	}

	WAIT_FOR_FLAG(track_position_read);
	printk("Track position read succeeded\n");
}

static void test_write_track_position(int32_t pos)
{
	int err;

	/* Invalid behavior - There are no invalid positions to test so only test conn */
	err = bt_mcc_set_track_position(NULL, pos);
	if (err == 0) {
		FAIL("bt_mcc_set_track_position did not fail with NULL conn");
		return;
	}

	/* Valid behavior */
	UNSET_FLAG(track_position_set);

	err = bt_mcc_set_track_position(default_conn, pos);
	if (err != 0) {
		FAIL("Failed to set track position: %d", err);
		return;
	}

	WAIT_FOR_FLAG(track_position_set);
	if (g_pos != pos) {
		/* In this controlled case, we expect that the resulting */
		/* position is the position given in the set command */
		FAIL("Track position set failed: Incorrect position\n");
	}

	printk("Track position set succeeded\n");
}

static void test_read_playback_speed(void)
{
	int err;

	/* Invalid behavior */
	err = bt_mcc_read_playback_speed(NULL);
	if (err == 0) {
		FAIL("bt_mcc_read_playback_speed did not fail with NULL conn");
		return;
	}

	/* Valid behavior */
	UNSET_FLAG(playback_speed_read);

	err = bt_mcc_read_playback_speed(default_conn);
	if (err != 0) {
		FAIL("Failed to read playback speed: %d", err);
		return;
	}

	WAIT_FOR_FLAG(playback_speed_read);
	printk("Playback speed read succeeded\n");
}

static void test_set_playback_speed(int8_t pb_speed)
{
	int err;

	/* Invalid behavior - There are no invalid speeds to test so only test conn */
	err = bt_mcc_set_playback_speed(NULL, pb_speed);
	if (err == 0) {
		FAIL("bt_mcc_set_playback_speed did not fail with NULL conn");
		return;
	}

	/* Valid behavior */
	UNSET_FLAG(playback_speed_set);

	err = bt_mcc_set_playback_speed(default_conn, pb_speed);
	if (err != 0) {
		FAIL("Failed to set playback speed: %d", err);
		return;
	}

	WAIT_FOR_FLAG(playback_speed_set);
	if (g_pb_speed != pb_speed) {
		FAIL("Playback speed failed: Incorrect playback speed\n");
	}

	printk("Playback speed set succeeded\n");
}

static void test_read_seeking_speed(void)
{
	int err;

	/* Invalid behavior */
	err = bt_mcc_read_seeking_speed(NULL);
	if (err == 0) {
		FAIL("bt_mcc_read_seeking_speed did not fail with NULL conn");
		return;
	}

	/* Valid behavior */
	UNSET_FLAG(seeking_speed_read);

	err = bt_mcc_read_seeking_speed(default_conn);
	if (err != 0) {
		FAIL("Failed to read seeking speed: %d", err);
		return;
	}

	WAIT_FOR_FLAG(seeking_speed_read);
	printk("Seeking speed read succeeded\n");
}

static void test_read_track_segments_obj_id(void)
{
	int err;

	/* Invalid behavior */
	err = bt_mcc_read_segments_obj_id(NULL);
	if (err == 0) {
		FAIL("bt_mcc_read_segments_obj_id did not fail with NULL conn");
		return;
	}

	/* Valid behavior */
	UNSET_FLAG(track_segments_object_id_read);

	err = bt_mcc_read_segments_obj_id(default_conn);
	if (err != 0) {
		FAIL("Failed to read track segments object ID: %d", err);
		return;
	}

	WAIT_FOR_FLAG(track_segments_object_id_read);
	printk("Track Segments Object ID read succeeded\n");
}

static void test_read_track_segments_object(void)
{
	int err;

	/* Invalid behavior */
	err = bt_mcc_otc_read_track_segments_object(NULL);
	if (err == 0) {
		FAIL("bt_mcc_otc_read_track_segments_object did not fail with NULL conn");
		return;
	}

	/* Valid behavior */
	UNSET_FLAG(object_read);

	err = bt_mcc_otc_read_track_segments_object(default_conn);
	if (err != 0) {
		FAIL("Failed to read track segments object\n");
		return;
	}

	WAIT_FOR_FLAG(object_read);
	printk("Reading Track Segments Object succeeded\n");
}

static void test_set_current_track_obj_id(uint64_t id)
{
	uint64_t invalid_id;
	int err;

	/* Invalid behavior */
	err = bt_mcc_set_current_track_obj_id(NULL, id);
	if (err == 0) {
		FAIL("bt_mcc_set_current_track_obj_id did not fail with NULL conn");
		return;
	}

	invalid_id = BT_OTS_OBJ_ID_MIN - 1;
	err = bt_mcc_set_current_track_obj_id(default_conn, invalid_id);
	if (err == 0) {
		FAIL("bt_mcc_set_current_track_obj_id did not fail with invalid ID 0x%016llx",
		     invalid_id);
		return;
	}

	invalid_id = BT_OTS_OBJ_ID_MAX + 1;
	err = bt_mcc_set_current_track_obj_id(default_conn, invalid_id);
	if (err == 0) {
		FAIL("bt_mcc_set_current_track_obj_id did not fail with invalid ID 0x%016llx",
		     invalid_id);
		return;
	}

	invalid_id = OTS_OBJ_ID_DIR_LIST;
	err = bt_mcc_set_current_track_obj_id(default_conn, invalid_id);
	if (err == 0) {
		FAIL("bt_mcc_set_current_track_obj_id did not fail with invalid ID 0x%016llx",
		     invalid_id);
		return;
	}

	/* Valid behavior */
	UNSET_FLAG(current_track_object_id_set);

	err = bt_mcc_set_current_track_obj_id(default_conn, id);
	if (err != 0) {
		FAIL("Failed to set current track object ID: %d", err);
		return;
	}

	WAIT_FOR_FLAG(current_track_object_id_set);

	if (g_current_track_object_id != id) {
		FAIL("Current track object ID not the one that was set");
		return;
	}

	printk("Current Track Object ID set succeeded\n");
}

static void test_read_current_track_obj_id(uint64_t expected_id)
{
	int err;

	/* Invalid behavior */
	err = bt_mcc_read_current_track_obj_id(NULL);
	if (err == 0) {
		FAIL("bt_mcc_read_current_track_obj_id did not fail with NULL conn");
		return;
	}

	/* Valid behavior */
	UNSET_FLAG(current_track_object_id_read);

	err = bt_mcc_read_current_track_obj_id(default_conn);
	if (err != 0) {
		FAIL("Failed to read current track object ID: %d", err);
		return;
	}

	WAIT_FOR_FLAG(current_track_object_id_read);

	if (g_current_track_object_id != expected_id) {
		FAIL("Current track object ID not the one that was set");
		return;
	}

	printk("Current Track Object ID read succeeded\n");
}

static void test_read_current_track_object(void)
{
	int err;

	/* Invalid behavior */
	err = bt_mcc_otc_read_current_track_object(NULL);
	if (err == 0) {
		FAIL("bt_mcc_otc_read_current_track_object did not fail with NULL conn");
		return;
	}

	/* Valid behavior */
	UNSET_FLAG(object_read);

	err = bt_mcc_otc_read_current_track_object(default_conn);

	if (err != 0) {
		FAIL("Failed to current track object\n");
		return;
	}

	WAIT_FOR_FLAG(object_read);
	printk("Current Track Object read succeeded\n");
}

static void test_set_next_track_obj_id(uint64_t id)
{
	uint64_t invalid_id;
	int err;

	/* Invalid behavior */
	err = bt_mcc_set_next_track_obj_id(NULL, id);
	if (err == 0) {
		FAIL("bt_mcc_set_next_track_obj_id did not fail with NULL conn");
		return;
	}

	invalid_id = BT_OTS_OBJ_ID_MIN - 1;
	err = bt_mcc_set_next_track_obj_id(default_conn, invalid_id);
	if (err == 0) {
		FAIL("bt_mcc_set_next_track_obj_id did not fail with invalid ID 0x%016llx",
		     invalid_id);
		return;
	}

	invalid_id = BT_OTS_OBJ_ID_MAX + 1;
	err = bt_mcc_set_next_track_obj_id(default_conn, invalid_id);
	if (err == 0) {
		FAIL("bt_mcc_set_next_track_obj_id did not fail with invalid ID 0x%016llx",
		     invalid_id);
		return;
	}

	invalid_id = OTS_OBJ_ID_DIR_LIST;
	err = bt_mcc_set_next_track_obj_id(default_conn, invalid_id);
	if (err == 0) {
		FAIL("bt_mcc_set_next_track_obj_id did not fail with invalid ID 0x%016llx",
		     invalid_id);
		return;
	}

	/* Valid behavior */
	UNSET_FLAG(next_track_object_id_set);

	err = bt_mcc_set_next_track_obj_id(default_conn, id);
	if (err != 0) {
		FAIL("Failed to set next track object ID: %d", err);
		return;
	}

	WAIT_FOR_FLAG(next_track_object_id_set);
	if (g_next_track_object_id != id) {
		FAIL("Next track object ID not the one that was set");
		return;
	}

	printk("Next Track Object ID set succeeded\n");
}

static void test_read_next_track_obj_id(uint64_t expected_id)
{
	int err;

	/* Invalid behavior */
	err = bt_mcc_read_next_track_obj_id(NULL);
	if (err == 0) {
		FAIL("bt_mcc_read_next_track_obj_id did not fail with NULL conn");
		return;
	}

	/* Valid behavior */
	UNSET_FLAG(next_track_object_id_read);

	err = bt_mcc_read_next_track_obj_id(default_conn);
	if (err != 0) {
		FAIL("Failed to read next track object ID: %d", err);
		return;
	}

	WAIT_FOR_FLAG(next_track_object_id_read);
	if (g_next_track_object_id != expected_id) {
		FAIL("Next track object ID not the one that was set");
		return;
	}

	printk("Next Track Object ID read succeeded\n");
}

static void test_read_next_track_object(void)
{
	int err;

	/* Invalid behavior */
	err = bt_mcc_otc_read_next_track_object(NULL);
	if (err == 0) {
		FAIL("bt_mcc_otc_read_next_track_object did not fail with NULL conn");
		return;
	}

	/* Valid behavior */
	UNSET_FLAG(object_read);

	err = bt_mcc_otc_read_next_track_object(default_conn);
	if (err != 0) {
		FAIL("Failed to read next track object\n");
		return;
	}

	WAIT_FOR_FLAG(object_read);
	printk("Next Track Object read succeeded\n");
}

static void test_read_parent_group_obj_id(void)
{
	int err;

	UNSET_FLAG(parent_group_object_id_read);

	err = bt_mcc_read_parent_group_obj_id(default_conn);
	if (err != 0) {
		FAIL("Failed to read parent group object ID: %d", err);
		return;
	}

	WAIT_FOR_FLAG(parent_group_object_id_read);
	printk("Parent Group Object ID read succeeded\n");
}

static void test_read_parent_group_object(void)
{
	int err;

	UNSET_FLAG(object_read);

	err = bt_mcc_otc_read_parent_group_object(default_conn);
	if (err != 0) {
		FAIL("Failed to read parent group object\n");
		return;
	}

	WAIT_FOR_FLAG(object_read);
	printk("Parent Group Object read succeeded\n");
}

static void test_set_current_group_obj_id(uint64_t id)
{
	int err;

	UNSET_FLAG(current_group_object_id_set);

	err = bt_mcc_set_current_group_obj_id(default_conn, id);
	if (err != 0) {
		FAIL("Failed to set current group object ID: %d", err);
		return;
	}

	WAIT_FOR_FLAG(current_group_object_id_set);
	if (g_current_group_object_id != id) {
		FAIL("Current group object ID not the one that was set");
		return;
	}

	printk("Current Group Object ID set succeeded\n");
}

static void test_read_current_group_obj_id(uint64_t expected_id)
{
	int err;

	UNSET_FLAG(current_group_object_id_read);

	err = bt_mcc_read_current_group_obj_id(default_conn);
	if (err != 0) {
		FAIL("Failed to read current group object ID: %d", err);
		return;
	}

	WAIT_FOR_FLAG(current_group_object_id_read);
	if (g_current_group_object_id != expected_id) {
		FAIL("Current group object ID not the one that was set");
		return;
	}

	printk("Current Group Object ID read succeeded\n");
}

static void test_read_current_group_object(void)
{
	int err;

	UNSET_FLAG(object_read);

	err = bt_mcc_otc_read_current_group_object(default_conn);
	if (err != 0) {
		FAIL("Failed to read current group object\n");
		return;
	}

	WAIT_FOR_FLAG(object_read);
	printk("Current Group Object read succeeded\n");
}

static void test_read_playing_order(void)
{
	int err;

	UNSET_FLAG(playing_order_read);

	err = bt_mcc_read_playing_order(default_conn);
	if (err != 0) {
		FAIL("Failed to read playing order: %d", err);
		return;
	}

	WAIT_FOR_FLAG(playing_order_read);
	printk("Playing order read succeeded\n");
}

static void test_set_playing_order(void)
{
	uint8_t new_playing_order;
	int err;

	if (g_playing_order == BT_MCS_PLAYING_ORDER_SHUFFLE_ONCE) {
		new_playing_order = BT_MCS_PLAYING_ORDER_SINGLE_ONCE;
	} else {
		new_playing_order = BT_MCS_PLAYING_ORDER_SHUFFLE_ONCE;
	}

	UNSET_FLAG(playing_order_set);

	err = bt_mcc_set_playing_order(default_conn, new_playing_order);
	if (err != 0) {
		FAIL("Failed to set playing_order: %d", err);
		return;
	}

	WAIT_FOR_FLAG(playing_order_set);
	if (g_playing_order != new_playing_order) {
		FAIL("Playing order set failed: Incorrect playing_order\n");
	}
	printk("Playing order set succeeded\n");
}

static void test_read_playing_orders_supported(void)
{
	int err;

	UNSET_FLAG(playing_orders_supported_read);

	err = bt_mcc_read_playing_orders_supported(default_conn);
	if (err != 0) {
		FAIL("Failed to read playing orders supported: %d", err);
		return;
	}

	WAIT_FOR_FLAG(playing_orders_supported_read);
	printk("Playing orders supported read succeeded\n");
}

static void test_read_media_state(void)
{
	int err;

	UNSET_FLAG(media_state_read);

	err = bt_mcc_read_media_state(default_conn);
	if (err != 0) {
		FAIL("Failed to read media state: %d", err);
		return;
	}

	WAIT_FOR_FLAG(media_state_read);
	printk("Media state read succeeded\n");
}

static void test_read_content_control_id(void)
{
	int err;

	UNSET_FLAG(ccid_read);

	err = bt_mcc_read_content_control_id(default_conn);
	if (err != 0) {
		FAIL("Failed to read content control ID: %d", err);
		return;
	}

	WAIT_FOR_FLAG(ccid_read);
	printk("Content control ID read succeeded\n");
}

/* This function tests all commands in the API in sequence
 * The order of the sequence follows the order of the characterstics in the
 * Media Control Service specification
 */
void test_main(void)
{
	const uint64_t new_current_track_object_id = 0x103;
	const uint64_t new_next_track_object = 0x102;
	uint64_t new_current_group_object_id = 0x10e;
	int err;

	printk("Media Control Client test application.  Board: %s\n", CONFIG_BOARD);

	UNSET_FLAG(ble_is_initialized);
	err = bt_enable(bt_ready);
	if (err != 0) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	WAIT_FOR_FLAG(ble_is_initialized);
	printk("Bluetooth initialized\n");

	/* Connect ******************************************/
	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err != 0) {
		FAIL("Failed to start scanning (err %d\n)", err);
	} else {
		printk("Scanning started successfully\n");
	}

	WAIT_FOR_FLAG(flag_connected);

	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(default_conn), addr, sizeof(addr));
	printk("Connected: %s\n", addr);

	/* Initialize MCC  ********************************************/
	err = do_mcc_init();
	if (err != 0) {
		FAIL("Could not initialize MCC (err %d\n)", err);
	} else {
		printk("MCC init succeeded\n");
	}

	test_discover();
	test_read_media_state();
	test_read_content_control_id();
	test_read_player_name();
	test_read_seeking_speed();
	test_read_playing_orders_supported();
	test_read_supported_opcodes();
	test_read_playing_order();
	test_set_playing_order();
	test_invalid_send_cmd();

	test_read_icon_obj_id();
	test_select_obj_id(g_icon_object_id);
	test_read_object_meta();
	test_read_icon_obj();
	test_read_icon_url();

	/* Track changed ************************************************
	 *
	 * The track changed characteristic is tested as part of the control
	 * point next track test
	 */

	test_read_track_title();
	test_read_track_duration();
	test_read_track_position();

	int32_t pos = g_pos + 1200; /*12 seconds further into the track */

	test_write_track_position(pos);

	test_read_playback_speed();

	int8_t pb_speed = g_pb_speed + 8; /* 2^(8/64) faster than current speed */

	test_set_playback_speed(pb_speed);

	/* Track segments  */
	test_read_track_segments_obj_id();
	test_select_obj_id(g_track_segments_object_id);
	test_read_object_meta();
	test_read_track_segments_object();

	/* Current track */
	test_set_current_track_obj_id(new_current_track_object_id);
	test_read_current_track_obj_id(new_current_track_object_id);
	test_select_obj_id(g_current_track_object_id);
	test_read_object_meta();
	test_read_current_track_object();

	/* Next track */
	test_set_next_track_obj_id(new_next_track_object);
	test_read_next_track_obj_id(new_next_track_object);
	test_select_obj_id(g_next_track_object_id);
	test_read_object_meta();
	test_read_next_track_object();

	/* Parent group */
	test_read_parent_group_obj_id();
	test_select_obj_id(g_parent_group_object_id);
	test_read_object_meta();
	test_read_parent_group_object();

	/* Current group object ******************************************/
	test_set_current_group_obj_id(new_current_group_object_id);
	test_read_current_group_obj_id(new_current_group_object_id);
	test_select_obj_id(g_current_group_object_id);
	test_read_object_meta();
	test_read_current_group_object();

	/* Set current group back to first group, so that later tests (segments) will work.
	 * (Only the tracks of the first group has segments in the MPL.)
	 */
	new_current_group_object_id = 0x106; /* ID of first group */
	test_set_current_group_obj_id(new_current_group_object_id);

	/* This part of the test not only checks that the opcodes are accepted
	 * by the server, but also that they actually do lead to the expected
	 * state changes. This may lean too much upon knowledge or assumptions,
	 * and therefore be too fragile.
	 * It may be more robust to just give commands and check for the success
	 * code in the control point notifications
	 */

	/* It is assumed that the server starts the test in the paused state */
	test_verify_media_state_wait_flags(BT_MCS_MEDIA_STATE_PAUSED);

	/* The tests are ordered to ensure that each command changes state */
	test_cp_play();
	test_cp_fast_forward();
	test_cp_pause();
	test_cp_fast_rewind();
	test_cp_stop();

	/* Control point - move relative opcode */
	test_cp_move_relative();

	/* Control point - segment change opcodes */
	test_cp_prev_segment();
	test_cp_next_segment();
	test_cp_first_segment();
	test_cp_last_segment();
	test_cp_goto_segment();

	/* Control point - track change opcodes */
	/* The tests are ordered to ensure that each command changes track */
	/* Assumes we are not starting on the last track */
	test_cp_next_track_and_track_changed();
	test_cp_prev_track();
	test_cp_last_track();
	test_cp_first_track();
	test_cp_goto_track();

	/* Control point - group change opcodes *******************************/
	/* The tests are ordered to ensure that each command changes group */
	/* Assumes we are not starting on the last group */
	test_cp_next_group();
	test_cp_prev_group();
	test_cp_last_group();
	test_cp_first_group();
	test_cp_goto_group();

	/* Search control point */
	test_search();

	/* TEST IS COMPLETE */
	PASS("MCC passed\n");
}

static const struct bst_test_instance test_mcs[] = {
	{
		.test_id = "mcc",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_mcc_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_mcs);
}

#else

struct bst_test_list *test_mcc_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_MCC */
