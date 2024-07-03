/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/audio/mcc.h>
#include <zephyr/bluetooth/audio/media_proxy.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/services/ots.h>
#include <zephyr/sys/printk.h>

#include "bstests.h"
#include "common.h"

#ifdef CONFIG_BT_MCS
extern enum bst_result_t bst_result;

static uint64_t g_icon_object_id;
static uint64_t g_track_segments_object_id;
static uint64_t g_current_track_object_id;
static uint64_t g_next_track_object_id;
static uint64_t g_parent_group_object_id;
static uint64_t g_current_group_object_id;
static uint64_t g_search_results_object_id;

static int32_t  g_pos;
static int8_t   g_pb_speed;
static uint8_t  g_playing_order;
static uint8_t  g_state;
static uint8_t  g_command_result;
static uint32_t g_commands_supported;
static uint8_t  g_search_control_point_result_code;

CREATE_FLAG(ble_is_initialized);
CREATE_FLAG(local_player_instance);
CREATE_FLAG(remote_player_instance);
CREATE_FLAG(player_name_read);
CREATE_FLAG(icon_object_id_read);
CREATE_FLAG(icon_url_read);
CREATE_FLAG(track_title_read);
CREATE_FLAG(track_duration_read);
CREATE_FLAG(track_position);
CREATE_FLAG(playback_speed);
CREATE_FLAG(seeking_speed_read);
CREATE_FLAG(track_segments_object_id_read);
CREATE_FLAG(current_track_object_id_read);
CREATE_FLAG(next_track_object_id_read);
CREATE_FLAG(parent_group_object_id_read);
CREATE_FLAG(current_group_object_id_read);
CREATE_FLAG(search_results_object_id_read);
CREATE_FLAG(playing_order_flag);
CREATE_FLAG(playing_orders_supported_read);
CREATE_FLAG(ccid_read);
CREATE_FLAG(media_state_read);
CREATE_FLAG(command_sent_flag);
CREATE_FLAG(command_results_flag);
CREATE_FLAG(commands_supported);
CREATE_FLAG(search_sent_flag);
CREATE_FLAG(search_result_code_flag);


static struct media_proxy_ctrl_cbs cbs;
static struct media_player *local_player;
static struct media_player *remote_player;
static struct media_player *current_player;

static void local_player_instance_cb(struct media_player *player, int err)
{
	if (err) {
		FAIL("Local player instance failed (%d)", err);
		return;
	}

	local_player = player;
	SET_FLAG(local_player_instance);
}

static void discover_player_cb(struct media_player *player, int err)
{
	if (err) {
		FAIL("Discover player failed (%d)\n", err);
		return;
	}

	remote_player = player;
	SET_FLAG(remote_player_instance);
}

static void player_name_cb(struct media_player *plr, int err, const char *name)
{
	if (err) {
		FAIL("Player Name read failed (%d)\n", err);
		return;
	}

	if (plr != current_player) {
		FAIL("Wrong player\n");
		return;
	}

	SET_FLAG(player_name_read);
}

static void icon_id_cb(struct media_player *plr, int err, uint64_t id)
{
	if (err) {
		FAIL("Icon Object ID read failed (%d)", err);
		return;
	}

	if (plr != current_player) {
		FAIL("Wrong player\n");
		return;
	}

	g_icon_object_id = id;
	SET_FLAG(icon_object_id_read);
}

static void icon_url_cb(struct media_player *plr, int err, const char *url)
{
	if (err) {
		FAIL("Icon URL read failed (%d)", err);
		return;
	}

	if (plr != current_player) {
		FAIL("Wrong player\n");
	}

	SET_FLAG(icon_url_read);
}

static void track_title_cb(struct media_player *plr, int err, const char *title)
{
	if (err) {
		FAIL("Track title read failed (%d)", err);
		return;
	}

	if (plr != current_player) {
		FAIL("Wrong player\n");
		return;
	}

	SET_FLAG(track_title_read);
}

static void track_duration_cb(struct media_player *plr, int err, int32_t duration)
{
	if (err) {
		FAIL("Track duration read failed (%d)", err);
		return;
	}

	if (plr != current_player) {
		FAIL("Wrong player\n");
		return;
	}

	SET_FLAG(track_duration_read);
}

static void track_position_recv_cb(struct media_player *plr, int err, int32_t position)
{
	if (err) {
		FAIL("Track position read failed (%d)", err);
		return;
	}

	if (plr != current_player) {
		FAIL("Wrong player\n");
		return;
	}

	g_pos = position;
	SET_FLAG(track_position);
}

