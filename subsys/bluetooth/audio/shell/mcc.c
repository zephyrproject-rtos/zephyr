/** @file
 *  @brief Media Control Client shell implementation
 *
 */

/*
 * Copyright (c) 2020 - 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/mcc.h>
#include <zephyr/bluetooth/audio/mcs.h>
#include <zephyr/bluetooth/audio/media_proxy.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/services/ots.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_string_conv.h>
#include <zephyr/sys/util.h>

#include "shell/bt.h"

#include "../media_proxy_internal.h"

LOG_MODULE_REGISTER(bt_mcc_shell, CONFIG_BT_MCC_LOG_LEVEL);

static struct bt_mcc_cb cb;

#ifdef CONFIG_BT_MCC_OTS
struct object_ids_t {
	uint64_t icon_obj_id;
	uint64_t track_segments_obj_id;
	uint64_t current_track_obj_id;
	uint64_t next_track_obj_id;
	uint64_t parent_group_obj_id;
	uint64_t current_group_obj_id;
	uint64_t search_results_obj_id;
};
static struct object_ids_t obj_ids;
#endif /* CONFIG_BT_MCC_OTS */


static void mcc_discover_mcs_cb(struct bt_conn *conn, int err)
{
	if (err) {
		shell_error(ctx_shell, "Discovery failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Discovery complete");
}

static void mcc_read_player_name_cb(struct bt_conn *conn, int err, const char *name)
{
	if (err) {
		shell_error(ctx_shell, "Player Name read failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Player name: %s", name);
}

#ifdef CONFIG_BT_MCC_OTS
static void mcc_read_icon_obj_id_cb(struct bt_conn *conn, int err, uint64_t id)
{
	char str[BT_OTS_OBJ_ID_STR_LEN];

	if (err) {
		shell_error(ctx_shell, "Icon Object ID read failed (%d)", err);
		return;
	}

	(void)bt_ots_obj_id_to_str(id, str, sizeof(str));
	shell_print(ctx_shell, "Icon object ID: %s", str);

	obj_ids.icon_obj_id = id;
}
#endif /* CONFIG_BT_MCC_OTS */

#if defined(CONFIG_BT_MCC_READ_MEDIA_PLAYER_ICON_URL)
static void mcc_read_icon_url_cb(struct bt_conn *conn, int err, const char *url)
{
	if (err) {
		shell_error(ctx_shell, "Icon URL read failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Icon URL: 0x%s", url);
}
#endif /* defined(CONFIG_BT_MCC_READ_MEDIA_PLAYER_ICON_URL) */

#if defined(CONFIG_BT_MCC_READ_TRACK_TITLE)
static void mcc_read_track_title_cb(struct bt_conn *conn, int err, const char *title)
{
	if (err) {
		shell_error(ctx_shell, "Track title read failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Track title: %s", title);
}
#endif /* defined(CONFIG_BT_MCC_READ_TRACK_TITLE) */

static void mcc_track_changed_ntf_cb(struct bt_conn *conn, int err)
{
	if (err) {
		shell_error(ctx_shell, "Track changed notification failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Track changed");
}

#if defined(CONFIG_BT_MCC_READ_TRACK_DURATION)
static void mcc_read_track_duration_cb(struct bt_conn *conn, int err, int32_t dur)
{
	if (err) {
		shell_error(ctx_shell, "Track duration read failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Track duration: %d", dur);
}
#endif /* defined(CONFIG_BT_MCC_READ_TRACK_DURATION) */

#if defined(CONFIG_BT_MCC_READ_TRACK_POSITION)
static void mcc_read_track_position_cb(struct bt_conn *conn, int err, int32_t pos)
{
	if (err) {
		shell_error(ctx_shell, "Track position read failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Track Position: %d", pos);
}
#endif /* defined(CONFIG_BT_MCC_READ_TRACK_POSITION) */

#if defined(CONFIG_BT_MCC_SET_TRACK_POSITION)
static void mcc_set_track_position_cb(struct bt_conn *conn, int err, int32_t pos)
{
	if (err) {
		shell_error(ctx_shell, "Track Position set failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Track Position: %d", pos);
}
#endif /* defined(CONFIG_BT_MCC_SET_TRACK_POSITION) */

#if defined(CONFIG_BT_MCC_READ_PLAYBACK_SPEED)
static void mcc_read_playback_speed_cb(struct bt_conn *conn, int err,
				       int8_t speed)
{
	if (err) {
		shell_error(ctx_shell, "Playback speed read failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Playback speed: %d", speed);
}
#endif /* defined (CONFIG_BT_MCC_READ_PLAYBACK_SPEED) */

#if defined(CONFIG_BT_MCC_SET_PLAYBACK_SPEED)
static void mcc_set_playback_speed_cb(struct bt_conn *conn, int err, int8_t speed)
{
	if (err) {
		shell_error(ctx_shell, "Playback speed set failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Playback speed: %d", speed);
}
#endif /* defined (CONFIG_BT_MCC_SET_PLAYBACK_SPEED) */

#if defined(CONFIG_BT_MCC_READ_SEEKING_SPEED)
static void mcc_read_seeking_speed_cb(struct bt_conn *conn, int err,
				      int8_t speed)
{
	if (err) {
		shell_error(ctx_shell, "Seeking speed read failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Seeking speed: %d", speed);
}
#endif /* defined (CONFIG_BT_MCC_READ_SEEKING_SPEED) */


#ifdef CONFIG_BT_MCC_OTS
static void mcc_read_segments_obj_id_cb(struct bt_conn *conn, int err,
					uint64_t id)
{
	char str[BT_OTS_OBJ_ID_STR_LEN];

	if (err) {
		shell_error(ctx_shell,
			    "Track Segments Object ID read failed (%d)", err);
		return;
	}

	(void)bt_ots_obj_id_to_str(id, str, sizeof(str));
	shell_print(ctx_shell, "Track Segments Object ID: %s", str);

	obj_ids.track_segments_obj_id = id;
}


static void mcc_read_current_track_obj_id_cb(struct bt_conn *conn, int err,
					     uint64_t id)
{
	char str[BT_OTS_OBJ_ID_STR_LEN];

	if (err) {
		shell_error(ctx_shell, "Current Track Object ID read failed (%d)",
			    err);
		return;
	}

	(void)bt_ots_obj_id_to_str(id, str, sizeof(str));
	shell_print(ctx_shell, "Current Track Object ID: %s", str);

	obj_ids.current_track_obj_id = id;
}


static void mcc_set_current_track_obj_id_cb(struct bt_conn *conn, int err,
					    uint64_t id)
{
	char str[BT_OTS_OBJ_ID_STR_LEN];

	if (err) {
		shell_error(ctx_shell, "Current Track Object ID set failed (%d)", err);
		return;
	}

	(void)bt_ots_obj_id_to_str(id, str, sizeof(str));
	shell_print(ctx_shell, "Current Track Object ID written: %s", str);
}


static void mcc_read_next_track_obj_id_cb(struct bt_conn *conn, int err,
					  uint64_t id)
{
	char str[BT_OTS_OBJ_ID_STR_LEN];

	if (err) {
		shell_error(ctx_shell, "Next Track Object ID read failed (%d)",
			    err);
		return;
	}

	if (id == MPL_NO_TRACK_ID) {
		shell_print(ctx_shell, "Next Track Object ID is empty");
	} else {
		(void)bt_ots_obj_id_to_str(id, str, sizeof(str));
		shell_print(ctx_shell, "Next Track Object ID: %s", str);
	}

	obj_ids.next_track_obj_id = id;
}


static void mcc_set_next_track_obj_id_cb(struct bt_conn *conn, int err,
					    uint64_t id)
{
	char str[BT_OTS_OBJ_ID_STR_LEN];

	if (err) {
		shell_error(ctx_shell, "Next Track Object ID set failed (%d)", err);
		return;
	}

	(void)bt_ots_obj_id_to_str(id, str, sizeof(str));
	shell_print(ctx_shell, "Next Track Object ID written: %s", str);
}


static void mcc_read_parent_group_obj_id_cb(struct bt_conn *conn, int err,
					    uint64_t id)
{
	char str[BT_OTS_OBJ_ID_STR_LEN];

	if (err) {
		shell_error(ctx_shell,
			    "Parent Group Object ID read failed (%d)", err);
		return;
	}

	(void)bt_ots_obj_id_to_str(id, str, sizeof(str));
	shell_print(ctx_shell, "Parent Group Object ID: %s", str);

	obj_ids.parent_group_obj_id = id;
}


static void mcc_read_current_group_obj_id_cb(struct bt_conn *conn, int err,
					     uint64_t id)
{
	char str[BT_OTS_OBJ_ID_STR_LEN];

	if (err) {
		shell_error(ctx_shell,
			    "Current Group Object ID read failed (%d)", err);
		return;
	}

	(void)bt_ots_obj_id_to_str(id, str, sizeof(str));
	shell_print(ctx_shell, "Current Group Object ID: %s", str);

	obj_ids.current_group_obj_id = id;
}

static void mcc_set_current_group_obj_id_cb(struct bt_conn *conn, int err,
					    uint64_t id)
{
	char str[BT_OTS_OBJ_ID_STR_LEN];

	if (err) {
		shell_error(ctx_shell, "Current Group Object ID set failed (%d)", err);
		return;
	}

	(void)bt_ots_obj_id_to_str(id, str, sizeof(str));
	shell_print(ctx_shell, "Current Group Object ID written: %s", str);
}
#endif /* CONFIG_BT_MCC_OTS */


#if defined(CONFIG_BT_MCC_READ_PLAYING_ORDER)
static void mcc_read_playing_order_cb(struct bt_conn *conn, int err, uint8_t order)
{
	if (err) {
		shell_error(ctx_shell, "Playing order read failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Playing order: %d", order);
}
#endif /* defined(CONFIG_BT_MCC_READ_PLAYING_ORDER) */

#if defined(CONFIG_BT_MCC_SET_PLAYING_ORDER)
static void mcc_set_playing_order_cb(struct bt_conn *conn, int err, uint8_t order)
{
	if (err) {
		shell_error(ctx_shell, "Playing order set failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Playing order: %d", order);
}
#endif /* defined(CONFIG_BT_MCC_SET_PLAYING_ORDER) */

#if defined(CONFIG_BT_MCC_READ_PLAYING_ORDER_SUPPORTED)
static void mcc_read_playing_orders_supported_cb(struct bt_conn *conn, int err,
						 uint16_t orders)
{
	if (err) {
		shell_error(ctx_shell,
			    "Playing orders supported read failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Playing orders supported: %d", orders);
}
#endif /* defined(CONFIG_BT_MCC_READ_PLAYING_ORDER_SUPPORTED) */

#if defined(CONFIG_BT_MCC_READ_MEDIA_STATE)
static void mcc_read_media_state_cb(struct bt_conn *conn, int err, uint8_t state)
{
	if (err) {
		shell_error(ctx_shell, "Media State read failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Media State: %d", state);
}
#endif /* defined(CONFIG_BT_MCC_READ_MEDIA_STATE) */

#if defined(CONFIG_BT_MCC_SET_MEDIA_CONTROL_POINT)
static void mcc_send_cmd_cb(struct bt_conn *conn, int err, const struct mpl_cmd *cmd)
{
	if (err) {
		shell_error(ctx_shell,
			    "Command send failed (%d) - opcode: %d, param: %d",
			    err, cmd->opcode, cmd->param);
		return;
	}

	shell_print(ctx_shell, "Command opcode: %d, param: %d", cmd->opcode, cmd->param);
}
#endif /* defined(CONFIG_BT_MCC_SET_MEDIA_CONTROL_POINT) */

static void mcc_cmd_ntf_cb(struct bt_conn *conn, int err,
			   const struct mpl_cmd_ntf *ntf)
{
	if (err) {
		shell_error(ctx_shell,
			    "Command notification error (%d) - opcode: %d, result: %d",
			    err, ntf->requested_opcode, ntf->result_code);
		return;
	}

	shell_print(ctx_shell, "Command opcode: %d, result: %d",
		    ntf->requested_opcode, ntf->result_code);
}

#if defined(CONFIG_BT_MCC_READ_MEDIA_CONTROL_POINT_OPCODES_SUPPORTED)
static void mcc_read_opcodes_supported_cb(struct bt_conn *conn, int err,
					    uint32_t opcodes)
{
	if (err) {
		shell_error(ctx_shell, "Opcodes supported read failed (%d)",
			    err);
		return;
	}

	shell_print(ctx_shell, "Opcodes supported: %d", opcodes);
}
#endif /* defined(CONFIG_BT_MCC_READ_MEDIA_CONTROL_POINT_OPCODES_SUPPORTED) */

#ifdef CONFIG_BT_MCC_OTS
static void mcc_send_search_cb(struct bt_conn *conn, int err,
			       const struct mpl_search *search)
{
	if (err) {
		shell_error(ctx_shell,
			    "Search send failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Search sent");
}

static void mcc_search_ntf_cb(struct bt_conn *conn, int err, uint8_t result_code)
{
	if (err) {
		shell_error(ctx_shell,
			    "Search notification error (%d), result code: %d",
			    err, result_code);
		return;
	}

	shell_print(ctx_shell, "Search notification result code: %d",
		    result_code);
}

static void mcc_read_search_results_obj_id_cb(struct bt_conn *conn, int err,
					      uint64_t id)
{
	char str[BT_OTS_OBJ_ID_STR_LEN];

	if (err) {
		shell_error(ctx_shell,
			    "Search Results Object ID read failed (%d)", err);
		return;
	}

	if (id == 0) {
		shell_print(ctx_shell, "Search Results Object ID: 0x000000000000");
	} else {
		(void)bt_ots_obj_id_to_str(id, str, sizeof(str));
		shell_print(ctx_shell, "Search Results Object ID: %s", str);
	}

	obj_ids.search_results_obj_id = id;
}
#endif /* CONFIG_BT_MCC_OTS */

#if defined(CONFIG_BT_MCC_READ_CONTENT_CONTROL_ID)
static void mcc_read_content_control_id_cb(struct bt_conn *conn, int err, uint8_t ccid)
{
	if (err) {
		shell_error(ctx_shell, "Content Control ID read failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Content Control ID: %d", ccid);
}
#endif /* defined(CONFIG_BT_MCC_READ_CONTENT_CONTROL_ID) */

#ifdef CONFIG_BT_MCC_OTS
/**** Callback functions for the included Object Transfer service *************/
static void mcc_otc_obj_selected_cb(struct bt_conn *conn, int err)
{
	if (err) {
		shell_error(ctx_shell,
			    "Error in selecting object (err %d)", err);
		return;
	}

	shell_print(ctx_shell, "Selecting object succeeded");
}

static void mcc_otc_obj_metadata_cb(struct bt_conn *conn, int err)
{
	if (err) {
		shell_error(ctx_shell,
			    "Error in reading object metadata (err %d)", err);
		return;
	}

	shell_print(ctx_shell, "Reading object metadata succeeded\n");
}

static void mcc_icon_object_read_cb(struct bt_conn *conn, int err,
				    struct net_buf_simple *buf)
{
	if (err) {
		shell_error(ctx_shell,
			    "Icon Object read failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Icon content (%d octets)", buf->len);
	shell_hexdump(ctx_shell, buf->data, buf->len);
}

/* TODO: May want to use a parsed type, instead of the raw buf, here */
static void mcc_track_segments_object_read_cb(struct bt_conn *conn, int err,
					      struct net_buf_simple *buf)
{
	if (err) {
		shell_error(ctx_shell,
			    "Track Segments Object read failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Track Segments content (%d octets)", buf->len);
	shell_hexdump(ctx_shell, buf->data, buf->len);
}

static void mcc_otc_read_current_track_object_cb(struct bt_conn *conn, int err,
						 struct net_buf_simple *buf)
{
	if (err) {
		shell_error(ctx_shell,
			    "Current Track Object read failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Current Track content (%d octets)", buf->len);
	shell_hexdump(ctx_shell, buf->data, buf->len);
}

static void mcc_otc_read_next_track_object_cb(struct bt_conn *conn, int err,
						 struct net_buf_simple *buf)
{
	if (err) {
		shell_error(ctx_shell,
			    "Next Track Object read failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Next Track content (%d octets)", buf->len);
	shell_hexdump(ctx_shell, buf->data, buf->len);
}

static void mcc_otc_read_parent_group_object_cb(struct bt_conn *conn, int err,
						struct net_buf_simple *buf)
{
	if (err) {
		shell_error(ctx_shell,
			    "Parent Group Object read failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Parent Group content (%d octets)", buf->len);
	shell_hexdump(ctx_shell, buf->data, buf->len);
}

static void mcc_otc_read_current_group_object_cb(struct bt_conn *conn, int err,
						 struct net_buf_simple *buf)
{
	if (err) {
		shell_error(ctx_shell,
			    "Current Group Object read failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Current Group content (%d octets)", buf->len);
	shell_hexdump(ctx_shell, buf->data, buf->len);
}

#endif /* CONFIG_BT_MCC_OTS */


static int cmd_mcc_init(const struct shell *sh, size_t argc, char **argv)
{
	int result;

	if (!ctx_shell) {
		ctx_shell = sh;
	}

	/* Set up the callbacks */
	cb.discover_mcs                  = mcc_discover_mcs_cb;
	cb.read_player_name              = mcc_read_player_name_cb;
#ifdef CONFIG_BT_MCC_OTS
	cb.read_icon_obj_id              = mcc_read_icon_obj_id_cb;
#endif /* CONFIG_BT_MCC_OTS */
#if defined(CONFIG_BT_MCC_READ_MEDIA_PLAYER_ICON_URL)
	cb.read_icon_url                 = mcc_read_icon_url_cb;
#endif /* defined(CONFIG_BT_MCC_READ_MEDIA_PLAYER_ICON_URL) */
	cb.track_changed_ntf             = mcc_track_changed_ntf_cb;
#if defined(CONFIG_BT_MCC_READ_TRACK_TITLE)
	cb.read_track_title              = mcc_read_track_title_cb;
#endif /* defined(CONFIG_BT_MCC_READ_TRACK_TITLE) */
#if defined(CONFIG_BT_MCC_READ_TRACK_DURATION)
	cb.read_track_duration           = mcc_read_track_duration_cb;
#endif /* defined(CONFIG_BT_MCC_READ_TRACK_DURATION) */
#if defined(CONFIG_BT_MCC_READ_TRACK_POSITION)
	cb.read_track_position           = mcc_read_track_position_cb;
#endif /* defined(CONFIG_BT_MCC_READ_TRACK_POSITION) */
#if defined(CONFIG_BT_MCC_SET_TRACK_POSITION)
	cb.set_track_position            = mcc_set_track_position_cb;
#endif /* defined(CONFIG_BT_MCC_SET_TRACK_POSITION) */
#if defined(CONFIG_BT_MCC_READ_PLAYBACK_SPEED)
	cb.read_playback_speed           = mcc_read_playback_speed_cb;
#endif /* defined (CONFIG_BT_MCC_READ_PLAYBACK_SPEED) */
#if defined(CONFIG_BT_MCC_SET_PLAYBACK_SPEED)
	cb.set_playback_speed            = mcc_set_playback_speed_cb;
#endif /* defined (CONFIG_BT_MCC_SET_PLAYBACK_SPEED) */
#if defined(CONFIG_BT_MCC_READ_SEEKING_SPEED)
	cb.read_seeking_speed            = mcc_read_seeking_speed_cb;
#endif /* defined (CONFIG_BT_MCC_READ_SEEKING_SPEED) */
#ifdef CONFIG_BT_MCC_OTS
	cb.read_segments_obj_id          = mcc_read_segments_obj_id_cb;
	cb.read_current_track_obj_id     = mcc_read_current_track_obj_id_cb;
	cb.set_current_track_obj_id      = mcc_set_current_track_obj_id_cb;
	cb.read_next_track_obj_id        = mcc_read_next_track_obj_id_cb;
	cb.set_next_track_obj_id         = mcc_set_next_track_obj_id_cb;
	cb.read_parent_group_obj_id      = mcc_read_parent_group_obj_id_cb;
	cb.read_current_group_obj_id     = mcc_read_current_group_obj_id_cb;
	cb.set_current_group_obj_id      = mcc_set_current_group_obj_id_cb;
#endif /* CONFIG_BT_MCC_OTS */
#if defined(CONFIG_BT_MCC_READ_PLAYING_ORDER)
	cb.read_playing_order            = mcc_read_playing_order_cb;
#endif /* defined(CONFIG_BT_MCC_READ_PLAYING_ORDER) */
#if defined(CONFIG_BT_MCC_SET_PLAYING_ORDER)
	cb.set_playing_order             = mcc_set_playing_order_cb;
#endif /* defined(CONFIG_BT_MCC_SET_PLAYING_ORDER) */
#if defined(CONFIG_BT_MCC_READ_PLAYING_ORDER_SUPPORTED)
	cb.read_playing_orders_supported = mcc_read_playing_orders_supported_cb;
#endif /* defined(CONFIG_BT_MCC_READ_PLAYING_ORDER_SUPPORTED) */
#if defined(CONFIG_BT_MCC_READ_MEDIA_STATE)
	cb.read_media_state              = mcc_read_media_state_cb;
#endif /* defined(CONFIG_BT_MCC_READ_MEDIA_STATE) */
#if defined(CONFIG_BT_MCC_SET_MEDIA_CONTROL_POINT)
	cb.send_cmd                      = mcc_send_cmd_cb;
#endif /* defined(CONFIG_BT_MCC_SET_MEDIA_CONTROL_POINT) */
	cb.cmd_ntf                       = mcc_cmd_ntf_cb;
#if defined(CONFIG_BT_MCC_READ_MEDIA_CONTROL_POINT_OPCODES_SUPPORTED)
	cb.read_opcodes_supported        = mcc_read_opcodes_supported_cb;
#endif /* defined(CONFIG_BT_MCC_READ_MEDIA_CONTROL_POINT_OPCODES_SUPPORTED) */
#ifdef CONFIG_BT_MCC_OTS
	cb.send_search                   = mcc_send_search_cb;
	cb.search_ntf                    = mcc_search_ntf_cb;
	cb.read_search_results_obj_id    = mcc_read_search_results_obj_id_cb;
#endif /* CONFIG_BT_MCC_OTS */
#if defined(CONFIG_BT_MCC_READ_CONTENT_CONTROL_ID)
	cb.read_content_control_id       = mcc_read_content_control_id_cb;
#endif /* defined(CONFIG_BT_MCC_READ_CONTENT_CONTROL_ID) */
#ifdef CONFIG_BT_MCC_OTS
	cb.otc_obj_selected              = mcc_otc_obj_selected_cb;
	cb.otc_obj_metadata              = mcc_otc_obj_metadata_cb;
	cb.otc_icon_object               = mcc_icon_object_read_cb;
	cb.otc_track_segments_object     = mcc_track_segments_object_read_cb;
	cb.otc_current_track_object      = mcc_otc_read_current_track_object_cb;
	cb.otc_next_track_object         = mcc_otc_read_next_track_object_cb;
	cb.otc_parent_group_object       = mcc_otc_read_parent_group_object_cb;
	cb.otc_current_group_object      = mcc_otc_read_current_group_object_cb;
#endif /* CONFIG_BT_MCC_OTS */

	/* Initialize the module */
	result = bt_mcc_init(&cb);

	if (result) {
		shell_error(sh, "Fail: %d", result);
	}
	return result;
}

static int cmd_mcc_discover_mcs(const struct shell *sh, size_t argc,
				char **argv)
{
	bool subscribe = true;
	int result = 0;

	if (argc > 1) {
		subscribe = shell_strtobool(argv[1], 0, &result);
		if (result != 0) {
			shell_error(sh, "Could not parse subscribe: %d",
				    result);

			return -ENOEXEC;
		}
	}

	result = bt_mcc_discover_mcs(default_conn, (bool)subscribe);
	if (result) {
		shell_error(sh, "Fail: %d", result);
	}
	return result;
}

static int cmd_mcc_read_player_name(const struct shell *sh, size_t argc,
				    char *argv[])
{
	int result;

	result = bt_mcc_read_player_name(default_conn);
	if (result) {
		shell_error(sh, "Fail: %d", result);
	}
	return result;
}

#ifdef CONFIG_BT_MCC_OTS
static int cmd_mcc_read_icon_obj_id(const struct shell *sh, size_t argc,
				    char *argv[])
{
	int result;

	result = bt_mcc_read_icon_obj_id(default_conn);
	if (result) {
		shell_error(sh, "Fail: %d", result);
	}
	return result;
}
#endif /* CONFIG_BT_MCC_OTS */

#if defined(CONFIG_BT_MCC_READ_MEDIA_PLAYER_ICON_URL)
static int cmd_mcc_read_icon_url(const struct shell *sh, size_t argc,
				 char *argv[])
{
	int result;

	result = bt_mcc_read_icon_url(default_conn);
	if (result) {
		shell_error(sh, "Fail: %d", result);
	}
	return result;
}
#endif /* defined(CONFIG_BT_MCC_READ_MEDIA_PLAYER_ICON_URL) */

#if defined(CONFIG_BT_MCC_READ_TRACK_TITLE)
static int cmd_mcc_read_track_title(const struct shell *sh, size_t argc,
				    char *argv[])
{
	int result;

	result = bt_mcc_read_track_title(default_conn);
	if (result) {
		shell_error(sh, "Fail: %d", result);
	}
	return result;
}
#endif /* defined(CONFIG_BT_MCC_READ_TRACK_TITLE) */

#if defined(CONFIG_BT_MCC_READ_TRACK_DURATION)
static int cmd_mcc_read_track_duration(const struct shell *sh, size_t argc,
				       char *argv[])
{
	int result;

	result = bt_mcc_read_track_duration(default_conn);
	if (result) {
		shell_error(sh, "Fail: %d", result);
	}
	return result;
}
#endif /* defined(CONFIG_BT_MCC_READ_TRACK_DURATION) */

#if defined(CONFIG_BT_MCC_READ_TRACK_POSITION)
static int cmd_mcc_read_track_position(const struct shell *sh, size_t argc,
				       char *argv[])
{
	int result;

	result = bt_mcc_read_track_position(default_conn);
	if (result) {
		shell_error(sh, "Fail: %d", result);
	}
	return result;
}
#endif /* defined(CONFIG_BT_MCC_READ_TRACK_POSITION) */

#if defined(CONFIG_BT_MCC_SET_TRACK_POSITION)
static int cmd_mcc_set_track_position(const struct shell *sh, size_t argc,
				      char *argv[])
{
	int result = 0;
	long pos;

	pos = shell_strtol(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Could not parse pos: %d", result);

		return -ENOEXEC;
	}

	if (sizeof(long) != sizeof(int32_t) && !IN_RANGE(pos, INT32_MIN, INT32_MAX)) {
		shell_error(sh, "Invalid pos: %ld", pos);

		return -ENOEXEC;
	}

	result = bt_mcc_set_track_position(default_conn, pos);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}
	return result;
}
#endif /* defined(CONFIG_BT_MCC_SET_TRACK_POSITION) */


#if defined(CONFIG_BT_MCC_READ_PLAYBACK_SPEED)
static int cmd_mcc_read_playback_speed(const struct shell *sh, size_t argc,
				       char *argv[])
{
	int result;

	result = bt_mcc_read_playback_speed(default_conn);
	if (result) {
		shell_error(sh, "Fail: %d", result);
	}
	return result;
}
#endif /* defined (CONFIG_BT_MCC_READ_PLAYBACK_SPEED) */


#if defined(CONFIG_BT_MCC_SET_PLAYBACK_SPEED)
static int cmd_mcc_set_playback_speed(const struct shell *sh, size_t argc,
				      char *argv[])
{
	int result = 0;
	long speed;

	speed = shell_strtol(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Could not parse speed: %d", result);

		return -ENOEXEC;
	}

	if (!IN_RANGE(speed, INT8_MIN, INT8_MAX)) {
		shell_error(sh, "Invalid speed: %ld", speed);

		return -ENOEXEC;
	}

	result = bt_mcc_set_playback_speed(default_conn, speed);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}
	return result;
}
#endif /* defined (CONFIG_BT_MCC_SET_PLAYBACK_SPEED) */

#if defined(CONFIG_BT_MCC_READ_SEEKING_SPEED)
static int cmd_mcc_read_seeking_speed(const struct shell *sh, size_t argc,
				      char *argv[])
{
	int result;

	result = bt_mcc_read_seeking_speed(default_conn);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}
	return result;
}
#endif /* defined (CONFIG_BT_MCC_READ_SEEKING_SPEED) */


#ifdef CONFIG_BT_MCC_OTS
static int cmd_mcc_read_track_segments_obj_id(const struct shell *sh,
					      size_t argc, char *argv[])
{
	int result;

	result = bt_mcc_read_segments_obj_id(default_conn);
	if (result) {
		shell_error(sh, "Fail: %d", result);
	}
	return result;
}


static int cmd_mcc_read_current_track_obj_id(const struct shell *sh,
					     size_t argc, char *argv[])
{
	int result;

	result = bt_mcc_read_current_track_obj_id(default_conn);
	if (result) {
		shell_error(sh, "Fail: %d", result);
	}
	return result;
}

static int cmd_mcc_set_current_track_obj_id(const struct shell *sh, size_t argc,
					    char *argv[])
{
	unsigned long long id;
	int result = 0;

	id = shell_strtoull(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Could not parse id: %d", result);

		return -ENOEXEC;
	}

	if (!IN_RANGE(id, BT_OTS_OBJ_ID_MIN, BT_OTS_OBJ_ID_MAX)) {
		shell_error(sh, "Invalid id: %llu", id);

		return -ENOEXEC;
	}

	result = bt_mcc_set_current_track_obj_id(default_conn, id);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}
	return result;
}

static int cmd_mcc_read_next_track_obj_id(const struct shell *sh, size_t argc,
					  char *argv[])
{
	int result;

	result = bt_mcc_read_next_track_obj_id(default_conn);
	if (result) {
		shell_error(sh, "Fail: %d", result);
	}
	return result;
}

static int cmd_mcc_set_next_track_obj_id(const struct shell *sh, size_t argc,
					 char *argv[])
{
	unsigned long long id;
	int result = 0;

	id = shell_strtoull(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Could not parse id: %d", result);

		return -ENOEXEC;
	}

	if (!IN_RANGE(id, BT_OTS_OBJ_ID_MIN, BT_OTS_OBJ_ID_MAX)) {
		shell_error(sh, "Invalid id: %llu", id);

		return -ENOEXEC;
	}

	result = bt_mcc_set_next_track_obj_id(default_conn, id);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}
	return result;
}

static int cmd_mcc_read_parent_group_obj_id(const struct shell *sh, size_t argc,
					    char *argv[])
{
	int result;

	result = bt_mcc_read_parent_group_obj_id(default_conn);
	if (result) {
		shell_error(sh, "Fail: %d", result);
	}
	return result;
}

static int cmd_mcc_read_current_group_obj_id(const struct shell *sh,
					     size_t argc, char *argv[])
{
	int result;

	result = bt_mcc_read_current_group_obj_id(default_conn);
	if (result) {
		shell_error(sh, "Fail: %d", result);
	}
	return result;
}

static int cmd_mcc_set_current_group_obj_id(const struct shell *sh, size_t argc,
					    char *argv[])
{
	unsigned long long id;
	int result = 0;

	id = shell_strtoull(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Could not parse id: %d", result);

		return -ENOEXEC;
	}

	if (!IN_RANGE(id, BT_OTS_OBJ_ID_MIN, BT_OTS_OBJ_ID_MAX)) {
		shell_error(sh, "Invalid id: %llu", id);

		return -ENOEXEC;
	}

	result = bt_mcc_set_current_group_obj_id(default_conn, id);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}
	return result;
}
#endif /* CONFIG_BT_MCC_OTS */

#if defined(CONFIG_BT_MCC_READ_PLAYING_ORDER)
static int cmd_mcc_read_playing_order(const struct shell *sh, size_t argc,
				      char *argv[])
{
	int result;

	result = bt_mcc_read_playing_order(default_conn);
	if (result) {
		shell_error(sh, "Fail: %d", result);
	}
	return result;
}
#endif /* defined(CONFIG_BT_MCC_READ_PLAYING_ORDER) */

#if defined(CONFIG_BT_MCC_SET_PLAYING_ORDER)
static int cmd_mcc_set_playing_order(const struct shell *sh, size_t argc,
				     char *argv[])
{
	unsigned long order;
	int result = 0;

	order = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Could not parse order: %d", result);

		return -ENOEXEC;
	}

	if (order > UINT8_MAX) {
		shell_error(sh, "Invalid order: %lu", order);

		return -ENOEXEC;
	}

	result = bt_mcc_set_playing_order(default_conn, order);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}
	return result;
}
#endif /* defined(CONFIG_BT_MCC_SET_PLAYING_ORDER) */

#if defined(CONFIG_BT_MCC_READ_PLAYING_ORDER_SUPPORTED)
static int cmd_mcc_read_playing_orders_supported(const struct shell *sh,
						 size_t argc, char *argv[])
{
	int result;

	result = bt_mcc_read_playing_orders_supported(default_conn);
	if (result) {
		shell_error(sh, "Fail: %d", result);
	}
	return result;
}
#endif /* defined(CONFIG_BT_MCC_READ_PLAYING_ORDER_SUPPORTED) */

#if defined(CONFIG_BT_MCC_READ_MEDIA_STATE)
static int cmd_mcc_read_media_state(const struct shell *sh, size_t argc,
				    char *argv[])
{
	int result;

	result = bt_mcc_read_media_state(default_conn);
	if (result) {
		shell_error(sh, "Fail: %d", result);
	}
	return result;
}
#endif /* defined(CONFIG_BT_MCC_READ_MEDIA_STATE) */

#if defined(CONFIG_BT_MCC_SET_MEDIA_CONTROL_POINT)
static int cmd_mcc_play(const struct shell *sh, size_t argc, char *argv[])
{
	const struct mpl_cmd cmd = {
		.opcode = BT_MCS_OPC_PLAY,
		.use_param = false,
		.param = 0,
	};
	int err;

	err = bt_mcc_send_cmd(default_conn, &cmd);
	if (err != 0) {
		shell_error(sh, "MCC play failed: %d", err);
	}

	return err;
}

static int cmd_mcc_pause(const struct shell *sh, size_t argc, char *argv[])
{
	const struct mpl_cmd cmd = {
		.opcode = BT_MCS_OPC_PAUSE,
		.use_param = false,
		.param = 0,
	};
	int err;

	err = bt_mcc_send_cmd(default_conn, &cmd);
	if (err != 0) {
		shell_error(sh, "MCC pause failed: %d", err);
	}

	return err;
}

static int cmd_mcc_fast_rewind(const struct shell *sh, size_t argc,
			       char *argv[])
{
	const struct mpl_cmd cmd = {
		.opcode = BT_MCS_OPC_FAST_REWIND,
		.use_param = false,
		.param = 0,
	};
	int err;

	err = bt_mcc_send_cmd(default_conn, &cmd);
	if (err != 0) {
		shell_error(sh, "MCC fast rewind failed: %d", err);
	}

	return err;
}

static int cmd_mcc_fast_forward(const struct shell *sh, size_t argc,
				char *argv[])
{
	const struct mpl_cmd cmd = {
		.opcode = BT_MCS_OPC_FAST_FORWARD,
		.use_param = false,
		.param = 0,
	};
	int err;

	err = bt_mcc_send_cmd(default_conn, &cmd);
	if (err != 0) {
		shell_error(sh, "MCC fast forward failed: %d", err);
	}

	return err;
}

static int cmd_mcc_stop(const struct shell *sh, size_t argc, char *argv[])
{
	const struct mpl_cmd cmd = {
		.opcode = BT_MCS_OPC_STOP,
		.use_param = false,
		.param = 0,
	};
	int err;

	err = bt_mcc_send_cmd(default_conn, &cmd);
	if (err != 0) {
		shell_error(sh, "MCC stop failed: %d", err);
	}

	return err;
}

static int cmd_mcc_move_relative(const struct shell *sh, size_t argc,
				 char *argv[])
{
	struct mpl_cmd cmd = {
		.opcode = BT_MCS_OPC_MOVE_RELATIVE,
		.use_param = true,
	};
	long offset;
	int err;

	err = 0;
	offset = shell_strtol(argv[1], 10, &err);
	if (err != 0) {
		shell_error(sh, "Failed to parse offset: %d", err);

		return err;
	}

	if (sizeof(long) != sizeof(int32_t) && !IN_RANGE(offset, INT32_MIN, INT32_MAX)) {
		shell_error(sh, "Invalid offset: %ld", offset);

		return -ENOEXEC;
	}

	cmd.param = (int32_t)offset;

	err = bt_mcc_send_cmd(default_conn, &cmd);
	if (err != 0) {
		shell_error(sh, "MCC move relative failed: %d", err);
	}

	return err;
}

static int cmd_mcc_prev_segment(const struct shell *sh, size_t argc,
				char *argv[])
{
	const struct mpl_cmd cmd = {
		.opcode = BT_MCS_OPC_PREV_SEGMENT,
		.use_param = false,
		.param = 0,
	};
	int err;

	err = bt_mcc_send_cmd(default_conn, &cmd);
	if (err != 0) {
		shell_error(sh, "MCC previous segment failed: %d", err);
	}

	return err;
}

static int cmd_mcc_next_segment(const struct shell *sh, size_t argc,
				char *argv[])
{
	const struct mpl_cmd cmd = {
		.opcode = BT_MCS_OPC_NEXT_SEGMENT,
		.use_param = false,
		.param = 0,
	};
	int err;

	err = bt_mcc_send_cmd(default_conn, &cmd);
	if (err != 0) {
		shell_error(sh, "MCC next segment failed: %d", err);
	}

	return err;
}

static int cmd_mcc_first_segment(const struct shell *sh, size_t argc,
				 char *argv[])
{
	const struct mpl_cmd cmd = {
		.opcode = BT_MCS_OPC_FIRST_SEGMENT,
		.use_param = false,
		.param = 0,
	};
	int err;

	err = bt_mcc_send_cmd(default_conn, &cmd);
	if (err != 0) {
		shell_error(sh, "MCC first segment failed: %d", err);
	}

	return err;
}

static int cmd_mcc_last_segment(const struct shell *sh, size_t argc,
				char *argv[])
{
	const struct mpl_cmd cmd = {
		.opcode = BT_MCS_OPC_LAST_SEGMENT,
		.use_param = false,
		.param = 0,
	};
	int err;

	err = bt_mcc_send_cmd(default_conn, &cmd);
	if (err != 0) {
		shell_error(sh, "MCC last segment failed: %d", err);
	}

	return err;
}

static int cmd_mcc_goto_segment(const struct shell *sh, size_t argc,
				char *argv[])
{
	struct mpl_cmd cmd = {
		.opcode = BT_MCS_OPC_GOTO_SEGMENT,
		.use_param = true,
	};
	long segment;
	int err;

	err = 0;
	segment = shell_strtol(argv[1], 10, &err);
	if (err != 0) {
		shell_error(sh, "Failed to parse segment: %d", err);

		return err;
	}

	if (sizeof(long) != sizeof(int32_t) && !IN_RANGE(segment, INT32_MIN, INT32_MAX)) {
		shell_error(sh, "Invalid segment: %ld", segment);

		return -ENOEXEC;
	}

	cmd.param = (int32_t)segment;

	err = bt_mcc_send_cmd(default_conn, &cmd);
	if (err != 0) {
		shell_error(sh, "MCC goto segment failed: %d", err);
	}

	return err;
}

static int cmd_mcc_prev_track(const struct shell *sh, size_t argc, char *argv[])
{
	const struct mpl_cmd cmd = {
		.opcode = BT_MCS_OPC_PREV_TRACK,
		.use_param = false,
		.param = 0,
	};
	int err;

	err = bt_mcc_send_cmd(default_conn, &cmd);
	if (err != 0) {
		shell_error(sh, "MCC previous track failed: %d", err);
	}

	return err;
}

static int cmd_mcc_next_track(const struct shell *sh, size_t argc, char *argv[])
{
	const struct mpl_cmd cmd = {
		.opcode = BT_MCS_OPC_NEXT_TRACK,
		.use_param = false,
		.param = 0,
	};
	int err;

	err = bt_mcc_send_cmd(default_conn, &cmd);
	if (err != 0) {
		shell_error(sh, "MCC next track failed: %d", err);
	}

	return err;
}

static int cmd_mcc_first_track(const struct shell *sh, size_t argc,
			       char *argv[])
{
	const struct mpl_cmd cmd = {
		.opcode = BT_MCS_OPC_FIRST_TRACK,
		.use_param = false,
		.param = 0,
	};
	int err;

	err = bt_mcc_send_cmd(default_conn, &cmd);
	if (err != 0) {
		shell_error(sh, "MCC first track failed: %d", err);
	}

	return err;
}

static int cmd_mcc_last_track(const struct shell *sh, size_t argc, char *argv[])
{
	const struct mpl_cmd cmd = {
		.opcode = BT_MCS_OPC_LAST_TRACK,
		.use_param = false,
		.param = 0,
	};
	int err;

	err = bt_mcc_send_cmd(default_conn, &cmd);
	if (err != 0) {
		shell_error(sh, "MCC last track failed: %d", err);
	}

	return err;
}

static int cmd_mcc_goto_track(const struct shell *sh, size_t argc, char *argv[])
{
	struct mpl_cmd cmd = {
		.opcode = BT_MCS_OPC_GOTO_TRACK,
		.use_param = true,
	};
	long track;
	int err;

	err = 0;
	track = shell_strtol(argv[1], 10, &err);
	if (err != 0) {
		shell_error(sh, "Failed to parse track: %d", err);

		return err;
	}

	if (sizeof(long) != sizeof(int32_t) && !IN_RANGE(track, INT32_MIN, INT32_MAX)) {
		shell_error(sh, "Invalid track: %ld", track);

		return -ENOEXEC;
	}

	cmd.param = (int32_t)track;

	err = bt_mcc_send_cmd(default_conn, &cmd);
	if (err != 0) {
		shell_error(sh, "MCC goto track failed: %d", err);
	}

	return err;
}

static int cmd_mcc_prev_group(const struct shell *sh, size_t argc, char *argv[])
{
	const struct mpl_cmd cmd = {
		.opcode = BT_MCS_OPC_PREV_GROUP,
		.use_param = false,
		.param = 0,
	};
	int err;

	err = bt_mcc_send_cmd(default_conn, &cmd);
	if (err != 0) {
		shell_error(sh, "MCC previous group failed: %d", err);
	}

	return err;
}

static int cmd_mcc_next_group(const struct shell *sh, size_t argc, char *argv[])
{
	const struct mpl_cmd cmd = {
		.opcode = BT_MCS_OPC_NEXT_GROUP,
		.use_param = false,
		.param = 0,
	};
	int err;

	err = bt_mcc_send_cmd(default_conn, &cmd);
	if (err != 0) {
		shell_error(sh, "MCC next group failed: %d", err);
	}

	return err;
}

static int cmd_mcc_first_group(const struct shell *sh, size_t argc,
			       char *argv[])
{
	const struct mpl_cmd cmd = {
		.opcode = BT_MCS_OPC_FIRST_GROUP,
		.use_param = false,
		.param = 0,
	};
	int err;

	err = bt_mcc_send_cmd(default_conn, &cmd);
	if (err != 0) {
		shell_error(sh, "MCC first group failed: %d", err);
	}

	return err;
}

static int cmd_mcc_last_group(const struct shell *sh, size_t argc, char *argv[])
{
	const struct mpl_cmd cmd = {
		.opcode = BT_MCS_OPC_LAST_GROUP,
		.use_param = false,
		.param = 0,
	};
	int err;

	err = bt_mcc_send_cmd(default_conn, &cmd);
	if (err != 0) {
		shell_error(sh, "MCC last group failed: %d", err);
	}

	return err;
}

static int cmd_mcc_goto_group(const struct shell *sh, size_t argc, char *argv[])
{
	struct mpl_cmd cmd = {
		.opcode = BT_MCS_OPC_GOTO_GROUP,
		.use_param = true,
	};
	long group;
	int err;

	err = 0;
	group = shell_strtol(argv[1], 10, &err);
	if (err != 0) {
		shell_error(sh, "Failed to parse group: %d", err);

		return err;
	}

	if (sizeof(long) != sizeof(int32_t) && !IN_RANGE(group, INT32_MIN, INT32_MAX)) {
		shell_error(sh, "Invalid group: %ld", group);

		return -ENOEXEC;
	}

	cmd.param = (int32_t)group;

	err = bt_mcc_send_cmd(default_conn, &cmd);
	if (err != 0) {
		shell_error(sh, "MCC goto group failed: %d", err);
	}

	return err;
}
#endif /* defined(CONFIG_BT_MCC_SET_MEDIA_CONTROL_POINT) */

#if defined(CONFIG_BT_MCC_READ_MEDIA_CONTROL_POINT_OPCODES_SUPPORTED)
static int cmd_mcc_read_opcodes_supported(const struct shell *sh, size_t argc,
					  char *argv[])
{
	int result;

	result = bt_mcc_read_opcodes_supported(default_conn);
	if (result) {
		shell_error(sh, "Fail: %d", result);
	}
	return result;
}
#endif /* defined(CONFIG_BT_MCC_READ_MEDIA_CONTROL_POINT_OPCODES_SUPPORTED) */

#ifdef CONFIG_BT_MCC_OTS
static int cmd_mcc_send_search_raw(const struct shell *sh, size_t argc,
				   char *argv[])
{
	int result;
	size_t len;
	struct mpl_search search;

	len = strlen(argv[1]);
	if (len > sizeof(search.search)) {
		shell_print(sh, "Fail: Invalid argument");
		return -EINVAL;
	}

	search.len = len;
	memcpy(search.search, argv[1], search.len);
	LOG_DBG("Search string: %s", argv[1]);

	result = bt_mcc_send_search(default_conn, &search);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}
	return result;
}

static int cmd_mcc_send_search_ioptest(const struct shell *sh, size_t argc,
				       char *argv[])
{
	/* Implementation follows Media control service testspec 0.9.0r13 */
	/* Testcase MCS/SR/SCP/BV-01-C [Search Control Point], rounds 1 - 9 */
	struct mpl_sci sci_1 = {0};
	struct mpl_sci sci_2 = {0};
	struct mpl_search search;
	unsigned long testround;
	int result = 0;

	testround = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Could not parse testround: %d", result);

		return -ENOEXEC;
	}

	switch (testround) {
	case 1:
	case 8:
	case 9:
		/* 1, 8 and 9 have the same first SCI */
		sci_1.type = BT_MCS_SEARCH_TYPE_TRACK_NAME;
		strcpy(sci_1.param, "TSPX_Track_Name");
		break;
	case 2:
		sci_1.type = BT_MCS_SEARCH_TYPE_ARTIST_NAME;
		strcpy(sci_1.param, "TSPX_Artist_Name");
		break;
	case 3:
		sci_1.type = BT_MCS_SEARCH_TYPE_ALBUM_NAME;
		strcpy(sci_1.param, "TSPX_Album_Name");
		break;
	case 4:
		sci_1.type = BT_MCS_SEARCH_TYPE_GROUP_NAME;
		strcpy(sci_1.param, "TSPX_Group_Name");
		break;
	case 5:
		sci_1.type = BT_MCS_SEARCH_TYPE_EARLIEST_YEAR;
		strcpy(sci_1.param, "TSPX_Earliest_Year");
		break;
	case 6:
		sci_1.type = BT_MCS_SEARCH_TYPE_LATEST_YEAR;
		strcpy(sci_1.param, "TSPX_Latest_Year");
		break;
	case 7:
		sci_1.type = BT_MCS_SEARCH_TYPE_GENRE;
		strcpy(sci_1.param, "TSPX_Genre");
		break;
	default:
		shell_error(sh, "Invalid parameter");
		return -ENOEXEC;
	}


	switch (testround) {
	case 8:
		sci_2.type = BT_MCS_SEARCH_TYPE_ONLY_TRACKS;
		break;
	case 9:
		sci_2.type = BT_MCS_SEARCH_TYPE_ONLY_GROUPS;
		break;
	}

	/* Length is length of type, plus length of param w/o termination */
	sci_1.len = sizeof(sci_1.type) + strlen(sci_1.param);

	search.len = 0;
	memcpy(&search.search[search.len], &sci_1.len, sizeof(sci_1.len));
	search.len += sizeof(sci_1.len);

	memcpy(&search.search[search.len], &sci_1.type, sizeof(sci_1.type));
	search.len += sizeof(sci_1.type);

	memcpy(&search.search[search.len], &sci_1.param, strlen(sci_1.param));
	search.len += strlen(sci_1.param);

	if (testround == 8 || testround == 9) {

		sci_2.len = sizeof(sci_2.type); /* The type only, no param */

		memcpy(&search.search[search.len], &sci_2.len,
		       sizeof(sci_2.len));
		search.len += sizeof(sci_2.len);

		memcpy(&search.search[search.len], &sci_2.type,
		       sizeof(sci_2.type));
		search.len += sizeof(sci_2.type);
	}

	shell_print(sh, "Search string: ");
	shell_hexdump(sh, (uint8_t *)&search.search, search.len);

	result = bt_mcc_send_search(default_conn, &search);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

#if defined(CONFIG_BT_MCC_LOG_LEVEL_DBG) && defined(CONFIG_BT_TESTING)
static int cmd_mcc_test_send_search_iop_invalid_type(const struct shell *sh,
						     size_t argc, char *argv[])
{
	int result;
	struct mpl_search search;

	search.search[0] = 2;
	search.search[1] = (char)14; /* Invalid type value */
	search.search[2] = 't';  /* Anything */
	search.len = 3;

	shell_print(sh, "Search string: ");
	shell_hexdump(sh, (uint8_t *)&search.search, search.len);

	result = bt_mcc_send_search(default_conn, &search);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_mcc_test_send_search_invalid_sci_len(const struct shell *sh,
						    size_t argc, char *argv[])
{
	/* Reproduce a search that caused hard fault when sent from peer */
	/* in IOP testing */

	int result;
	struct mpl_search search;

	char offending_search[9] = {6, 1, 't', 'r', 'a', 'c', 'k', 0, 1 };

	search.len = 9;
	memcpy(&search.search, offending_search, search.len);

	shell_print(sh, "Search string: ");
	shell_hexdump(sh, (uint8_t *)&search.search, search.len);

	result = bt_mcc_send_search(default_conn, &search);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}
#endif /* CONFIG_BT_MCC_LOG_LEVEL_DBG && CONFIG_BT_TESTING */

static int cmd_mcc_read_search_results_obj_id(const struct shell *sh,
					      size_t argc, char *argv[])
{
	int result;

	result = bt_mcc_read_search_results_obj_id(default_conn);
	if (result) {
		shell_error(sh, "Fail: %d", result);
	}
	return result;
}
#endif /* CONFIG_BT_MCC_OTS */

#if defined(CONFIG_BT_MCC_READ_CONTENT_CONTROL_ID)
static int cmd_mcc_read_content_control_id(const struct shell *sh, size_t argc,
					   char *argv[])
{
	int result;

	result = bt_mcc_read_content_control_id(default_conn);
	if (result) {
		shell_error(sh, "Fail: %d", result);
	}
	return result;
}
#endif /* defined(CONFIG_BT_MCC_READ_CONTENT_CONTROL_ID) */


#ifdef CONFIG_BT_MCC_OTS
static int cmd_mcc_otc_read_features(const struct shell *sh, size_t argc,
				     char *argv[])
{
	int result;

	result = bt_ots_client_read_feature(bt_mcc_otc_inst(default_conn),
					    default_conn);
	if (result) {
		shell_error(sh, "Fail: %d", result);
	}
	return result;
}

static int cmd_mcc_otc_read(const struct shell *sh, size_t argc, char *argv[])
{
	int result;

	result = bt_ots_client_read_object_data(bt_mcc_otc_inst(default_conn),
						default_conn);
	if (result) {
		shell_error(sh, "Fail: %d", result);
	}
	return result;
}

static int cmd_mcc_otc_read_metadata(const struct shell *sh, size_t argc,
				     char *argv[])
{
	int result;

	result = bt_ots_client_read_object_metadata(bt_mcc_otc_inst(default_conn),
						    default_conn,
						    BT_OTS_METADATA_REQ_ALL);
	if (result) {
		shell_error(sh, "Fail: %d", result);
	}
	return result;
}

static int cmd_mcc_otc_select(const struct shell *sh, size_t argc, char *argv[])
{
	unsigned long long id;
	int result = 0;

	id = shell_strtoull(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Could not parse id: %d", result);

		return -ENOEXEC;
	}

	if (!IN_RANGE(id, BT_OTS_OBJ_ID_MIN, BT_OTS_OBJ_ID_MAX)) {
		shell_error(sh, "Invalid id: %llu", id);

		return -ENOEXEC;
	}

	result = bt_ots_client_select_id(bt_mcc_otc_inst(default_conn),
					 default_conn, id);
	if (result) {
		shell_error(sh, "Fail: %d", result);
	}
	return result;
}

static int cmd_mcc_otc_select_first(const struct shell *sh, size_t argc,
				    char *argv[])
{
	int result;

	result = bt_ots_client_select_first(bt_mcc_otc_inst(default_conn),
					    default_conn);
	if (result) {
		shell_error(sh, "Fail: %d", result);
	}
	return result;
}

static int cmd_mcc_otc_select_last(const struct shell *sh, size_t argc,
				   char *argv[])
{
	int result;

	result = bt_ots_client_select_last(bt_mcc_otc_inst(default_conn),
					   default_conn);
	if (result) {
		shell_error(sh, "Fail: %d", result);
	}
	return result;
}

static int cmd_mcc_otc_select_next(const struct shell *sh, size_t argc,
				   char *argv[])
{
	int result;

	result = bt_ots_client_select_next(bt_mcc_otc_inst(default_conn),
					   default_conn);
	if (result) {
		shell_error(sh, "Fail: %d", result);
	}
	return result;
}

static int cmd_mcc_otc_select_prev(const struct shell *sh, size_t argc,
				   char *argv[])
{
	int result;

	result = bt_ots_client_select_prev(bt_mcc_otc_inst(default_conn),
					   default_conn);
	if (result) {
		shell_error(sh, "Fail: %d", result);
	}
	return result;
}

static int cmd_mcc_otc_read_icon_object(const struct shell *sh, size_t argc,
					char *argv[])
{
	/* Assumes the Icon Object has already been selected by ID */

	int result;

	result = bt_mcc_otc_read_icon_object(default_conn);
	if (result) {
		shell_error(sh, "Fail: %d", result);
	}
	return result;
}

static int cmd_mcc_otc_read_track_segments_object(const struct shell *sh,
						  size_t argc, char *argv[])
{
	/* Assumes the Segment Object has already been selected by ID */

	int result;

	result = bt_mcc_otc_read_track_segments_object(default_conn);
	if (result) {
		shell_error(sh, "Fail: %d", result);
	}
	return result;
}

static int cmd_mcc_otc_read_current_track_object(const struct shell *sh,
						 size_t argc, char *argv[])
{
	/* Assumes the Current Track Object has already been selected by ID */

	int result;

	result = bt_mcc_otc_read_current_track_object(default_conn);
	if (result) {
		shell_error(sh, "Fail: %d", result);
	}
	return result;
}

static int cmd_mcc_otc_read_next_track_object(const struct shell *sh,
					      size_t argc, char *argv[])
{
	/* Assumes the Next Track Object has already been selected by ID */

	int result;

	result = bt_mcc_otc_read_next_track_object(default_conn);
	if (result) {
		shell_error(sh, "Fail: %d", result);
	}
	return result;
}

static int cmd_mcc_otc_read_parent_group_object(const struct shell *sh,
						size_t argc, char *argv[])
{
	/* Assumes the Parent Group Object has already been selected by ID */

	int result;

	result = bt_mcc_otc_read_parent_group_object(default_conn);
	if (result) {
		shell_error(sh, "Fail: %d", result);
	}
	return result;
}

static int cmd_mcc_otc_read_current_group_object(const struct shell *sh,
						 size_t argc, char *argv[])
{
	/* Assumes the Current Group Object has already been selected by ID */

	int result;

	result = bt_mcc_otc_read_current_group_object(default_conn);
	if (result) {
		shell_error(sh, "Fail: %d", result);
	}
	return result;
}
#endif /* CONFIG_BT_MCC_OTS */

static int cmd_mcc(const struct shell *sh, size_t argc, char **argv)
{
	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);

	return -ENOEXEC;
}

SHELL_STATIC_SUBCMD_SET_CREATE(mcc_cmds,
	SHELL_CMD_ARG(init, NULL, "Initialize client",
		      cmd_mcc_init, 1, 0),
	SHELL_CMD_ARG(discover_mcs, NULL,
		      "Discover Media Control Service [subscribe]",
		      cmd_mcc_discover_mcs, 1, 1),
	SHELL_CMD_ARG(read_player_name, NULL, "Read Media Player Name",
		      cmd_mcc_read_player_name, 1, 0),
#ifdef CONFIG_BT_MCC_OTS
	SHELL_CMD_ARG(read_icon_obj_id, NULL, "Read Icon Object ID",
		      cmd_mcc_read_icon_obj_id, 1, 0),
#endif /* CONFIG_BT_MCC_OTS */
#if defined(CONFIG_BT_MCC_READ_MEDIA_PLAYER_ICON_URL)
	SHELL_CMD_ARG(read_icon_url, NULL, "Read Icon URL",
		      cmd_mcc_read_icon_url, 1, 0),
#endif /* defined(CONFIG_BT_MCC_READ_MEDIA_PLAYER_ICON_URL) */
#if defined(CONFIG_BT_MCC_READ_TRACK_TITLE)
	SHELL_CMD_ARG(read_track_title, NULL, "Read Track Title",
		      cmd_mcc_read_track_title, 1, 0),
#endif /* defined(CONFIG_BT_MCC_READ_TRACK_TITLE) */
#if defined(CONFIG_BT_MCC_READ_TRACK_DURATION)
	SHELL_CMD_ARG(read_track_duration, NULL, "Read Track Duration",
		      cmd_mcc_read_track_duration, 1, 0),
#endif /* defined(CONFIG_BT_MCC_READ_TRACK_DURATION) */
#if defined(CONFIG_BT_MCC_READ_TRACK_POSITION)
	SHELL_CMD_ARG(read_track_position, NULL, "Read Track Position",
		      cmd_mcc_read_track_position, 1, 0),
#endif /* defined(CONFIG_BT_MCC_READ_TRACK_POSITION) */
#if defined(CONFIG_BT_MCC_SET_TRACK_POSITION)
	SHELL_CMD_ARG(set_track_position, NULL, "Set Track position <position>",
		      cmd_mcc_set_track_position, 2, 0),
#endif /* defined(CONFIG_BT_MCC_SET_TRACK_POSITION) */
#if defined(CONFIG_BT_MCC_READ_PLAYBACK_SPEED)
	SHELL_CMD_ARG(read_playback_speed, NULL, "Read Playback Speed",
		      cmd_mcc_read_playback_speed, 1, 0),
#endif /* defined (CONFIG_BT_MCC_READ_PLAYBACK_SPEED */
#if defined(CONFIG_BT_MCC_SET_PLAYBACK_SPEED)
	SHELL_CMD_ARG(set_playback_speed, NULL, "Set Playback Speed <speed>",
		      cmd_mcc_set_playback_speed, 2, 0),
#endif /* defined (CONFIG_BT_MCC_SET_PLAYBACK_SPEED) */
#if defined(CONFIG_BT_MCC_READ_SEEKING_SPEED)
	SHELL_CMD_ARG(read_seeking_speed, NULL, "Read Seeking Speed",
		      cmd_mcc_read_seeking_speed, 1, 0),
#endif /* defined (CONFIG_BT_MCC_READ_SEEKING_SPEED) */
#ifdef CONFIG_BT_MCC_OTS
	SHELL_CMD_ARG(read_track_segments_obj_id, NULL,
		      "Read Track Segments Object ID",
		      cmd_mcc_read_track_segments_obj_id, 1, 0),
	SHELL_CMD_ARG(read_current_track_obj_id, NULL,
		      "Read Current Track Object ID",
		      cmd_mcc_read_current_track_obj_id, 1, 0),
	SHELL_CMD_ARG(set_current_track_obj_id, NULL,
		      "Set Current Track Object ID <id: 48 bits or less>",
		      cmd_mcc_set_current_track_obj_id, 2, 0),
	SHELL_CMD_ARG(read_next_track_obj_id, NULL,
		      "Read Next Track Object ID",
		      cmd_mcc_read_next_track_obj_id, 1, 0),
	SHELL_CMD_ARG(set_next_track_obj_id, NULL,
		      "Set Next Track Object ID <id: 48 bits or less>",
		      cmd_mcc_set_next_track_obj_id, 2, 0),
	SHELL_CMD_ARG(read_current_group_obj_id, NULL,
		      "Read Current Group Object ID",
		      cmd_mcc_read_current_group_obj_id, 1, 0),
	SHELL_CMD_ARG(read_parent_group_obj_id, NULL,
		      "Read Parent Group Object ID",
		      cmd_mcc_read_parent_group_obj_id, 1, 0),
	SHELL_CMD_ARG(set_current_group_obj_id, NULL,
		      "Set Current Group Object ID <id: 48 bits or less>",
		      cmd_mcc_set_current_group_obj_id, 2, 0),
#endif /* CONFIG_BT_MCC_OTS */
#if defined(CONFIG_BT_MCC_READ_PLAYING_ORDER)
	SHELL_CMD_ARG(read_playing_order, NULL, "Read Playing Order",
		      cmd_mcc_read_playing_order, 1, 0),
#endif /* defined(CONFIG_BT_MCC_READ_PLAYING_ORDER) */
#if defined(CONFIG_BT_MCC_SET_PLAYING_ORDER)
	SHELL_CMD_ARG(set_playing_order, NULL, "Set Playing Order <order>",
		      cmd_mcc_set_playing_order, 2, 0),
#endif /* defined(CONFIG_BT_MCC_SET_PLAYING_ORDER) */
#if defined(CONFIG_BT_MCC_READ_PLAYING_ORDER_SUPPORTED)
	SHELL_CMD_ARG(read_playing_orders_supported, NULL,
		     "Read Playing Orders Supported",
		      cmd_mcc_read_playing_orders_supported, 1, 0),
#endif /* defined(CONFIG_BT_MCC_READ_PLAYING_ORDER_SUPPORTED) */
#if defined(CONFIG_BT_MCC_READ_MEDIA_STATE)
	SHELL_CMD_ARG(read_media_state, NULL, "Read Media State",
		      cmd_mcc_read_media_state, 1, 0),
#endif /* defined(CONFIG_BT_MCC_READ_MEDIA_STATE) */
#if defined(CONFIG_BT_MCC_SET_MEDIA_CONTROL_POINT)
	SHELL_CMD_ARG(play, NULL, "Send the play command", cmd_mcc_play, 1, 0),
	SHELL_CMD_ARG(pause, NULL, "Send the pause command",
		      cmd_mcc_pause, 1, 0),
	SHELL_CMD_ARG(fast_rewind, NULL, "Send the fast rewind command",
		      cmd_mcc_fast_rewind, 1, 0),
	SHELL_CMD_ARG(fast_forward, NULL, "Send the fast forward command",
		      cmd_mcc_fast_forward, 1, 0),
	SHELL_CMD_ARG(stop, NULL, "Send the stop command", cmd_mcc_stop, 1, 0),
	SHELL_CMD_ARG(move_relative, NULL,
		      "Send the move relative command <int32_t: offset>",
		      cmd_mcc_move_relative, 2, 0),
	SHELL_CMD_ARG(prev_segment, NULL, "Send the prev segment command",
		      cmd_mcc_prev_segment, 1, 0),
	SHELL_CMD_ARG(next_segment, NULL, "Send the next segment command",
		      cmd_mcc_next_segment, 1, 0),
	SHELL_CMD_ARG(first_segment, NULL, "Send the first segment command",
		      cmd_mcc_first_segment, 1, 0),
	SHELL_CMD_ARG(last_segment, NULL, "Send the last segment command",
		      cmd_mcc_last_segment, 1, 0),
	SHELL_CMD_ARG(goto_segment, NULL,
		      "Send the goto segment command <int32_t: segment>",
		      cmd_mcc_goto_segment, 2, 0),
	SHELL_CMD_ARG(prev_track, NULL, "Send the prev track command",
		      cmd_mcc_prev_track, 1, 0),
	SHELL_CMD_ARG(next_track, NULL, "Send the next track command",
		      cmd_mcc_next_track, 1, 0),
	SHELL_CMD_ARG(first_track, NULL, "Send the first track command",
		      cmd_mcc_first_track, 1, 0),
	SHELL_CMD_ARG(last_track, NULL, "Send the last track command",
		      cmd_mcc_last_track, 1, 0),
	SHELL_CMD_ARG(goto_track, NULL,
		      "Send the goto track command  <int32_t: track>",
		      cmd_mcc_goto_track, 2, 0),
	SHELL_CMD_ARG(prev_group, NULL, "Send the prev group command",
		      cmd_mcc_prev_group, 1, 0),
	SHELL_CMD_ARG(next_group, NULL, "Send the next group command",
		      cmd_mcc_next_group, 1, 0),
	SHELL_CMD_ARG(first_group, NULL, "Send the first group command",
		      cmd_mcc_first_group, 1, 0),
	SHELL_CMD_ARG(last_group, NULL, "Send the last group command",
		      cmd_mcc_last_group, 1, 0),
	SHELL_CMD_ARG(goto_group, NULL,
		      "Send the goto group command <int32_t: group>",
		      cmd_mcc_goto_group, 2, 0),
#endif /* defined(CONFIG_BT_MCC_SET_MEDIA_CONTROL_POINT) */
#if defined(CONFIG_BT_MCC_READ_MEDIA_CONTROL_POINT_OPCODES_SUPPORTED)
	SHELL_CMD_ARG(read_opcodes_supported, NULL, "Send the Read Opcodes Supported",
		      cmd_mcc_read_opcodes_supported, 1, 0),
#endif /* defined(CONFIG_BT_MCC_READ_MEDIA_CONTROL_POINT_OPCODES_SUPPORTED) */
#ifdef CONFIG_BT_MCC_OTS
	SHELL_CMD_ARG(send_search_raw, NULL, "Send search <search control item sequence>",
		      cmd_mcc_send_search_raw, 2, 0),
	SHELL_CMD_ARG(send_search_scp_ioptest, NULL,
		      "Send search - IOP test round as input <round number>",
		      cmd_mcc_send_search_ioptest, 2, 0),
#if defined(CONFIG_BT_MCC_LOG_LEVEL_DBG) && defined(CONFIG_BT_TESTING)
	SHELL_CMD_ARG(test_send_search_iop_invalid_type, NULL,
		      "Send search - IOP test, invalid type value (test)",
		      cmd_mcc_test_send_search_iop_invalid_type, 1, 0),
	SHELL_CMD_ARG(test_send_Search_invalid_sci_len, NULL,
		      "Send search - invalid sci length (test)",
		      cmd_mcc_test_send_search_invalid_sci_len, 1, 0),
#endif /* CONFIG_BT_MCC_LOG_LEVEL_DBG && CONFIG_BT_TESTING */
	SHELL_CMD_ARG(read_search_results_obj_id, NULL,
		      "Read Search Results Object ID",
		      cmd_mcc_read_search_results_obj_id, 1, 0),
#endif /* CONFIG_BT_MCC_OTS */
#if defined(CONFIG_BT_MCC_READ_CONTENT_CONTROL_ID)
	SHELL_CMD_ARG(read_content_control_id, NULL, "Read Content Control ID",
		      cmd_mcc_read_content_control_id, 1, 0),
#endif /* defined(CONFIG_BT_MCC_READ_CONTENT_CONTROL_ID) */
#ifdef CONFIG_BT_MCC_OTS
	SHELL_CMD_ARG(ots_read_features, NULL, "Read OTC Features",
		      cmd_mcc_otc_read_features, 1, 0),
	SHELL_CMD_ARG(ots_oacp_read, NULL, "Read current object",
		      cmd_mcc_otc_read, 1, 0),
	SHELL_CMD_ARG(ots_read_metadata, NULL, "Read current object's metadata",
		      cmd_mcc_otc_read_metadata, 1, 0),
	SHELL_CMD_ARG(ots_select, NULL, "Select an object by its ID <ID>",
		      cmd_mcc_otc_select, 2, 0),
	SHELL_CMD_ARG(ots_read_icon_object, NULL, "Read Icon Object",
		      cmd_mcc_otc_read_icon_object, 1, 0),
	SHELL_CMD_ARG(ots_read_track_segments_object, NULL,
		      "Read Track Segments Object",
		      cmd_mcc_otc_read_track_segments_object, 1, 0),
	SHELL_CMD_ARG(ots_read_current_track_object, NULL,
		      "Read Current Track Object",
		      cmd_mcc_otc_read_current_track_object, 1, 0),
	SHELL_CMD_ARG(ots_read_next_track_object, NULL,
		      "Read Next Track Object",
		      cmd_mcc_otc_read_next_track_object, 1, 0),
	SHELL_CMD_ARG(ots_read_parent_group_object, NULL,
		      "Read Parent Group Object",
		      cmd_mcc_otc_read_parent_group_object, 1, 0),
	SHELL_CMD_ARG(ots_read_current_group_object, NULL,
		      "Read Current Group Object",
		      cmd_mcc_otc_read_current_group_object, 1, 0),
	SHELL_CMD_ARG(ots_select_first, NULL, "Select first object",
		      cmd_mcc_otc_select_first, 1, 0),
	SHELL_CMD_ARG(ots_select_last, NULL, "Select last object",
		      cmd_mcc_otc_select_last, 1, 0),
	SHELL_CMD_ARG(ots_select_next, NULL, "Select next object",
		      cmd_mcc_otc_select_next, 1, 0),
	SHELL_CMD_ARG(ots_select_previous, NULL, "Select previous object",
		      cmd_mcc_otc_select_prev, 1, 0),
#endif /* CONFIG_BT_MCC_OTS */
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_ARG_REGISTER(mcc, &mcc_cmds, "MCC commands",
		       cmd_mcc, 1, 1);
