/** @file
 *  @brief Media Controller shell implementation
 *
 */

/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <shell/shell.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>

#include "bt.h"
#include <bluetooth/services/ots.h>

#include "../host/audio/media_proxy.h"
#include "../host/audio/media_proxy_internal.h" /* For MPL_NO_TRACK_ID - TODO: Fix */

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_MCS)
#define LOG_MODULE_NAME bt_media_shell
#include "common/log.h"

static struct media_proxy_ctrl_cbs cbs;

static struct media_player *local_player; /* TODO: Use the player pointer */
static struct media_player *current_player;

static void local_player_instance_cb(struct media_player *plr, int err)
{
	if (err) {
		shell_error(ctx_shell, "Local player instance failed (%d)", err);
		return;
	}

	local_player = plr;
	shell_print(ctx_shell, "Local player instance: %p", local_player);

	if (!current_player) {
		current_player = local_player;
	}
}

static void player_name_cb(struct media_player *plr, int err, char *name)
{
	if (err) {
		shell_error(ctx_shell, "Player name failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Player name: %s", name);
}

static void icon_id_cb(struct media_player *plr, int err, uint64_t id)
{
	char str[BT_OTS_OBJ_ID_STR_LEN];

	if (err) {
		shell_error(ctx_shell, "Icon ID failed (%d)", err);
		return;
	}

	(void)bt_ots_obj_id_to_str(id, str, sizeof(str));
	shell_print(ctx_shell, "Icon Object ID: %s", str);
}

static void icon_url_cb(struct media_player *plr, int err, char *url)
{
	if (err) {
		shell_error(ctx_shell, "Icon URL failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Icon URL: %s", url);
}

static void track_changed_cb(struct media_player *plr, int err)
{
	if (err) {
		shell_error(ctx_shell, "Track change failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Track changed");
}

static void track_title_cb(struct media_player *plr, int err, char *title)
{
	if (err) {
		shell_error(ctx_shell, "Track title failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Track title: %s", title);
}

static void track_duration_cb(struct media_player *plr, int err, int32_t duration)
{
	if (err) {
		shell_error(ctx_shell, "Track duration failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Track duration: %d", duration);
}

static void track_position_cb(struct media_player *plr, int err, int32_t position)
{
	if (err) {
		shell_error(ctx_shell, "Track position failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Track Position: %d", position);
}

static void playback_speed_cb(struct media_player *plr, int err, int8_t speed)
{
	if (err) {
		shell_error(ctx_shell, "Playback speed failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Playback speed: %d", speed);
}

static void seeking_speed_cb(struct media_player *plr, int err, int8_t speed)
{
	if (err) {
		shell_error(ctx_shell, "Seeking speed failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Seeking speed: %d", speed);
}

#ifdef CONFIG_BT_OTS
static void track_segments_id_cb(struct media_player *plr, int err, uint64_t id)
{
	char str[BT_OTS_OBJ_ID_STR_LEN];

	if (err) {
		shell_error(ctx_shell, "Track segments ID failed (%d)", err);
		return;
	}

	(void)bt_ots_obj_id_to_str(id, str, sizeof(str));
	shell_print(ctx_shell, "Track Segments Object ID: %s", str);
}

static void current_track_id_cb(struct media_player *plr, int err, uint64_t id)
{
	char str[BT_OTS_OBJ_ID_STR_LEN];

	if (err) {
		shell_error(ctx_shell, "Current track ID failed (%d)", err);
		return;
	}

	(void)bt_ots_obj_id_to_str(id, str, sizeof(str));
	shell_print(ctx_shell, "Current Track Object ID: %s", str);
}

static void next_track_id_cb(struct media_player *plr, int err, uint64_t id)
{
	char str[BT_OTS_OBJ_ID_STR_LEN];

	if (err) {
		shell_error(ctx_shell, "Next track ID failed (%d)", err);
		return;
	}

	if (id == MPL_NO_TRACK_ID) {
		shell_print(ctx_shell, "Next Track Object ID is empty");
	} else {
		(void)bt_ots_obj_id_to_str(id, str, sizeof(str));
		shell_print(ctx_shell, "Next Track Object ID: %s", str);
	}
}

static void current_group_id_cb(struct media_player *plr, int err, uint64_t id)
{
	char str[BT_OTS_OBJ_ID_STR_LEN];

	if (err) {
		shell_error(ctx_shell, "Current group ID failed (%d)", err);
		return;
	}

	(void)bt_ots_obj_id_to_str(id, str, sizeof(str));
	shell_print(ctx_shell, "Current Group Object ID: %s", str);
}

static void parent_group_id_cb(struct media_player *plr, int err, uint64_t id)
{
	char str[BT_OTS_OBJ_ID_STR_LEN];

	if (err) {
		shell_error(ctx_shell, "Parent group ID failed (%d)", err);
		return;
	}

	(void)bt_ots_obj_id_to_str(id, str, sizeof(str));
	shell_print(ctx_shell, "Parent Group Object ID: %s", str);
}
#endif /* CONFIG_BT_OTS */

static void playing_order_cb(struct media_player *plr, int err, uint8_t order)
{
	if (err) {
		shell_error(ctx_shell, "Playing order failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Playing order: %u", order);
}

static void playing_orders_supported_cb(struct media_player *plr, int err, uint16_t orders)
{
	if (err) {
		shell_error(ctx_shell, "Playing orders supported failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Playing orders supported: %u", orders);
	/* TODO: Parse bitmap and output list of playing orders */
}

static void media_state_cb(struct media_player *plr, int err, uint8_t state)
{
	if (err) {
		shell_error(ctx_shell, "Media state failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Media State: %u", state);
	/* TODO: Parse state and output state name (e.g. "Playing") */
}

static void operation_cb(struct media_player *plr, int err, struct mpl_op_ntf_t op_ntf)
{
	if (err) {
		shell_error(ctx_shell, "Operation failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Operation: %u, result: %u",
		    op_ntf.requested_opcode, op_ntf.result_code);
}

static void operations_supported_cb(struct media_player *plr, int err, uint32_t operations)
{
	if (err) {
		shell_error(ctx_shell, "Operations supported failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Operations supported: %u", operations);
	/* TODO: Parse bitmap and output list of operations */
}

#ifdef CONFIG_BT_OTS
static void search_cb(struct media_player *plr, int err, uint8_t result_code)
{
	if (err) {
		shell_error(ctx_shell, "Search failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Search result code: %u",
		    result_code);
}

static void search_results_id_cb(struct media_player *plr, int err, uint64_t id)
{
	char str[BT_OTS_OBJ_ID_STR_LEN];

	if (err) {
		shell_error(ctx_shell, "Search results ID failed (%d)", err);
		return;
	}

	if (id == 0) {
		shell_print(ctx_shell, "Search result not avilable");
	}

	(void)bt_ots_obj_id_to_str(id, str, sizeof(str));
	shell_print(ctx_shell, "Search Results Object ID: %s", str);
}
#endif /* CONFIG_BT_OTS */

static void content_ctrl_id_cb(struct media_player *plr, int err, uint8_t ccid)
{
	if (err) {
		shell_error(ctx_shell, "Content control ID failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Content Control ID: %u", ccid);
}

int cmd_media_init(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (!ctx_shell) {
		ctx_shell = sh;
	}

	err = media_proxy_pl_init();  /* TODO: Fix direct call to player */
	if (err) {
		shell_error(ctx_shell, "Could not init mpl");
	}

	/* Set up the callback structure */
	cbs.local_player_instance    = local_player_instance_cb;
	cbs.player_name              = player_name_cb;
	cbs.icon_id                  = icon_id_cb;
	cbs.icon_url                 = icon_url_cb;
	cbs.track_changed            = track_changed_cb;
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
		shell_error(ctx_shell, "Could not register media shell as controller");
	}

	return err;
}

static int cmd_media_read_player_name(const struct shell *sh, size_t argc, char *argv[])
{
	int err = media_proxy_ctrl_player_name_get(current_player);

	if (err) {
		shell_error(ctx_shell, "Player name get failed (%d)", err);
	}

	return err;
}

#ifdef CONFIG_BT_OTS
static int cmd_media_read_icon_obj_id(const struct shell *sh, size_t argc, char *argv[])
{
	int err = media_proxy_ctrl_icon_id_get(current_player);

	if (err) {
		shell_error(ctx_shell, "Icon ID get failed (%d)", err);
	}

	return err;
}
#endif /* CONFIG_BT_OTS */

static int cmd_media_read_icon_url(const struct shell *sh, size_t argc, char *argv[])
{
	int err = media_proxy_ctrl_icon_url_get(current_player);

	if (err) {
		shell_error(ctx_shell, "Icon URL get failed (%d)", err);
	}

	return err;
}

static int cmd_media_read_track_title(const struct shell *sh, size_t argc, char *argv[])
{
	int err = media_proxy_ctrl_track_title_get(current_player);

	if (err) {
		shell_error(ctx_shell, "Track title get failed (%d)", err);
	}

	return err;
}

static int cmd_media_read_track_duration(const struct shell *sh, size_t argc, char *argv[])
{
	int err = media_proxy_ctrl_track_duration_get(current_player);

	if (err) {
		shell_error(ctx_shell, "Track duration get failed (%d)", err);
	}

	return err;
}

static int cmd_media_read_track_position(const struct shell *sh, size_t argc, char *argv[])
{
	int err = media_proxy_ctrl_track_position_get(current_player);

	if (err) {
		shell_error(ctx_shell, "Track position get failed (%d)", err);
	}

	return err;
}

static int cmd_media_set_track_position(const struct shell *sh, size_t argc,
			       char *argv[])
{
	/* Todo: Check input "pos" for validity, or for errors in conversion? */
	int32_t position = strtol(argv[1], NULL, 0);

	int err = media_proxy_ctrl_track_position_set(current_player, position);

	if (err) {
		shell_error(ctx_shell, "Track position set failed (%d)", err);
	}

	return err;
}

static int cmd_media_read_playback_speed(const struct shell *sh, size_t argc, char *argv[])
{
	int err = media_proxy_ctrl_playback_speed_get(current_player);

	if (err) {
		shell_error(ctx_shell, "Playback speed get get failed (%d)", err);
	}

	return err;
}


static int cmd_media_set_playback_speed(const struct shell *sh, size_t argc, char *argv[])
{
	int8_t speed = strtol(argv[1], NULL, 0);
	int err = media_proxy_ctrl_playback_speed_set(current_player, speed);

	if (err) {
		shell_error(ctx_shell, "Playback speed set failed (%d)", err);
	}

	return err;
}

static int cmd_media_read_seeking_speed(const struct shell *sh, size_t argc, char *argv[])
{
	int err = media_proxy_ctrl_seeking_speed_get(current_player);

	if (err) {
		shell_error(ctx_shell, "Seeking speed get failed (%d)", err);
	}

	return err;
}

#ifdef CONFIG_BT_OTS
static int cmd_media_read_track_segments_obj_id(const struct shell *sh, size_t argc, char *argv[])
{
	int err = media_proxy_ctrl_track_segments_id_get(current_player);

	if (err) {
		shell_error(ctx_shell, "Track segments ID get failed (%d)", err);
	}

	return err;
}

static int cmd_media_read_current_track_obj_id(const struct shell *sh, size_t argc, char *argv[])
{
	int err = media_proxy_ctrl_current_track_id_get(current_player);

	if (err) {
		shell_error(ctx_shell, "Current track ID get failed (%d)", err);
	}

	return err;
}

/* TDOD: cmd_media_set_current_track_obj_id */

static int cmd_media_read_next_track_obj_id(const struct shell *sh, size_t argc, char *argv[])
{
	int err = media_proxy_ctrl_next_track_id_get(current_player);

	if (err) {
		shell_error(ctx_shell, "Nect track ID get failed (%d)", err);
	}

	return err;
}

static int cmd_media_read_current_group_obj_id(const struct shell *sh, size_t argc, char *argv[])
{
	int err = media_proxy_ctrl_current_group_id_get(current_player);

	if (err) {
		shell_error(ctx_shell, "Current group ID get failed (%d)", err);

	}

	return err;
}

static int cmd_media_read_parent_group_obj_id(const struct shell *sh, size_t argc, char *argv[])
{
	int err = media_proxy_ctrl_parent_group_id_get(current_player);

	if (err) {
		shell_error(ctx_shell, "Parent group ID get failed (%d)", err);
	}

	return err;
}
#endif /* CONFIG_BT_OTS */

static int cmd_media_read_playing_order(const struct shell *sh, size_t argc, char *argv[])
{
	int err = media_proxy_ctrl_playing_order_get(current_player);

	if (err) {
		shell_error(ctx_shell, "Playing order get failed (%d)", err);
	}

	return err;
}

static int cmd_media_set_playing_order(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t order = strtol(argv[1], NULL, 0);

	int err = media_proxy_ctrl_playing_order_set(current_player, order);

	if (err) {
		shell_error(ctx_shell, "Playing order set failed (%d)", err);
	}

	return err;
}

static int cmd_media_read_playing_orders_supported(const struct shell *sh, size_t argc,
						   char *argv[])
{
	int err = media_proxy_ctrl_playing_orders_supported_get(current_player);

	if (err) {
		shell_error(ctx_shell, "Icon URL get failed (%d)", err);
	}

	return err;
}

static int cmd_media_read_media_state(const struct shell *sh, size_t argc, char *argv[])
{
	int err = media_proxy_ctrl_media_state_get(current_player);

	if (err) {
		shell_error(ctx_shell, "Icon URL get failed (%d)", err);
	}

	return err;
}

static int cmd_media_set_cp(const struct shell *sh, size_t argc, char *argv[])
{
	struct mpl_op_t op;
	int err;

	op.opcode = strtol(argv[1], NULL, 0);

	if (argc > 2) {
		op.use_param = true;
		op.param = strtol(argv[2], NULL, 0);
	} else {
		op.use_param = false;
		op.param = 0;
	}

	err = media_proxy_ctrl_operation_set(current_player, op);

	if (err) {
		shell_error(ctx_shell, "Operation set failed (%d)", err);
	}

	return err;
}

static int cmd_media_read_opcodes_supported(const struct shell *sh, size_t argc, char *argv[])
{
	int err = media_proxy_ctrl_operations_supported_get(current_player);

	if (err) {
		shell_error(ctx_shell, "Operation supported read failed (%d)", err);
	}

	return err;
}

#ifdef CONFIG_BT_OTS
int cmd_media_set_search(const struct shell *sh, size_t argc, char *argv[])
{
	/* TODO: Currently takes the raw search as input - add parameters
	 * and build the search item here
	 */

	struct mpl_search_t search;
	int err;

	search.len = strlen(argv[1]);
	memcpy(search.search, argv[1], search.len);
	BT_DBG("Search string: %s", log_strdup(argv[1]));

	err = media_proxy_ctrl_search_set(current_player, search);
	if (err) {
		shell_error(ctx_shell, "Search set failed (%d)", err);
	}

	return err;
}

static int cmd_media_read_search_results_obj_id(const struct shell *sh, size_t argc,
						char *argv[])
{
	int err = media_proxy_ctrl_search_results_id_get(current_player);

	if (err) {
		shell_error(ctx_shell, "Search results ID get failed (%d)", err);
	}

	return err;
}
#endif /* CONFIG_BT_OTS */

static int cmd_media_read_content_control_id(const struct shell *sh, size_t argc,
					     char *argv[])
{
	int err = media_proxy_ctrl_content_ctrl_id_get(current_player);

	if (err) {
		shell_error(ctx_shell, "Content control ID get failed (%d)", err);
	}

	return err;
}

static int cmd_media(const struct shell *sh, size_t argc, char **argv)
{
	shell_error(ctx_shell, "%s unknown parameter: %s", argv[0], argv[1]);

	return -ENOEXEC;
}

SHELL_STATIC_SUBCMD_SET_CREATE(media_cmds,
	SHELL_CMD_ARG(init, NULL,
		      "Initialize media player",
		      cmd_media_init, 1, 0),
	SHELL_CMD_ARG(read_player_name, NULL, "Read Media Player Name",
		      cmd_media_read_player_name, 1, 0),
#ifdef CONFIG_BT_OTS
	SHELL_CMD_ARG(read_icon_obj_id, NULL, "Read Icon Object ID",
		      cmd_media_read_icon_obj_id, 1, 0),
#endif /* CONFIG_BT_OTS */
	SHELL_CMD_ARG(read_icon_url, NULL, "Read Icon URL",
		      cmd_media_read_icon_url, 1, 0),
	SHELL_CMD_ARG(read_track_title, NULL, "Read Track Title",
		      cmd_media_read_track_title, 1, 0),
	SHELL_CMD_ARG(read_track_duration, NULL, "Read Track Duration",
		      cmd_media_read_track_duration, 1, 0),
	SHELL_CMD_ARG(read_track_position, NULL, "Read Track Position",
		      cmd_media_read_track_position, 1, 0),
	SHELL_CMD_ARG(set_track_position, NULL, "Set Track position <position>",
		      cmd_media_set_track_position, 2, 0),
	SHELL_CMD_ARG(read_playback_speed, NULL, "Read Playback Speed",
		      cmd_media_read_playback_speed, 1, 0),
	SHELL_CMD_ARG(set_playback_speed, NULL, "Set Playback Speed <speed>",
		      cmd_media_set_playback_speed, 2, 0),
	SHELL_CMD_ARG(read_seeking_speed, NULL, "Read Seeking Speed",
		      cmd_media_read_seeking_speed, 1, 0),
#ifdef CONFIG_BT_OTS
	SHELL_CMD_ARG(read_track_segments_obj_id, NULL,
		      "Read Track Segments Object ID",
		      cmd_media_read_track_segments_obj_id, 1, 0),
	SHELL_CMD_ARG(read_current_track_obj_id, NULL,
		      "Read Current Track Object ID",
		      cmd_media_read_current_track_obj_id, 1, 0),
	SHELL_CMD_ARG(read_next_track_obj_id, NULL,
		      "Read Next Track Object ID",
		      cmd_media_read_next_track_obj_id, 1, 0),
	SHELL_CMD_ARG(read_current_group_obj_id, NULL,
		      "Read Current Group Object ID",
		      cmd_media_read_current_group_obj_id, 1, 0),
	SHELL_CMD_ARG(read_parent_group_obj_id, NULL,
		      "Read Parent Group Object ID",
		      cmd_media_read_parent_group_obj_id, 1, 0),
#endif /* CONFIG_BT_OTS */
	SHELL_CMD_ARG(read_playing_order, NULL, "Read Playing Order",
		      cmd_media_read_playing_order, 1, 0),
	SHELL_CMD_ARG(set_playing_order, NULL, "Set Playing Order <order>",
		      cmd_media_set_playing_order, 2, 0),
	SHELL_CMD_ARG(read_playing_orders_supported, NULL,
		     "Read Playing Orders Supported",
		      cmd_media_read_playing_orders_supported, 1, 0),
	SHELL_CMD_ARG(read_media_state, NULL, "Read Media State",
		      cmd_media_read_media_state, 1, 0),
	SHELL_CMD_ARG(set_cp, NULL, "Set opcode/operation <opcode> [argument]",
		      cmd_media_set_cp, 2, 1),
	SHELL_CMD_ARG(read_opcodes_supported, NULL, "Read Opcodes Supported",
		      cmd_media_read_opcodes_supported, 1, 0),
#ifdef CONFIG_BT_OTS
	SHELL_CMD_ARG(set_search, NULL, "Set search <search control item sequence>",
		      cmd_media_set_search, 2, 0),
	SHELL_CMD_ARG(read_search_results_obj_id, NULL,
		      "Read Search Results Object ID",
		      cmd_media_read_search_results_obj_id, 1, 0),
#endif /* CONFIG_BT_OTS */
	SHELL_CMD_ARG(read_content_control_id, NULL, "Read Content Control ID",
		      cmd_media_read_content_control_id, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_ARG_REGISTER(media, &media_cmds, "Media commands",
		       cmd_media, 1, 1);
