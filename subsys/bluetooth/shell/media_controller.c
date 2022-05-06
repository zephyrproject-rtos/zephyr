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
#include <zephyr/shell/shell.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>

#include "bt.h"
#include <zephyr/bluetooth/services/ots.h>

#include <zephyr/bluetooth/audio/media_proxy.h>
#include "../audio/media_proxy_internal.h" /* For MPL_NO_TRACK_ID - TODO: Fix */

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_MCS)
#define LOG_MODULE_NAME bt_media_controller_shell
#include "common/log.h"

static struct media_proxy_ctrl_cbs cbs;

/* Media player instances - the local player, the remote player and
 * the current player (pointing to either the local or the remote)
 *
 * TODO: Add command to select player, ensure that the player pointer is used
 */
static struct media_player *local_player;
static struct media_player *remote_player;
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

#ifdef CONFIG_MCTL_REMOTE_PLAYER_CONTROL
static void discover_player_cb(struct media_player *plr, int err)
{
	if (err) {
		shell_error(ctx_shell, "Discover player failed (%d)", err);
		return;
	}

	remote_player = plr;
	shell_print(ctx_shell, "Discovered player instance: %p", remote_player);

	/* Assuming that since discovery was called, the remote player is wanted */
	current_player = remote_player;
}
#endif /* CONFIG_MCTL_REMOTE_PLAYER_CONTROL */

static void player_name_cb(struct media_player *plr, int err, const char *name)
{
	if (err) {
		shell_error(ctx_shell, "Player: %p, Player name failed (%d)", plr, err);
		return;
	}

	shell_print(ctx_shell, "Player: %p, Player name: %s", plr, name);
}

static void icon_id_cb(struct media_player *plr, int err, uint64_t id)
{
	char str[BT_OTS_OBJ_ID_STR_LEN];

	if (err) {
		shell_error(ctx_shell, "Player: %p, Icon ID failed (%d)", plr, err);
		return;
	}

	(void)bt_ots_obj_id_to_str(id, str, sizeof(str));
	shell_print(ctx_shell, "Player: %p, Icon Object ID: %s", plr, str);
}

static void icon_url_cb(struct media_player *plr, int err, const char *url)
{
	if (err) {
		shell_error(ctx_shell, "Player: %p, Icon URL failed (%d)", plr, err);
		return;
	}

	shell_print(ctx_shell, "Player: %p, Icon URL: %s", plr, url);
}

static void track_changed_cb(struct media_player *plr, int err)
{
	if (err) {
		shell_error(ctx_shell, "Player: %p, Track change failed (%d)", plr, err);
		return;
	}

	shell_print(ctx_shell, "Player: %p, Track changed", plr);
}

static void track_title_cb(struct media_player *plr, int err, const char *title)
{
	if (err) {
		shell_error(ctx_shell, "Player: %p, Track title failed (%d)", plr, err);
		return;
	}

	shell_print(ctx_shell, "Player: %p, Track title: %s", plr, title);
}

static void track_duration_cb(struct media_player *plr, int err, int32_t duration)
{
	if (err) {
		shell_error(ctx_shell, "Player: %p, Track duration failed (%d)", plr, err);
		return;
	}

	shell_print(ctx_shell, "Player: %p, Track duration: %d", plr, duration);
}

static void track_position_recv_cb(struct media_player *plr, int err, int32_t position)
{
	if (err) {
		shell_error(ctx_shell, "Player: %p, Track position receive failed (%d)", plr, err);
		return;
	}

	shell_print(ctx_shell, "Player: %p, Track Position received: %d", plr, position);
}

static void track_position_write_cb(struct media_player *plr, int err, int32_t position)
{
	if (err) {
		shell_error(ctx_shell, "Player: %p, Track position write failed (%d)", plr, err);
		return;
	}

	shell_print(ctx_shell, "Player: %p, Track Position write: %d", plr, position);
}

static void playback_speed_recv_cb(struct media_player *plr, int err, int8_t speed)
{
	if (err) {
		shell_error(ctx_shell, "Player: %p, Playback speed receive failed (%d)", plr, err);
		return;
	}

	shell_print(ctx_shell, "Player: %p, Playback speed received: %d", plr, speed);
}

static void playback_speed_write_cb(struct media_player *plr, int err, int8_t speed)
{
	if (err) {
		shell_error(ctx_shell, "Player: %p, Playback speed write failed (%d)", plr, err);
		return;
	}

	shell_print(ctx_shell, "Player: %p, Playback speed write: %d", plr, speed);
}

