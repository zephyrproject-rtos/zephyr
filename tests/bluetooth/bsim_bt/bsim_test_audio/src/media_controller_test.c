/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <sys/byteorder.h>
#include <sys/sem.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <bluetooth/mcc.h>

#include "../../../../subsys/bluetooth/host/audio/media_proxy.h"
#include "../../../../subsys/bluetooth/host/audio/otc.h"

#include "common.h"

extern enum bst_result_t bst_result;

static uint64_t g_icon_object_id;
static uint64_t g_track_segments_object_id;
static uint64_t g_current_track_object_id;
static uint64_t g_next_track_object_id;
static uint64_t g_current_group_object_id;
static uint64_t g_parent_group_object_id;
static uint64_t g_search_results_object_id;

static int32_t  g_pos;
static int8_t   g_pb_speed;
static uint8_t  g_playing_order;
static uint8_t  g_state;
static uint8_t  g_control_point_result;
static uint32_t g_operations_supported;
static uint8_t  g_search_control_point_result;

CREATE_FLAG(ble_is_initialized);
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
CREATE_FLAG(current_group_object_id_read);
CREATE_FLAG(parent_group_object_id_read);
CREATE_FLAG(search_results_object_id_read);
CREATE_FLAG(playing_order_flag);
CREATE_FLAG(playing_orders_supported_read);
CREATE_FLAG(ccid_read);
CREATE_FLAG(media_state_read);
CREATE_FLAG(operation_flag);
CREATE_FLAG(operations_supported);
CREATE_FLAG(search_flag);


static struct media_proxy_ctrl_cbs cbs;
static struct media_player *plr; /* TODO: Use the player pointer */

