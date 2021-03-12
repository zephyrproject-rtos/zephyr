/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
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

#include "../../../../subsys/bluetooth/host/audio/mpl.h"
#include "../../../../subsys/bluetooth/host/audio/otc.h"

#include "common.h"

extern enum bst_result_t bst_result;
static uint8_t expected_passes = 1;
static uint8_t passes;

static struct bt_conn *default_conn;
static struct bt_mcc_cb_t mcc_cb;

static uint64_t g_icon_object_id;
static uint64_t g_track_segments_object_id;
static uint64_t g_current_track_object_id;
static uint64_t g_next_track_object_id;
static uint64_t g_current_group_object_id;
static uint64_t g_parent_group_object_id;

static int32_t g_pos;
static int8_t  g_pb_speed;
static uint8_t g_playing_order;

CREATE_FLAG(ble_is_initialized);
CREATE_FLAG(ble_link_is_ready);
CREATE_FLAG(mcc_is_initialized);
CREATE_FLAG(discovery_done);
CREATE_FLAG(player_name_read);
CREATE_FLAG(icon_object_id_read);
CREATE_FLAG(icon_uri_read);
CREATE_FLAG(track_title_read);
CREATE_FLAG(track_duration_read);
CREATE_FLAG(track_position_read);
CREATE_FLAG(track_position_set);
CREATE_FLAG(playback_speed_read);
CREATE_FLAG(playback_speed_set);
CREATE_FLAG(seeking_speed_read);
CREATE_FLAG(track_segments_object_id_read);
CREATE_FLAG(current_track_object_id_read);
CREATE_FLAG(next_track_object_id_read);
CREATE_FLAG(current_group_object_id_read);
CREATE_FLAG(parent_group_object_id_read);
CREATE_FLAG(playing_order_read);
CREATE_FLAG(playing_order_set);
CREATE_FLAG(playing_orders_supported_read);
CREATE_FLAG(ccid_read);
CREATE_FLAG(media_state_read);
CREATE_FLAG(object_selected);
CREATE_FLAG(metadata_read);
CREATE_FLAG(object_read);


static void mcc_init_cb(struct bt_conn *conn, int err)
{
	if (err) {
		FAIL("MCC init failed (%d)\n", err);
		return;
	}

	SET_FLAG(mcc_is_initialized);
}

static void mcc_discover_mcs_cb(struct bt_conn *conn, int err)
{
	if (err) {
		FAIL("Discovery of MCS failed (%d)\n", err);
		return;
	}

	SET_FLAG(discovery_done);
}

static void mcc_player_name_read_cb(struct bt_conn *conn, int err, char *name)
{
	if (err) {
		FAIL("Player Name read failed (%d)\n", err);
		return;
	}

	SET_FLAG(player_name_read);
}

static void mcc_icon_obj_id_read_cb(struct bt_conn *conn, int err, uint64_t id)
{
	if (err) {
		FAIL("Icon Object ID read failed (%d)", err);
		return;
	}

	g_icon_object_id = id;
	SET_FLAG(icon_object_id_read);
}

static void mcc_icon_uri_read_cb(struct bt_conn *conn, int err, char *uri)
{
	if (err) {
		FAIL("Icon URI read failed (%d)", err);
		return;
	}

	SET_FLAG(icon_uri_read);
}

static void mcc_track_title_read_cb(struct bt_conn *conn, int err, char *title)
{
	if (err) {
		FAIL("Track title read failed (%d)", err);
		return;
	}

	SET_FLAG(track_title_read);
}

static void mcc_track_dur_read_cb(struct bt_conn *conn, int err, int32_t dur)
{
	if (err) {
		FAIL("Track duration read failed (%d)", err);
		return;
	}

	SET_FLAG(track_duration_read);
}

static void mcc_track_position_read_cb(struct bt_conn *conn, int err, int32_t pos)
{
	if (err) {
		FAIL("Track position read failed (%d)", err);
		return;
	}

	g_pos = pos;
	SET_FLAG(track_position_read);
}