static void seeking_speed_cb(struct media_player *plr, int err, int8_t speed)
{
	if (err) {
		shell_error(ctx_shell, "Player: %p, Seeking speed failed (%d)", plr, err);
		return;
	}

	shell_print(ctx_shell, "Player: %p, Seeking speed: %d", plr, speed);
}

#ifdef CONFIG_BT_OTS
static void track_segments_id_cb(struct media_player *plr, int err, uint64_t id)
{
	char str[BT_OTS_OBJ_ID_STR_LEN];

	if (err) {
		shell_error(ctx_shell, "Player: %p, Track segments ID failed (%d)", plr, err);
		return;
	}

	(void)bt_ots_obj_id_to_str(id, str, sizeof(str));
	shell_print(ctx_shell, "Player: %p, Track Segments Object ID: %s", plr, str);
}

static void current_track_id_cb(struct media_player *plr, int err, uint64_t id)
{
	char str[BT_OTS_OBJ_ID_STR_LEN];

	if (err) {
		shell_error(ctx_shell, "Player: %p, Current track ID failed (%d)", plr, err);
		return;
	}

	(void)bt_ots_obj_id_to_str(id, str, sizeof(str));
	shell_print(ctx_shell, "Player: %p, Current Track Object ID: %s", plr, str);
}

static void next_track_id_cb(struct media_player *plr, int err, uint64_t id)
{
	char str[BT_OTS_OBJ_ID_STR_LEN];

	if (err) {
		shell_error(ctx_shell, "Player: %p, Next track ID failed (%d)", plr, err);
		return;
	}

	if (id == MPL_NO_TRACK_ID) {
		shell_print(ctx_shell, "Player: %p, Next Track Object ID is empty", plr);
	} else {
		(void)bt_ots_obj_id_to_str(id, str, sizeof(str));
		shell_print(ctx_shell, "Player: %p, Next Track Object ID: %s", plr, str);
	}
}

static void current_group_id_cb(struct media_player *plr, int err, uint64_t id)
{
	char str[BT_OTS_OBJ_ID_STR_LEN];

	if (err) {
		shell_error(ctx_shell, "Player: %p, Current group ID failed (%d)", plr, err);
		return;
	}

	(void)bt_ots_obj_id_to_str(id, str, sizeof(str));
	shell_print(ctx_shell, "Player: %p, Current Group Object ID: %s", plr, str);
}

static void parent_group_id_cb(struct media_player *plr, int err, uint64_t id)
{
	char str[BT_OTS_OBJ_ID_STR_LEN];

	if (err) {
		shell_error(ctx_shell, "Player: %p, Parent group ID failed (%d)", plr, err);
		return;
	}

	(void)bt_ots_obj_id_to_str(id, str, sizeof(str));
	shell_print(ctx_shell, "Player: %p, Parent Group Object ID: %s", plr, str);
}
#endif /* CONFIG_BT_OTS */

static void playing_order_recv_cb(struct media_player *plr, int err, uint8_t order)
{
	if (err) {
		shell_error(ctx_shell, "Player: %p, Playing order receive_failed (%d)", plr, err);
		return;
	}

	shell_print(ctx_shell, "Player: %p, Playing received: %u", plr, order);
}

static void playing_order_write_cb(struct media_player *plr, int err, uint8_t order)
{
	if (err) {
		shell_error(ctx_shell, "Player: %p, Playing order write_failed (%d)", plr, err);
		return;
	}

	shell_print(ctx_shell, "Player: %p, Playing written: %u", plr, order);
}

static void playing_orders_supported_cb(struct media_player *plr, int err, uint16_t orders)
{
	if (err) {
		shell_error(ctx_shell, "Player: %p, Playing orders supported failed (%d)",
			    plr, err);
		return;
	}

	shell_print(ctx_shell, "Player: %p, Playing orders supported: %u", plr, orders);
	/* TODO: Parse bitmap and output list of playing orders */
}

static void media_state_cb(struct media_player *plr, int err, uint8_t state)
{
	if (err) {
		shell_error(ctx_shell, "Player: %p, Media state failed (%d)", plr, err);
		return;
	}

	shell_print(ctx_shell, "Player: %p, Media State: %u", plr, state);
	/* TODO: Parse state and output state name (e.g. "Playing") */
}

static void command_send_cb(struct media_player *plr, int err, const struct mpl_cmd *cmd)
{
	if (err) {
		shell_error(ctx_shell, "Player: %p, Command send failed (%d)", plr, err);
		return;
	}

	shell_print(ctx_shell, "Player: %p, Command opcode sent: %u", plr, cmd->opcode);
}