static void player_name_cb(struct media_player *plr, int err, char *name)
{
	if (err) {
		FAIL("Player Name read failed (%d)\n", err);
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

	g_icon_object_id = id;
	SET_FLAG(icon_object_id_read);
}

static void icon_url_cb(struct media_player *plr, int err, char *url)
{
	if (err) {
		FAIL("Icon URL read failed (%d)", err);
		return;
	}

	SET_FLAG(icon_url_read);
}

static void track_title_cb(struct media_player *plr, int err, char *title)
{
	if (err) {
		FAIL("Track title read failed (%d)", err);
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

	SET_FLAG(track_duration_read);
}

static void track_position_cb(struct media_player *plr, int err, int32_t position)
{
	if (err) {
		FAIL("Track position read failed (%d)", err);
		return;
	}

	g_pos = position;
	SET_FLAG(track_position);
}

static void playback_speed_cb(struct media_player *plr, int err, int8_t speed)
{
	if (err) {
		FAIL("Playback speed read failed (%d)", err);
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

	SET_FLAG(seeking_speed_read);
}

static void track_segments_id_cb(struct media_player *plr, int err, uint64_t id)
{
	if (err) {
		FAIL("Track Segments ID read failed (%d)\n", err);
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

	g_current_track_object_id = id;
	SET_FLAG(current_track_object_id_read);
}

static void next_track_id_cb(struct media_player *plr, int err, uint64_t id)
{
	if (err) {
		FAIL("Next Track Object ID read failed (%d)\n", err);
		return;
	}

	g_next_track_object_id = id;
	SET_FLAG(next_track_object_id_read);
}

static void current_group_id_cb(struct media_player *plr, int err, uint64_t id)
{
	if (err) {
		FAIL("Current Group Object ID read failed (%d)\n", err);
		return;
	}

	g_current_group_object_id = id;
	SET_FLAG(current_group_object_id_read);
}

static void parent_group_id_cb(struct media_player *plr, int err, uint64_t id)
{
	if (err) {
		FAIL("Parent Group Object ID read failed (%d)\n", err);
		return;
	}

	g_parent_group_object_id = id;
	SET_FLAG(parent_group_object_id_read);
}

static void playing_order_cb(struct media_player *plr, int err, uint8_t order)
{
	if (err) {
		FAIL("Playing order read failed (%d)", err);
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

	SET_FLAG(playing_orders_supported_read);
}

static void media_state_cb(struct media_player *plr, int err, uint8_t state)
{
	if (err) {
		FAIL("Media State read failed (%d)", err);
		return;
	}

	g_state = state;
	SET_FLAG(media_state_read);
}

static void operation_cb(struct media_player *plr, int err, struct mpl_op_ntf_t op_ntf)
{
	if (err) {
		FAIL("Operation failed (%d)", err);
		return;
	}

	/* TODO: Improve write callback / notification callback unification */
	g_control_point_result = op_ntf.result_code;
	SET_FLAG(operation_flag);
}

static void operations_supported_cb(struct media_player *plr, int err, uint32_t operations)
{
	if (err) {
		FAIL("Operations supported failed (%d)", err);
		return;
	}

	g_operations_supported = operations;
	SET_FLAG(operations_supported);
}



static void search_cb(struct media_player *plr, int err, uint8_t result_code)
{
	if (err) {
		FAIL("Search failed (%d), result code: %u", err, result_code);
		return;
	}

	g_search_control_point_result = result_code;
	SET_FLAG(search_flag);
}

static void search_results_id_cb(struct media_player *plr, int err, uint64_t id)
{
	if (err) {
		FAIL("Search Results Object ID read failed (%d)", err);
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

	SET_FLAG(ccid_read);
}

int do_media_init(void)
{
	int err = media_proxy_pl_init();  /* TODO: Fix direct call to player */

	if (err) {
		FAIL("Could not init mpl: %d", err);
		return err;
	}

	/* Set up the callback structure */
	cbs.player_name              = player_name_cb;
	cbs.icon_id                  = icon_id_cb;
	cbs.icon_url                 = icon_url_cb;
	cbs.track_title              = track_title_cb;
	cbs.track_duration           = track_duration_cb;
	cbs.track_position           = track_position_cb;
	cbs.playback_speed           = playback_speed_cb;
	cbs.seeking_speed            = seeking_speed_cb;
#ifdef CONFIG_BT_OTS
	cbs.track_segments_id        = track_segments_id_cb;
	cbs.current_track_id         = current_track_id_cb;
	cbs.next_track_id            = next_track_id_cb;
	cbs.current_group_id         = current_group_id_cb;
	cbs.parent_group_id          = parent_group_id_cb;
#endif /* CONFIG_BT_OTS */
	cbs.playing_order            = playing_order_cb;
	cbs.playing_orders_supported = playing_orders_supported_cb;
	cbs.media_state              = media_state_cb;
	cbs.operation                = operation_cb;
	cbs.operations_supported     = operations_supported_cb;
#ifdef CONFIG_BT_OTS
	cbs.search                   = search_cb;
	cbs.search_results_id        = search_results_id_cb;
#endif /* CONFIG_BT_OTS */
	cbs.content_ctrl_id          = content_ctrl_id_cb;

	err = media_proxy_ctrl_register(&cbs);

	if (err) {
		FAIL("Could not init mpl: %d", err);
	}

	return err;
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
	err = media_proxy_ctrl_media_state_get(plr);
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

/* Helper function to set the control point, including the flag handling.
 * Will FAIL on error to set the control point.
 * Will WAIT for the required flags before returning.
 */
static void test_set_cp_wait_flags(struct mpl_op_t op)
{
	int err;

	/* TODO:
	 * Improve unification of control point write callback and control
	 * point notification callback
	 */

	UNSET_FLAG(operation_flag);
	err = media_proxy_ctrl_operation_set(plr, op);
	if (err) {
		FAIL("Failed to write to control point: %d, operation: %u",
		     err, op.opcode);
		return;
	}

	WAIT_FOR_FLAG(operation_flag);
}

static void test_cp_play(void)
{
	struct mpl_op_t op;

	op.opcode = MEDIA_PROXY_OP_PLAY;
	op.use_param = false;

	test_set_cp_wait_flags(op);

	if (g_control_point_result != MEDIA_PROXY_OP_SUCCESS) {
		FAIL("PLAY operation failed\n");
		return;
	}

	if (test_verify_media_state_wait_flags(MEDIA_PROXY_STATE_PLAYING)) {
		printk("PLAY operation succeeded\n");
	}
}

static void test_cp_pause(void)
{
	struct mpl_op_t op;

	op.opcode = MEDIA_PROXY_OP_PAUSE;
	op.use_param = false;

	test_set_cp_wait_flags(op);

	if (g_control_point_result != MEDIA_PROXY_OP_SUCCESS) {
		FAIL("PAUSE operation failed\n");
		return;
	}

	if (test_verify_media_state_wait_flags(MEDIA_PROXY_STATE_PAUSED)) {
		printk("PAUSE operation succeeded\n");
	}
}

static void test_cp_fast_rewind(void)
{
	struct mpl_op_t op;

	op.opcode = MEDIA_PROXY_OP_FAST_REWIND;
	op.use_param = false;

	test_set_cp_wait_flags(op);

	if (g_control_point_result != MEDIA_PROXY_OP_SUCCESS) {
		FAIL("FAST REWIND operation failed\n");
		return;
	}

	if (test_verify_media_state_wait_flags(MEDIA_PROXY_STATE_SEEKING)) {
		printk("FAST REWIND operation succeeded\n");
	}
}

static void test_cp_fast_forward(void)
{
	struct mpl_op_t op;

	op.opcode = MEDIA_PROXY_OP_FAST_FORWARD;
	op.use_param = false;

	test_set_cp_wait_flags(op);

	if (g_control_point_result != MEDIA_PROXY_OP_SUCCESS) {
		FAIL("FAST FORWARD operation failed\n");
		return;
	}

	if (test_verify_media_state_wait_flags(MEDIA_PROXY_STATE_SEEKING)) {
		printk("FAST FORWARD operation succeeded\n");
	}
}

static void test_cp_stop(void)
{
	struct mpl_op_t op;

	op.opcode = MEDIA_PROXY_OP_STOP;
	op.use_param = false;

	test_set_cp_wait_flags(op);

	if (g_control_point_result != MEDIA_PROXY_OP_SUCCESS) {
		FAIL("STOP operation failed\n");
		return;
	}

	/* There is no "STOPPED" state in the spec - STOP goes to PAUSED */
	if (test_verify_media_state_wait_flags(MEDIA_PROXY_STATE_PAUSED)) {
		printk("STOP operation succeeded\n");
	}
}

static void test_cp_move_relative(void)
{
	int err;
	struct mpl_op_t op;

	/* Assumes that the server is in a state where it is  able to change
	 * the current track position
	 * Also assumes position will not change by itself, which is wrong if
	 * if the player is playing.
	 */
	UNSET_FLAG(track_position);
	err = media_proxy_ctrl_track_position_get(plr);
	if (err) {
		FAIL("Failed to read track position: %d\n", err);
		return;
	}

	WAIT_FOR_FLAG(track_position);
	uint32_t tmp_pos = g_pos;

	op.opcode = MEDIA_PROXY_OP_MOVE_RELATIVE;
	op.use_param = true;
	op.param = 1000;  /* Position change, measured in 1/100 of a second */

	test_set_cp_wait_flags(op);

	if (g_control_point_result != MEDIA_PROXY_OP_SUCCESS) {
		FAIL("MOVE RELATIVE operation failed\n");
		return;
	}

	UNSET_FLAG(track_position);
	err = media_proxy_ctrl_track_position_get(plr);
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

	printk("MOVE RELATIVE operation succeeded\n");
}

static void test_cp_prev_segment(void)
{
	struct mpl_op_t op;

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

	op.opcode = MEDIA_PROXY_OP_PREV_SEGMENT;
	op.use_param = false;

	test_set_cp_wait_flags(op);

	if (g_control_point_result != MEDIA_PROXY_OP_SUCCESS) {
		FAIL("PREV SEGMENT operation failed\n");
		return;
	}

	printk("PREV SEGMENT operation succeeded\n");
}

static void test_cp_next_segment(void)
{
	struct mpl_op_t op;

	op.opcode = MEDIA_PROXY_OP_NEXT_SEGMENT;
	op.use_param = false;

	test_set_cp_wait_flags(op);

	if (g_control_point_result != MEDIA_PROXY_OP_SUCCESS) {
		FAIL("NEXT SEGMENT operation failed\n");
		return;
	}

	printk("NEXT SEGMENT operation succeeded\n");
}

static void test_cp_first_segment(void)
{
	struct mpl_op_t op;

	op.opcode = MEDIA_PROXY_OP_FIRST_SEGMENT;
	op.use_param = false;

	test_set_cp_wait_flags(op);

	if (g_control_point_result != MEDIA_PROXY_OP_SUCCESS) {
		FAIL("FIRST SEGMENT operation failed\n");
		return;
	}

	printk("FIRST SEGMENT operation succeeded\n");
}

static void test_cp_last_segment(void)
{
	struct mpl_op_t op;

	op.opcode = MEDIA_PROXY_OP_LAST_SEGMENT;
	op.use_param = false;

	test_set_cp_wait_flags(op);

	if (g_control_point_result != MEDIA_PROXY_OP_SUCCESS) {
		FAIL("LAST SEGMENT operation failed\n");
		return;
	}

	printk("LAST SEGMENT operation succeeded\n");
}

static void test_cp_goto_segment(void)
{
	struct mpl_op_t op;

	op.opcode = MEDIA_PROXY_OP_GOTO_SEGMENT;
	op.use_param = true;
	op.param = 2;    /* Second segment - not the first, maybe not last */

	test_set_cp_wait_flags(op);

	if (g_control_point_result != MEDIA_PROXY_OP_SUCCESS) {
		FAIL("GOTO SEGMENT operation failed\n");
		return;
	}

	printk("GOTO SEGMENT operation succeeded\n");
}

/* Helper function to read the current track object ID, including flag handling
 * Will FAIL on error reading object ID
 * Will WAIT until the read is completed (object ID flag read flag is set)
 */
static void test_read_current_track_object_id_wait_flags(void)
{
	int err;

	UNSET_FLAG(current_track_object_id_read);
	err = media_proxy_ctrl_current_track_id_get(plr);
	if (err) {
		FAIL("Failed to read current track object ID: %d", err);
		return;
	}

	WAIT_FOR_FLAG(current_track_object_id_read);
}

static void test_cp_prev_track(void)
{
	uint64_t object_id;
	struct mpl_op_t op;

	/* Assumes that the server is in a state where it has multiple tracks
	 * and can change between them.
	 */

	/* To verify that a track change has happeded, the test checks that the
	 * current track object ID has changed.
	 */

	op.opcode = MEDIA_PROXY_OP_PREV_TRACK;
	op.use_param = false;

	test_read_current_track_object_id_wait_flags();
	object_id = g_current_track_object_id;

	test_set_cp_wait_flags(op);

	if (g_control_point_result != MEDIA_PROXY_OP_SUCCESS) {
		FAIL("PREV TRACK operation failed\n");
		return;
	}

	test_read_current_track_object_id_wait_flags();

	if (g_current_track_object_id == object_id) {
		/* Track did not change */
		FAIL("Server did not change track\n");
		return;
	}

	printk("PREV TRACK operation succeeded\n");
}

static void test_cp_next_track(void)
{
	uint64_t object_id;
	struct mpl_op_t op;

	op.opcode = MEDIA_PROXY_OP_NEXT_TRACK;
	op.use_param = false;

	test_read_current_track_object_id_wait_flags();
	object_id = g_current_track_object_id;

	test_set_cp_wait_flags(op);

	if (g_control_point_result != MEDIA_PROXY_OP_SUCCESS) {
		FAIL("NEXT TRACK operation failed\n");
		return;
	}

	test_read_current_track_object_id_wait_flags();

	if (g_current_track_object_id == object_id) {
		FAIL("Server did not change track\n");
		return;
	}

	printk("NEXT TRACK operation succeeded\n");
}

static void test_cp_first_track(void)
{
	uint64_t object_id;
	struct mpl_op_t op;

	op.opcode = MEDIA_PROXY_OP_FIRST_TRACK;
	op.use_param = false;

	test_read_current_track_object_id_wait_flags();
	object_id = g_current_track_object_id;

	test_set_cp_wait_flags(op);

	if (g_control_point_result != MEDIA_PROXY_OP_SUCCESS) {
		FAIL("FIRST TRACK operation failed\n");
		return;
	}

	test_read_current_track_object_id_wait_flags();

	if (g_current_track_object_id == object_id) {
		FAIL("Server did not change track\n");
		return;
	}

	printk("FIRST TRACK operation succeeded\n");
}

static void test_cp_last_track(void)
{
	uint64_t object_id;
	struct mpl_op_t op;

	op.opcode = MEDIA_PROXY_OP_LAST_TRACK;
	op.use_param = false;

	test_read_current_track_object_id_wait_flags();
	object_id = g_current_track_object_id;

	test_set_cp_wait_flags(op);

	if (g_control_point_result != MEDIA_PROXY_OP_SUCCESS) {
		FAIL("LAST TRACK operation failed\n");
		return;
	}

	test_read_current_track_object_id_wait_flags();

	if (g_current_track_object_id == object_id) {
		FAIL("Server did not change track\n");
		return;
	}

	printk("LAST TRACK operation succeeded\n");
}

static void test_cp_goto_track(void)
{
	uint64_t object_id;
	struct mpl_op_t op;

	op.opcode = MEDIA_PROXY_OP_GOTO_TRACK;
	op.use_param = true;
	op.param = 2; /* Second track, not the first, maybe not the last */

	test_read_current_track_object_id_wait_flags();
	object_id = g_current_track_object_id;

	test_set_cp_wait_flags(op);

	if (g_control_point_result != MEDIA_PROXY_OP_SUCCESS) {
		FAIL("GOTO TRACK operation failed\n");
		return;
	}

	test_read_current_track_object_id_wait_flags();

	if (g_current_track_object_id == object_id) {
		FAIL("Server did not change track\n");
		return;
	}

	printk("GOTO TRACK operation succeeded\n");
}

/* Helper function to read the current group object ID, including flag handling
 * Will FAIL on error reading object ID
 * Will WAIT until the read is completed (object ID flag read flag is set)
 */
static void test_read_current_group_object_id_wait_flags(void)
{
	int err;

	UNSET_FLAG(current_group_object_id_read);
	err = media_proxy_ctrl_current_group_id_get(plr);
	if (err) {
		FAIL("Failed to read current group object ID: %d", err);
		return;
	}

	WAIT_FOR_FLAG(current_group_object_id_read);
}

static void test_cp_prev_group(void)
{
	uint64_t object_id;
	struct mpl_op_t op;

	/* Assumes that the server is in a state where it has multiple groups
	 * and can change between them.
	 */

	/* To verify that a group change has happeded, the test checks that the
	 * current group object ID has changed.
	 */

	op.opcode = MEDIA_PROXY_OP_PREV_GROUP;
	op.use_param = false;

	test_read_current_group_object_id_wait_flags();
	object_id = g_current_group_object_id;

	test_set_cp_wait_flags(op);

	if (g_control_point_result != MEDIA_PROXY_OP_SUCCESS) {
		FAIL("PREV GROUP operation failed\n");
		return;
	}

	test_read_current_group_object_id_wait_flags();

	if (g_current_group_object_id == object_id) {
		/* Group did not change */
		FAIL("Server did not change group\n");
		return;
	}

	printk("PREV GROUP operation succeeded\n");
}

static void test_cp_next_group(void)
{
	uint64_t object_id;
	struct mpl_op_t op;

	op.opcode = MEDIA_PROXY_OP_NEXT_GROUP;
	op.use_param = false;

	test_read_current_group_object_id_wait_flags();
	object_id = g_current_group_object_id;

	test_set_cp_wait_flags(op);

	if (g_control_point_result != MEDIA_PROXY_OP_SUCCESS) {
		FAIL("NEXT GROUP operation failed\n");
		return;
	}

	test_read_current_group_object_id_wait_flags();

	if (g_current_group_object_id == object_id) {
		FAIL("Server did not change group\n");
		return;
	}

	printk("NEXT GROUP operation succeeded\n");
}

static void test_cp_first_group(void)
{
	uint64_t object_id;
	struct mpl_op_t op;

	op.opcode = MEDIA_PROXY_OP_FIRST_GROUP;
	op.use_param = false;

	test_read_current_group_object_id_wait_flags();
	object_id = g_current_group_object_id;

	test_set_cp_wait_flags(op);

	if (g_control_point_result != MEDIA_PROXY_OP_SUCCESS) {
		FAIL("FIRST GROUP operation failed\n");
		return;
	}

	test_read_current_group_object_id_wait_flags();

	if (g_current_group_object_id == object_id) {
		FAIL("Server did not change group\n");
		return;
	}

	printk("FIRST GROUP operation succeeded\n");
}

static void test_cp_last_group(void)
{
	uint64_t object_id;
	struct mpl_op_t op;

	op.opcode = MEDIA_PROXY_OP_LAST_GROUP;
	op.use_param = false;

	test_read_current_group_object_id_wait_flags();
	object_id = g_current_group_object_id;

	test_set_cp_wait_flags(op);

	if (g_control_point_result != MEDIA_PROXY_OP_SUCCESS) {
		FAIL("LAST GROUP operation failed\n");
		return;
	}

	test_read_current_group_object_id_wait_flags();

	if (g_current_group_object_id == object_id) {
		FAIL("Server did not change group\n");
		return;
	}

	printk("LAST GROUP operation succeeded\n");
}

static void test_cp_goto_group(void)
{
	uint64_t object_id;
	struct mpl_op_t op;

	op.opcode = MEDIA_PROXY_OP_GOTO_GROUP;
	op.use_param = true;
	op.param = 2; /* Second group, not the first, maybe not the last */

	test_read_current_group_object_id_wait_flags();
	object_id = g_current_group_object_id;

	test_set_cp_wait_flags(op);

	if (g_control_point_result != MEDIA_PROXY_OP_SUCCESS) {
		FAIL("GOTO GROUP operation failed\n");
		return;
	}

	test_read_current_group_object_id_wait_flags();

	if (g_current_group_object_id == object_id) {
		FAIL("Server did not change group\n");
		return;
	}

	printk("GOTO GROUP operation succeeded\n");
}

static void test_scp(void)
{
	struct mpl_search_t search;
	struct mpl_sci_t sci = {0};
	int err;

	/* Test outline:
	 * - verify that the search results object ID is zero before search
	 * - write a search (one search control item) to the search control point,
	 *   get write callback and notification
	 * - verify that the search results object ID is non-zero
	 */

	UNSET_FLAG(search_results_object_id_read);
	err = media_proxy_ctrl_search_results_id_get(plr);

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

	UNSET_FLAG(search_flag);
	UNSET_FLAG(search_results_object_id_read);

	err = media_proxy_ctrl_search_set(plr, search);
	if (err) {
		FAIL("Failed to write to search control point\n");
		return;
	}

	WAIT_FOR_FLAG(search_flag);

	if (g_search_control_point_result != MEDIA_PROXY_SEARCH_SUCCESS) {
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

/* This function tests all commands in the API in sequence
 * The order of the sequence follows the order of the characterstics in the
 * Media Control Service specification
 */
void test_media_controller_standalone(void)
{
	int err;

	printk("Media Control Client test application.  Board: %s\n", CONFIG_BOARD);

	UNSET_FLAG(ble_is_initialized);
	err = bt_enable(bt_ready);
	if (err) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	WAIT_FOR_FLAG(ble_is_initialized);
	printk("Bluetooth initialized\n");

	/* Initialize media  ********************************************/
	do_media_init();
	printk("media init succeeded\n");

	/* Read media player name ******************************************/
	UNSET_FLAG(player_name_read);
	err = media_proxy_ctrl_player_name_get(plr);
	if (err) {
		FAIL("Failed to read media player name ID: %d", err);
		return;
	}

	WAIT_FOR_FLAG(player_name_read);
	printk("Player Name read succeeded\n");

	/* Read icon object id  ******************************************/
	UNSET_FLAG(icon_object_id_read);
	err = media_proxy_ctrl_icon_id_get(plr);
	if (err) {
		FAIL("Failed to read icon object ID: %d", err);
		return;
	}

	WAIT_FOR_FLAG(icon_object_id_read);
	printk("Icon Object ID read succeeded\n");

	/* Read icon url *************************************************/
	UNSET_FLAG(icon_url_read);
	err =  media_proxy_ctrl_icon_url_get(plr);
	if (err) {
		FAIL("Failed to read icon url: %d", err);
		return;
	}

	WAIT_FOR_FLAG(icon_url_read);
	printk("Icon URL read succeeded\n");

	/* Read track_title ******************************************/
	UNSET_FLAG(track_title_read);
	err = media_proxy_ctrl_track_title_get(plr);
	if (err) {
		FAIL("Failed to read track_title: %d", err);
		return;
	}

	WAIT_FOR_FLAG(track_title_read);
	printk("Track title read succeeded\n");

	/* Read track_duration ******************************************/
	UNSET_FLAG(track_duration_read);
	err = media_proxy_ctrl_track_duration_get(plr);
	if (err) {
		FAIL("Failed to read track_duration: %d", err);
		return;
	}

	WAIT_FOR_FLAG(track_duration_read);
	printk("Track duration read succeeded\n");

	/* Read and set track_position *************************************/
	UNSET_FLAG(track_position);
	err = media_proxy_ctrl_track_position_get(plr);
	if (err) {
		FAIL("Failed to read track position: %d", err);
		return;
	}

	WAIT_FOR_FLAG(track_position);
	printk("Track position read succeeded\n");

	int32_t pos = g_pos + 1200; /*12 seconds further into the track */

	UNSET_FLAG(track_position);
	err = media_proxy_ctrl_track_position_set(plr, pos);
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
	err = media_proxy_ctrl_playback_speed_get(plr);
	if (err) {
		FAIL("Failed to read playback speed: %d", err);
		return;
	}

	WAIT_FOR_FLAG(playback_speed);
	printk("Playback speed read succeeded\n");

	int8_t pb_speed = g_pb_speed + 8; /* 2^(8/64) faster than current speed */

	UNSET_FLAG(playback_speed);
	err = media_proxy_ctrl_playback_speed_set(plr, pb_speed);
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
	err = media_proxy_ctrl_seeking_speed_get(plr);
	if (err) {
		FAIL("Failed to read seeking speed: %d", err);
		return;
	}

	WAIT_FOR_FLAG(seeking_speed_read);
	printk("Seeking speed read succeeded\n");

	/* Read track segments object *****************************************/
	UNSET_FLAG(track_segments_object_id_read);
	err = media_proxy_ctrl_track_segments_id_get(plr);
	if (err) {
		FAIL("Failed to read track segments object ID: %d", err);
		return;
	}

	WAIT_FOR_FLAG(track_segments_object_id_read);
	printk("Track Segments Object ID read succeeded\n");

	/* Read current track object ******************************************/
	UNSET_FLAG(current_track_object_id_read);
	err = media_proxy_ctrl_current_track_id_get(plr);
	if (err) {
		FAIL("Failed to read current track object ID: %d", err);
		return;
	}

	WAIT_FOR_FLAG(current_track_object_id_read);
	printk("Current Track Object ID read succeeded\n");

	/* Read next track object ******************************************/
	UNSET_FLAG(next_track_object_id_read);
	err = media_proxy_ctrl_next_track_id_get(plr);
	if (err) {
		FAIL("Failed to read next track object ID: %d", err);
		return;
	}

	WAIT_FOR_FLAG(next_track_object_id_read);
	printk("Next Track Object ID read succeeded\n");

	/* Read current group object ******************************************/
	UNSET_FLAG(current_group_object_id_read);
	err = media_proxy_ctrl_current_group_id_get(plr);
	if (err) {
		FAIL("Failed to read current group object ID: %d", err);
		return;
	}

	WAIT_FOR_FLAG(current_group_object_id_read);
	printk("Current Group Object ID read succeeded\n");

	/* Read parent group object ******************************************/
	UNSET_FLAG(parent_group_object_id_read);
	err = media_proxy_ctrl_parent_group_id_get(plr);
	if (err) {
		FAIL("Failed to read parent group object ID: %d", err);
		return;
	}

	WAIT_FOR_FLAG(parent_group_object_id_read);
	printk("Parent Group Object ID read succeeded\n");

	/* Read and set playing order *************************************/
	UNSET_FLAG(playing_order_flag);
	err = media_proxy_ctrl_playing_order_get(plr);
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
	err = media_proxy_ctrl_playing_order_set(plr, playing_order);
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
	err = media_proxy_ctrl_playing_orders_supported_get(plr);
	if (err) {
		FAIL("Failed to read playing orders supported: %d", err);
		return;
	}

	WAIT_FOR_FLAG(playing_orders_supported_read);
	printk("Playing orders supported read succeeded\n");

	/* Read media state  ***************************************************/
	UNSET_FLAG(media_state_read);
	err = media_proxy_ctrl_media_state_get(plr);
	if (err) {
		FAIL("Failed to read media state: %d", err);
		return;
	}

	WAIT_FOR_FLAG(media_state_read);
	printk("Media state read succeeded\n");

	/* Read content control ID  *******************************************/
	UNSET_FLAG(ccid_read);
	err = media_proxy_ctrl_content_ctrl_id_get(plr);
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

	/* The tests are ordered to ensure that each operation changes state */
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
	/* The tests are ordered to ensure that each operation changes track */
	/* Assumes we are not starting on the last track */
	test_cp_next_track();
	test_cp_prev_track();
	test_cp_last_track();
	test_cp_first_track();
	test_cp_goto_track();


	/* Control point - group change opcodes *******************************/
	/* The tests are ordered to ensure that each operation changes group */
	/* Assumes we are not starting on the last group */
	test_cp_next_group();
	test_cp_prev_group();
	test_cp_last_group();
	test_cp_first_group();
	test_cp_goto_group();


	/* Search control point */
	test_scp();


	/* TEST IS COMPLETE */
	PASS("Test media_controller_standalone passed\n");
}

static const struct bst_test_instance test_media_controller[] = {
	{
		.test_id = "media_controller_standalone",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_media_controller_standalone
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_media_controller_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_media_controller);
}