static void mcc_track_position_set_cb(struct bt_conn *conn, int err, int32_t pos)
{
	if (err) {
		FAIL("Track Position set failed (%d)", err);
		return;
	}

	g_pos = pos;
	SET_FLAG(track_position_set);
}

static void mcc_playback_speed_read_cb(struct bt_conn *conn, int err,
				       int8_t speed)
{
	if (err) {
		FAIL("Playback speed read failed (%d)", err);
		return;
	}

	g_pb_speed = speed;
	SET_FLAG(playback_speed_read);
}

static void mcc_playback_speed_set_cb(struct bt_conn *conn, int err, int8_t speed)
{
	if (err) {
		FAIL("Playback speed set failed (%d)", err);
		return;
	}

	g_pb_speed = speed;
	SET_FLAG(playback_speed_set);
}

static void mcc_seeking_speed_read_cb(struct bt_conn *conn, int err,
				      int8_t speed)
{
	if (err) {
		FAIL("Seeking speed read failed (%d)", err);
		return;
	}

	SET_FLAG(seeking_speed_read);
}

static void mcc_segments_obj_id_read_cb(struct bt_conn *conn, int err,
					uint64_t id)
{
	if (err) {
		FAIL("Track Segments ID read failed (%d)\n", err);
		return;
	}

	g_track_segments_object_id = id;
	SET_FLAG(track_segments_object_id_read);
}

static void mcc_current_track_obj_id_read_cb(struct bt_conn *conn, int err,
					     uint64_t id)
{
	if (err) {
		FAIL("Current Track Object ID read failed (%d)\n", err);
		return;
	}

	g_current_track_object_id = id;
	SET_FLAG(current_track_object_id_read);
}

static void mcc_next_track_obj_id_read_cb(struct bt_conn *conn, int err,
					     uint64_t id)
{
	if (err) {
		FAIL("Next Track Object ID read failed (%d)\n", err);
		return;
	}

	g_next_track_object_id = id;
	SET_FLAG(next_track_object_id_read);
}

static void mcc_current_group_obj_id_read_cb(struct bt_conn *conn, int err,
					     uint64_t id)
{
	if (err) {
		FAIL("Current Group Object ID read failed (%d)\n", err);
		return;
	}

	g_current_group_object_id = id;
	SET_FLAG(current_group_object_id_read);
}

static void mcc_parent_group_obj_id_read_cb(struct bt_conn *conn, int err,
					    uint64_t id)
{
	if (err) {
		FAIL("Parent Group Object ID read failed (%d)\n", err);
		return;
	}

	g_parent_group_object_id = id;
	SET_FLAG(parent_group_object_id_read);
}

static void mcc_playing_order_read_cb(struct bt_conn *conn, int err, uint8_t order)
{
	if (err) {
		FAIL("Playing order read failed (%d)", err);
		return;
	}

	g_playing_order = order;
	SET_FLAG(playing_order_read);
}

static void mcc_playing_order_set_cb(struct bt_conn *conn, int err, uint8_t order)
{
	if (err) {
		FAIL("Playing order set failed (%d)", err);
		return;
	}

	g_playing_order = order;
	SET_FLAG(playing_order_set);
}

static void mcc_playing_orders_supported_read_cb(struct bt_conn *conn, int err,
						 uint16_t orders)
{
	if (err) {
		FAIL("Playing orders supported read failed (%d)", err);
		return;
	}

	SET_FLAG(playing_orders_supported_read);
}

static void mcc_media_state_read_cb(struct bt_conn *conn, int err, uint8_t state)
{
	if (err) {
		FAIL("Media State read failed (%d)", err);
		return;
	}

	SET_FLAG(media_state_read);
}

static void mcc_content_control_id_read_cb(struct bt_conn *conn, int err, uint8_t ccid)
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