static void command_recv_cb(struct media_player *plr, int err, const struct mpl_cmd_ntf *cmd_ntf)
{
	if (err) {
		shell_error(ctx_shell, "Player: %p, Command failed (%d)", plr, err);
		return;
	}

	shell_print(ctx_shell, "Player: %p, Command opcode: %u, result: %u",
		    plr, cmd_ntf->requested_opcode, cmd_ntf->result_code);
}

static void commands_supported_cb(struct media_player *plr, int err, uint32_t opcodes)
{
	if (err) {
		shell_error(ctx_shell, "Player: %p, Commands supported failed (%d)", plr, err);
		return;
	}

	shell_print(ctx_shell, "Player: %p, Command opcodes supported: %u", plr, opcodes);
	/* TODO: Parse bitmap and output list of opcodes */
}

#ifdef CONFIG_BT_OTS
static void search_send_cb(struct media_player *plr, int err, const struct mpl_search *search)
{
	if (err) {
		shell_error(ctx_shell, "Player: %p, Search send failed (%d)", plr, err);
		return;
	}

	shell_print(ctx_shell, "Player: %p, Search sent with len %u", plr, search->len);
}

static void search_recv_cb(struct media_player *plr, int err, uint8_t result_code)
{
	if (err) {
		shell_error(ctx_shell, "Player: %p, Search failed (%d)", plr, err);
		return;
	}

	shell_print(ctx_shell, "Player: %p, Search result code: %u", plr, result_code);
}

static void search_results_id_cb(struct media_player *plr, int err, uint64_t id)
{
	char str[BT_OTS_OBJ_ID_STR_LEN];

	if (err) {
		shell_error(ctx_shell, "Player: %p, Search results ID failed (%d)", plr, err);
		return;
	}

	if (id == 0) {
		shell_print(ctx_shell, "Player: %p, Search result not avilable", plr);
	}

	(void)bt_ots_obj_id_to_str(id, str, sizeof(str));
	shell_print(ctx_shell, "Player: %p, Search Results Object ID: %s", plr, str);
}
#endif /* CONFIG_BT_OTS */

static void content_ctrl_id_cb(struct media_player *plr, int err, uint8_t ccid)
{
	if (err) {
		shell_error(ctx_shell, "Player: %p, Content control ID failed (%d)", plr, err);
		return;
	}

	shell_print(ctx_shell, "Player: %p, Content Control ID: %u", plr, ccid);
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
#ifdef CONFIG_MCTL_REMOTE_PLAYER_CONTROL
	cbs.discover_player               = discover_player_cb;
#endif /* CONFIG_MCTL_REMOTE_PLAYER_CONTROL */
	cbs.local_player_instance         = local_player_instance_cb;
	cbs.player_name_recv              = player_name_cb;
	cbs.icon_id_recv                  = icon_id_cb;
	cbs.icon_url_recv                 = icon_url_cb;
	cbs.track_changed_recv            = track_changed_cb;
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
	cbs.current_group_id_recv         = current_group_id_cb;
	cbs.parent_group_id_recv          = parent_group_id_cb;
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

	err = media_proxy_ctrl_register(&cbs);
	if (err) {
		shell_error(ctx_shell, "Could not register media shell as controller");
	}

	return err;
}

static int cmd_media_set_player(const struct shell *sh, size_t argc, char *argv[])
{
	if (!strcmp(argv[1], "local")) {
		if (local_player) {
			current_player = local_player;
			shell_print(ctx_shell, "Current player set to local player: %p",
				    current_player);
			return 0;
		}

		shell_print(ctx_shell, "No local player");
		return -EOPNOTSUPP;

	} else if (!strcmp(argv[1], "remote")) {
		if (remote_player) {
			current_player = remote_player;
			shell_print(ctx_shell, "Current player set to remote player: %p",
				    current_player);
			return 0;
		}

		shell_print(ctx_shell, "No remote player");
		return -EOPNOTSUPP;

	} else {
		shell_error(ctx_shell, "Input argument must be either \"local\" or \"remote\"");
		return -EINVAL;
	}
}

static int cmd_media_show_players(const struct shell *sh, size_t argc, char *argv[])
{
	shell_print(ctx_shell, "Local player: %p", local_player);
	shell_print(ctx_shell, "Remote player: %p", remote_player);

	if (current_player == NULL) {
		shell_print(ctx_shell, "Current player is not set");
	} else if (current_player == local_player) {
		shell_print(ctx_shell, "Current player is set to local player: %p",
			    current_player);
	} else if (current_player == remote_player) {
		shell_print(ctx_shell, "Current player is set to remote player: %p",
			    current_player);
	} else {
		shell_print(ctx_shell, "Current player is not set to valid player");
	}

	return 0;
}