static void track_position_write_cb(struct media_player *plr, int err, int32_t position)
{
	if (err) {
		FAIL("Track position write failed (%d)", err);
		return;
	}

	if (plr != current_player) {
		FAIL("Wrong player\n");
		return;
	}

	g_pos = position;
	SET_FLAG(track_position);
}

static void playback_speed_recv_cb(struct media_player *plr, int err, int8_t speed)
{
	if (err) {
		FAIL("Playback speed read failed (%d)", err);
		return;
	}

	if (plr != current_player) {
		FAIL("Wrong player\n");
		return;
	}

	g_pb_speed = speed;
	SET_FLAG(playback_speed);
}

static void playback_speed_write_cb(struct media_player *plr, int err, int8_t speed)
{
	if (err) {
		FAIL("Playback speed write failed (%d)", err);
		return;
	}

	if (plr != current_player) {
		FAIL("Wrong player\n");
		return;
	}

	g_pb_speed = speed;
	SET_FLAG(playback_speed);
}

static void seeking_speed_cb(struct media_player *plr, int err, int8_t speed)
{
	if (err) {
		FAIL("Seeking speed read failed (%d)", err);
		return;
	}

	if (plr != current_player) {
		FAIL("Wrong player\n");
		return;
	}

	SET_FLAG(seeking_speed_read);
}

static void track_segments_id_cb(struct media_player *plr, int err, uint64_t id)
{
	if (err) {
		FAIL("Track Segments ID read failed (%d)\n", err);
		return;
	}

	if (plr != current_player) {
		FAIL("Wrong player\n");
		return;
	}

	g_track_segments_object_id = id;
	SET_FLAG(track_segments_object_id_read);
}

static void current_track_id_cb(struct media_player *plr, int err, uint64_t id)
{
	if (err) {
		FAIL("Current Track Object ID read failed (%d)\n", err);
		return;
	}

	if (plr != current_player) {
		FAIL("Wrong player\n");
		return;
	}

	g_current_track_object_id = id;
	SET_FLAG(current_track_object_id_read);
}

static void next_track_id_cb(struct media_player *plr, int err, uint64_t id)
{
	if (err) {
		FAIL("Next Track Object ID read failed (%d)\n", err);
		return;
	}

	if (plr != current_player) {
		FAIL("Wrong player\n");
		return;
	}

	g_next_track_object_id = id;
	SET_FLAG(next_track_object_id_read);
}

static void parent_group_id_cb(struct media_player *plr, int err, uint64_t id)
{
	if (err) {
		FAIL("Parent Group Object ID read failed (%d)\n", err);
		return;
	}

	if (plr != current_player) {
		FAIL("Wrong player\n");
		return;
	}

	g_parent_group_object_id = id;
	SET_FLAG(parent_group_object_id_read);
}

static void current_group_id_cb(struct media_player *plr, int err, uint64_t id)
{
	if (err) {
		FAIL("Current Group Object ID read failed (%d)\n", err);
		return;
	}

	if (plr != current_player) {
		FAIL("Wrong player\n");
		return;
	}

	g_current_group_object_id = id;
	SET_FLAG(current_group_object_id_read);
}

static void playing_order_recv_cb(struct media_player *plr, int err, uint8_t order)
{
	if (err) {
		FAIL("Playing order read failed (%d)", err);
		return;
	}

	if (plr != current_player) {
		FAIL("Wrong player\n");
		return;
	}

	g_playing_order = order;
	SET_FLAG(playing_order_flag);
}

static void playing_order_write_cb(struct media_player *plr, int err, uint8_t order)
{
	if (err) {
		FAIL("Playing order write failed (%d)", err);
		return;
	}

	if (plr != current_player) {
		FAIL("Wrong player\n");
		return;
	}

	g_playing_order = order;
	SET_FLAG(playing_order_flag);
}

static void playing_orders_supported_cb(struct media_player *plr, int err, uint16_t orders)
{
	if (err) {
		FAIL("Playing orders supported read failed (%d)", err);
		return;
	}

	if (plr != current_player) {
		FAIL("Wrong player\n");
		return;
	}

	SET_FLAG(playing_orders_supported_read);
}

static void media_state_cb(struct media_player *plr, int err, uint8_t state)
{
	if (err) {
		FAIL("Media State read failed (%d)", err);
		return;
	}

	if (plr != current_player) {
		FAIL("Wrong player\n");
		return;
	}

	g_state = state;
	SET_FLAG(media_state_read);
}

static void command_send_cb(struct media_player *plr, int err, const struct mpl_cmd *cmd)
{
	if (err) {
		FAIL("Command send failed (%d)", err);
		return;
	}

	if (plr != current_player) {
		FAIL("Wrong player\n");
		return;
	}

	SET_FLAG(command_sent_flag);
}