static void mcc_otc_read_current_group_object_cb(struct bt_conn *conn, int err,
						 struct net_buf_simple *buf)
{
	if (err) {
		FAIL("Current Group Object read failed (%d)", err);
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

int do_mcc_init(void)
{
	/* Set up the callbacks */
	mcc_cb.init             = &mcc_init_cb;
	mcc_cb.discover_mcs     = &mcc_discover_mcs_cb;
	mcc_cb.player_name_read = &mcc_player_name_read_cb;
	mcc_cb.icon_obj_id_read = &mcc_icon_obj_id_read_cb;
	mcc_cb.icon_uri_read    = &mcc_icon_uri_read_cb;
	mcc_cb.track_title_read = &mcc_track_title_read_cb;
	mcc_cb.track_dur_read   = &mcc_track_dur_read_cb;
	mcc_cb.track_position_read = &mcc_track_position_read_cb;
	mcc_cb.track_position_set  = &mcc_track_position_set_cb;
	mcc_cb.playback_speed_read = &mcc_playback_speed_read_cb;
	mcc_cb.playback_speed_set  = &mcc_playback_speed_set_cb;
	mcc_cb.seeking_speed_read  = &mcc_seeking_speed_read_cb;
	mcc_cb.current_track_obj_id_read = &mcc_current_track_obj_id_read_cb;
	mcc_cb.next_track_obj_id_read    = &mcc_next_track_obj_id_read_cb;
	mcc_cb.segments_obj_id_read      = &mcc_segments_obj_id_read_cb;
	mcc_cb.current_group_obj_id_read = &mcc_current_group_obj_id_read_cb;
	mcc_cb.parent_group_obj_id_read  = &mcc_parent_group_obj_id_read_cb;
	mcc_cb.playing_order_read        = &mcc_playing_order_read_cb;
	mcc_cb.playing_order_set         = &mcc_playing_order_set_cb;
	mcc_cb.playing_orders_supported_read = &mcc_playing_orders_supported_read_cb;
	mcc_cb.media_state_read = &mcc_media_state_read_cb;
	mcc_cb.content_control_id_read = &mcc_content_control_id_read_cb;
	mcc_cb.otc_obj_selected = &mcc_otc_obj_selected_cb;
	mcc_cb.otc_obj_metadata = &mcc_otc_obj_metadata_cb;
	mcc_cb.otc_icon_object  = &mcc_icon_object_read_cb;
	mcc_cb.otc_track_segments_object = &mcc_track_segments_object_read_cb;
	mcc_cb.otc_current_track_object  = &mcc_otc_read_current_track_object_cb;
	mcc_cb.otc_next_track_object     = &mcc_otc_read_next_track_object_cb;
	mcc_cb.otc_current_group_object  = &mcc_otc_read_current_group_object_cb;
	mcc_cb.otc_parent_group_object   = &mcc_otc_read_parent_group_object_cb;

	/* Initialize the module */
	return bt_mcc_init(default_conn, &mcc_cb);
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

/* Callback on connection */
static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		FAIL("Failed to connect to %s (%u)\n", addr, err);
		return;
	}

	default_conn = conn;
	SET_FLAG(ble_link_is_ready);
}

/* Helper function - select object and read the object metadata
 *
 * Will FAIL the test on errors calling select and read metadata.
 * Will WAIT (hang) until callbacks are received.
 * If callbacks are not received, the test fill FAIL due to timeout.
 *
 * @param object_id    ID of the object to select and read metadata for
 */
static void select_read_meta(int64_t id)
{
	int err;

	/* TODO: Fix the instance pointer - it is neither valid nor used */
	UNSET_FLAG(object_selected);
	err = bt_otc_select_id(default_conn, bt_mcc_otc_inst(), id);
	if (err) {
		FAIL("Failed to select object\n");
		return;
	}

	WAIT_FOR_FLAG(object_selected);
	printk("Selecting object succeeded\n");

	/* TODO: Fix the instance pointer - it is neither valid nor used */
	UNSET_FLAG(metadata_read);
	err = bt_otc_obj_metadata_read(default_conn, bt_mcc_otc_inst(),
				       BT_OTC_METADATA_REQ_ALL);
	if (err) {
		FAIL("Failed to read object metadata\n");
		return;
	}

	WAIT_FOR_FLAG(metadata_read);
	printk("Reading object metadata succeeded\n");
}

/* This function tests all commands in the API in sequence
 * The order of the sequence follows the order of the characterstics in the
 * Media Control Service specification
 */
void test_main(void)
{
	int err;
	static struct bt_conn_cb conn_callbacks = {
		.connected = connected,
		.disconnected = disconnected,
	};

	printk("Media Control Client test application.  Board: %s\n", CONFIG_BOARD);

	UNSET_FLAG(ble_is_initialized);
	err = bt_enable(bt_ready);
	if (err) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	WAIT_FOR_FLAG(ble_is_initialized);
	printk("Bluetooth initialized\n");

	bt_conn_cb_register(&conn_callbacks);

	/* Connect ******************************************/
	UNSET_FLAG(ble_link_is_ready);
	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err) {
		FAIL("Failed to start scanning (err %d\n)", err);
	} else {
		printk("Scanning started successfully\n");
	}