#ifdef CONFIG_MCTL_REMOTE_PLAYER_CONTROL
static int cmd_media_discover_player(const struct shell *sh, size_t argc, char *argv[])
{
	int err = media_proxy_ctrl_discover_player(default_conn);

	if (err) {
		shell_error(ctx_shell, "Discover player failed (%d)", err);
	}

	return err;
}
#endif /* CONFIG_MCTL_REMOTE_PLAYER_CONTROL */

static int cmd_media_read_player_name(const struct shell *sh, size_t argc, char *argv[])
{
	int err = media_proxy_ctrl_get_player_name(current_player);

	if (err) {
		shell_error(ctx_shell, "Player name get failed (%d)", err);
	}

	return err;
}

#ifdef CONFIG_BT_OTS
static int cmd_media_read_icon_obj_id(const struct shell *sh, size_t argc, char *argv[])
{
	int err = media_proxy_ctrl_get_icon_id(current_player);

	if (err) {
		shell_error(ctx_shell, "Icon ID get failed (%d)", err);
	}

	return err;
}
#endif /* CONFIG_BT_OTS */

static int cmd_media_read_icon_url(const struct shell *sh, size_t argc, char *argv[])
{
	int err = media_proxy_ctrl_get_icon_url(current_player);

	if (err) {
		shell_error(ctx_shell, "Icon URL get failed (%d)", err);
	}

	return err;
}

static int cmd_media_read_track_title(const struct shell *sh, size_t argc, char *argv[])
{
	int err = media_proxy_ctrl_get_track_title(current_player);

	if (err) {
		shell_error(ctx_shell, "Track title get failed (%d)", err);
	}

	return err;
}

static int cmd_media_read_track_duration(const struct shell *sh, size_t argc, char *argv[])
{
	int err = media_proxy_ctrl_get_track_duration(current_player);

	if (err) {
		shell_error(ctx_shell, "Track duration get failed (%d)", err);
	}

	return err;
}

static int cmd_media_read_track_position(const struct shell *sh, size_t argc, char *argv[])
{
	int err = media_proxy_ctrl_get_track_position(current_player);

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

	int err = media_proxy_ctrl_set_track_position(current_player, position);

	if (err) {
		shell_error(ctx_shell, "Track position set failed (%d)", err);
	}

	return err;
}

static int cmd_media_read_playback_speed(const struct shell *sh, size_t argc, char *argv[])
{
	int err = media_proxy_ctrl_get_playback_speed(current_player);

	if (err) {
		shell_error(ctx_shell, "Playback speed get get failed (%d)", err);
	}

	return err;
}


static int cmd_media_set_playback_speed(const struct shell *sh, size_t argc, char *argv[])
{
	int8_t speed = strtol(argv[1], NULL, 0);
	int err = media_proxy_ctrl_set_playback_speed(current_player, speed);

	if (err) {
		shell_error(ctx_shell, "Playback speed set failed (%d)", err);
	}

	return err;
}

static int cmd_media_read_seeking_speed(const struct shell *sh, size_t argc, char *argv[])
{
	int err = media_proxy_ctrl_get_seeking_speed(current_player);

	if (err) {
		shell_error(ctx_shell, "Seeking speed get failed (%d)", err);
	}

	return err;
}

#ifdef CONFIG_BT_OTS
static int cmd_media_read_track_segments_obj_id(const struct shell *sh, size_t argc, char *argv[])
{
	int err = media_proxy_ctrl_get_track_segments_id(current_player);

	if (err) {
		shell_error(ctx_shell, "Track segments ID get failed (%d)", err);
	}

	return err;
}

static int cmd_media_read_current_track_obj_id(const struct shell *sh, size_t argc, char *argv[])
{
	int err = media_proxy_ctrl_get_current_track_id(current_player);

	if (err) {
		shell_error(ctx_shell, "Current track ID get failed (%d)", err);
	}

	return err;
}

/* TDOD: cmd_media_set_current_track_obj_id */

static int cmd_media_read_next_track_obj_id(const struct shell *sh, size_t argc, char *argv[])
{
	int err = media_proxy_ctrl_get_next_track_id(current_player);

	if (err) {
		shell_error(ctx_shell, "Nect track ID get failed (%d)", err);
	}

	return err;
}