static void command_recv_cb(struct media_player *plr, int err, const struct mpl_cmd_ntf *cmd_ntf)
{
	if (err) {
		FAIL("Command failed (%d)", err);
		return;
	}

	if (plr != current_player) {
		FAIL("Wrong player\n");
		return;
	}

	g_command_result = cmd_ntf->result_code;
	SET_FLAG(command_results_flag);
}

static void commands_supported_cb(struct media_player *plr, int err, uint32_t opcodes)
{
	if (err) {
		FAIL("Commands supported failed (%d)", err);
		return;
	}

	if (plr != current_player) {
		FAIL("Wrong player\n");
		return;
	}

	g_commands_supported = opcodes;
	SET_FLAG(commands_supported);
}



static void search_send_cb(struct media_player *plr, int err, const struct mpl_search *search)
{
	if (err) {
		FAIL("Search failed (%d)", err);
		return;
	}

	if (plr != current_player) {
		FAIL("Wrong player\n");
		return;
	}

	SET_FLAG(search_sent_flag);
}

static void search_recv_cb(struct media_player *plr, int err, uint8_t result_code)
{
	if (err) {
		FAIL("Search failed (%d), result code: %u", err, result_code);
		return;
	}

	if (plr != current_player) {
		FAIL("Wrong player\n");
		return;
	}

	g_search_control_point_result_code = result_code;
	SET_FLAG(search_result_code_flag);
}

static void search_results_id_cb(struct media_player *plr, int err, uint64_t id)
{
	if (err) {
		FAIL("Search Results Object ID read failed (%d)", err);
		return;
	}

	if (plr != current_player) {
		FAIL("Wrong player\n");
		return;
	}

	g_search_results_object_id = id;
	SET_FLAG(search_results_object_id_read);
}

static void content_ctrl_id_cb(struct media_player *plr, int err, uint8_t ccid)
{
	if (err) {
		FAIL("Content control ID read failed (%d)", err);
		return;
	}

	if (plr != current_player) {
		FAIL("Wrong player\n");
		return;
	}

	SET_FLAG(ccid_read);
}