	WAIT_FOR_FLAG(ble_link_is_ready);

	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(default_conn), addr, sizeof(addr));
	printk("Connected: %s\n", addr);

	/* Initialize MCC  ********************************************/
	UNSET_FLAG(mcc_is_initialized);
	do_mcc_init();
	WAIT_FOR_FLAG(mcc_is_initialized);
	printk("MCC init succeeded\n");

	/* Discover MCS, subscribe to notifications *******************/
	UNSET_FLAG(discovery_done);
	err = bt_mcc_discover_mcs(default_conn, true);
	if (err) {
		FAIL("Failed to start discovery of MCS: %d\n", err);
	}

	WAIT_FOR_FLAG(discovery_done);
	printk("Discovery of MCS succeeded\n");

	/* Read media player name ******************************************/
	UNSET_FLAG(player_name_read);
	err = bt_mcc_read_player_name(default_conn);
	if (err) {
		FAIL("Failed to read media player name ID: %d", err);
		return;
	}

	WAIT_FOR_FLAG(player_name_read);
	printk("Player Name read succeeded\n");

	/* Read icon object ******************************************/
	UNSET_FLAG(icon_object_id_read);
	err = bt_mcc_read_icon_obj_id(default_conn);
	if (err) {
		FAIL("Failed to read icon object ID: %d", err);
		return;
	}

	WAIT_FOR_FLAG(icon_object_id_read);
	printk("Icon Object ID read succeeded\n");

	select_read_meta(g_icon_object_id);
	UNSET_FLAG(object_read);
	err = bt_mcc_otc_read_icon_object(default_conn);

	if (err) {
		FAIL("Failed to read icon object\n");
		return;
	}

	WAIT_FOR_FLAG(object_read);
	printk("Reading Icon Object succeeded\n");

	/* Read icon uri *************************************************/
	UNSET_FLAG(icon_uri_read);
	err = bt_mcc_read_icon_uri(default_conn);
	if (err) {
		FAIL("Failed to read icon uri: %d", err);
		return;
	}

	WAIT_FOR_FLAG(icon_uri_read);
	printk("Icon URI read succeeded\n");

	/* Read track_title ******************************************/
	UNSET_FLAG(track_title_read);
	err = bt_mcc_read_track_title(default_conn);
	if (err) {
		FAIL("Failed to read track_title: %d", err);
		return;
	}

	WAIT_FOR_FLAG(track_title_read);
	printk("Track title read succeeded\n");

	/* Read track_duration ******************************************/
	UNSET_FLAG(track_duration_read);
	err = bt_mcc_read_track_dur(default_conn);
	if (err) {
		FAIL("Failed to read track_duration: %d", err);
		return;
	}

	WAIT_FOR_FLAG(track_duration_read);
	printk("Track duration read succeeded\n");

	/* Read and set track_position *************************************/
	UNSET_FLAG(track_position_read);
	err = bt_mcc_read_track_position(default_conn);
	if (err) {
		FAIL("Failed to read track position: %d", err);
		return;
	}

	WAIT_FOR_FLAG(track_position_read);
	printk("Track position read succeeded\n");

	int32_t pos = g_pos + 1200; /*12 seconds further into the track */

	UNSET_FLAG(track_position_set);
	err = bt_mcc_set_track_position(default_conn, pos);
	if (err) {
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

	/* Read and set playback speed *************************************/
	UNSET_FLAG(playback_speed_read);
	err = bt_mcc_read_playback_speed(default_conn);
	if (err) {
		FAIL("Failed to read playback speed: %d", err);
		return;
	}

	WAIT_FOR_FLAG(playback_speed_read);
	printk("Playback speed read succeeded\n");

	int8_t pb_speed = g_pb_speed + 8; /* 2^(8/64) faster than current speed */

	UNSET_FLAG(playback_speed_set);
	err = bt_mcc_set_playback_speed(default_conn, pb_speed);
	if (err) {
		FAIL("Failed to set playback speed: %d", err);
		return;
	}

	WAIT_FOR_FLAG(playback_speed_set);
	if (g_pb_speed != pb_speed) {
		FAIL("Playback speed failed: Incorrect playback speed\n");
	}
	printk("Playback speed set succeeded\n");

	/* Read seeking speed *************************************/
	UNSET_FLAG(seeking_speed_read);
	err = bt_mcc_read_seeking_speed(default_conn);
	if (err) {
		FAIL("Failed to read seeking speed: %d", err);
		return;
	}

	WAIT_FOR_FLAG(seeking_speed_read);
	printk("Seeking speed read succeeded\n");

	/* Read track segments object *****************************************/
	UNSET_FLAG(track_segments_object_id_read);
	err = bt_mcc_read_segments_obj_id(default_conn);
	if (err) {
		FAIL("Failed to read track segments object ID: %d", err);
		return;
	}

	WAIT_FOR_FLAG(track_segments_object_id_read);
	printk("Track Segments Object ID read succeeded\n");

	select_read_meta(g_track_segments_object_id);
	UNSET_FLAG(object_read);
	err = bt_mcc_otc_read_track_segments_object(default_conn);

	if (err) {
		FAIL("Failed to read track segments object\n");
		return;
	}

	WAIT_FOR_FLAG(object_read);
	printk("Reading Track Segments Object succeeded\n");

	/* Read current track object ******************************************/
	UNSET_FLAG(current_track_object_id_read);
	err = bt_mcc_read_current_track_obj_id(default_conn);
	if (err) {
		FAIL("Failed to read current track object ID: %d", err);
		return;
	}

	WAIT_FOR_FLAG(current_track_object_id_read);
	printk("Current Track Object ID read succeeded\n");

	UNSET_FLAG(object_read);
	select_read_meta(g_current_track_object_id);
	err = bt_mcc_otc_read_current_track_object(default_conn);

	if (err) {
		FAIL("Failed to current track object\n");
		return;
	}

	WAIT_FOR_FLAG(object_read);
	printk("Current Track Object read succeeded\n");

	/* Read next track object ******************************************/
	UNSET_FLAG(next_track_object_id_read);
	err = bt_mcc_read_next_track_obj_id(default_conn);
	if (err) {
		FAIL("Failed to read next track object ID: %d", err);
		return;
	}

	WAIT_FOR_FLAG(next_track_object_id_read);
	printk("Next Track Object ID read succeeded\n");

	select_read_meta(g_next_track_object_id);
	UNSET_FLAG(object_read);
	err = bt_mcc_otc_read_next_track_object(default_conn);

	if (err) {
		FAIL("Failed to read next track object\n");
		return;
	}

	WAIT_FOR_FLAG(object_read);
	printk("Next Track Object read succeeded\n");

	/* Read current group object ******************************************/
	UNSET_FLAG(current_group_object_id_read);
	err = bt_mcc_read_current_group_obj_id(default_conn);
	if (err) {
		FAIL("Failed to read current group object ID: %d", err);
		return;
	}

	WAIT_FOR_FLAG(current_group_object_id_read);
	printk("Current Group Object ID read succeeded\n");

	select_read_meta(g_current_group_object_id);
	UNSET_FLAG(object_read);
	err = bt_mcc_otc_read_current_group_object(default_conn);

	if (err) {
		FAIL("Failed to read current group object\n");
		return;
	}

	WAIT_FOR_FLAG(object_read);
	printk("Current Group Object read succeeded\n");

	/* Read parent group object ******************************************/
	UNSET_FLAG(parent_group_object_id_read);
	err = bt_mcc_read_parent_group_obj_id(default_conn);
	if (err) {
		FAIL("Failed to read parent group object ID: %d", err);
		return;
	}

	WAIT_FOR_FLAG(parent_group_object_id_read);
	printk("Parent Group Object ID read succeeded\n");

	select_read_meta(g_parent_group_object_id);
	UNSET_FLAG(object_read);
	err = bt_mcc_otc_read_parent_group_object(default_conn);

	if (err) {
		FAIL("Failed to read parent group object\n");
		return;
	}

	WAIT_FOR_FLAG(object_read);
	printk("Parent Group Object read succeeded\n");

	/* Read and set playing order *************************************/
	UNSET_FLAG(playing_order_read);
	err = bt_mcc_read_playing_order(default_conn);
	if (err) {
		FAIL("Failed to read playing order: %d", err);
		return;
	}

	WAIT_FOR_FLAG(playing_order_read);
	printk("Playing order read succeeded\n");

	uint8_t playing_order;

	if (g_playing_order != MPL_PLAYING_ORDER_SHUFFLE_ONCE) {
		playing_order = MPL_PLAYING_ORDER_SHUFFLE_ONCE;
	} else {
		playing_order = MPL_PLAYING_ORDER_SINGLE_ONCE;
	}

	UNSET_FLAG(playing_order_set);
	err = bt_mcc_set_playing_order(default_conn, playing_order);
	if (err) {
		FAIL("Failed to set playing_order: %d", err);
		return;
	}

	WAIT_FOR_FLAG(playing_order_set);
	if (g_playing_order != playing_order) {
		FAIL("Playing order set failed: Incorrect playing_order\n");
	}
	printk("Playing order set succeeded\n");

	/* Read playing orders supported  *************************************/
	UNSET_FLAG(playing_orders_supported_read);
	err = bt_mcc_read_playing_orders_supported(default_conn);
	if (err) {
		FAIL("Failed to read playing orders supported: %d", err);
		return;
	}

	WAIT_FOR_FLAG(playing_orders_supported_read);
	printk("Playing orders supported read succeeded\n");

	/* Read media state  ***************************************************/
	UNSET_FLAG(media_state_read);
	err = bt_mcc_read_media_state(default_conn);
	if (err) {
		FAIL("Failed to read media state: %d", err);
		return;
	}

	WAIT_FOR_FLAG(media_state_read);
	printk("Media state read succeeded\n");

	/* Read content control ID  *******************************************/
	UNSET_FLAG(ccid_read);
	err = bt_mcc_read_content_control_id(default_conn);
	if (err) {
		FAIL("Failed to read content control ID: %d", err);
		return;
	}

	WAIT_FOR_FLAG(ccid_read);
	printk("Content control ID read succeeded\n");

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