static int cmd_media_read_current_group_obj_id(const struct shell *sh, size_t argc, char *argv[])
{
	int err = media_proxy_ctrl_get_current_group_id(current_player);

	if (err) {
		shell_error(ctx_shell, "Current group ID get failed (%d)", err);

	}

	return err;
}

static int cmd_media_read_parent_group_obj_id(const struct shell *sh, size_t argc, char *argv[])
{
	int err = media_proxy_ctrl_get_parent_group_id(current_player);

	if (err) {
		shell_error(ctx_shell, "Parent group ID get failed (%d)", err);
	}

	return err;
}
#endif /* CONFIG_BT_OTS */

static int cmd_media_read_playing_order(const struct shell *sh, size_t argc, char *argv[])
{
	int err = media_proxy_ctrl_get_playing_order(current_player);

	if (err) {
		shell_error(ctx_shell, "Playing order get failed (%d)", err);
	}

	return err;
}

static int cmd_media_set_playing_order(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t order = strtol(argv[1], NULL, 0);

	int err = media_proxy_ctrl_set_playing_order(current_player, order);

	if (err) {
		shell_error(ctx_shell, "Playing order set failed (%d)", err);
	}

	return err;
}

static int cmd_media_read_playing_orders_supported(const struct shell *sh, size_t argc,
						   char *argv[])
{
	int err = media_proxy_ctrl_get_playing_orders_supported(current_player);

	if (err) {
		shell_error(ctx_shell, "Icon URL get failed (%d)", err);
	}

	return err;
}

static int cmd_media_read_media_state(const struct shell *sh, size_t argc, char *argv[])
{
	int err = media_proxy_ctrl_get_media_state(current_player);

	if (err) {
		shell_error(ctx_shell, "Icon URL get failed (%d)", err);
	}

	return err;
}

static int cmd_media_send_command(const struct shell *sh, size_t argc, char *argv[])
{
	struct mpl_cmd cmd;
	int err;

	cmd.opcode = strtol(argv[1], NULL, 0);

	if (argc > 2) {
		cmd.use_param = true;
		cmd.param = strtol(argv[2], NULL, 0);
	} else {
		cmd.use_param = false;
		cmd.param = 0;
	}

	err = media_proxy_ctrl_send_command(current_player, &cmd);

	if (err) {
		shell_error(ctx_shell, "Command send failed (%d)", err);
	}

	return err;
}

static int cmd_media_read_commands_supported(const struct shell *sh, size_t argc, char *argv[])
{
	int err = media_proxy_ctrl_get_commands_supported(current_player);

	if (err) {
		shell_error(ctx_shell, "Commands supported read failed (%d)", err);
	}

	return err;
}

#ifdef CONFIG_BT_OTS
int cmd_media_set_search(const struct shell *sh, size_t argc, char *argv[])
{
	/* TODO: Currently takes the raw search as input - add parameters
	 * and build the search item here
	 */

	struct mpl_search search;
	int err;

	search.len = strlen(argv[1]);
	memcpy(search.search, argv[1], search.len);
	BT_DBG("Search string: %s", log_strdup(argv[1]));

	err = media_proxy_ctrl_send_search(current_player, &search);
	if (err) {
		shell_error(ctx_shell, "Search send failed (%d)", err);
	}

	return err;
}

static int cmd_media_read_search_results_obj_id(const struct shell *sh, size_t argc,
						char *argv[])
{
	int err = media_proxy_ctrl_get_search_results_id(current_player);

	if (err) {
		shell_error(ctx_shell, "Search results ID get failed (%d)", err);
	}

	return err;
}
#endif /* CONFIG_BT_OTS */

static int cmd_media_read_content_control_id(const struct shell *sh, size_t argc,
					     char *argv[])
{
	int err = media_proxy_ctrl_get_content_ctrl_id(current_player);

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
	SHELL_CMD_ARG(set_player, NULL, "Set current player [local || remote]",
		      cmd_media_set_player, 2, 0),
	SHELL_CMD_ARG(show_players, NULL, "Show local, remote and current player",
		      cmd_media_show_players, 1, 0),
#ifdef CONFIG_BT_MCC
	SHELL_CMD_ARG(discover_player, NULL, "Discover remote media player",
		      cmd_media_discover_player, 1, 0),
#endif /* CONFIG_BT_MCC */
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
	SHELL_CMD_ARG(send_command, NULL, "Send command <opcode> [argument]",
		      cmd_media_send_command, 2, 1),
	SHELL_CMD_ARG(read_commands_supported, NULL, "Read Commands Supported",
		      cmd_media_read_commands_supported, 1, 0),
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