void initialize_media(void)
{
	int err = media_proxy_pl_init();  /* TODO: Fix direct call to player */

	if (err) {
		FAIL("Could not init mpl: %d", err);
		return;
	}

	/* Set up the callback structure */
	cbs.local_player_instance         = local_player_instance_cb;
	cbs.discover_player               = discover_player_cb;
	cbs.player_name_recv              = player_name_cb;
	cbs.icon_id_recv                  = icon_id_cb;
	cbs.icon_url_recv                 = icon_url_cb;
	cbs.track_title_recv              = track_title_cb;
	cbs.track_duration_recv           = track_duration_cb;
	cbs.track_position_recv           = track_position_recv_cb;
	cbs.track_position_write          = track_position_write_cb;
	cbs.playback_speed_recv           = playback_speed_recv_cb;
	cbs.playback_speed_write          = playback_speed_write_cb;
	cbs.seeking_speed_recv            = seeking_speed_cb;
#ifdef CONFIG_BT_OTS
	cbs.track_segments_id_recv        = track_segments_id_cb;
	cbs.current_track_id_recv         = current_track_id_cb;
	cbs.next_track_id_recv            = next_track_id_cb;
	cbs.parent_group_id_recv          = parent_group_id_cb;
	cbs.current_group_id_recv         = current_group_id_cb;
#endif /* CONFIG_BT_OTS */
	cbs.playing_order_recv            = playing_order_recv_cb;
	cbs.playing_order_write           = playing_order_write_cb;
	cbs.playing_orders_supported_recv = playing_orders_supported_cb;
	cbs.media_state_recv              = media_state_cb;
	cbs.command_send                  = command_send_cb;
	cbs.command_recv                  = command_recv_cb;
	cbs.commands_supported_recv       = commands_supported_cb;
#ifdef CONFIG_BT_OTS
	cbs.search_send                   = search_send_cb;
	cbs.search_recv                   = search_recv_cb;
	cbs.search_results_id_recv        = search_results_id_cb;
#endif /* CONFIG_BT_OTS */
	cbs.content_ctrl_id_recv          = content_ctrl_id_cb;

	UNSET_FLAG(local_player_instance);

	err = media_proxy_ctrl_register(&cbs);
	if (err) {
		FAIL("Could not init mpl: %d", err);
		return;
	}

	WAIT_FOR_FLAG(local_player_instance);
	printk("media init and local player instance succeeded\n");
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
	err = media_proxy_ctrl_get_media_state(current_player);
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

/* Helper function to write commands to the control point, including the
 * flag handling.
 * Will FAIL on error to send the command.
 * Will WAIT for the required flags before returning.
 */
static void test_send_cmd_wait_flags(struct mpl_cmd *cmd)
{
	int err;

	UNSET_FLAG(command_sent_flag);
	UNSET_FLAG(command_results_flag);
	err = media_proxy_ctrl_send_command(current_player, cmd);
	if (err) {
		FAIL("Failed to send command: %d, opcode: %u",
		     err, cmd->opcode);
		return;
	}

	WAIT_FOR_FLAG(command_sent_flag);
	WAIT_FOR_FLAG(command_results_flag);
}

static void test_cp_play(void)
{
	struct mpl_cmd cmd;

	cmd.opcode = MEDIA_PROXY_OP_PLAY;
	cmd.use_param = false;

	test_send_cmd_wait_flags(&cmd);

	if (g_command_result != MEDIA_PROXY_CMD_SUCCESS) {
		FAIL("PLAY command failed\n");
		return;
	}

	if (test_verify_media_state_wait_flags(MEDIA_PROXY_STATE_PLAYING)) {
		printk("PLAY command succeeded\n");
	}
}

static void test_cp_pause(void)
{
	struct mpl_cmd cmd;

	cmd.opcode = MEDIA_PROXY_OP_PAUSE;
	cmd.use_param = false;

	test_send_cmd_wait_flags(&cmd);

	if (g_command_result != MEDIA_PROXY_CMD_SUCCESS) {
		FAIL("PAUSE command failed\n");
		return;
	}

	if (test_verify_media_state_wait_flags(MEDIA_PROXY_STATE_PAUSED)) {
		printk("PAUSE command succeeded\n");
	}
}

static void test_cp_fast_rewind(void)
{
	struct mpl_cmd cmd;

	cmd.opcode = MEDIA_PROXY_OP_FAST_REWIND;
	cmd.use_param = false;

	test_send_cmd_wait_flags(&cmd);

	if (g_command_result != MEDIA_PROXY_CMD_SUCCESS) {
		FAIL("FAST REWIND command failed\n");
		return;
	}

	if (test_verify_media_state_wait_flags(MEDIA_PROXY_STATE_SEEKING)) {
		printk("FAST REWIND command succeeded\n");
	}
}

static void test_cp_fast_forward(void)
{
	struct mpl_cmd cmd;

	cmd.opcode = MEDIA_PROXY_OP_FAST_FORWARD;
	cmd.use_param = false;

	test_send_cmd_wait_flags(&cmd);

	if (g_command_result != MEDIA_PROXY_CMD_SUCCESS) {
		FAIL("FAST FORWARD command failed\n");
		return;
	}

	if (test_verify_media_state_wait_flags(MEDIA_PROXY_STATE_SEEKING)) {
		printk("FAST FORWARD command succeeded\n");
	}
}

static void test_cp_stop(void)
{
	struct mpl_cmd cmd;

	cmd.opcode = MEDIA_PROXY_OP_STOP;
	cmd.use_param = false;

	test_send_cmd_wait_flags(&cmd);

	if (g_command_result != MEDIA_PROXY_CMD_SUCCESS) {
		FAIL("STOP command failed\n");
		return;
	}

	/* There is no "STOPPED" state in the spec - STOP goes to PAUSED */
	if (test_verify_media_state_wait_flags(MEDIA_PROXY_STATE_PAUSED)) {
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
	UNSET_FLAG(track_position);
	err = media_proxy_ctrl_get_track_position(current_player);
	if (err) {
		FAIL("Failed to read track position: %d\n", err);
		return;
	}

	WAIT_FOR_FLAG(track_position);
	uint32_t tmp_pos = g_pos;

	cmd.opcode = MEDIA_PROXY_OP_MOVE_RELATIVE;
	cmd.use_param = true;
	cmd.param = 1000;  /* Position change, measured in 1/100 of a second */

	test_send_cmd_wait_flags(&cmd);

	if (g_command_result != MEDIA_PROXY_CMD_SUCCESS) {
		FAIL("MOVE RELATIVE command failed\n");
		return;
	}

	UNSET_FLAG(track_position);
	err = media_proxy_ctrl_get_track_position(current_player);
	if (err) {
		FAIL("Failed to read track position: %d\n", err);
		return;
	}

	WAIT_FOR_FLAG(track_position);
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

	cmd.opcode = MEDIA_PROXY_OP_PREV_SEGMENT;
	cmd.use_param = false;

	test_send_cmd_wait_flags(&cmd);

	if (g_command_result != MEDIA_PROXY_CMD_SUCCESS) {
		FAIL("PREV SEGMENT command failed\n");
		return;
	}

	printk("PREV SEGMENT command succeeded\n");
}

static void test_cp_next_segment(void)
{
	struct mpl_cmd cmd;

	cmd.opcode = MEDIA_PROXY_OP_NEXT_SEGMENT;
	cmd.use_param = false;

	test_send_cmd_wait_flags(&cmd);

	if (g_command_result != MEDIA_PROXY_CMD_SUCCESS) {
		FAIL("NEXT SEGMENT command failed\n");
		return;
	}

	printk("NEXT SEGMENT command succeeded\n");
}

static void test_cp_first_segment(void)
{
	struct mpl_cmd cmd;

	cmd.opcode = MEDIA_PROXY_OP_FIRST_SEGMENT;
	cmd.use_param = false;

	test_send_cmd_wait_flags(&cmd);

	if (g_command_result != MEDIA_PROXY_CMD_SUCCESS) {
		FAIL("FIRST SEGMENT command failed\n");
		return;
	}

	printk("FIRST SEGMENT command succeeded\n");
}

static void test_cp_last_segment(void)
{
	struct mpl_cmd cmd;

	cmd.opcode = MEDIA_PROXY_OP_LAST_SEGMENT;
	cmd.use_param = false;

	test_send_cmd_wait_flags(&cmd);

	if (g_command_result != MEDIA_PROXY_CMD_SUCCESS) {
		FAIL("LAST SEGMENT command failed\n");
		return;
	}

	printk("LAST SEGMENT command succeeded\n");
}

static void test_cp_goto_segment(void)
{
	struct mpl_cmd cmd;

	cmd.opcode = MEDIA_PROXY_OP_GOTO_SEGMENT;
	cmd.use_param = true;
	cmd.param = 2;    /* Second segment - not the first, maybe not last */

	test_send_cmd_wait_flags(&cmd);

	if (g_command_result != MEDIA_PROXY_CMD_SUCCESS) {
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
	err = media_proxy_ctrl_get_current_track_id(current_player);
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

	/* To verify that a track change has happened, the test checks that the
	 * current track object ID has changed.
	 */

	cmd.opcode = MEDIA_PROXY_OP_PREV_TRACK;
	cmd.use_param = false;

	test_read_current_track_object_id_wait_flags();
	object_id = g_current_track_object_id;

	test_send_cmd_wait_flags(&cmd);

	if (g_command_result != MEDIA_PROXY_CMD_SUCCESS) {
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

static void test_cp_next_track(void)
{
	uint64_t object_id;
	struct mpl_cmd cmd;

	cmd.opcode = MEDIA_PROXY_OP_NEXT_TRACK;
	cmd.use_param = false;

	test_read_current_track_object_id_wait_flags();
	object_id = g_current_track_object_id;

	test_send_cmd_wait_flags(&cmd);

	if (g_command_result != MEDIA_PROXY_CMD_SUCCESS) {
		FAIL("NEXT TRACK command failed\n");
		return;
	}

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

	cmd.opcode = MEDIA_PROXY_OP_FIRST_TRACK;
	cmd.use_param = false;

	test_read_current_track_object_id_wait_flags();
	object_id = g_current_track_object_id;

	test_send_cmd_wait_flags(&cmd);

	if (g_command_result != MEDIA_PROXY_CMD_SUCCESS) {
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

	cmd.opcode = MEDIA_PROXY_OP_LAST_TRACK;
	cmd.use_param = false;

	test_read_current_track_object_id_wait_flags();
	object_id = g_current_track_object_id;

	test_send_cmd_wait_flags(&cmd);

	if (g_command_result != MEDIA_PROXY_CMD_SUCCESS) {
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

	cmd.opcode = MEDIA_PROXY_OP_GOTO_TRACK;
	cmd.use_param = true;
	cmd.param = 2; /* Second track, not the first, maybe not the last */

	test_read_current_track_object_id_wait_flags();
	object_id = g_current_track_object_id;

	test_send_cmd_wait_flags(&cmd);

	if (g_command_result != MEDIA_PROXY_CMD_SUCCESS) {
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
	err = media_proxy_ctrl_get_current_group_id(current_player);
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

	/* To verify that a group change has happened, the test checks that the
	 * current group object ID has changed.
	 */

	cmd.opcode = MEDIA_PROXY_OP_PREV_GROUP;
	cmd.use_param = false;

	test_read_current_group_object_id_wait_flags();
	object_id = g_current_group_object_id;

	test_send_cmd_wait_flags(&cmd);

	if (g_command_result != MEDIA_PROXY_CMD_SUCCESS) {
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

	cmd.opcode = MEDIA_PROXY_OP_NEXT_GROUP;
	cmd.use_param = false;

	test_read_current_group_object_id_wait_flags();
	object_id = g_current_group_object_id;

	test_send_cmd_wait_flags(&cmd);

	if (g_command_result != MEDIA_PROXY_CMD_SUCCESS) {
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

	cmd.opcode = MEDIA_PROXY_OP_FIRST_GROUP;
	cmd.use_param = false;

	test_read_current_group_object_id_wait_flags();
	object_id = g_current_group_object_id;

	test_send_cmd_wait_flags(&cmd);

	if (g_command_result != MEDIA_PROXY_CMD_SUCCESS) {
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

	cmd.opcode = MEDIA_PROXY_OP_LAST_GROUP;
	cmd.use_param = false;

	test_read_current_group_object_id_wait_flags();
	object_id = g_current_group_object_id;

	test_send_cmd_wait_flags(&cmd);

	if (g_command_result != MEDIA_PROXY_CMD_SUCCESS) {
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

	cmd.opcode = MEDIA_PROXY_OP_GOTO_GROUP;
	cmd.use_param = true;
	cmd.param = 2; /* Second group, not the first, maybe not the last */

	test_read_current_group_object_id_wait_flags();
	object_id = g_current_group_object_id;

	test_send_cmd_wait_flags(&cmd);

	if (g_command_result != MEDIA_PROXY_CMD_SUCCESS) {
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

static void test_scp(void)
{
	struct mpl_search search;
	struct mpl_sci sci = {0};
	int err;

	/* Test outline:
	 * - verify that the search results object ID is zero before search
	 * - write a search (one search control item) to the search control point,
	 *   get write callback and notification
	 * - verify that the search results object ID is non-zero
	 */

	UNSET_FLAG(search_results_object_id_read);
	err = media_proxy_ctrl_get_search_results_id(current_player);

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
	sci.type = MEDIA_PROXY_SEARCH_TYPE_TRACK_NAME;
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

	UNSET_FLAG(search_sent_flag);
	UNSET_FLAG(search_result_code_flag);
	UNSET_FLAG(search_results_object_id_read);

	err = media_proxy_ctrl_send_search(current_player, &search);
	if (err) {
		FAIL("Failed to write to search control point\n");
		return;
	}

	WAIT_FOR_FLAG(search_sent_flag);
	WAIT_FOR_FLAG(search_result_code_flag);

	if (g_search_control_point_result_code != MEDIA_PROXY_SEARCH_SUCCESS) {
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

/* This function tests all commands in the API in sequence for the provided player.
 * (Works by setting the provided player as the "current player".)
 *
 * The order of the sequence follows the order of the characterstics in the
 * Media Control Service specification
 */
void test_media_controller_player(struct media_player *player)
{
	current_player = player;
	int err;

	UNSET_FLAG(player_name_read);
	err = media_proxy_ctrl_get_player_name(current_player);
	if (err) {
		FAIL("Failed to read media player name ID: %d", err);
		return;
	}

	WAIT_FOR_FLAG(player_name_read);
	printk("Player Name read succeeded\n");

	/* Read icon object id  ******************************************/
	UNSET_FLAG(icon_object_id_read);
	err = media_proxy_ctrl_get_icon_id(current_player);
	if (err) {
		FAIL("Failed to read icon object ID: %d", err);
		return;
	}

	WAIT_FOR_FLAG(icon_object_id_read);
	printk("Icon Object ID read succeeded\n");

	/* Read icon url *************************************************/
	UNSET_FLAG(icon_url_read);
	err =  media_proxy_ctrl_get_icon_url(current_player);
	if (err) {
		FAIL("Failed to read icon url: %d", err);
		return;
	}

	WAIT_FOR_FLAG(icon_url_read);
	printk("Icon URL read succeeded\n");

	/* Read track_title ******************************************/
	UNSET_FLAG(track_title_read);
	err = media_proxy_ctrl_get_track_title(current_player);
	if (err) {
		FAIL("Failed to read track_title: %d", err);
		return;
	}

	WAIT_FOR_FLAG(track_title_read);
	printk("Track title read succeeded\n");

	/* Read track_duration ******************************************/
	UNSET_FLAG(track_duration_read);
	err = media_proxy_ctrl_get_track_duration(current_player);
	if (err) {
		FAIL("Failed to read track_duration: %d", err);
		return;
	}

	WAIT_FOR_FLAG(track_duration_read);
	printk("Track duration read succeeded\n");

	/* Read and set track_position *************************************/
	UNSET_FLAG(track_position);
	err = media_proxy_ctrl_get_track_position(current_player);
	if (err) {
		FAIL("Failed to read track position: %d", err);
		return;
	}

	WAIT_FOR_FLAG(track_position);
	printk("Track position read succeeded\n");

	int32_t pos = g_pos + 1200; /*12 seconds further into the track */

	UNSET_FLAG(track_position);
	err = media_proxy_ctrl_set_track_position(current_player, pos);
	if (err) {
		FAIL("Failed to set track position: %d", err);
		return;
	}

	WAIT_FOR_FLAG(track_position);
	if (g_pos != pos) {
		/* In this controlled case, we expect that the resulting */
		/* position is the position given in the set command */
		FAIL("Track position set failed: Incorrect position\n");
	}
	printk("Track position set succeeded\n");

	/* Read and set playback speed *************************************/
	UNSET_FLAG(playback_speed);
	err = media_proxy_ctrl_get_playback_speed(current_player);
	if (err) {
		FAIL("Failed to read playback speed: %d", err);
		return;
	}

	WAIT_FOR_FLAG(playback_speed);
	printk("Playback speed read succeeded\n");

	int8_t pb_speed = g_pb_speed + 8; /* 2^(8/64) faster than current speed */

	UNSET_FLAG(playback_speed);
	err = media_proxy_ctrl_set_playback_speed(current_player, pb_speed);
	if (err) {
		FAIL("Failed to set playback speed: %d", err);
		return;
	}

	WAIT_FOR_FLAG(playback_speed);
	if (g_pb_speed != pb_speed) {
		FAIL("Playback speed failed: Incorrect playback speed\n");
	}
	printk("Playback speed set succeeded\n");

	/* Read seeking speed *************************************/
	UNSET_FLAG(seeking_speed_read);
	err = media_proxy_ctrl_get_seeking_speed(current_player);
	if (err) {
		FAIL("Failed to read seeking speed: %d", err);
		return;
	}

	WAIT_FOR_FLAG(seeking_speed_read);
	printk("Seeking speed read succeeded\n");

	/* Read track segments object *****************************************/
	UNSET_FLAG(track_segments_object_id_read);
	err = media_proxy_ctrl_get_track_segments_id(current_player);
	if (err) {
		FAIL("Failed to read track segments object ID: %d", err);
		return;
	}

	WAIT_FOR_FLAG(track_segments_object_id_read);
	printk("Track Segments Object ID read succeeded\n");

	/* Read current track object ******************************************/
	UNSET_FLAG(current_track_object_id_read);
	err = media_proxy_ctrl_get_current_track_id(current_player);
	if (err) {
		FAIL("Failed to read current track object ID: %d", err);
		return;
	}

	WAIT_FOR_FLAG(current_track_object_id_read);
	printk("Current Track Object ID read succeeded\n");

	/* Read next track object ******************************************/
	UNSET_FLAG(next_track_object_id_read);
	err = media_proxy_ctrl_get_next_track_id(current_player);
	if (err) {
		FAIL("Failed to read next track object ID: %d", err);
		return;
	}

	WAIT_FOR_FLAG(next_track_object_id_read);
	printk("Next Track Object ID read succeeded\n");

	/* Read parent group object ******************************************/
	UNSET_FLAG(parent_group_object_id_read);
	err = media_proxy_ctrl_get_parent_group_id(current_player);
	if (err) {
		FAIL("Failed to read parent group object ID: %d", err);
		return;
	}

	WAIT_FOR_FLAG(parent_group_object_id_read);
	printk("Parent Group Object ID read succeeded\n");

	/* Read current group object ******************************************/
	UNSET_FLAG(current_group_object_id_read);
	err = media_proxy_ctrl_get_current_group_id(current_player);
	if (err) {
		FAIL("Failed to read current group object ID: %d", err);
		return;
	}

	WAIT_FOR_FLAG(current_group_object_id_read);
	printk("Current Group Object ID read succeeded\n");

	/* Read and set playing order *************************************/
	UNSET_FLAG(playing_order_flag);
	err = media_proxy_ctrl_get_playing_order(current_player);
	if (err) {
		FAIL("Failed to read playing order: %d", err);
		return;
	}

	WAIT_FOR_FLAG(playing_order_flag);
	printk("Playing order read succeeded\n");

	uint8_t playing_order;

	if (g_playing_order != MEDIA_PROXY_PLAYING_ORDER_INORDER_ONCE) {
		playing_order = MEDIA_PROXY_PLAYING_ORDER_INORDER_ONCE;
	} else {
		playing_order = MEDIA_PROXY_PLAYING_ORDER_INORDER_REPEAT;
	}

	UNSET_FLAG(playing_order_flag);
	err = media_proxy_ctrl_set_playing_order(current_player, playing_order);
	if (err) {
		FAIL("Failed to set playing_order: %d", err);
		return;
	}

	WAIT_FOR_FLAG(playing_order_flag);
	if (g_playing_order != playing_order) {
		FAIL("Playing order set failed: Incorrect playing_order\n");
	}
	printk("Playing order set succeeded\n");

	/* Read playing orders supported  *************************************/
	UNSET_FLAG(playing_orders_supported_read);
	err = media_proxy_ctrl_get_playing_orders_supported(current_player);
	if (err) {
		FAIL("Failed to read playing orders supported: %d", err);
		return;
	}

	WAIT_FOR_FLAG(playing_orders_supported_read);
	printk("Playing orders supported read succeeded\n");

	/* Read media state  ***************************************************/
	UNSET_FLAG(media_state_read);
	err = media_proxy_ctrl_get_media_state(current_player);
	if (err) {
		FAIL("Failed to read media state: %d", err);
		return;
	}

	WAIT_FOR_FLAG(media_state_read);
	printk("Media state read succeeded\n");

	/* Read content control ID  *******************************************/
	UNSET_FLAG(ccid_read);
	err = media_proxy_ctrl_get_content_ctrl_id(current_player);
	if (err) {
		FAIL("Failed to read content control ID: %d", err);
		return;
	}

	WAIT_FOR_FLAG(ccid_read);
	printk("Content control ID read succeeded\n");

	/* Control point - "state" opcodes */

	/* This part of the test not only checks that the opcodes are accepted
	 * by the server, but also that they actually do lead to the expected
	 * state changes.  This may lean too much upon knowledge or assumptions,
	 * and therefore be too fragile.
	 * It may be more robust to just give commands and check for the success
	 * code in the control point notifications
	 */

	/* It is assumed that the server starts the test in the paused state */
	test_verify_media_state_wait_flags(MEDIA_PROXY_STATE_PAUSED);

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
	test_cp_next_track();
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
	test_scp();
}

void initialize_bluetooth(void)
{
	int err;

	UNSET_FLAG(ble_is_initialized);
	err = bt_enable(bt_ready);
	if (err) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	WAIT_FOR_FLAG(ble_is_initialized);
	printk("Bluetooth initialized\n");

	bt_le_scan_cb_register(&common_scan_cb);
}

void scan_and_connect(void)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL);
	if (err) {
		FAIL("Failed to start scanning (err %d\n)", err);
		return;
	}

	printk("Scanning started successfully\n");

	WAIT_FOR_FLAG(flag_connected);

	bt_addr_le_to_str(bt_conn_get_dst(default_conn), addr, sizeof(addr));
	printk("Connected: %s\n", addr);
}

void discover_remote_player(void)
{
	int err;

	UNSET_FLAG(remote_player_instance);
	err = media_proxy_ctrl_discover_player(default_conn);
	if (err) {
		FAIL("Remote player discovery failed (err %d)\n", err);
		return;
	}

	WAIT_FOR_FLAG(remote_player_instance);
}

/* BabbleSim entry point for local player test */
void test_media_controller_local_player(void)
{
	printk("Media Control local player test application.  Board: %s\n", CONFIG_BOARD);

	initialize_bluetooth();
	initialize_media();  /* Sets local_player global variable */

	printk("Local player instance: %p\n", local_player);

	test_media_controller_player(local_player);

	/* TEST IS COMPLETE */
	PASS("Test media_controller_local_player passed\n");
}

/* BabbleSim entry point for remote player test */
void test_media_controller_remote_player(void)
{
	int err;
	printk("Media Control remote player test application.  Board: %s\n", CONFIG_BOARD);

	initialize_bluetooth();
	initialize_media();

	err = bt_le_adv_start(BT_LE_ADV_CONN_ONE_TIME, ad, AD_SIZE, NULL, 0);
	if (err) {
		FAIL("Advertising failed to start (err %d)\n", err);
	}

	WAIT_FOR_FLAG(flag_connected);

	discover_remote_player(); /* Sets global variable */
	printk("Remote player instance: %p\n", remote_player);

	test_media_controller_player(remote_player);

	/* TEST IS COMPLETE */
	PASS("Test media_controller_remote_player passed\n");
}

/* BabbleSim entry point for server for remote player test */
void test_media_controller_server(void)
{

	printk("Media Control server test application.  Board: %s\n", CONFIG_BOARD);

	initialize_bluetooth();
	initialize_media();

	/* The server side will also get callbacks, from its local player.
	 * And if the current player is not set, the callbacks will fail the test.
	 */
	printk("Local player instance: %p\n", local_player);
	current_player = local_player;

	scan_and_connect();

	/* TEST IS COMPLETE */
	PASS("Test media_controller_server passed\n");
}

static const struct bst_test_instance test_media_controller[] = {
	{
		.test_id = "media_controller_local_player",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_media_controller_local_player
	},
	{
		.test_id = "media_controller_remote_player",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_media_controller_remote_player
	},
	{
		.test_id = "media_controller_server",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_media_controller_server
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_media_controller_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_media_controller);
}

#else

struct bst_test_list *test_media_controller_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_MCS */
